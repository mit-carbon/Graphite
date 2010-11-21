using namespace std;

#include "req_queue_list.h"
#include "log.h"

namespace PrL1PrL1PrL2DramDirectoryMSI
{
   ReqQueueList::ReqQueueList() {}

   ReqQueueList::~ReqQueueList() {}

   void
   ReqQueueList::enqueue(IntPtr address, ShmemReq* shmem_req)
   {
      if (m_req_queue_list[address] == NULL)
      {
         m_req_queue_list[address] = new queue<ShmemReq*>();
      }
      m_req_queue_list[address]->push(shmem_req);
   }

   ShmemReq*
   ReqQueueList::dequeue(IntPtr address)
   {
      LOG_ASSERT_ERROR(m_req_queue_list[address] != NULL,
            "Could not find a Shmem request with address(0x%x)", address);

      ShmemReq* shmem_req = m_req_queue_list[address]->front();
      m_req_queue_list[address]->pop();
      if (m_req_queue_list[address]->empty())
      {
         delete m_req_queue_list[address];
         m_req_queue_list[address] = NULL;
      }
      return shmem_req;
   }

   ShmemReq*
   ReqQueueList::front(IntPtr address)
   {
      LOG_ASSERT_ERROR(m_req_queue_list[address] != NULL,
            "Could not find a Shmem request with address(0x%x)", address);

      return m_req_queue_list[address]->front();
   }

   UInt32
   ReqQueueList::size(IntPtr address)
   {
      if (m_req_queue_list[address] == NULL)
         return 0;
      else
         return m_req_queue_list[address]->size();
   }

   bool
   ReqQueueList::empty(IntPtr address)
   {
      if (m_req_queue_list[address] == NULL)
         return true;
      else
         return false;
   }
}
