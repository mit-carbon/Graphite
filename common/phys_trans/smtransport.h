// Harshad Kasture
//

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <sched.h>
#include <syscall.h>
#include <linux/futex.h>
#include <linux/kernel.h>
#include <unistd.h>
#include <errno.h>
#include <queue>
#include "pin.H"
#include "config.h"
#include "fixed_types.h"

class Transport{
   private:
      SInt32 pt_tid;
      SInt32 pt_num_mod;

      //***** Data structures for normal communication *****//
      typedef struct PTQueue
      {
         queue <char*, deque<void*> > pt_queue;
			PIN_LOCK pt_q_lock;
      } PTQueue;

      static PTQueue *pt_queue;

      typedef struct Futex
      {
         SInt32 futx;
			PIN_LOCK futx_lock;
      } Futex;

      static Futex *pt_futx;

      //***** Miscellaneous *****//
      static UInt32 s_pt_num_mod; // used for bounds checking only
	
   public:	
      SInt32 ptInit(SInt32 tid, SInt32 num_mod);

      // This does nothing but is needed so the interface matches the
      //  other versions of the PT layer
      static void ptFinish() {}

      // CommID and ThreadID are the same in this version of the PT layer
      SInt32 ptCommID() { return pt_tid; }
		

      SInt32 ptSend(SInt32 receiver, void *buffer, SInt32 length);
      void* ptRecv();
      Boolean ptQuery();
      static void ptInitQueue(SInt32 num_mod);
};

#endif


