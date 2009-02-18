#include <cassert>
#include "smtransport.h"
#include "log.h"

#define LOG_DEFAULT_RANK pt_tid
#define LOG_DEFAULT_MODULE SMTRANSPORT

Transport::PTQueue* Transport::pt_queue = NULL;
Transport::Futex* Transport::pt_futx = NULL;

void Transport::ptGlobalInit()
{
   g_config->setProcessNum(0);
   LOG_PRINT_EXPLICIT(-1, SMTRANSPORT, "SM Transport::ptGlobalInit");

   pt_queue = new PTQueue[g_config->getTotalCores()];
   pt_futx = new Futex[g_config->getTotalCores()];

   for (UInt32 i = 0; i < g_config->getTotalCores(); i++)
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
