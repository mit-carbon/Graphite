#ifndef __CLOCK_SKEW_MANAGEMENT_H__
#define __CLOCK_SKEW_MANAGEMENT_H__

#include "pin.H"

void handlePeriodicSync(THREADID thread_id);
void addPeriodicSync(INS ins);

#endif /* __CLOCK_SKEW_MANAGEMENT_H__ */
