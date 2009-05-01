#include "dram_directory.h"

#include "log.h"
#include "simulator.h"

DramDirectory::DramDirectory(core_id_t core_id, Network* network)
{
   try
   {
      m_knob_dram_access_cost = Sim()->getCfg()->getInt("dram/dram_access_cost");
      m_knob_dir_max_sharers = Sim()->getCfg()->getInt("dram/max_sharers");
   }
   catch(...)
   {
      LOG_ASSERT_ERROR(false, "DramDirectory obtained a bad value from config.");
   }

   m_core_id = core_id;
   m_network = network;
   
   m_total_cores = Config::getSingleton()->getTotalCores();
   m_cache_line_size = Config::getSingleton()->getCacheLineSize();

   // Initialize the static variables in DramDirectoryEntry
   // This will be done as many times as the number of cores but since 
   // this is all done by a single thread, there is no race condition
   DramDirectoryEntry::setCacheLineSize(m_cache_line_size);
   DramDirectoryEntry::setMaxSharers(Config::getSingleton()->getDirMaxSharers());
   DramDirectoryEntry::setTotalCores(Config::getSingleton()->getTotalCores());
}

DramDirectory::~DramDirectory()
{
   std::map<IntPtr, DramDirectoryEntry*>::iterator dram_entry_it;
   for (dram_entry_it = dram_directory_entries.begin(); dram_entry_it != dram_directory_entries.end(); dram_entry_it++)
   {
      delete dram_entry_it->second;
   }
   dram_directory_entries.clear();

   assert (dram_request_list.empty());
}

/*
 * returns the associated DRAM directory entry given a memory address
 */
DramDirectoryEntry* DramDirectory::getEntry(IntPtr address)
{
   // Note: The directory is a map key'ed by addresss.
   DramDirectoryEntry* entry_ptr = dram_directory_entries[address];

   // FIXME: I guess this is the place we need to call the code for transferring data from the host machine to the simulated machine
   if (entry_ptr == NULL)
   {
      dram_directory_entries[address] =  new DramDirectoryEntry(address, (Byte*) address);
   }

   return dram_directory_entries[address];
}

void DramDirectory::copyDataToDram(IntPtr address, Byte* data_buffer)
{
   DramDirectoryEntry* entry_ptr = dram_directory_entries[address];

   assert(entry_ptr != NULL);

   entry_ptr->fillDramDataLine(data_buffer);

}

// FIXME: In case this was a writeback, we need to set the state bits also properly in addition to the sharers list
void DramDirectory::processWriteBack(NetPacket& wb_packet)
{
   // Only evictions come here from now on
   MemoryManager::AckPayload payload;
   Byte data_buffer[m_cache_line_size];

   MemoryManager::extractAckPayloadBuffer(&wb_packet, &payload, data_buffer);

   DramDirectoryEntry* dir_entry = getEntry(payload.ack_address);

   LOG_PRINT("Got eviction for address: 0x%x, sharer = %i, numSharers = %i, DState = %s", (UInt32)payload.ack_address, wb_packet.sender, dir_entry->numSharers(), DramDirectoryEntry::dStateToString(dir_entry->getDState()).c_str());

   dir_entry->removeSharer(wb_packet.sender);

   if (dir_entry->numSharers() == 0)
   {
      dir_entry->setDState(DramDirectoryEntry::UNCACHED);
   }

   // FIXME: This will change when we allow evictions with clean status
   LOG_ASSERT_ERROR(payload.is_writeback, "Change this when you implement eviction with clean/dirty status. Right now, only dirty is allowed");

   if (payload.is_writeback)
   {
      assert(payload.data_size == m_cache_line_size);
      copyDataToDram(payload.ack_address, data_buffer); //TODO verify that all wb_packets would give us the entire cache_line
   }
}


// The Dram Directory serializes requests with the same address.
void DramDirectory::startSharedMemRequest(NetPacket& req_packet)
{
   // This function adds the shared memory request on the queue and starts processing it if possible
   IntPtr address = ((MemoryManager::RequestPayload*)(req_packet.data))->request_address;
   shmem_req_t shmem_req_type = ((MemoryManager::RequestPayload*)(req_packet.data))->request_type;
   UInt32 requestor = req_packet.sender;

   DramRequestsForSingleAddress* dram_reqs = dram_request_list[address];

   LOG_PRINT("Got shared memory request for address: 0x%x, req_type = %s, start now - %i", address, (shmem_req_type == READ) ? "READ" : "WRITE", dram_reqs == NULL);

   if (dram_reqs == NULL)
   {
      // No requests for this address exist currently
      dram_request_list[address] = new DramRequestsForSingleAddress;

      dram_reqs = dram_request_list[address];
      assert(dram_reqs != NULL);

      // FIXME: Dont delete SingleDramRequest now !!
      SingleDramRequest* single_dram_req = new SingleDramRequest(address, shmem_req_type, requestor);
      dram_reqs->addRequestToQueue(single_dram_req);

      startNextSharedMemRequest(address);
   }

   else
   {
      // There exists some requests for this address already
      SingleDramRequest* single_dram_req = new SingleDramRequest(address, shmem_req_type, requestor);
      dram_reqs->addRequestToQueue(single_dram_req);
   }
}

void DramDirectory::finishSharedMemRequest(IntPtr address)
{
   // We need to process the next request on the queue here
   DramRequestsForSingleAddress* dram_reqs = dram_request_list[address];
   assert(dram_reqs != NULL);

   LOG_PRINT("Finished DRAM request for address: 0x%x, start next %i", address, dram_reqs->numWaitingRequests() != 0);

   if (dram_reqs->numWaitingRequests() == 0)
   {
      dram_reqs->deleteCurrRequest();
      delete dram_reqs;
      dram_request_list.erase(address);
   }
   else
   {
      dram_reqs->deleteCurrRequest();
      startNextSharedMemRequest(address);
   }
}

void DramDirectory::startNextSharedMemRequest(IntPtr address)
{

   // This function is ONLY called when there is another request on the queue and it is ready to be processed
   DramRequestsForSingleAddress* dram_reqs = dram_request_list[address];
   assert(dram_reqs != NULL);
   assert(dram_reqs->numWaitingRequests() != 0);

   // Extract the next request from the queue
   SingleDramRequest* single_dram_req = dram_reqs->getNextRequest();

   DramDirectoryEntry* dram_dir_entry = getEntry(address);
   assert(dram_dir_entry != NULL);

   DramDirectoryEntry::dstate_t old_dstate = dram_dir_entry->getDState();
   UInt32 num_acks_to_recv = dram_dir_entry->numSharers();

   dram_reqs->setCurrRequestAttributes(single_dram_req, old_dstate, num_acks_to_recv);

   processSharedMemRequest(single_dram_req->requestor, single_dram_req->shmem_req_type, single_dram_req->address);

}

void DramDirectory::processSharedMemRequest(UInt32 requestor, shmem_req_t shmem_req_type, IntPtr address)
{

   // This function should not depend on the state of the DRAM directory
   // The relevant arguments must be passed in from the calling function

   DramDirectoryEntry* dram_dir_entry = getEntry(address);
   DramDirectoryEntry::dstate_t curr_dstate = dram_dir_entry->getDState();

   LOG_PRINT("Start processing DRAM request for address: 0x%x, req_type = %s, <- %u, current state = %s", (UInt32) address, (shmem_req_type == READ) ? "read" : "write", requestor, DramDirectoryEntry::dStateToString(curr_dstate).c_str());

   switch (shmem_req_type)
   {

   case WRITE:
      if (curr_dstate == DramDirectoryEntry::EXCLUSIVE)
      {
         startDemoteOwner(dram_dir_entry, CacheState::INVALID);
      }
      else if (curr_dstate == DramDirectoryEntry::SHARED) // Number of sharers > 0
      {
         startInvalidateAllSharers(dram_dir_entry);
      }
      else if (curr_dstate == DramDirectoryEntry::UNCACHED)
      {
         // Return the data to the requestor
         dram_dir_entry->addExclusiveSharer(requestor);
         dram_dir_entry->setDState(DramDirectoryEntry::EXCLUSIVE);

         sendDataLine(dram_dir_entry, requestor, CacheState::EXCLUSIVE);
         finishSharedMemRequest(address);
      }
      else
      {
         assert(false);  // We should not reach here
      }
      break;

   case READ:
      // handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
      if (curr_dstate == DramDirectoryEntry::EXCLUSIVE)
      {
         startDemoteOwner(dram_dir_entry, CacheState::SHARED);
      }
      else if (
         (curr_dstate == DramDirectoryEntry::SHARED)  ||
         (curr_dstate == DramDirectoryEntry::UNCACHED)
      )
      {
         // Check if the maximum limit on the number of sharers has been reached
         // addSharer() return whether there is an eviction
         if (! dram_dir_entry->addSharer(requestor))
         {
            // Success, I can now just return the data to the requestor
            dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
            sendDataLine(dram_dir_entry, requestor, CacheState::SHARED);
            finishSharedMemRequest(address);
         }
         else
         {
            // We need to remove another sharer before we can add this sharer
            UInt32 eviction_id = dram_dir_entry->getEvictionId();

            startInvalidateSingleSharer(dram_dir_entry, eviction_id);
         }
      }
      else
      {
         // We should not reach here
         assert(false);
      }
      break;

   default:
      LOG_PRINT_ERROR("unsupported memory transaction type.");
      break;
   }

}


/* ============================================================== */
//
//
/* ============================================================== */
/* Added by George */

// XXX: Here, I add the DRAM access cost
// TODO: This is just a temporary hack. The actual DRAM access cost has to incorporated
//       while adding data storage to all the coherence messages
//
// 1) READ request:
//    a) Case (EXCLUSIVE):
//       There is only 1 AckPayload message we need to look at
//     i) AckPayload::remove_from_sharers = false
//       * Do '2' runDramAccessModel()
//     ii) AckPayload::remove_from_sharers = true
//       * Do '1' runDramAccessModel()
//  b) Case (SHARED or INVALID):
//    * Do '1' runDramAccessModel()
//
// 2) WRITE request
//    a) Case (EXCLUSIVE):
//      There is only 1 AckPayload message we need to look at
//       i) AckPayload::remove_from_sharers = false
//         * Do '2' runDramAccessModel()
//       ii) AckPayload::remove_from_sharers = true
//         * Do '1' runDramAccessModel()
//    b) Case (SHARED or INVALID):
//      * Do '1' runDramAccessModel()
//

/* =============================================================== */

//TODO go through this code again. i think a lot of it is unnecessary.
//TODO rename this to something more descriptive about what it does (sending a memory line to another core)
void DramDirectory::sendDataLine(DramDirectoryEntry* dram_dir_entry, UInt32 requestor, CacheState::cstate_t new_cstate)
{
   // initialize packet payload
   MemoryManager::UpdatePayload payload;

   payload.update_new_cstate = new_cstate;
   payload.update_address = dram_dir_entry->getCacheLineAddress();
   payload.data_size = m_cache_line_size;

   Byte data_buffer[m_cache_line_size];
   dram_dir_entry->getDramDataLine(data_buffer);

   UInt32 payload_size = sizeof(payload) + m_cache_line_size;
   Byte payload_buffer[payload_size];

   MemoryManager::createUpdatePayloadBuffer(&payload, data_buffer, payload_buffer, payload_size);
   // This is the response to a shared memory request
   NetPacket packet = MemoryManager::makePacket(SHARED_MEM_RESPONSE, payload_buffer, payload_size, m_core_id, requestor);

   m_network->netSend(packet);

}

void DramDirectory::startDemoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate)
{
   //demote exclusive owner
   assert(dram_dir_entry->numSharers() == 1);
   assert(dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE);
   assert(new_cstate == CacheState::SHARED || new_cstate == CacheState::INVALID);

   UInt32 owner = dram_dir_entry->getExclusiveSharerRank();

   IntPtr address = dram_dir_entry->getCacheLineAddress();

   // reqeust a write back data payload and downgrade to new_dstate
   MemoryManager::UpdatePayload upd_payload;
   upd_payload.update_new_cstate = new_cstate;
   upd_payload.update_address = address;
   upd_payload.data_size = 0;
   NetPacket packet = MemoryManager::makePacket(SHARED_MEM_UPDATE_UNEXPECTED, (Byte *)(&upd_payload), sizeof(MemoryManager::UpdatePayload), m_core_id, owner);

   m_network->netSend(packet);
}



// Send invalidate messages to all of the sharers and wait on acks.
// TODO: Add in ability to do a broadcast invalidateSharers if #of sharers is greater than #of pointers.
void DramDirectory::startInvalidateAllSharers(DramDirectoryEntry* dram_dir_entry)
{
   assert(dram_dir_entry->numSharers() >= 1);
   assert(dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);

   // invalidate current sharers
   vector<UInt32> sharers_list = dram_dir_entry->getSharersList();

   // send message to sharer to invalidate it
   for (UInt32 i = 0; i < sharers_list.size(); i++)
   {
      startInvalidateSingleSharer(dram_dir_entry, sharers_list[i]);
   }

}

// Invalidate Only a single sharer
void DramDirectory::startInvalidateSingleSharer(DramDirectoryEntry* dram_dir_entry, UInt32 sharer_id)
{
   assert(dram_dir_entry->numSharers() >= 1);
   assert(dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);

   IntPtr address = dram_dir_entry->getCacheLineAddress();

   MemoryManager::UpdatePayload payload;
   payload.update_new_cstate = CacheState::INVALID;
   payload.update_address = address;
   payload.data_size = 0;

   NetPacket packet = MemoryManager::makePacket(SHARED_MEM_UPDATE_UNEXPECTED, (Byte *)(&payload), sizeof(MemoryManager::UpdatePayload), m_core_id, sharer_id);
   m_network->netSend(packet);
}

void DramDirectory::processAck(NetPacket& ack_packet)
{
   /*
    * Create a map of the following form
    * key :- address (cache aligned)
    * value :- <shmem_req_type, requestor, numAcksToRecv, old_dstate>
    */

   IntPtr address = ((MemoryManager::AckPayload*)(ack_packet.data))->ack_address;
   DramRequestsForSingleAddress* dram_reqs = dram_request_list[address];

   assert(dram_reqs != NULL);   // A request must have already been created
   shmem_req_t shmem_req_type = dram_reqs->getShmemReqType();
   UInt32 requestor = dram_reqs->getRequestor();
   DramDirectoryEntry::dstate_t old_dstate = dram_reqs->getOldDState();

   DramDirectoryEntry* dram_dir_entry = getEntry(address);

   assert(dram_dir_entry != NULL);

   LOG_PRINT("Got ack <- %i for %i, address = 0x%x, numAcksToRecv %i", ack_packet.sender, requestor, (UInt32) address, dram_reqs->getNumAcksToRecv());

   Byte data_buffer[m_cache_line_size];
   MemoryManager::AckPayload ack_payload;
   MemoryManager::extractAckPayloadBuffer(&ack_packet, &ack_payload, data_buffer);

   switch (shmem_req_type)
   {

   case(WRITE):

      if (old_dstate == DramDirectoryEntry::EXCLUSIVE)
      {
         processDemoteOwnerAck(ack_packet.sender, dram_dir_entry, &ack_payload, data_buffer, CacheState::INVALID);
         processSharedMemRequest(requestor, WRITE, address);
      }
      else if (old_dstate == DramDirectoryEntry::SHARED)
      {
         processInvalidateSharerAck(ack_packet.sender, dram_dir_entry, &ack_payload);
         dram_reqs->decNumAcksToRecv();
         if (dram_reqs->getNumAcksToRecv() == 0)
         {
            processSharedMemRequest(requestor, WRITE, address);
         }
      }
      else
      {
         // Should never reach here
         assert(false);
      }

      break;

   case(READ):

      if (old_dstate == DramDirectoryEntry::EXCLUSIVE)
      {
         processDemoteOwnerAck(ack_packet.sender, dram_dir_entry, &ack_payload, data_buffer, CacheState::SHARED);
         processSharedMemRequest(requestor, READ, address);
      }
      else if (old_dstate == DramDirectoryEntry::SHARED)
      {
         // We only need to evict the cache line from 1 sharer
         processInvalidateSharerAck(ack_packet.sender, dram_dir_entry, &ack_payload);
         processSharedMemRequest(requestor, READ, address);
      }
      else
      {
         // Should never reach here
         assert(false);
      }

      break;

   default:

      LOG_PRINT("Invalid Shared Memory Request Type");
      assert(false);

      break;
   }

}

void DramDirectory::processDemoteOwnerAck(
   UInt32 sender,
   DramDirectoryEntry* dram_dir_entry,
   MemoryManager::AckPayload* ack_payload,
   Byte *data_buffer,
   CacheState::cstate_t new_cstate
)
{
   // wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
   // The 'dram_dir_entry' was in the exclusive state before

   if (ack_payload->remove_from_sharers)
   {

      assert(ack_payload->data_size == 0);
      assert(ack_payload->ack_new_cstate == CacheState::INVALID);
      assert(ack_payload->is_writeback == false);
      assert(dram_dir_entry->getDState() == DramDirectoryEntry::UNCACHED);
      assert(dram_dir_entry->numSharers() == 0);

      // We cant say anything about the exclusive owner
   }

   else
   {

      // There has been no eviction
      assert(ack_payload->data_size == m_cache_line_size);
      assert (ack_payload->ack_new_cstate == new_cstate);
      assert(ack_payload->is_writeback == true);
      assert(dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE);
      assert(dram_dir_entry->numSharers() == 1);
      assert(dram_dir_entry->getExclusiveSharerRank() == sender);

      // Write Back the cache line
      // Change the interface for better efficiency
      copyDataToDram(dram_dir_entry->getCacheLineAddress(), data_buffer);
      if (new_cstate == CacheState::INVALID)
      {
         // It was a write request
         dram_dir_entry->removeSharer(sender);
      }
      assert( (new_cstate == CacheState::SHARED)  ||  (new_cstate == CacheState::INVALID) );
      if (new_cstate == CacheState::SHARED)
      {
         dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
      }
      else
      {
         dram_dir_entry->setDState(DramDirectoryEntry::UNCACHED);
      }
   }

}

void DramDirectory::processInvalidateSharerAck(
   UInt32 sender,
   DramDirectoryEntry* dram_dir_entry,
   MemoryManager::AckPayload* ack_payload
)
{
   assert(ack_payload->ack_new_cstate == CacheState::INVALID);
   assert(ack_payload->data_size == 0);
   assert(ack_payload->is_writeback == false);

   if (ack_payload->remove_from_sharers)
   {
      assert((dram_dir_entry->getDState() == DramDirectoryEntry::SHARED)  || (dram_dir_entry->getDState() == DramDirectoryEntry::UNCACHED));
      assert(!dram_dir_entry->hasSharer(sender));
   }

   else
   {
      assert(dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);

      // Remove the sharer from the sharer's list
      dram_dir_entry->removeSharer(sender);
      if (dram_dir_entry->numSharers() == 0)
      {
         dram_dir_entry->setDState(DramDirectoryEntry::UNCACHED);
      }
   }

}
