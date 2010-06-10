#ifndef FXSUPPORT_H
#define FXSUPPORT_H

#include <vector>
using namespace std;

#include "fixed_types.h"

class Fxsupport
{
   public:
      static void init();
      static void fini();

      static Fxsupport *getSingleton();
      static bool isInitialized();

      void fxsave();
      void fxrstor();
   private:
      Fxsupport(core_id_t core_count);
      ~Fxsupport();

      // Per-thread buffers for storing fx state
      char** m_fx_buf;
      bool* m_context_saved;
      core_id_t m_num_local_cores;

      static Fxsupport *m_singleton;
};

#endif
