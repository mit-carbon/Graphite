#include "smtransport.h"
#include <cassert>


Transport::PTQueue* Transport::pt_queue = NULL;
Transport::Futex* Transport::pt_futx = NULL;

void Transport::ptInitQueue(SInt32 num_mod)
{
   SInt32 i;
   assert(num_mod > 0);
   pt_queue = new PTQueue[num_mod];
   pt_futx = new Futex[num_mod];

   for (i = 0; i < num_mod; i++)
   {
      InitLock(&(pt_queue[i].pt_q_lock));
      pt_futx[i].futx = 1;
      InitLock(&(pt_futx[i].futx_lock));

   }

}

Transport::Transport(SInt32 tid)
{
   pt_tid = tid;
   InitLock(&(pt_futx[tid].futx_lock));
}

SInt32 Transport::ptSend(SInt32 receiver, void *buffer, SInt32 size)
{
   // memory will be deleted by network, so we must copy it
   Byte *bufcopy = new Byte[size];
   memcpy(bufcopy, buffer, size);

   GetLock(&(pt_queue[receiver].pt_q_lock), 1);
   pt_queue[receiver].pt_queue.push(bufcopy);
   ReleaseLock(&(pt_queue[receiver].pt_q_lock));

   GetLock(&(pt_futx[receiver].futx_lock), 1);

   if (pt_futx[receiver].futx == 0)
   {
      pt_futx[receiver].futx = 1;

      // FIXME: Make a macro for this
      syscall(SYS_futex, (void*)&(pt_futx[receiver].futx), FUTEX_WAKE, 1, NULL, NULL, 1);
   }

   ReleaseLock(&(pt_futx[receiver].futx_lock));
   return size;
}

void* Transport::ptRecv()
{
   void *ptr;

   while (1)
   {
      GetLock(&(pt_futx[pt_tid].futx_lock), 1);

      if (pt_queue[pt_tid].pt_queue.empty())
         pt_futx[pt_tid].futx = 0;

      ReleaseLock(&(pt_futx[pt_tid].futx_lock));

      syscall(SYS_futex, (void*)&(pt_futx[pt_tid].futx), FUTEX_WAIT, 0, NULL, NULL, 1);
      if (!pt_queue[pt_tid].pt_queue.empty())
         break;
   }

   GetLock(&(pt_queue[pt_tid].pt_q_lock), 1);

   ptr = pt_queue[pt_tid].pt_queue.front();
   pt_queue[pt_tid].pt_queue.pop();
   ReleaseLock(&(pt_queue[pt_tid].pt_q_lock));

   return ptr;
}

Boolean Transport::ptQuery()
{
   //original code
   return !(pt_queue[pt_tid].pt_queue.empty());
}
