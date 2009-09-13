#ifndef __DRAM_CNTLR_H__
#define __DRAM_CNTLR_H__

#include <map>

#include "fixed_types.h"

namespace PrL1PrL2DramDirectory
{
   class DramCntlr
   {
      private:
         std::map<IntPtr, Byte*> m_data_map;
         UInt32 m_cache_block_size;

         UInt32 getCacheBlockSize() { return m_cache_block_size; }

      public:
         DramCntlr(UInt32 cache_block_size);
         ~DramCntlr();

         void getDataFromDram(IntPtr address, Byte* data_buf);
         void putDataToDram(IntPtr address, Byte* data_buf);
   };
}

#endif /* __DRAM_CNTLR_H__ */
