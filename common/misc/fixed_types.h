#ifndef __FIXED_TYPES_H
#define __FIXED_TYPES_H

typedef unsigned int UInt32;
typedef signed short SInt16;
typedef signed char SInt8;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef signed int Int32;
typedef signed int SInt32;
typedef signed long long Int64;

typedef UInt32 DataWord;
typedef UInt32 InstrWord;



typedef long long SInt64;
typedef unsigned long long UInt64;
typedef UInt64 NetworkWord;
typedef UInt64 SwitchInstrWord;
typedef UInt8 byte;
typedef UInt8 Boolean;

#ifdef __cplusplus
extern "C" {
UInt32 convert_to_little_endian(UInt32 convertMe);
};
#else
UInt32 convert_to_little_endian(UInt32 convertMe);
#endif



#ifdef __cplusplus
template<class T>
T & ref(T  &foo) { return foo; }
#endif

//typedef UInt8 bool;
#endif // __FIXED_TYPES_H
