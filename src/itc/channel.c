#include "config.h"

#if !IS_ENABLED(CONFIG_STRICTLY_POSIX)
# define _GNU_SOURCE
#endif

#include <limits.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "panic.h"
#include "channel.h"

// POSIX defines that PIPE_BUF shall atleast 512 bytes
// but I'll check anyway
static_assert(PIPE_BUF >= 512);

static int createPipe(int* read, int* write) {
  int fds[2] = {};
  int ret = 0;
# if IS_ENABLED(CONFIG_STRICTLY_POSIX)
  ret = pipe(fds);
# else
  ret = pipe2(fds, O_CLOEXEC | O_DIRECT);
  if (ret == -1 && errno == -EINVAL)
    ret = pipe2(fds, O_CLOEXEC);
# endif
  *read = fds[0];
  *write = fds[1];
  return ret;
}

struct channel* channel_new() {
  struct channel* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  if (createPipe(&self->readFD, &self->writeFD) < 0)
    goto failure;
  return self;
failure:
  channel_free(self);
  return NULL;
}

void channel_free(struct channel* self) {
  while (close(self->readFD) == -1 && errno == EINTR)
    ;
  while (close(self->writeFD) == -1 && errno == EINTR)
    ;
  free(self);
}

void channel_send(struct channel* self, struct channel_message* msg) {
  void* buf = msg;
  size_t size = sizeof(*msg);
  ssize_t res = 0;
  
  while (size > 0 && (res = write(self->writeFD, buf, size)) != (ssize_t) size) {
    if(res < 0 && errno == EINTR) 
      continue;
    
    if(res < 0)
      panic();
    
    size -= res;
    *(char**) &buf += res;
  }
}

void channel_recv(struct channel* self, struct channel_message* msg) {
  *msg = (struct channel_message) {};
  
  void* buf = msg;
  size_t size = sizeof(*msg);
  ssize_t res = 0;
  
  while (size > 0 && (res = read(self->readFD, buf, size)) != (ssize_t) size) {
    if(res < 0 && errno == EINTR) 
      continue;
    
    if(res < 0)
      panic();
    
    size -= res;
    *(char**) &buf += res;
  }
}
