// Jason Miller
//
// Version of the physical transport layer that uses MPI to communicate
// between cores.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <errno.h>
#include <assert.h>
#include "fixed_types.h"

//#define PHYS_TRANS_USE_LOCKS

class Lock;

class Transport
{
   public:
      // This routine should be called once within in each process.
      static void ptGlobalInit();

      // This routine should be called once within each thread.
      Transport(SInt32 core_id);

      // This routine should be called once when everything is done
      static void ptFinish();

      // This routine should be called once when everything is done
      static void ptBarrier();

      // Send a message to another core.  This call returns immediately.
      SInt32 ptSend(SInt32 receiver, void *buffer, SInt32 length);

      // Receive the next incoming message from any sender.  This call is
      //  blocking and will not return until a message has been received.
      void* ptRecv();

      // Returns TRUE if there is a message waiting to be received.
      Boolean ptQuery();

   private:
      const SInt32 m_core_id;

#ifdef PHYS_TRANS_USE_LOCKS
      static Lock *pt_lock;
#endif
};

#endif


