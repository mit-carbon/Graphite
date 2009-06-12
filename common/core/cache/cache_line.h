#ifndef CACHE_LINE_H
#define CACHE_LINE_H

#include "fixed_types.h"
#include "cache_state.h"

using namespace std;

class CacheBlockInfo
{
   // This can be extended later to include other information
   // for different cache coherence protocols
   private:
      IntPtr m_tag;
      bool m_dirty;
      CacheState::cstate_t m_cstate;

   public:
      CacheBlockInfo(IntPtr tag = ~0, CacheState::cstate_t cstate = CacheState::INVALID) :
            m_tag(tag),
            m_dirty(false),
            m_cstate(cstate)
      {}
      ~CacheBlockInfo() {}

      bool isValid() const { return (m_tag != ((IntPtr) ~0)); }
      bool isDirty() const { return m_dirty; }
      
      IntPtr getTag() const { return m_tag; }
      CacheState::cstate_t getCState() const { return m_cstate; }

      void setCState(CacheState::cstate_t cstate) { m_cstate = cstate; }
      void setDirty() { m_dirty = true; }
};

#endif /* CACHE_LINE_H */
