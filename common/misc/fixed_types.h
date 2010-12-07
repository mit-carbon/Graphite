#ifndef __FIXED_TYPES_H
#define __FIXED_TYPES_H

#include <stdint.h>

// Why are we using a C library...
//#ifdef __cplusplus
//extern "C++" {
//#include <utility>
//}
//#endif

typedef uint64_t UInt64;
typedef uint32_t UInt32;
typedef uint16_t UInt16;
typedef uint8_t  UInt8;

typedef int64_t SInt64;
typedef int32_t SInt32;
typedef int16_t SInt16;
typedef int8_t  SInt8;

typedef UInt8 Byte;
typedef UInt8 Boolean;

typedef uintptr_t IntPtr;

typedef uintptr_t carbon_reg_t;
typedef enum core_type_t { MAIN_CORE_TYPE = 0, PEP_CORE_TYPE } core_type_t;

// Carbon core types
//typedef std::pair<SInt32, UInt32> core_id_t;
typedef struct {
   SInt32 first;
   UInt32 second;
} core_id_t;

typedef SInt32 tile_id_t;

#define INVALID_CORE_ID ((core_id_t) {-1,0})
#define INVALID_TILE_ID ((tile_id_t) -1)
#define INVALID_ADDRESS  ((IntPtr) -1)


#endif
