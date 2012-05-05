#pragma once

#include "fixed_types.h"
#include "directory_type.h"

namespace PrL1ShL2MSI
{

class L2DirectoryCfg
{
public:
   static DirectoryType getDirectoryType()   { return _directory_type; }
   static SInt32 getMaxHWSharers()           { return _max_hw_sharers; }
   static SInt32 getMaxNumSharers()          { return _max_num_sharers; }

   static void setDirectoryType(DirectoryType directory_type)     { _directory_type = directory_type; }
   static void setMaxHWSharers(SInt32 max_hw_sharers)             { _max_hw_sharers = max_hw_sharers; }
   static void setMaxNumSharers(SInt32 max_num_sharers)           { _max_num_sharers = max_num_sharers; }

private:
   static DirectoryType _directory_type;
   static SInt32 _max_hw_sharers;
   static SInt32 _max_num_sharers;
};

}
