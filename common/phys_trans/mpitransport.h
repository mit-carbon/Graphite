// Jason Miller
//
// Version of the physical transport layer that uses MPI to communicate
// between cores.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <errno.h>
#include <assert.h>
#include "mpi.h"
#include "config.h"
#include "fixed_types.h"

//#define PHYS_TRANS_USE_LOCKS

class Lock;

class Transport {
   private:
      SInt32 pt_tid;
      SInt32 comm_id;

      //***** Private helper functions *****//

      // Return the process number for this process.  Process numbers are
      //  integers between 0 and (g_config->numProcs() - 1), inclusive.
      static UInt32 ptProcessNum();

#ifdef PHYS_TRANS_USE_LOCKS
      static Lock *pt_lock;
#endif

   public:
      // This routine should be called once within in each process.
      static void ptGlobalInit();

      // This routine should be called once within each thread.
      SInt32 ptInit(SInt32 tid, SInt32 num_mod);

      // The MCP should use this initialization routine instead of ptInit
      void ptInitMCP();

      // This routine should be called once when everything is done
      static void ptFinish() { MPI_Finalize(); }

      // Return the communications ID for this node
      SInt32 ptCommID() { return comm_id; }

      // Send a message to another core.  This call returns immediately.
      SInt32 ptSend(SInt32 receiver, void *buffer, SInt32 length);

      // Receive the next incoming message from any sender.  This call is
      //  blocking and will not return until a message has been received.
      void* ptRecv();

      // Returns TRUE if there is a message waiting to be received.
      Boolean ptQuery();
};

#endif


