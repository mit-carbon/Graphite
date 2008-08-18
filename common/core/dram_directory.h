#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "pin.H"
#include "dram_directory_entry.h"

class DramDirectory
{
 private:
  UINT32 num_lines;
  unsigned int bytes_per_cache_line;
  DramDirectoryEntry *dram_directory_entries;
  UINT32 dram_id;
 public:
  DramDirectory(UINT32 num_lines, unsigned int bytes_per_cache_line, UINT32 dram_id_arg);
  virtual ~DramDirectory();
  DramDirectoryEntry getEntry(ADDRINT address);
};

#endif
