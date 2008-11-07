#include "smtransport.h"
#include <cassert>


Transport::PTQueue* Transport::pt_queue = NULL;

Transport::Futex* Transport::pt_futx = NULL;

UINT32 Transport::s_pt_num_mod = 0;

void Transport::ptInitQueue(int num_mod)
{
   int i;
   assert(num_mod > 0);
   pt_queue = new PTQueue[num_mod];
   pt_futx = new Futex[num_mod];
   s_pt_num_mod = (UINT32)num_mod;

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
   assert((UINT32)pt_tid < s_pt_num_mod);
   assert((UINT32)pt_num_mod == s_pt_num_mod);
   InitLock(&(pt_futx[tid].futx_lock));
   return 0;
}

int Transport::ptSend(int receiver, char *buffer, int size)
{
   assert(0 <= receiver && receiver < pt_num_mod);
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
   assert(0 <= pt_tid && pt_tid < pt_num_mod);

//		cerr << "[" << pt_tid << "] TRANSPORT: before WHILE LOOP, inside ptRecv" << endl;
   while(1)
   {
      GetLock(&(pt_futx[pt_tid].futx_lock), 1);
//   	cerr << "[" << pt_tid << "] TRANSPORT: before futex, gotten lock 1" << endl;

      if(pt_queue[pt_tid].pt_queue.empty())
         pt_futx[pt_tid].futx = 0;

      ReleaseLock(&(pt_futx[pt_tid].futx_lock));
   	cerr << "[" << pt_tid << "] TRANSPORT: before SYSCALL futex" << endl;
//both cores are freezing before syscall for futex.
      syscall(SYS_futex, (void*)&(pt_futx[pt_tid].futx), FUTEX_WAIT, 0, NULL, NULL, 1);
   	cerr << "[" << pt_tid << "] TRANSPORT: after SYSCALL futex" << endl;
      if(!pt_queue[pt_tid].pt_queue.empty())
         break;
    }
                                                           
//	cerr << "[" << pt_tid << "] TRANSPORT: after futex, getting lock " << endl;
    GetLock(&(pt_queue[pt_tid].pt_q_lock), 1);



    // HK
    // FIXME: Should this be pt_queue.top()?
	 ptr = pt_queue[pt_tid].pt_queue.front();
    pt_queue[pt_tid].pt_queue.pop();
    ReleaseLock(&(pt_queue[pt_tid].pt_q_lock));                               
//	cerr << "[" << pt_tid << "] TRANSPORT: after futex, releasing lock , RETURNING from ptRecv" << endl;

    return ptr;
}

bool Transport::ptQuery()
{
	//original code
//   assert(0 <= pt_tid && pt_tid < pt_num_mod);
//   return !(pt_queue[pt_tid].pt_queue.empty());
   
	//debugging version
	assert(0 <= pt_tid && pt_tid < pt_num_mod);
//	g_chip->getGlobalLock();
   bool ret = !(pt_queue[pt_tid].pt_queue.empty());
//	g_chip->releaseGlobalLock();
   return ret;
}
