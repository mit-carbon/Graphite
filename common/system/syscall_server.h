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

#include "packetize.h"
#include "transport.h"
#include "fixed_types.h"
#include "network.h"

// -- Special Class to Handle Futexes
class SimFutex
{
   private:
      typedef std::queue<tile_id_t> ThreadQueue;
      ThreadQueue m_waiting;

   public:
      SimFutex();
      ~SimFutex();
      void enqueueWaiter(tile_id_t core_id);
      tile_id_t dequeueWaiter();
};

class SyscallServer
{
   public:
      SyscallServer(Network &network,
                    UnstructuredBuffer &send_buff_, UnstructuredBuffer &recv_buff_,
                    const UInt32 SERVER_MAX_BUFF,
                    char *scratch_);

      ~SyscallServer();

      void handleSyscall(tile_id_t core_id);

   private:
      void marshallOpenCall(tile_id_t core_id);
      void marshallReadCall(tile_id_t core_id);
      void marshallWriteCall(tile_id_t core_id);
      void marshallWritevCall(tile_id_t core_id);
      void marshallCloseCall(tile_id_t core_id);
      void marshallLseekCall(tile_id_t core_id);
      void marshallAccessCall(tile_id_t core_id);
#ifdef TARGET_X86_64
      void marshallStatCall(IntPtr syscall_number, tile_id_t core_id);
      void marshallFstatCall(tile_id_t core_id);
#endif
#ifdef TARGET_IA32
      void marshallFstat64Call(tile_id_t core_id);
#endif
      void marshallIoctlCall(tile_id_t core_id);
      void marshallGetpidCall(tile_id_t core_id);
      void marshallReadaheadCall(tile_id_t core_id);
      void marshallPipeCall(tile_id_t core_id);
      void marshallMmapCall(tile_id_t core_id);
#ifdef TARGET_IA32
      void marshallMmap2Call(tile_id_t core_id);
#endif
      void marshallMunmapCall(tile_id_t core_id);
      void marshallBrkCall(tile_id_t core_id);
      void marshallFutexCall(tile_id_t core_id);

      // Handling Futexes 
      void futexWait(tile_id_t core_id, int *uaddr, int val, int act_val, UInt64 curr_time);
      void futexWake(tile_id_t core_id, int *uaddr, int val, UInt64 curr_time);
      void futexCmpRequeue(tile_id_t core_id, int *uaddr, int val, int *uaddr2, int val3, int act_val, UInt64 curr_time);

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
