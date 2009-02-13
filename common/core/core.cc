#include "core.h"

#include "network.h"
#include "ocache.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager.h"

#include "log.h"
#define LOG_DEFAULT_RANK   m_core_id
#define LOG_DEFAULT_MODULE CORE

using namespace std;

Core::Core(SInt32 id)
{
   m_core_id = id;

   m_network = new Network(this);

   if ( g_config->getEnablePerformanceModeling() ) 
   {
      m_perf_model = new PerfModel("performance modeler");
      LOG_PRINT("Instantiated performance model.");
   }
   else
   {
      m_perf_model = (PerfModel *) NULL;
   }

   if ( g_config->getEnableDCacheModeling() || g_config->getEnableICacheModeling() )
   {
      m_ocache = new OCache("organic cache");
      LOG_PRINT("instantiated organic cache model");
   }
   else
   {
      m_ocache = (OCache *) NULL;
   }

   if ( g_config->isSimulatingSharedMemory() ) {
      assert( g_config->getEnableDCacheModeling() );

      LOG_PRINT("instantiated memory manager model");
      m_memory_manager = new MemoryManager(this, m_ocache);
   } 
   else
   {
      m_memory_manager = (MemoryManager *) NULL;
      LOG_PRINT("No Memory Manager being used");
   }

   m_syscall_model = new SyscallMdl(m_network);
   m_sync_client = new SyncClient(this);
}

Core::~Core()
{
   delete m_sync_client;
   delete m_syscall_model;
   delete m_ocache;
   delete m_perf_model;
   delete m_network;
}

int Core::coreSendW(int sender, int receiver, char *buffer, int size)
{
    // Create a net packet
    NetPacket packet;
    packet.sender= sender;
    packet.receiver= receiver;
    packet.type = USER;
    packet.length = size;
    packet.data = buffer;

    SInt32 sent = m_network->netSend(packet);

    assert(sent == size);

    return sent == size ? 0 : -1;
}

int Core::coreRecvW(int sender, int receiver, char *buffer, int size)
{
   NetPacket packet;

   packet = m_network->netRecv(sender, USER);

   LOG_PRINT("Got packet: from %i, to %i, type %i, len %i", packet.sender, packet.receiver, (SInt32)packet.type, packet.length);

   assert((unsigned)size == packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ?? 
   delete [] (Byte*)packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}


// organic cache wrappers

bool Core::icacheRunLoadModel(IntPtr i_addr, UInt32 size)
{ return m_ocache->runICacheLoadModel(i_addr, size).first; }

/*
 * dcacheRunModel (mem_operation_t operation, IntPtr d_addr, char* data_buffer, UInt32 data_size)
 *
 * Arguments:
 *   d_addr :: address of location we want to access (read or write)
 *   shmem_req_t :: READ or WRITE
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *
 * Return Value:
 *   hit :: Say whether there has been at least one cache hit or not
 */
bool Core::dcacheRunModel(CacheBase::AccessType operation, IntPtr d_addr, char* data_buffer, UInt32 data_size)
{
    shmem_req_t shmem_operation;

    if (operation == CacheBase::k_ACCESS_TYPE_LOAD) {
        shmem_operation = READ;
    }
    else {
        shmem_operation = WRITE;
    }

    if (g_config->isSimulatingSharedMemory()) {
        LOG_PRINT("%s - ADDR: %x, data_size: %u, END!!", ((operation==CacheBase::k_ACCESS_TYPE_LOAD) ? " READ " : " WRITE "), d_addr, data_size);

        bool all_hits = true;

        if (data_size <= 0) {
            return (true);
            // TODO: this is going to affect the statistics even though no shared_mem action is taking place
        }

        IntPtr begin_addr = d_addr;
        IntPtr end_addr = d_addr + data_size;
        IntPtr begin_addr_aligned = begin_addr - (begin_addr % m_ocache->dCacheLineSize());
        IntPtr end_addr_aligned = end_addr - (end_addr % m_ocache->dCacheLineSize());
        char *curr_data_buffer_head = data_buffer;

        //TODO set the size parameter correctly, based on the size of the data buffer
        //TODO does this spill over to another line? should shared_mem test look at other DRAM entries?
        for (IntPtr curr_addr_aligned = begin_addr_aligned ; curr_addr_aligned <= end_addr_aligned /* Note <= */; curr_addr_aligned += m_ocache->dCacheLineSize())
        {
            // Access the cache one line at a time
            UInt32 curr_offset;
            UInt32 curr_size;

            // Determine the offset
            // TODO fix curr_size calculations
            // FIXME: Check if all this is correct
            if (curr_addr_aligned == begin_addr_aligned) {
                curr_offset = begin_addr % m_ocache->dCacheLineSize();
            }
            else {
                curr_offset = 0;
            }

            // Determine the size
            if (curr_addr_aligned == end_addr_aligned) {
                curr_size = (end_addr % m_ocache->dCacheLineSize()) - (curr_offset);
                if (curr_size == 0) {
                    continue;
                }
            }
            else {
                curr_size = m_ocache->dCacheLineSize() - (curr_offset);
            }

            LOG_PRINT("Start InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

            if (!m_memory_manager->initiateSharedMemReq(shmem_operation, curr_addr_aligned, curr_offset, curr_data_buffer_head, curr_size)) {
                // If it is a LOAD operation, 'initiateSharedMemReq' causes curr_data_buffer_head to be automatically filled in
                // If it is a STORE operation, 'initiateSharedMemReq' reads the data from curr_data_buffer_head
                all_hits = false;
            }

            LOG_PRINT("End InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

            // Increment the buffer head
            curr_data_buffer_head += curr_size;
        }

        LOG_PRINT("%s - ADDR: %x, data_size: %u, END!!", ((operation==CacheBase::k_ACCESS_TYPE_LOAD) ? " READ " : " WRITE "), d_addr, data_size);

        return all_hits;		    

    } 
    else 
    {
        // run this if we aren't using shared_memory
        // FIXME: I am not sure this is right
        // What if the initial data for this address is in some other core's DRAM (which is on some other host machine)
        if(operation == CacheBase::k_ACCESS_TYPE_LOAD)
            return m_ocache->runDCacheLoadModel(d_addr, data_size).first;
        else
            return m_ocache->runDCacheStoreModel(d_addr, data_size).first;
    }
}
