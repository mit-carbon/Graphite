// Includes temporary hooks for the syscall server. Otherwise, this is
// the syscall server. The runSyscallServer hook function is to be
// manually inserted in main thread of the user app. It calls the code
// the implements the real server. Putting the hook in the user code's
// main thread gives the server a place to run; we avoid having to spawn
// a thread or something in the simulator to house the server code.

#ifndef SYSCALL_SERVER_H
#define SYSCALL_SERVER_H

#include <iostream>
#include <map>
#include <queue>

// -- For futexes --
#include <linux/futex.h>
#include <sys/time.h>
#include <errno.h>

#include "packetize.h"
#include "transport.h"
#include "fixed_types.h"
#include "network.h"

// -- Special Class to Handle Futexes
class SimFutex
{
private:
   typedef std::queue<core_id_t> ThreadQueue;
   ThreadQueue m_waiting;

public:
   SimFutex();
   ~SimFutex();
   void enqueueWaiter(core_id_t core_id);
   core_id_t dequeueWaiter();
};

class SyscallServer
{
public:
   SyscallServer(Network &network,
                 UnstructuredBuffer &send_buff_, UnstructuredBuffer &recv_buff_,
                 const UInt32 SERVER_MAX_BUFF,
                 char *scratch_);
   ~SyscallServer();

   void handleSyscall(core_id_t core_id);

private:
   void marshallOpenCall(core_id_t core_id);
   void marshallReadCall(core_id_t core_id);
   void marshallWriteCall(core_id_t core_id);
   void marshallWritevCall(core_id_t core_id);
   void marshallCloseCall(core_id_t core_id);
   void marshallLseekCall(core_id_t core_id);
   void marshallAccessCall(core_id_t core_id);
   void marshallStatCall(IntPtr syscall_number, core_id_t core_id);
   void marshallFstatCall(core_id_t core_id);
   void marshallIoctlCall(core_id_t core_id);
   void marshallGetpidCall(core_id_t core_id);
   void marshallReadaheadCall(core_id_t core_id);
   void marshallPipeCall(core_id_t core_id);
   void marshallMmapCall(core_id_t core_id);
   void marshallMunmapCall(core_id_t core_id);
   void marshallBrkCall(core_id_t core_id);
   void marshallFutexCall(core_id_t core_id);

   void marshallRmdirCall(core_id_t core_id);
   void marshallUnlinkCall(core_id_t core_id);
   void marshallGetCwdCall(core_id_t core_id);
   void marshallSchedSetAffinityCall(core_id_t core_id);
   void marshallSchedGetAffinityCall(core_id_t core_id);

   // Handling Futexes
   void futexWait(core_id_t core_id, int *addr, int val, UInt64 curr_time);
#ifdef KERNEL_SQUEEZE
   void futexWaitClockReal(core_id_t core_id, int *addr, int val, UInt64 curr_time);
#endif
   void futexWake(core_id_t core_id, int *addr, int val, UInt64 curr_time);
   void futexWakeOp(core_id_t core_id, int *addr1, int val1, int val2, int *addr2, int val3, UInt64 curr_time);
   void futexCmpRequeue(core_id_t core_id, int *addr1, int val1, int val2, int *addr2, int val3, UInt64 curr_time);

   int __futexWake(int *addr, int val, UInt64 curr_time);

   // Private data fields
   // Note: These structures are shared with the MCP
   Network & m_network;
   UnstructuredBuffer & m_send_buff;
   UnstructuredBuffer & m_recv_buff;
   const UInt32 m_SYSCALL_SERVER_MAX_BUFF;
   char * const m_scratch;

   // Handling Futexes
   typedef std::map<IntPtr, SimFutex> FutexMap;
   FutexMap m_futexes;
};

#endif
