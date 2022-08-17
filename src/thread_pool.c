#include <Block.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "thread_pool.h"
#include "util.h"

static void* worker(void* _self);
struct thread_pool* thread_pool_new(int poolSize, const char* prefix) { 
  // For ensuring each thread get 
  // one worker task to set the
  // thread name
  __block pthread_barrier_t barrier;
  
  struct thread_pool* self = malloc(sizeof(*self));
  if (!self)
    goto failure;
  
  atomic_init(&self->shuttingDown, false);
  self->numberOfIdleThreads = 0;
  self->threads = NULL;
  self->threadsCreated = 0;
  self->poolSize = poolSize;
  self->shuttingDown = false;
  self->itsSafeToContinueUwU = false;
  self->taskArrived = false;
  self->workUnit.worker = NULL;
  self->workUnit.data = NULL;
  self->taskArrivedMutexInited = false;
  self->taskCompletedCondInited = false;
  self->taskCompletedMutexInited = false;
  self->taskArrivedCondInited = false;
  self->itsSafeToContinueUwUCondInited = false;
  self->itsSafeToContinueUwUMutexInited = false;

  if (pthread_cond_init(&self->taskArrivedCond, NULL) != 0)
    goto failure;
  self->taskArrivedCondInited = true;
  if (pthread_cond_init(&self->taskCompletedCond, NULL) != 0)
    goto failure;
  self->taskCompletedCondInited = true;
  if (pthread_cond_init(&self->itsSafeToContinueUwUCond, NULL) != 0)
    goto failure;
  self->itsSafeToContinueUwUCondInited = true;
  if (pthread_mutex_init(&self->taskArrivedMutex, NULL) != 0)
    goto failure;
  self->taskArrivedMutexInited = true;
  if (pthread_mutex_init(&self->taskCompletedMutex, NULL) != 0)
    goto failure;
  self->taskCompletedMutexInited = true;
  if (pthread_mutex_init(&self->submitWorkMutex, NULL) != 0)
    goto failure;
  self->submitWorkMutexInited = true;
  if (pthread_mutex_init(&self->itsSafeToContinueUwUMutex, NULL) != 0)
    goto failure;
  self->itsSafeToContinueUwUMutexInited = true;

  self->threads = calloc(poolSize, sizeof(*self->threads));
  if (!self->threads)
    goto failure;

  char* prefix2 = strdup(prefix);

  if (!prefix2)
    goto failure;

  pthread_barrier_init(&barrier, NULL, self->poolSize);

  for (; self->threadsCreated < self->poolSize; self->threadsCreated++) {
    if (pthread_create(&self->threads[self->threadsCreated], NULL, worker, self))
      goto failure;
    
    struct thread_pool_work_unit work = {
      .canRelease = false,
      .intData = self->threadsCreated + 1,
      .worker = ^void (const struct thread_pool_work_unit* work) {
        pthread_barrier_wait(&barrier);
        
        size_t bufSize = snprintf(NULL, 0, "%s%d", prefix2, work->intData) + 1;
        char* name = malloc(bufSize);
        snprintf(name, bufSize, "%s%d", prefix2, work->intData);
        util_set_thread_name(name);
        free(name);
      }
    };
    thread_pool_submit_work(self, &work);
  }

  thread_pool_wait_all(self);
  pthread_barrier_destroy(&barrier);

  free(prefix2);
  return self;

  failure:
  thread_pool_free(self);
  return NULL;
}

void thread_pool_free(struct thread_pool* self) {
  if (!self)
    return;

  atomic_store(&self->shuttingDown, true);
  pthread_mutex_lock(&self->taskArrivedMutex);
  self->taskArrived = true;
  pthread_mutex_unlock(&self->taskArrivedMutex);
  pthread_cond_broadcast(&self->taskArrivedCond);

  for (int i = 0; i < self->threadsCreated; i++)
    pthread_join(self->threads[i], NULL);

  if ((self->taskArrivedCondInited && pthread_cond_destroy(&self->taskArrivedCond) != 0) ||
      (self->taskCompletedCondInited && pthread_cond_destroy(&self->taskCompletedCond) != 0) ||
      (self->itsSafeToContinueUwUCondInited && pthread_cond_destroy(&self->itsSafeToContinueUwUCond) != 0) ||

      (self->taskArrivedMutexInited && pthread_mutex_destroy(&self->taskArrivedMutex) != 0) ||
      (self->taskCompletedMutexInited && pthread_mutex_destroy(&self->taskCompletedMutex) != 0) ||
      (self->submitWorkMutexInited && pthread_mutex_destroy(&self->submitWorkMutex) != 0) ||
      (self->itsSafeToContinueUwUMutexInited && pthread_mutex_destroy(&self->itsSafeToContinueUwUMutex) != 0)) {
    abort();  
  }
  
  free(self->threads);
  free(self);
}

static bool submitWorkCommon(struct thread_pool* self, struct thread_pool_work_unit* workUnit, bool nonBlocking) {
  pthread_mutex_lock(&self->submitWorkMutex);
  pthread_mutex_lock(&self->taskCompletedMutex);
  if (nonBlocking) {
    if (self->numberOfIdleThreads == 0) 
      goto failure;
  } else {
    while (self->numberOfIdleThreads == 0)
      pthread_cond_wait(&self->taskCompletedCond, &self->taskCompletedMutex);
  }

  self->numberOfIdleThreads--;
  pthread_mutex_unlock(&self->taskCompletedMutex);
  
  pthread_mutex_lock(&self->taskArrivedMutex);
  assert(self->taskArrived == false);
  self->workUnit = *workUnit;
  self->taskArrived = true;
  pthread_mutex_unlock(&self->taskArrivedMutex); 
  pthread_cond_signal(&self->taskArrivedCond);
  
  pthread_mutex_lock(&self->itsSafeToContinueUwUMutex);
  while (!self->itsSafeToContinueUwU)
    pthread_cond_wait(&self->itsSafeToContinueUwUCond, &self->itsSafeToContinueUwUMutex);
  
  self->itsSafeToContinueUwU = false;
  pthread_mutex_unlock(&self->itsSafeToContinueUwUMutex);
  
  pthread_mutex_unlock(&self->submitWorkMutex); 
  return true;

  failure:
  pthread_mutex_unlock(&self->taskCompletedMutex);
  pthread_mutex_unlock(&self->submitWorkMutex);
  return false;
}

void thread_pool_submit_work(struct thread_pool* self, struct thread_pool_work_unit* workUnit) {
  submitWorkCommon(self, workUnit, false);
}

bool thread_pool_try_submit_work(struct thread_pool* self, struct thread_pool_work_unit* workUnit) {
  return submitWorkCommon(self, workUnit, true);
}

static void* worker(void* _self) {
  struct thread_pool* self = _self;
  
  while (true) { 
    // Tell that this thread is 
    // available for hiring
    pthread_mutex_lock(&self->taskCompletedMutex);
    self->numberOfIdleThreads++;
    pthread_mutex_unlock(&self->taskCompletedMutex);
    
    pthread_mutex_lock(&self->taskArrivedMutex);
    pthread_cond_broadcast(&self->taskCompletedCond);
   
    while (!self->taskArrived)
      pthread_cond_wait(&self->taskArrivedCond, &self->taskArrivedMutex);
    
    bool shuttingDown = atomic_load(&self->shuttingDown);
    if (!shuttingDown)
      self->taskArrived = false;
    
    struct thread_pool_work_unit work = self->workUnit;
    pthread_mutex_unlock(&self->taskArrivedMutex);
    
    pthread_mutex_lock(&self->itsSafeToContinueUwUMutex);
    self->itsSafeToContinueUwU = true;
    pthread_mutex_unlock(&self->itsSafeToContinueUwUMutex);
    pthread_cond_broadcast(&self->itsSafeToContinueUwUCond);

    if (shuttingDown)
      break;

    if (work.worker) {
      work.worker(&work);

      if (work.canRelease)
        Block_release(work.worker);
    }
  }

  return NULL;
}

void thread_pool_wait_all(struct thread_pool* self) {
  pthread_mutex_lock(&self->taskCompletedMutex);
  while (self->numberOfIdleThreads < self->poolSize)
    pthread_cond_wait(&self->taskCompletedCond, &self->taskCompletedMutex);
  pthread_mutex_unlock(&self->taskCompletedMutex);
}

