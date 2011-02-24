#include <stdlib.h>
using namespace std;

#include "fxsupport.h"
#include "tile_manager.h"
#include "simulator.h"
#include "tile.h"

FloatingPointHandler::FloatingPointHandler()
{
   is_saved = Fxsupport::getSingleton()->fxsave();
}

FloatingPointHandler::~FloatingPointHandler()
{
   if (is_saved)
      Fxsupport::getSingleton()->fxrstor();
}

Fxsupport *Fxsupport::m_singleton = NULL;

Fxsupport::Fxsupport(tile_id_t num_local_cores):
   m_num_local_cores(num_local_cores)
{
   m_fx_buf = (char**) malloc(m_num_local_cores * sizeof(char*));
   m_context_saved = (bool*) malloc(m_num_local_cores * sizeof(bool));
   for (int i = 0; i < m_num_local_cores; i++)
   {
      // FIXME: I think it is not nice to hard-code these values, esp. 512
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

void Fxsupport::allocate()
{
   assert (m_singleton == NULL);
   tile_id_t num_local_cores = Sim()->getConfig()->getNumLocalTiles();
   m_singleton = new Fxsupport(num_local_cores);
}

void Fxsupport::release()
{
   assert (m_singleton);
   delete m_singleton;
}

Fxsupport *Fxsupport::getSingleton()
{
   assert (m_singleton);
   return m_singleton;
}

bool Fxsupport::fxsave()
{
   if (Sim()->getTileManager()->amiUserThread())
   {
      LOG_PRINT("fxsave() start");

      UInt32 tile_index = Sim()->getTileManager()->getCurrentTileIndex();
      // This check is done to ensure that the thread has not exited
      if (tile_index < Config::getSingleton()->getNumLocalTiles())
      {
         if (!m_context_saved[tile_index])
         {
            m_context_saved[tile_index] = true;

            char *buf = m_fx_buf[tile_index];
            asm volatile ("fxsave %0\n\t"
                          "emms"
                          :"=m"(*buf));
            return true;
         }
         else
         {
            return false;
         }
      }
   
      LOG_PRINT("fxsave() end");
   }
   return false;
}

void Fxsupport::fxrstor()
{
   if (Sim()->getTileManager()->amiUserThread())
   {
      LOG_PRINT("fxrstor() start");
   
      UInt32 tile_index = Sim()->getTileManager()->getCurrentTileIndex();
      if (tile_index < Config::getSingleton()->getNumLocalTiles())
      {
         LOG_ASSERT_ERROR(m_context_saved[tile_index], "Context Not Saved(%u)", tile_index);
         
         m_context_saved[tile_index] = false;

         char *buf = m_fx_buf[tile_index];
         asm volatile ("fxrstor %0"::"m"(*buf));
      }
   
      LOG_PRINT("fxrstor() end");
   }
}
