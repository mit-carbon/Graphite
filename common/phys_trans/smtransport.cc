#include "smtransport.h"
#include <cassert>


Transport::PTQueue* Transport::pt_queue = NULL;
Transport::Futex* Transport::pt_futx = NULL;

Transport::MCPQueue* Transport::mcp_queue = NULL;
Transport::Futex* Transport::mcp_futx = NULL;
UInt32 Transport::mcp_idx = 0;

UINT32 Transport::s_pt_num_mod = 0;

void Transport::ptInitQueue(int num_mod)
{
   int i;
   assert(num_mod > 0);
   pt_queue = new PTQueue[num_mod];
   pt_futx = new Futex[num_mod];
   mcp_queue = new MCPQueue[num_mod+1];
   mcp_futx = new Futex[num_mod+1];
   mcp_idx = (UInt32)num_mod;
   s_pt_num_mod = (UINT32)num_mod;

   for(i = 0; i < num_mod; i++)
   {
      InitLock(&(pt_queue[i].pt_q_lock));
      pt_futx[i].futx = 1;
      InitLock(&(pt_futx[i].futx_lock));

      // Initialize the queues that the MCP will use to send messages
      //  back to the modules
      InitLock(&(mcp_queue[i].q_lock));
      mcp_futx[i].futx = 1;
      InitLock(&(mcp_futx[i].futx_lock));
   }

   // Initialize the extra queue for the MCP 
   InitLock(&(mcp_queue[mcp_idx].q_lock));
   mcp_futx[mcp_idx].futx = 1;
   InitLock(&(mcp_futx[mcp_idx].futx_lock));
   
}

int Transport::ptInit(int tid, int num_mod)
{
   pt_tid = tid;
   pt_num_mod = num_mod;
   assert((UINT32)pt_tid < s_pt_num_mod);
   assert((UINT32)pt_num_mod == s_pt_num_mod);
   InitLock(&(pt_futx[tid].futx_lock));
   i_am_the_MCP = false;
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

    // HK
    // FIXME: Should this be pt_queue.top()?
    ptr = pt_queue[pt_tid].pt_queue.front();
    pt_queue[pt_tid].pt_queue.pop();
    ReleaseLock(&(pt_queue[pt_tid].pt_q_lock));

    return ptr;
}

bool Transport::ptQuery()
{
   assert(0 <= pt_tid && pt_tid < pt_num_mod);
   return !(pt_queue[pt_tid].pt_queue.empty());
}

//********** Begin MCP communication functions **********//

void Transport::ptSendToMCP(UInt8* buffer, UInt32 num_bytes)
{
   //assert(!i_am_the_MCP);
   ptMCPSendHelper(mcp_idx, buffer, num_bytes);
}

UInt8* Transport::ptRecvFromMCP(UInt32* num_bytes)
{
   assert(!i_am_the_MCP);
   return ptMCPRecvHelper(pt_tid, num_bytes);
}

void Transport::ptMCPSend(UInt32 dest, UInt8* buffer, UInt32 num_bytes)
{
   assert(i_am_the_MCP);
   ptMCPSendHelper(dest, buffer, num_bytes);
}

UInt8* Transport::ptMCPRecv(UInt32* num_bytes)
{
   assert(i_am_the_MCP);
   return ptMCPRecvHelper(mcp_idx, num_bytes);
}

void Transport::ptMCPSendHelper(UInt32 dest, UInt8* buffer, UInt32 num_bytes)
{
   // Add an entry to the intended recipient's queue
   GetLock(&(mcp_queue[dest].q_lock), 1);
   mcp_queue[dest].q.push(new MCPEntry(buffer, num_bytes));
   ReleaseLock(&(mcp_queue[dest].q_lock));

   // Wake up the recipient to let them know it's ready
   GetLock(&(mcp_futx[dest].futx_lock), 1);
   if(mcp_futx[dest].futx == 0) {
      mcp_futx[dest].futx = 1;
      // FIXME: Make a macro for this
      syscall(SYS_futex, (void*)&(mcp_futx[dest].futx), FUTEX_WAKE, 1, NULL, NULL, 1);
   }
   ReleaseLock(&(mcp_futx[dest].futx_lock));

   return;
}

UInt8* Transport::ptMCPRecvHelper(UInt32 my_idx, UInt32* num_bytes)
{
   // Wait until a message is ready for us
   while(1) {
      GetLock(&(mcp_futx[my_idx].futx_lock), 1);      
      if(mcp_queue[my_idx].q.empty())
	 mcp_futx[my_idx].futx = 0;
      ReleaseLock(&(mcp_futx[my_idx].futx_lock));

      syscall(SYS_futex, (void*)&(mcp_futx[my_idx].futx), FUTEX_WAIT, 0, NULL, NULL, 1);

      if(!mcp_queue[my_idx].q.empty()) break;
   }

   // Retrieve the next entry from the queue   
   GetLock(&(mcp_queue[my_idx].q_lock), 1);   
   MCPEntry* entry = mcp_queue[my_idx].q.front();
   mcp_queue[my_idx].q.pop();
   ReleaseLock(&(mcp_queue[my_idx].q_lock));
   
   // Breakout the pieces of the entry and then delete it 
   UInt8* ptr = entry->first;
   *num_bytes = entry->second;
   delete entry;

   return ptr;
}
