#include "smtransport.h"
#include "debug.h"
#include <cassert>


Transport::PTQueue* Transport::pt_queue = NULL;
Transport::Futex* Transport::pt_futx = NULL;

UInt32 Transport::s_pt_num_mod = 0;

void Transport::ptInitQueue(SInt32 num_mod)
{
   SInt32 i;
   assert(num_mod > 0);
   pt_queue = new PTQueue[num_mod];
   pt_futx = new Futex[num_mod];

	/*
   mcp_queue = new MCPQueue[num_mod+1];
   mcp_futx = new Futex[num_mod+1];
   mcp_idx = (UInt32)num_mod;
	 */

   s_pt_num_mod = (UInt32)num_mod;

   for(i = 0; i < num_mod; i++)
   {
      InitLock(&(pt_queue[i].pt_q_lock));
      pt_futx[i].futx = 1;
      InitLock(&(pt_futx[i].futx_lock));

      // Initialize the queues that the MCP will use to send messages
      //  back to the modules
		/*
      InitLock(&(mcp_queue[i].q_lock));
      mcp_futx[i].futx = 1;
      InitLock(&(mcp_futx[i].futx_lock));
		 */
   }

   // Initialize the extra queue for the MCP
	/*
   InitLock(&(mcp_queue[mcp_idx].q_lock));
   mcp_futx[mcp_idx].futx = 1;
   InitLock(&(mcp_futx[mcp_idx].futx_lock));
	 */
   
}

SInt32 Transport::ptInit(SInt32 tid, SInt32 num_mod)
{
   pt_tid = tid;
   pt_num_mod = num_mod;
   assert((UInt32)pt_tid < s_pt_num_mod);
   assert((UInt32)pt_num_mod == s_pt_num_mod);
   InitLock(&(pt_futx[tid].futx_lock));
   return 0;
}

SInt32 Transport::ptSend(SInt32 receiver, void *buffer, SInt32 size)
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

void* Transport::ptRecv()
{     
   void *ptr;
   assert(0 <= pt_tid && pt_tid < pt_num_mod);

//		cerr << "[" << pt_tid << "] TRANSPORT: before WHILE LOOP, inside ptRecv" << endl;
   while(1)
   {
      GetLock(&(pt_futx[pt_tid].futx_lock), 1);
//   	cerr << "[" << pt_tid << "] TRANSPORT: before futex, gotten lock 1" << endl;

      if(pt_queue[pt_tid].pt_queue.empty())
         pt_futx[pt_tid].futx = 0;

      ReleaseLock(&(pt_futx[pt_tid].futx_lock));
   	// debugPrint (pt_tid, "TRANSPORT", "before SYSCALL futex");

      syscall(SYS_futex, (void*)&(pt_futx[pt_tid].futx), FUTEX_WAIT, 0, NULL, NULL, 1);
   	// debugPrint (pt_tid, "TRANSPORT", "after SYSCALL futex");
      if(!pt_queue[pt_tid].pt_queue.empty())
         break;
    }
                                                           
	// cerr << "[" << pt_tid << "] TRANSPORT: after futex, getting lock " << endl;
   GetLock(&(pt_queue[pt_tid].pt_q_lock), 1);


   // FIXME: Should this be pt_queue.top()?
	ptr = pt_queue[pt_tid].pt_queue.front();
   pt_queue[pt_tid].pt_queue.pop();
   ReleaseLock(&(pt_queue[pt_tid].pt_q_lock));                               
	//	cerr << "[" << pt_tid << "] TRANSPORT: after futex, releasing lock , RETURNING from ptRecv" << endl;

    return ptr;
}

Boolean Transport::ptQuery()
{
	//original code
   assert(0 <= pt_tid && pt_tid < pt_num_mod);
   return !(pt_queue[pt_tid].pt_queue.empty());
}
