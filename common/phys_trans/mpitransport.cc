#include "mpitransport.h"
#include "lock.h"
#include "log.h"
#include "mpi.h"
#include "config.h"

#define LOG_DEFAULT_RANK m_core_id
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

void Transport::ptFinish()
{
   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Entering finish");

   int err_code;
   err_code = MPI_Finalize();
   LOG_ASSERT_ERROR_EXPLICIT(err_code == MPI_SUCCESS, -1, TRANSPORT, "ptFinish : MPI_Finalize fail.");

   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Exiting finish");
}

void Transport::ptBarrier()
{
   // FIXME: This is potentially dangerous, but I don't see a way
   // around it using MPI_Barrier. If other threads are waiting on
   // this process (say, for shared memory response) in order to reach
   // this barrier, we will deadlock.
   //   Correct implementation should probably manually implement a
   // barrier via broadcast messages and counters.
   //   - NZB
   PT_LOCK();
   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Entering barrier");

   int err_code;
   err_code = MPI_Barrier(MPI_COMM_WORLD);
   LOG_ASSERT_ERROR_EXPLICIT(err_code == MPI_SUCCESS, -1, TRANSPORT, "ptBarrier : MPI_Barrier fail.");

   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Exiting barrier");
}

// This routine should be executed once in each process
void Transport::ptGlobalInit()
{
   int err_code;
   SInt32 rank;

#ifdef PHYS_TRANS_USE_LOCKS
   // initialize global phys trans lock
   pt_lock = Lock::create();
#endif

   //***** Initialize MPI *****//
   // NOTE: MPI barfs if I call MPI_Init_thread with MPI_THREAD_MULTIPLE
   //  in a non-threaded process.  I think this is a bug but I'll work
   //  around it for now.
   SInt32 required, provided;
   if (g_config->getProcessCount() > 1)
   {
      required = MPI_THREAD_MULTIPLE;
   }
   else
   {
      required = MPI_THREAD_SINGLE;
   }
   err_code = MPI_Init_thread(NULL, NULL, required, &provided);
   LOG_ASSERT_ERROR_EXPLICIT(err_code == MPI_SUCCESS, -1, TRANSPORT, "ptRecv : MPI_Get_count fail.");

   assert(provided >= required);

   //***** Fill in g_config with values that we are responsible for *****//
   err_code = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   LOG_ASSERT_ERROR_EXPLICIT(err_code == MPI_SUCCESS, -1, TRANSPORT, "ptProcessNum : MPI_Comm_rank fail.");

   g_config->setProcessNum(rank);

   LOG_PRINT_EXPLICIT(-1, TRANSPORT, "Process number set to %i", g_config->getCurrentProcessNum());
}

// This routine should be executed once in each thread
Transport::Transport(SInt32 core_id)
      : m_core_id(core_id)
{
}

SInt32 Transport::ptSend(SInt32 receiver, void *buffer, SInt32 size)
{
   int err_code;

   // Notes:
   //  - The data is sent using MPI_BYTE so that MPI won't do any conversions.
   //  - We use the receiver ID as the tag so that messages can be
   //    demultiplexed automatically by MPI in the receiving process.
   //
   UInt32 dest_proc = g_config->getProcessNumForCore(receiver);

   LOG_PRINT("sending msg -- from comm id: %i, size: %i, dest recvr: %d dest proc: %i", m_core_id, size, receiver, dest_proc);

   PT_LOCK();
   err_code = MPI_Send(buffer, size, MPI_BYTE, dest_proc, receiver, MPI_COMM_WORLD);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptSend : MPI_Send fail.");

   LOG_PRINT("message sent");

   // FIXME: Why do we need to return the size?
   return size;
}

void* Transport::ptRecv()
{
   MPI_Status status;
   SInt32 pkt_size, source;
   Byte* buffer;
   int err_code;

   LOG_PRINT("attempting receive -- m_core_id: %i", m_core_id);

   // Probe for a message from any source but with our ID tag.
#ifdef PHYS_TRANS_USE_LOCKS
   // When using phys_trans locks, we spin.
   while (true)
   {
      SInt32 flag;

      pt_lock->acquire();

      // this is essentially ptQuery without the locks
      err_code = MPI_Iprobe(MPI_ANY_SOURCE, m_core_id, MPI_COMM_WORLD, &flag, &status);
      LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptRecv : MPI_Iprobe fail.");

      // if a message is ready, leave the loop _without_ releasing the lock
      if (flag != 0)
         break;

      // otherwise, release and yield
      pt_lock->release();
      sched_yield();
   }
#else
   // Otherwise, blocking MPI call.
   err_code = MPI_Probe(MPI_ANY_SOURCE, m_core_id, MPI_COMM_WORLD, &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptRecv : MPI_Probe fail.");
#endif

   // Now we know that there is a message ready, check status to see how
   //  big it is and who the source is.
   err_code = MPI_Get_count(&status, MPI_BYTE, &pkt_size);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptRecv : MPI_Get_count fail.");
   assert(status.MPI_SOURCE != MPI_UNDEFINED);
   source = status.MPI_SOURCE;

   // Allocate a buffer for the incoming message
   buffer = new Byte[pkt_size];

   LOG_PRINT("msg found -- m_core_id: %i, size: %i, source: %i", m_core_id, pkt_size, source);

   // We need to make sure the source here is the same as the one returned
   //  by the call to Probe above.  Otherwise, we might get a message from
   //  a different sender that could have a different size.
   err_code = MPI_Recv(buffer, pkt_size, MPI_BYTE, source, m_core_id, MPI_COMM_WORLD,
                       &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptRecv : MPI_Recv fail.");

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
   int err_code;
   PT_LOCK();

   // Probe for a message from any source but with our ID tag
   err_code = MPI_Iprobe(MPI_ANY_SOURCE, m_core_id, MPI_COMM_WORLD, &flag, &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "ptQuery : MPI_Iprobe fail.");

   // flag == 0 indicates that no message is waiting
   return (flag != 0);
}

