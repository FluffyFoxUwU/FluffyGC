#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "userfaultfd.h"
#include "config.h"
#include "util.h"

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
  return -ENOSYS;
# endif
}

int uffd_create_descriptor(int flags) { 
  if (!uffd_is_supported())
    return -ENOSYS;
  return rawCreateUserfaulObject(flags);
}

bool uffd_is_available() {
  return IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT);
}

static pthread_once_t checkSupportOnce = PTHREAD_ONCE_INIT;
static void checkSupport();
static bool cachedIsSupported;

bool uffd_is_supported() {
  if (pthread_once(&checkSupportOnce, checkSupport) != 0)
    return false;
  
  return cachedIsSupported;
}

static void* testRoutine(void* target) {
  printf("Read access at: %p\n", target);
  void* res = *((void* volatile*) target);
  printf("Read result is: %p\n", res);
  return res;
}

static void checkSupport() {
# if IS_ENABLED(CONFIG_ENABLE_USERFAULTFD_SUPPORT)
  bool isSupported = false;
  
  int fd = rawCreateUserfaulObject(0);
  if (fd < 0)
    goto exit_fail_create_fd;
  
  // Do requirement check
  size_t len = util_get_pagesize();
  void* mem = util_mmap_anonymous(len, PROT_WRITE | PROT_READ);
  if (!mem)
    goto exit_fail_mmap;
  
  const char* testData = "Fox";
  
  pthread_t testThread;
  if (pthread_create(&testThread, NULL, testRoutine, mem))
    goto exit_fail_create_thread;
  
  void* result;
  pthread_join(testThread, &result);
  if (strncmp(testData, result, strlen(testData)) != 0)
    goto exit_failed_test;
  
  puts("Test sucess");
  isSupported = true;

exit_failed_test:
exit_fail_create_thread:
  munmap(mem, len);
exit_fail_mmap:
  close(fd);
exit_fail_create_fd:
  cachedIsSupported = isSupported;
# endif
}
