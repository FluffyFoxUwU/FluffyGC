#include <asm-generic/errno-base.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "userfaultfd.h"
#include "config.h"

#if IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT)
# include <syscall.h>
# include <sys/ioctl.h>
# include <linux/userfaultfd.h>
#else
# define syscall(n, ...) do { \
  abort(); \
} while (0)
# define ioctl(fd, n, ...) do { \
  abort(); \
} while (0) 
#endif

static int rawCreateUserfaulObject(int flags) {
# if IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT)
  int fd = syscall(SYS_userfaultfd, flags);
  if (fd < 0)
    return -EFAULT;
  
  struct uffdio_api initArg = {
    .api = UFFD_API,
    .features = 0,
    .ioctls = 0
  };
  
  // Unknown ioctl error
  if (ioctl(fd, UFFDIO_API, &initArg) != 0)
    return -EFAULT;
  
  // userfaultfd object successfully initialized
  return fd;
# else
  return -EFAULT;
# endif
}

int uffd_create_descriptor(int flags) {
  if (!uffd_is_supported())
    return -EFAULT;
  return rawCreateUserfaulObject(flags);
}

bool uffd_is_available() {
  return IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT);
}

static pthread_once_t checkSupportOnce = PTHREAD_ONCE_INIT;
static void checkSupport();
static bool isSupported = false;

bool uffd_is_supported() {
  if (pthread_once(&checkSupportOnce, checkSupport) != 0)
    return false;
  
  return isSupported;
}

static void checkSupport() {
# if IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT)
  int fd = rawCreateUserfaulObject(0);
  if (fd >= 0)
    close(fd);
  isSupported = fd >= 0;
  printf("%d\n", fd);
# endif
}
