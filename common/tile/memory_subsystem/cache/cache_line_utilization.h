#pragma once

class CacheLineUtilization
{
public:
   CacheLineUtilization()
      : read(0), write(0) {}
   CacheLineUtilization(const CacheLineUtilization& util)
      : read(util.read), write(util.write) {}
   ~CacheLineUtilization() {}

   UInt64 read;
   UInt64 write;

   UInt64 total()
   { return read + write; }

   void operator+=(const CacheLineUtilization& util)
   {
      read += util.read;
      write += util.write;
   }

   CacheLineUtilization operator/(UInt64 divisor)
   {
      CacheLineUtilization res;
      res.read = read / divisor;
      res.write = write / divisor;
      return res;
   }

   CacheLineUtilization operator*(UInt64 multiplier)
   {
      CacheLineUtilization res;
      res.read = read * multiplier;
      res.write = write * multiplier;
      return res;
   }
};
