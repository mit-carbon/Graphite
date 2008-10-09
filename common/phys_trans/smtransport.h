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

      // The MCP should use this initialization routine instead of ptInit
      void ptInitMCP() {}

      // This does nothing but is needed so the interface matches the
      //  other versions of the PT layer
      static void ptFinish() {}

      // CommID and ThreadID are the same in this version of the PT layer
      int ptCommID() { return pt_tid; }
      int ptSend(int receiver, char *buffer, int length);
      char* ptRecv();
      bool ptQuery();
      static void ptInitQueue(int num_mod);

      // These routines are used to communicate with the central server
      //  process (known as the "MCP").  There is exactly one server for
      //  the entire simulation but the user shouldn't have to know
      //  anything about where it is or how to get to it.
      void  ptSendToMCP(UInt8* buffer, UInt32 num_bytes) {}
      UInt8* ptRecvFromMCP(UInt32* num_bytes) { return NULL; }
      // These two should only be called from the MCP
      void  ptMCPSend(UInt32 dest, UInt8* buffer, UInt32 num_bytes) {}
      UInt8* ptMCPRecv(UInt32* num_bytes) { return NULL; }
};

#endif


