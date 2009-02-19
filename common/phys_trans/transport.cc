#define TRANSPORT_CC

#include <mpi.h>
#include <assert.h>

#include "transport.h"
#include "smtransport.h"
#include "mpitransport.h"

#include "config.h"
#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE TRANSPORT

// -- Transport -- //

Transport *Transport::m_singleton;

Transport::Transport()
{
}

Transport* Transport::create()
{
   // dynamically choose the transport based on number of processes
   // this is required since MPICH seems to break in single-process mode

   assert(m_singleton == NULL);
   
   if (g_config->getProcessCount() == 1)
      m_singleton = new SmTransport();

   else if (g_config->getProcessCount() > 1)
      m_singleton = new MpiTransport();
   
   else
      LOG_ASSERT_ERROR(false, "*ERROR* Negative no. processes ?!");

   return m_singleton;
}

Transport* Transport::getSingleton()
{
   return m_singleton;
}

// -- Node -- //

Transport::Node::Node(SInt32 core_id)
   : m_core_id(core_id)
{
}

SInt32 Transport::Node::getCoreId()
{
   return m_core_id;
}
