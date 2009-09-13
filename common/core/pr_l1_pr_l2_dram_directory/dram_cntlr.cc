#include <string.h>
using namespace std;

#include "dram_cntlr.h"
#include "log.h"

namespace PrL1PrL2DramDirectory
{

DramCntlr::DramCntlr(UInt32 cache_block_size):
   m_cache_block_size(cache_block_size)
{}

DramCntlr::~DramCntlr()
{}

void
DramCntlr::getDataFromDram(IntPtr address, Byte* data_buf)
{
   if (m_data_map[address] == NULL)
   {
      m_data_map[address] = new Byte[getCacheBlockSize()];
      memset((void*) m_data_map[address], 0x00, getCacheBlockSize());
   }
   memcpy((void*) data_buf, (void*) m_data_map[address], getCacheBlockSize());
}

void
DramCntlr::putDataToDram(IntPtr address, Byte* data_buf)
{
   if (m_data_map[address] == NULL)
   {
      LOG_PRINT_ERROR("Data Buffer does not exist");
   }
   memcpy((void*) m_data_map[address], (void*) data_buf, getCacheBlockSize());
}

}
