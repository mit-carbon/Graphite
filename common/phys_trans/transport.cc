#include "transport.h"


Transport::PTQueue* Transport::pt_queue = NULL;

Transport::Futex* Transport::pt_futx = NULL;

void Transport::ptInitQueue(int num_mod)
{
   int i;
   pt_queue = new PTQueue[num_mod];
   pt_futx = new Futex[num_mod];

   for(i = 0; i < num_mod; i++)
   {
      InitLock(&(pt_queue[i].pt_q_lock));
      pt_futx[i].futx = 1;
      InitLock(&(pt_futx[i].futx_lock));
   }
}

int Transport::ptInit(int tid, int num_mod)
{
   pt_tid = tid;
   pt_num_mod = num_mod;
   InitLock(&(pt_futx[tid].futx_lock));
   return 0;
}

int Transport::ptSend(int receiver, char *buffer, int size)
{
   GetLock(&(pt_queue[receiver].pt_q_lock), 1);
   pt_queue[receiver].pt_queue.push(buffer);
   ReleaseLock(&(pt_queue[receiver].pt_q_lock));

   GetLock(&(pt_futx[receiver].futx_lock), 1);

   if(pt_futx[receiver].futx == 0){
      pt_futx[receiver].futx = 1;

      // FIXME: Make a macro for this
      syscall(SYS_futex, (void*)&(pt_futx[receiver].futx), FUTEX_WAKE, 1, NULL, NULL, 1);
   }

   ReleaseLock(&(pt_futx[receiver].futx_lock));
   return size;
}

char* Transport::ptRecv()
{
   char *ptr;
   while(1)
   {
      GetLock(&(pt_futx[pt_tid].futx_lock), 1);

      if(pt_queue[pt_tid].pt_queue.empty())
         pt_futx[pt_tid].futx = 0;

      ReleaseLock(&(pt_futx[pt_tid].futx_lock));
      syscall(SYS_futex, (void*)&(pt_futx[pt_tid].futx), FUTEX_WAIT, 0, NULL, NULL, 1);
      if(!pt_queue[pt_tid].pt_queue.empty())
         break;
    }

    GetLock(&(pt_queue[pt_tid].pt_q_lock), 1);
    ptr = pt_queue[pt_tid].pt_queue.front();
    pt_queue[pt_tid].pt_queue.pop();
    ReleaseLock(&(pt_queue[pt_tid].pt_q_lock));

    return ptr;
}

bool Transport::ptQuery()
{
   return !(pt_queue[pt_tid].pt_queue.empty());
}
