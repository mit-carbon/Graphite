#define TRANSPORT_CC

#include <assert.h>

#include "transport.h"
#include "smtransport.h"
//#include "mpitransport.h"
#include "socktransport.h"

#include "config.h"
#include "log.h"

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

   if (true)
      m_singleton = new SockTransport();
   
   // else if (Config::getSingleton()->getProcessCount() == 1)
   //    m_singleton = new SmTransport();

   // else if (Config::getSingleton()->getProcessCount() > 1)
   //    m_singleton = new MpiTransport();
   
   else
      LOG_PRINT_ERROR("Negative no. processes ?!");

   return m_singleton;
}

Transport* Transport::getSingleton()
{
   return m_singleton;
}

// -- Node -- //

Transport::Node::Node(tile_id_t tile_id)
   : m_tile_id(tile_id)
{
}

tile_id_t Transport::Node::getTileId()
{
   return m_tile_id;
}
