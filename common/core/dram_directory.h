#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "dram_directory_entry.h"

class DramDirectory
{
 private:
  int num_lines;
  unsigned int bytes_per_cache_line;
  DramDirectoryEntry *dram_directory_entries;
  
 public:
  DramDirectory(int num_lines, unsigned int bytes_per_cache_line);
  virtual ~DramDirectory();
  DramDirectoryEntry getEntry(int address);
};

#endif
