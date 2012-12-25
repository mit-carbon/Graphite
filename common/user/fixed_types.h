#ifndef __FIXED_TYPES_H
#define __FIXED_TYPES_H

#include <stdint.h>

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
typedef enum { MAIN_CORE_TYPE = 0 } core_type_t;

// Carbon core types
typedef struct core_id_t {
   SInt32 tile_id;
   UInt32 core_type;
} core_id_t;

typedef SInt32 tile_id_t;
typedef SInt32 thread_id_t;

#define INVALID_THREAD_ID ((thread_id_t) -1)
#define INVALID_CORE_ID ((core_id_t) {-1,0})
#define INVALID_TILE_ID ((tile_id_t) -1)
#define INVALID_ADDRESS  ((IntPtr) -1)

#define UINT64_MAX_ ((UInt64)-1)
#define UINT32_MAX_ ((UInt32)-1)

#endif
