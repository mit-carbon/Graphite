#include "mpitransport.h"
#include "lock.h"
#include "log.h"

#define LOG_DEFAULT_RANK comm_id
#define LOG_DEFAULT_MODULE TRANSPORT


#include <iostream>
using namespace std;

#define PT_DEBUG 1

// Initialize class static variables (these are useless but required by C++)
SInt32 Transport::pt_num_mod = 0;
SInt32* Transport::dest_ranks = NULL;
// MCP_tag will be set to the maximum allowable tag later on.  Just in case
//  that fails, set the initial value to 32767.  According to the MPI spec,
//  all implementations must allow tags up to 32767.
UInt32 Transport::MCP_tag = 32767;
SInt32 Transport::MCP_rank = 0;

Lock* Transport::pt_lock;
#define PT_LOCK()                                                       \
   assert(Transport::pt_lock);                                          \
   ScopedLock __scopedLock(*Transport::pt_lock);                        \

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
void Transport::ptInitQueue(SInt32 num_mod)
{
   UInt32 i, j;
   UInt32 num_procs;    // Number of MPI processes
   UInt32 num_cores;
   SInt32 temp;
   SInt32* thread_counts, *displs, *rbuf;
  
   // initialize global phys trans lock
   pt_lock = Lock::create();
   
   // num_mod is the number of threads in this process
   pt_num_mod = num_mod;
  
   //***** Initialize MPI *****//
   // NOTE: MPI barfs if I call MPI_Init_thread with MPI_THREAD_MULTIPLE
   //  in a non-threaded process.  I think this is a bug but I'll work
   //  around it for now.
   SInt32 required, provided;
   if (g_config->numMods() > 1) {
      required = MPI_THREAD_MULTIPLE;
   } else {
      required = MPI_THREAD_SINGLE;
   }
   MPI_Init_thread(NULL, NULL, required, &provided);
   assert(provided >= required);

   //***** Fill in g_config with values that we are responsible for *****//
   g_config->setProcNum(ptProcessNum());
#ifdef PT_DEBUG
   cout << "Process number set to " << g_config->myProcNum() << endl;
#endif

   //***** Setup the info we need to communicate with the MCP *****//

   // FIXME: I should probably be separating the concepts of MPI rank and
   //  process number.  In that case, I would want to see if I'm in the MCP
   //  process and, if so, send my rank to everyone else.  However, that's
   //  a pain because I can't do a broadcast without everyone knowing who
   //  the root of the bcast is apriori.  For now, I'll just assume that
   //  g_config->MCPProcNum() can be directly used as a destination rank.
   //  This should be OK because I (the PT layer) get to assign process
   //  numbers and can therefore enforce (process num == MPI rank).
   MCP_rank = (SInt32)g_config->MCPProcNum();

   // Determine the tag to use for communication with the MCP by asking
   //  for the maximum allowable tag value (tags must be between 0 and
   //  this upper bound, inclusive).
   SInt32 attr_flag;
   SInt32* max_tag;
   MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &max_tag, &attr_flag);
   // flag==false means the attribute wasn't set so just use the default
   if(attr_flag == true) { MCP_tag = *max_tag; }

   //***** Setup the info we need to communicate between modules *****//

   // When using MPI, we can have multiple processes, each potentially
   //  containing multiple threads.  MPI treats each process (not each
   //  thread) as a communication endpoint with its own unique rank.
   //  Therefore, to communicate with a particular thread, we need to know
   //  the rank of the process that contains it.  Here we build a map from
   //  core (thread) number to MPI rank.  To do this, we ask every process
   //  for a list of the cores it contains.
   // FIXME?: We should probably just read the entire map of cores to
   //  processes from the config and skip all this swapping of core IDs.
  
   // Create an empty map from modules to processes
   dest_ranks = new SInt32[g_config->totalMods()];

   /* Quick hack
   MPI_Comm_rank(MPI_COMM_WORLD, &temp);
   my_threads = new SInt32[num_mod];
   for (i=0; i < (UInt32)num_mod; i++) { my_threads[i] = (temp*num_mod)+i; }
   */
   Config::CoreList mod_list = g_config->getModuleList();   
   SInt32* my_threads = new SInt32[mod_list.size()];
   i = 0;
   for (Config::CLCI m = mod_list.begin(); m != mod_list.end(); ++m) {
      my_threads[i++] = *m;
   }

   // First, find out how many cores each process contains
   // FIXME: I should just read num_procs from g_config instead of asking MPI
   MPI_Comm_size(MPI_COMM_WORLD, &temp);
   num_procs = temp;  // convert from SInt32 to UInt32
   if (g_config->numProcs() != num_procs) MPI_Abort(MPI_COMM_WORLD, -1);
#ifdef PT_DEBUG
   cout << "Number of processes: " << num_procs << endl;
#endif
   thread_counts = new SInt32[num_procs];
   MPI_Allgather(&num_mod, 1, MPI_INT, thread_counts, 1, MPI_INT, MPI_COMM_WORLD);

   // One more for the MCP
   thread_counts[num_procs - 1] += 1;
  
   // Create array of indexes that will be used to place values received
   //  from each process in the appropriate place in the receive buffer.
   displs = new SInt32[num_procs];
   displs[0] = 0;
   for (i=1; i<num_procs; i++) {
     displs[i] = displs[i-1]+thread_counts[i-1];
   }

   // Create the receive buffer
   num_cores = displs[num_procs-1] + thread_counts[num_procs-1];
   assert(num_cores == g_config->totalMods());
#ifdef PT_DEBUG
   cout << "Total number of cores: " << num_cores << endl;
#endif
   rbuf = new SInt32[num_cores];

   // Send out our core list and gather the lists from everyone else
   MPI_Allgatherv(&my_threads[0], num_mod, MPI_INT, rbuf, thread_counts,
		  displs, MPI_INT, MPI_COMM_WORLD);

   // Now we have a complete list of all the cores in each process.
   // Convert it into a map from core to process ID:
#ifdef PT_DEBUG
   cout << "Core to MPI rank map: ";
#endif
   for (i=0; i<num_procs; i++) {
      for (j=displs[i]; j<(UInt32)(displs[i]+thread_counts[i]); j++) {
         dest_ranks[rbuf[j]] = i;
#ifdef PT_DEBUG
	 cout << rbuf[j] << "->" << i << ", ";
#endif
      }
   }
#ifdef PT_DEBUG
   cout << endl;
#endif

   delete [] rbuf;
   delete [] displs;
   delete [] thread_counts;
   delete [] my_threads;
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
   if (g_config->numProcs() == 1) {
      // If we only have one process, we can make comm_id equal to tid
      comm_id = tid;

   } else if (g_config->numProcs() == g_config->totalMods()) {
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

#ifdef PT_DEBUG
   cout << "PT sending msg ==> tid:" << pt_tid
	<< ", comm_id:" << comm_id << ", recv:" << receiver
	<< ", size:" << size << " ... ";
   cout << "Dest rank: " << dest_ranks[receiver] << endl;
#endif
   PT_LOCK();
   MPI_Send(buffer, size, MPI_BYTE, dest_ranks[receiver], receiver,
            MPI_COMM_WORLD);

#ifdef PT_DEBUG
   cout << "done." << endl;
#endif
   // FIXME: Why do we need to return the size?
   return size;
}

void* Transport::ptRecv()
{
   MPI_Status status;
   SInt32 pkt_size, source;
   SInt32 flag;
   Byte* buffer;

#ifdef PT_DEBUG
   cout << "PT (tid:" << pt_tid << ",cid:" << comm_id
	<< ") attempting receive..." << endl;
#endif

   // Probe for a message from any source but with our ID tag.
   while (true)
   {
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
 
   // Now we know that there is a message ready, check status to see how
   //  big it is and who the source is.
   MPI_Get_count(&status, MPI_BYTE, &pkt_size);
   assert(status.MPI_SOURCE != MPI_UNDEFINED);
   source = status.MPI_SOURCE;
   
   // Allocate a buffer for the incoming message
   buffer = new Byte[pkt_size];

#ifdef PT_DEBUG
   cout << "PT found msg ==> tid:" << pt_tid
        << ", comm_id:" << comm_id << ", source rank:" << source
        << ", size:" << pkt_size << " ...";
#endif

   // We need to make sure the source here is the same as the one returned
   //  by the call to Probe above.  Otherwise, we might get a message from
   //  a different sender that could have a different size.
   MPI_Recv(buffer, pkt_size, MPI_BYTE, source, comm_id, MPI_COMM_WORLD,
            &status);
   
#ifdef PT_DEBUG
   cout << "received." << endl;
#endif

   pt_lock->release();

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
