#include <stdlib.h>
#include "fxsupport.h"
#include "core_manager.h"
#include "simulator.h"

Fxsupport *Fxsupport::m_singleton;

Fxsupport::Fxsupport(core_id_t core_count)
{
   m_core_count = core_count;
   m_fx_buf = (char**) malloc (m_core_count * sizeof (char*));
   for (int i = 0; i < m_core_count; i++)
   {
      posix_memalign ((void**) &m_fx_buf[i], 16, 512);
   }
}

Fxsupport::~Fxsupport()
{
   for (int i = 0; i < m_core_count; i++)
      free ((void*) m_fx_buf[i]);

   free (m_fx_buf);
}

void Fxsupport::init()
{
   assert (m_singleton == NULL);
   core_id_t core_count = Sim()->getConfig()->getNumLocalCores();
   m_singleton = new Fxsupport (core_count);
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

void Fxsupport::fxsave()
{
   LOG_PRINT("fxsave() start");
   
   core_id_t core_index = Sim()->getCoreManager()->getCurrentCoreIndex();
   char *buf = m_fx_buf[core_index];
   asm volatile ("fxsave %0\n\t"
                 "emms"
                 :"=m"(*buf));
   
   LOG_PRINT("fxsave() end");
}

void Fxsupport::fxrstor()
{
   LOG_PRINT("fxrstor() start");
   
   core_id_t core_index = Sim()->getCoreManager()->getCurrentCoreIndex();
   char *buf = m_fx_buf[core_index];
   asm volatile ("fxrstor %0"::"m"(*buf));
   
   LOG_PRINT("fxrstor() end");
}
