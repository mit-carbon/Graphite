#ifndef _TIME_TYPES_H_
#define _TIME_TYPES_H_

#include <fixed_types.h>
#include <log.h>
#include <cmath>

typedef volatile float Frequency;


class Latency
{
   public:
      Latency(UInt64 cycles, Frequency freq):_cycles(cycles), _freq(freq){};
      Latency(const Latency& lat):_cycles(lat._cycles),
                                  _freq(lat._freq) {};
      ~Latency(){};

      Latency operator+(const Latency& lat);

      UInt64 toPicosec() const;

      UInt64 getCycles() const {return _cycles; };

   private:
      UInt64 _cycles;
      Frequency _freq;

};

class Time
{
   public:
      Time(UInt64 picosec=0):_picosec(picosec){};
      Time(const Time& time):_picosec(time._picosec){};
      Time(const Latency& lat){_picosec = lat.toPicosec();};
      ~Time(){};

      Time operator+(const Time& time)
            { return Time(_picosec + time._picosec);};

      Time operator+(const Latency& lat)
            { return Time (_picosec + lat.toPicosec());};

      Time operator-(const Time& time) const
            { return Time(_picosec - time._picosec);};

      bool operator>(const Time& time)
            { return _picosec > time._picosec; };

      bool operator<(const Time& time)
            { return _picosec < time._picosec; };

      bool operator<=(const Time& time)
            { return _picosec <= time._picosec; };

      bool operator>=(const Time& time)
            { return _picosec >= time._picosec; };

      Time operator+=(const Time& time)
            { _picosec += time._picosec; return *this; };

      Time operator=(const Time& time)
            { _picosec = time._picosec; return *this; };

      Time operator=(const UInt64& picosec)
            {_picosec = picosec; return *this; };


      UInt64 getTime() const {return _picosec; };

      UInt64 toCycles(Frequency freq);

      UInt64 toNanosec() const;

   private:
      UInt64 _picosec;
};


inline UInt64 Latency::toPicosec() const
{
   UInt64 picosec = (UInt64) ceil( ((double) 1000*_cycles) /  ((double) _freq) );

   LOG_PRINT("Convert cycles(%llu) with frequency(%f) to picoseconds(%llu)",
             _cycles, _freq, _freq, picosec);

   return picosec;
}

inline Latency Latency::operator+(const Latency& lat)
{
   LOG_ASSERT_ERROR(_freq == lat._freq,
      "Attempting to add latencies from different frequencies");

   return Latency(_cycles + lat.getCycles(), _freq);
}

inline UInt64 Time::toCycles(Frequency freq) 
{
   UInt64 cycles = (UInt64) ceil(((double) (_picosec) * ((double) freq))/double(1000));

   LOG_PRINT("Convert picoseconds(%llu) with frequency(%f) to cycles(%llu)",
             _picosec, freq, cycles);

   return cycles;
}

inline UInt64 Time::toNanosec() const
{
   return (UInt64) ceil(((double) _picosec)/double(1000));
}



#endif
