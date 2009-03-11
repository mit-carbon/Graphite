divert(-1)
define(NEWPROC,) dnl

define(BARRIER, `{
	barrierWait(&($1));
}')

define(BARDEC, `
carbon_barrier_t ($1);
')

define(BARINIT, `{
	barrierInit(&($1), $2);
}')

define(BAREXCLUDE, `{;}')

define(BARINCLUDE, `{;}')

define(GSDEC, `long ($1);')
define(GSINIT, `{ ($1) = 0; }')
define(GETSUB, `{
  if (($1)<=($3))
    ($2) = ($1)++;
  else {
    ($2) = -1;
    ($1) = 0;
  }
}')

define(NU_GSDEC, `long ($1);')
define(NU_GSINIT, `{ ($1) = 0; }')
define(NU_GETSUB, `GETSUB($1,$2,$3,$4)')

define(ADEC, `long ($1);')
define(AINIT, `{;}')
define(PROBEND, `{;}')

define(LOCKDEC, `carbon_mutex_t ($1);')
define(LOCKINIT, `{mutexInit(&($1));}')
define(LOCK, `{mutexLock(&($1));}')
define(UNLOCK, `{mutexUnlock(&($1));}')

define(NLOCKDEC, `long ($1);')
define(NLOCKINIT, `{;}')
define(NLOCK, `{;}')
define(NUNLOCK, `{;}')

define(ALOCKDEC, `carbon_mutex_t $1[$2];')
define(ALOCKINIT, `{
	unsigned long	i, Error;

	for (i = 0; i < $2; i++) {
		mutexInit(&$1[i]);
	}
}')
define(ALOCK, `{mutexLock(&$1[$2]);}')
define(AULOCK, `{mutexUnlock(&$1[$2]);}')

define(PAUSEDEC, `
struct {
	carbon_mutex_t	Mutex;
	carbon_cond_t	CondVar;
	unsigned long	Flag;
} $1;
')
define(PAUSEINIT, `{
	mutexInit(&$1.Mutex, NULL);
	condInit(&$1.CondVar, NULL);
	$1.Flag = 0;
}
')
define(CLEARPAUSE, `{
	$1.Flag = 0;
   mutexUnlock(&$1.Mutex);}
')
define(SETPAUSE, `{
	mutexLock(&$1.Mutex);
	$1.Flag = 1;
	condBroadcast(&$1.CondVar);
	mutexUnlock(&$1.Mutex);}
')
define(EVENT, `{;}')
define(WAITPAUSE, `{
	mutexLock(&$1.Mutex);
	if ($1.Flag == 0) {
		condWait(&$1.CondVar, &$1.Mutex);
	}
}')
define(PAUSE, `{;}')

define(AUG_ON, ` ')
define(AUG_OFF, ` ')
define(TRACE_ON, ` ')
define(TRACE_OFF, ` ')
define(REF_TRACE_ON, ` ')
define(REF_TRACE_OFF, ` ')
define(DYN_TRACE_ON, `;')
define(DYN_TRACE_OFF, `;')
define(DYN_REF_TRACE_ON, `;')
define(DYN_REF_TRACE_OFF, `;')
define(DYN_SIM_ON, `;')
define(DYN_SIM_OFF, `;')
define(DYN_SCHED_ON, `;')
define(DYN_SCHED_OFF, `;')
define(AUG_SET_LOLIMIT, `;')
define(AUG_SET_HILIMIT, `;')

define(MENTER, `{;}')
define(DELAY, `{;}')
define(CONTINUE, `{;}')
define(MEXIT, `{;}')
define(MONINIT, `{;}')

define(WAIT_FOR_END, `{
	unsigned long	i, Error;
	for (i = 0; i < ($1) - 1; i++) {
		CarbonJoinThread(ThreadTable[i]);
	}
}')

define(CREATE, `{
	long	i, Error;

	for (i = 0; i < ($2) - 1; i++) {
		ThreadTable[i] = CarbonSpawnThread((void * (*)(void *))($1), NULL);
	}

	$1();
}')

define(MAIN_INITENV, `{
      }')
define(MAIN_END, `{
      exit(0);
      }')

define(MAIN_ENV,`
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sync_api.h>
#include <capi.h>
#define MAX_THREADS 32
carbon_thread_t ThreadTable[MAX_THREADS];
')

define(ENV, ` ')
define(EXTERN_ENV, `
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sync_api.h>
#include <capi.h>
extern carbon_thread_t ThreadTable[];
')

define(THREAD_INIT, `{
  }')

define(THREAD_INIT_FREE, `{
  }')

define(G_MALLOC, `valloc($1);')
define(G_FREE, `;')
define(G_MALLOC_F, `valloc($1)')
define(NU_MALLOC, `valloc($1);')
define(NU_FREE, `;')
define(NU_MALLOC_F, `valloc($1)')

define(GET_HOME, `{($1) = 0;}')
define(GET_PID, `{($1) = 0;}')
define(AUG_DELAY, `{sleep ($1);}')
define(ST_LOG, `{;}')
define(SET_HOME, `{;}')
define(CLOCK, `{
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	($1) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
}')
divert(0)
