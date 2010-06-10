#include <stdlib.h>
using namespace std;

#include "fxsupport.h"
#include "core_manager.h"
#include "simulator.h"
#include "core.h"

Fxsupport *Fxsupport::m_singleton = NULL;

Fxsupport::Fxsupport(core_id_t num_local_cores):
   m_num_local_cores(num_local_cores)
{
   m_fx_buf = (char**) malloc(m_num_local_cores * sizeof(char*));
   m_context_saved = (bool*) malloc(m_num_local_cores * sizeof(bool));
   for (int i = 0; i < m_num_local_cores; i++)
   {
      int status = posix_memalign ((void**) &m_fx_buf[i], 16, 512);
      assert (status == 0);
      m_context_saved[i] = false;
   }
}

Fxsupport::~Fxsupport()
{
   for (int i = 0; i < m_num_local_cores; i++)
      free ((void*) m_fx_buf[i]);
   free((void*) m_fx_buf);
   free((void*) m_context_saved);
}

void Fxsupport::init()
{
   assert (m_singleton == NULL);
   core_id_t num_local_cores = Sim()->getConfig()->getNumLocalCores();
   m_singleton = new Fxsupport(num_local_cores);
}

void Fxsupport::fini()
{
   assert (m_singleton);
   delete m_singleton;
}

Fxsupport *Fxsupport::getSingleton()
{
   assert (m_singleton);
   return m_singleton;
}

bool Fxsupport::isInitialized()
{
   return (m_singleton != NULL);
}

void Fxsupport::fxsave()
{
   if (Sim()->getCoreManager()->amiUserThread())
   {
      LOG_PRINT("fxsave() start");

      UInt32 core_index = Sim()->getCoreManager()->getCurrentCoreIndex();
      // This check is done to ensure that the thread has not exited
      if (core_index < Config::getSingleton()->getNumLocalCores())
      {
         // LOG_PRINT_WARNING("fxsave(%u) start", core_index);
         LOG_ASSERT_ERROR(!m_context_saved[core_index], "Context for Core(%i) already saved",
               Sim()->getCoreManager()->getCoreFromIndex(core_index)->getId());
         m_context_saved[core_index] = true;

         char *buf = m_fx_buf[core_index];
         asm volatile ("fxsave %0\n\t"
                       "emms"
                       :"=m"(*buf));
         
         // LOG_PRINT_WARNING("fxsave(%u) end", core_index);
      }
   
      LOG_PRINT("fxsave() end");
   }
}

void Fxsupport::fxrstor()
{
   if (Sim()->getCoreManager()->amiUserThread())
   {
      LOG_PRINT("fxrstor() start");
   
      UInt32 core_index = Sim()->getCoreManager()->getCurrentCoreIndex();
      if (core_index < Config::getSingleton()->getNumLocalCores())
      {
         // LOG_PRINT_WARNING("fxrstor(%u) start", core_index);
         LOG_ASSERT_ERROR(m_context_saved[core_index], "Context for Core(%i) not saved",
               Sim()->getCoreManager()->getCoreFromIndex(core_index)->getId());
         m_context_saved[core_index] = false;

         char *buf = m_fx_buf[core_index];
         asm volatile ("fxrstor %0"::"m"(*buf));
         
         // LOG_PRINT_WARNING("fxrstor(%u) end", core_index);
      }
   
      LOG_PRINT("fxrstor() end");
   }
}
