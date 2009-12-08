#ifndef FXSUPPORT_H
#define FXSUPPORT_H

#include "fixed_types.h"

class Fxsupport
{
   public:
      static void init();
      static void fini();

      static Fxsupport *getSingleton();

      void fxsave();
      void fxrstor();
   private:
      Fxsupport(core_id_t core_count);
      ~Fxsupport();

      // Per-thread buffers for storing fx state
      char **m_fx_buf;
      core_id_t m_core_count;

      static Fxsupport *m_singleton;
};

#endif
