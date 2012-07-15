#pragma once

#include "string"
using std::string;

class DirectoryState
{
public:
   enum Type
   {
      UNCACHED = 0,
      SHARED,
      OWNED,
      EXCLUSIVE,
      MODIFIED,
      NUM_DIRECTORY_STATES
   };

   static string getName(Type type);
};

#define SPELL_DSTATE(x)    (DirectoryState::getName(x).c_str())
