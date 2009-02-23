#include <mpi.h>

#include "mpitransport.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK   -1
#define LOG_DEFAULT_MODULE MPITRANSPORT

// -- MpiTransport -- //

MpiTransport::MpiTransport()
{
   int err_code;
   SInt32 rank;

   SInt32 provided;
   err_code = MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Init_thread fail.");
   LOG_ASSERT_ERROR(provided >= MPI_THREAD_MULTIPLE, "Did not get required thread support.");

   err_code = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Comm_rank fail.");

   Config::getSingleton()->setProcessNum(rank);
   LOG_PRINT("Process number set to %i", Config::getSingleton()->getCurrentProcessNum());

   SInt32 num_procs;
   err_code = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Comm_size fail.");
   LOG_ASSERT_ERROR(num_procs == (SInt32)Config::getSingleton()->getProcessCount(), "Config no. processes doesn't match MPI no. processes.");

   // FIXME: Although it might seem better to just use PROC_COMM_ID
   // here, doing so fails mysteriously. Using -1 (as in SmTransport)
   // seems to work.
   m_global_node = createNode(-1);
}

MpiTransport::~MpiTransport()
{
   LOG_PRINT("Entering dtor");

   delete m_global_node;

   int err_code;
   err_code = MPI_Finalize();
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Finalize fail.");
}

void MpiTransport::barrier()
{
   LOG_PRINT("Entering barrier");

   int err_code;
   err_code = MPI_Barrier(MPI_COMM_WORLD);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Barrier fail.");

   LOG_PRINT("Exiting barrier");
}

Transport::Node* MpiTransport::getGlobalNode()
{
   return m_global_node;
}

Transport::Node* MpiTransport::createNode(SInt32 core_id)
{
   return new MpiNode(core_id);
}

// -- MpiNode -- //

#undef  LOG_DEFAULT_RANK
#define LOG_DEFAULT_RANK  getCoreId()

MpiTransport::MpiNode::MpiNode(SInt32 core_id)
   : Node(core_id)
{
}

MpiTransport::MpiNode::~MpiNode()
{
}

void MpiTransport::MpiNode::globalSend(SInt32 dest_proc, const void *buffer, UInt32 length)
{
   LOG_PRINT("In globalSend");
   send(dest_proc, PROC_COMM_TAG, buffer, length);
}

void MpiTransport::MpiNode::send(SInt32 dest_core, const void *buffer, UInt32 length)
{
   LOG_PRINT("In send");
   int dest_proc = Config::getSingleton()->getProcessNumForCore(dest_core);
   send(dest_proc, dest_core, buffer, length);
}

void MpiTransport::MpiNode::send(SInt32 dest_proc, UInt32 tag, const void *buffer, UInt32 length)
{
   LOG_PRINT("sending msg -- size: %i, tag: %d, dest_proc: %i", length, tag, dest_proc);

   SInt32 err_code;
   err_code = MPI_Send((void*)buffer, length, MPI_BYTE, dest_proc, tag, MPI_COMM_WORLD);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Send fail.");
}

Byte* MpiTransport::MpiNode::recv()
{
   MPI_Status status;
   SInt32 pkt_size, source;
   Byte* buffer;
   int err_code;

   LOG_PRINT("attempting receive -- tag: %i", getCoreId());

   SInt32 tag = getCoreId() == -1 ? PROC_COMM_TAG : getCoreId();

   err_code = MPI_Probe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Probe fail.");

   err_code = MPI_Get_count(&status, MPI_BYTE, &pkt_size);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Get_count fail.");
   LOG_ASSERT_ERROR(status.MPI_SOURCE != MPI_UNDEFINED, "Message has undefined status?!");
   source = status.MPI_SOURCE;

   LOG_PRINT("msg found -- size: %i, tag: %i, source: %i", pkt_size, tag, source);

   // Allocate a buffer for the incoming message
   buffer = new Byte[pkt_size];

   err_code = MPI_Recv(buffer, pkt_size, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Recv fail.");

   LOG_PRINT("msg recv'd");

   return buffer;
   // NOTE: the caller should free the buffer when it's finished with it
}

bool MpiTransport::MpiNode::query()
{
   SInt32 flag;
   MPI_Status status;
   int err_code;

   SInt32 tag = getCoreId() == -1 ? PROC_COMM_TAG : getCoreId();

   // Probe for a message from any source but with our ID tag
   err_code = MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, &status);
   LOG_ASSERT_ERROR(err_code == MPI_SUCCESS, "MPI_Iprobe fail.");

   // flag == 0 indicates that no message is waiting
   return (flag != 0);
}
