#pragma once

class DirectoryState
{
public:
   enum Type
   {
      UNCACHED = 0,
      SHARED,
      OWNED,
      MODIFIED,
      NUM_DIRECTORY_STATES
   };
};
