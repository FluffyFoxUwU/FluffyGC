#ifndef header_1659439852_4c35005b_5cca_4dd8_bfce_48cc0c5ec592_thread_pool_h
#define header_1659439852_4c35005b_5cca_4dd8_bfce_48cc0c5ec592_thread_pool_h

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

// Obviously thread safe 
// thread pool

struct thread_pool_work_unit;
typedef void (^thread_pool_worker)(const struct thread_pool_work_unit* workUnit);

struct thread_pool_work_unit {
  void* data;
  int intData;
  bool canRelease;
  thread_pool_worker worker;
};

struct thread_pool {
  atomic_bool shuttingDown;
  
  bool submitWorkMutexInited;
  pthread_mutex_t submitWorkMutex;

  // Number of threads available
  // for executing work
  volatile int numberOfIdleThreads;
  pthread_cond_t taskCompletedCond;
  pthread_mutex_t taskCompletedMutex;
  
  volatile bool taskArrived;
  volatile struct thread_pool_work_unit workUnit;
  
  pthread_cond_t taskArrivedCond;
  pthread_mutex_t taskArrivedMutex;
  
  volatile bool itsSafeToContinueUwU;
  pthread_cond_t itsSafeToContinueUwUCond;
  pthread_mutex_t itsSafeToContinueUwUMutex;
  
  int poolSize;
  int threadsCreated;
  pthread_t* threads;
  
  bool taskArrivedMutexInited;
  bool taskCompletedCondInited;
  bool taskCompletedMutexInited;
  bool taskArrivedCondInited;
  bool itsSafeToContinueUwUCondInited;
  bool itsSafeToContinueUwUMutexInited;
};

struct thread_pool* thread_pool_new(int poolSize, const char* prefix);
void thread_pool_free(struct thread_pool* self);

void thread_pool_submit_work(struct thread_pool* self, struct thread_pool_work_unit* workUnit);
bool thread_pool_try_submit_work(struct thread_pool* self, struct thread_pool_work_unit* workUnit);
void thread_pool_wait_all(struct thread_pool* self);

#endif

