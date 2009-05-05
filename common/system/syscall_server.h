// Jonathan Eastep, Charles Gruenwald, Jason Miller
//
// FIXME: this is a hack.
//
// Includes temporary hooks for the syscall server. Otherwise, this is
// the syscall server. The runSyscallServer hook function is to be
// manually inserted in main thread of the user app. It calls the code
// the implements the real server. Putting the hook in the user code's
// main thread gives the server a place to run; we avoid having to spawn
//
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

#include "shmem_req_types.h"
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
      void marshallCloseCall(core_id_t core_id);
      void marshallAccessCall(core_id_t core_id);
      void marshallMmapCall(core_id_t core_id);
      void marshallMmap2Call(core_id_t core_id);
      void marshallMunmapCall(core_id_t core_id);
      void marshallBrkCall(core_id_t core_id);
      void marshallFutexCall(core_id_t core_id);

      // Handling Futexes 
      void futexWait(core_id_t core_id, int *uaddr, int val, int act_val);
      void futexWake(core_id_t core_id, int *uaddr, int val);

      //Note: These structures are shared with the MCP
   private:
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
