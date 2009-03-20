#ifndef THREAD_SUPPORT_PRIVATE_H
#define THREAD_SUPPORT_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

void CarbonGetThreadToSpawn(ThreadSpawnRequest **req);
void CarbonThreadStart(ThreadSpawnRequest *req);
void CarbonThreadExit();
void *CarbonSpawnManagedThread(void *p);
void *CarbonThreadSpawner(void *p);
int CarbonSpawnThreadSpawner();

int CarbonPthreadCreate(pthread_t *tid, int *attr, thread_func_t func, void *arg);
int CarbonPthreadJoin(pthread_t tid, void **pparg);

// A thin wrapper around pthread_create when we need it to be actually
// called for the simulator
int CarbonPthreadCreateWrapper(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void *), void * arg);

#ifdef __cplusplus
}
#endif

#endif
