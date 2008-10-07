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


class Transport{
   private:
      int pt_tid;
      int pt_num_mod;
      typedef struct PTQueue
      {
         queue <char*, deque<char*> > pt_queue;
	 PIN_LOCK pt_q_lock;
      } PTQueue;

      static PTQueue *pt_queue;
	
      typedef struct Futex
      {
         int futx;
	 PIN_LOCK futx_lock;
      } Futex;

      static Futex *pt_futx;

      static UINT32 s_pt_num_mod; // used for bounds checking only
	
   public:	
      int ptInit(int tid, int num_mod);
      int ptSend(int receiver, char *buffer, int length);
      char* ptRecv();
      bool ptQuery();
      static void ptInitQueue(int num_mod);
};

#endif


