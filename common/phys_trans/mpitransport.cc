#include "mpitransport.h"
#include "lock.h"
#include "log.h"

#define LOG_DEFAULT_RANK comm_id
#define LOG_DEFAULT_MODULE TRANSPORT

#include <iostream>
using namespace std;

#ifdef PHYS_TRANS_USE_LOCKS
Lock* Transport::pt_lock;
#define PT_LOCK()                                                       \
   assert(Transport::pt_lock);                                          \
   ScopedLock __scopedLock(*Transport::pt_lock);
#else
#define PT_LOCK() { }
#endif

UInt32 Transport::ptProcessNum()
{
   PT_LOCK();

   // MPI already takes care of assigning a number to each process so just
   //  return the MPI rank.
   SInt32 MPI_rank;
   SInt32 mpi_initialized;

   // MPI is initialized by ptInitQueue.  This routine cannot be called
   //  before ptInitQueue.
   MPI_Initialized(&mpi_initialized);
   assert(mpi_initialized == true);
   MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);

   return (UInt32)MPI_rank;
}

// This routine should be executed once in each process
void Transport::ptGlobalInit()
{
#ifdef PHYS_TRANS_USE_LOCKS
   // initialize global phys trans lock
   pt_lock = Lock::create();
#endif

   //***** Initialize MPI *****//
   // NOTE: MPI barfs if I call MPI_Init_thread with MPI_THREAD_MULTIPLE
   //  in a non-threaded process.  I think this is a bug but I'll work
   //  around it for now.
   SInt32 required, provided;
   if (g_config->getProcessCount() > 1) {
      required = MPI_THREAD_MULTIPLE;
   } else {
      required = MPI_THREAD_SINGLE;
   }
   MPI_Init_thread(NULL, NULL, required, &provided);
   assert(provided >= required);

   //***** Fill in g_config with values that we are responsible for *****//
   g_config->setProcessNum(ptProcessNum());

   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Process number set to %i", g_config->getCurrentProcessNum());
}

// This routine should be executed once in each thread
SInt32 Transport::ptInit(SInt32 tid, SInt32 num_mod)
{
   SInt32 MPI_rank;
   // tid is my thread ID
   pt_tid = tid;

   // comm_id is my communication network endpoint ID and is equivalent
   //  to module number

   // FIXME: Choose between a couple of fixed schemes until we get full
   //  support for multiple processes and multiple threads.  Eventually,
   //  we will either pick an ID from the list of IDs for this process,
   //  or our ID will be assigned by someone else and passed into this
   //  method.
   if (g_config->getProcessCount() == 1) {
      // If we only have one process, we can make comm_id equal to tid
      comm_id = tid;

   } else if (g_config->getProcessCount() == g_config->getTotalCores()) {
      // If the number of processes is equal to the number of modules, we
      //  have one module per process.  Therefore, we can just use the
      //  process's MPI rank as the comm_id.
      PT_LOCK();
      MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);
      comm_id = MPI_rank;  // Convert from int to whatever comm_id is

   } else {
      LOG_NOTIFY_WARNING();
      LOG_PRINT("WARNING: Multiple processes each with multiple threads is not fully supported!\
 Falling back on tid's which only works on single machine!");
      comm_id = tid;
   }

   return 0;
}

SInt32 Transport::ptSend(SInt32 receiver, void *buffer, SInt32 size)
{
   // Notes:
   //  - The data is sent using MPI_BYTE so that MPI won't do any conversions.
   //  - We use the receiver ID as the tag so that messages can be
   //    demultiplexed automatically by MPI in the receiving process.
   //
   UInt32 dest_proc = g_config->getProcessNumForCore(receiver);

   LOG_PRINT("sending msg -- tid: %i, comm id: %i, size: %i, dest: %i", pt_tid, comm_id, size, dest_proc);

   PT_LOCK();
   MPI_Send(buffer, size, MPI_BYTE, dest_proc, receiver, MPI_COMM_WORLD);

   LOG_PRINT("message sent");

   // FIXME: Why do we need to return the size?
   return size;
}

void* Transport::ptRecv()
{
   MPI_Status status;
   SInt32 pkt_size, source;
   Byte* buffer;

   LOG_PRINT("attempting receive -- tid: %i, comm_id: %i", pt_tid, comm_id);

   // Probe for a message from any source but with our ID tag.
#ifdef PHYS_TRANS_USE_LOCKS
   // When using phys_trans locks, we spin.
   while (true)
   {
      SInt32 flag;

      pt_lock->acquire();

      // this is essentially ptQuery without the locks
      MPI_Iprobe(MPI_ANY_SOURCE, comm_id, MPI_COMM_WORLD, &flag, &status);

      // if a message is ready, leave the loop _without_ releasing the lock
      if (flag != 0)
         break;

      // otherwise, release and yield
      pt_lock->release();
      sched_yield();
   }
#else
   // Otherwise, blocking MPI call.
   MPI_Probe(MPI_ANY_SOURCE, comm_id, MPI_COMM_WORLD, &status);
#endif

   LOG_PRINT("after probe");
 
   // Now we know that there is a message ready, check status to see how
   //  big it is and who the source is.
   MPI_Get_count(&status, MPI_BYTE, &pkt_size);
   assert(status.MPI_SOURCE != MPI_UNDEFINED);
   source = status.MPI_SOURCE;
   
   // Allocate a buffer for the incoming message
   buffer = new Byte[pkt_size];

   LOG_PRINT("msg found -- tid: %i, comm_id: %i, size: %i, source: %i", pt_tid, comm_id, pkt_size, source);

   // We need to make sure the source here is the same as the one returned
   //  by the call to Probe above.  Otherwise, we might get a message from
   //  a different sender that could have a different size.
   MPI_Recv(buffer, pkt_size, MPI_BYTE, source, comm_id, MPI_COMM_WORLD,
            &status);
   
   LOG_PRINT("msg received");

#ifdef PHYS_TRANS_USE_LOCKS
   pt_lock->release();
#endif

   return buffer;
   // NOTE: the caller should free the buffer when it's finished with it
}

Boolean Transport::ptQuery()
{
   SInt32 flag;
   MPI_Status status;
   PT_LOCK();

   // Probe for a message from any source but with our ID tag
   MPI_Iprobe(MPI_ANY_SOURCE, comm_id, MPI_COMM_WORLD, &flag, &status);

   // flag == 0 indicates that no message is waiting
   return (flag != 0);
}
