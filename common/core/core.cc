#include "core.h"

#include "network.h"
#include "ocache.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"

#include "log.h"
#define LOG_DEFAULT_RANK   m_core_id
#define LOG_DEFAULT_MODULE CORE

using namespace std;

Core::Core(SInt32 id)
   : m_core_id(id)
{
   LOG_PRINT_EXPLICIT(-1, CORE, "Core ctor for: %d", id);

   m_network = new Network(this);

   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      m_perf_model = new PerfModel("performance modeler");
   }
   else
   {
      m_perf_model = (PerfModel *) NULL;
   }

   if (Config::getSingleton()->getEnableDCacheModeling() || Config::getSingleton()->getEnableICacheModeling())
   {
      m_ocache = new OCache("organic cache", this);
   }
   else
   {
      m_ocache = (OCache *) NULL;
   }

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      assert(Config::getSingleton()->getEnableDCacheModeling());

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

   LOG_ASSERT_ERROR((unsigned)size == packet.length, "*ERROR* Core: User thread requested packet of size: %d, got a packet from %d of size: %d", size, sender, packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ??
   delete [](Byte*)packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}


