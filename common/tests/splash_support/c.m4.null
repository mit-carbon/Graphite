divert(-1)
define(NEWPROC,) dnl

define(BARRIER, `{;}')
define(BARDEC, `long ($1);')
define(BARINIT, `{;}')

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

define(LOCKDEC, `long ($1);')
define(LOCKINIT, `{;}')
define(LOCK, `{;}')
define(UNLOCK, `{;}')

define(NLOCKDEC, `long ($1);')
define(NLOCKINIT, `{;}')
define(NLOCK, `{;}')
define(NUNLOCK, `{;}')

define(ALOCKDEC, `long ($1);')
define(ALOCKINIT, `{;}')
define(ALOCK, `{;}')
define(AULOCK, `{;}')

define(PAUSEDEC, ` ')
define(PAUSEINIT, `{;}')
define(CLEARPAUSE, `{;}')
define(SETPAUSE, `{;}')
define(EVENT, `{;}')
define(WAITPAUSE, `{;}')
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

define(WAIT_FOR_END, `{;}')

define(CREATE, `{fprintf(stderr, "No more processors -- this is a uniprocessor version!\n"); exit(-1);}')
define(MAIN_INITENV, `{;}')
define(MAIN_END, `{exit(0);}')
define(MAIN_ENV,` ')
define(ENV, ` ')
define(EXTERN_ENV, ` ')

define(G_MALLOC, `malloc($1);')
define(G_FREE, `;')
define(G_MALLOC_F, `malloc($1)')
define(NU_MALLOC, `malloc($1);')
define(NU_FREE, `;')
define(NU_MALLOC_F, `malloc($1)')

define(GET_HOME, `{($1) = 0;}')
define(GET_PID, `{($1) = 0;}')
define(AUG_DELAY, `{sleep ($1);}')
define(ST_LOG, `{;}')
define(SET_HOME, `{;}')
define(CLOCK, `{long time(); ($1) = time(0);}')
divert(0)
