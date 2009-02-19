#ifndef SMTRANSPORT_H
#define SMTRANSPORT_H

#include "transport.h"

class SmTransport : public Transport
{
public:
   SmTransport();
   ~SmTransport();

   class Node : public Transport::Node
   {
   public:
      Node(SInt32 core_id);
      ~Node();

      void send(SInt32, void*, UInt32);
      Byte* recv();
      bool query();
   };

   Node* createNode(SInt32 core_id);

   void barrier();
   Node* getGlobalNode();
};

#endif

#if 0

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

class Transport
{
   public:
      Transport(SInt32 core_id);

      // This routine should be called once within in each process.
      static void ptGlobalInit();

      // This does nothing but is needed so the interface matches the
      //  other versions of the PT layer
      static void ptFinish() {}

      //FIXME: this should become a phtread barrier
      static void ptBarrier() {}


      SInt32 ptSend(SInt32 receiver, void *buffer, SInt32 length);
      void* ptRecv();
      Boolean ptQuery();

   private:
      SInt32 pt_tid;

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

};

#endif


