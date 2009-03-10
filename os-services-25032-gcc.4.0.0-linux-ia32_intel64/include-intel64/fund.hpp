// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: fund
// <FILE-TYPE>: component public header

#ifndef FUND_HPP
#define FUND_HPP

#if defined(__GNUC__)
#   include <stdint.h>
#endif
#include "fund/config.h"


/*!
 * The FUND namespace provides a small set of fundamental types that are used
 * by many other components.
 *
 * The build system may define the following macros.  In some installations, these
 * might be pre-defined, in which case it is not necessary for the build system to
 * define them.
 *
 *      FUND_TC_HOSTCPU
 *          Defined to one of FUND_CPU_IA32, FUND_CPU_INTEL64, FUND_CPU_IA64
 *      FUND_TC_HOSTOS
 *          Defined to one of FUND_OS_LINUX, FUND_OS_WINDOWS, FUND_OS_MAC
 *      FUND_TC_TARGETCPU
 *          Defined to one of FUND_CPU_NONE, FUND_CPU_IA32, FUND_CPU_INTEL64, FUND_CPU_IA64
 *      FUND_TC_TARGETOS
 *          Defined to one of FUND_OS_NONE, FUND_OS_LINUX, FUND_OS_WINDOWS, FUND_OS_MAC
 */
namespace FUND {

#if defined(_MSC_VER)

// Microsoft Visual C/C++ compiler

typedef unsigned __int8 UINT8;      ///< 8-bit unsigned integer
typedef unsigned __int16 UINT16;    ///< 16-bit unsigned integer
typedef unsigned __int32 UINT32;    ///< 32-bit unsigned integer
typedef unsigned __int64 UINT64;    ///< 64-bit unsigned integer
typedef __int8 INT8;                ///< 8-bit signed integer
typedef __int16 INT16;              ///< 16-bit signed integer
typedef __int32 INT32;              ///< 32-bit signed integer
typedef __int64 INT64;              ///< 64-bit signed integer

/*!
 * Use this when defining a symbol that should be exported from a DLL.
 */
#define FUND_EXPORT __declspec(dllexport)

#elif defined(__GNUC__)

// GNU C/C++ compiler

typedef uint8_t  UINT8;             ///< 8-bit unsigned integer
typedef uint16_t UINT16;            ///< 16-bit unsigned integer
typedef uint32_t UINT32;            ///< 32-bit unsigned integer
typedef uint64_t UINT64;            ///< 64-bit unsigned integer
typedef int8_t  INT8;               ///< 8-bit signed integer
typedef int16_t INT16;              ///< 16-bit signed integer
typedef int32_t INT32;              ///< 32-bit signed integer
typedef int64_t INT64;              ///< 64-bit signed integer

/*!
 * Use this when defining a symbol that should be exported from a DLL.
 */
#define FUND_EXPORT

#endif


#if defined(FUND_HOST_IA32)

/*!
 * Unsigned integer of the same size as a pointer on the host system
 * (the system where the program runs).  Conversions to/from PTRINT and
 * pointers do not lose data.
 */
typedef UINT32 PTRINT;
#define FUND_PTRINT_SIZE 32

#elif defined(FUND_HOST_INTEL64) || defined(FUND_HOST_IA64)

/*!
 * Unsigned integer of the same size as a pointer on the host system
 * (the system where the program runs).  Conversions to/from PTRINT and
 * pointers do not lose data.
 */
typedef UINT64 PTRINT;
#define FUND_PTRINT_SIZE 64

#endif


#if defined(FUND_TARGET_IA32)

/*!
 * Unsigned integer of the same size as a pointer on the target system.
 * The concept of "target" system makes sense only for programs that analyze or
 * generate code.  Consider a cross-compiler as an example that runs on system
 * X and generates code for system Y.  In this example, PTRINT represents a pointer
 * on system X (the host system) and ADDRINT represents a pointer on system Y (the
 * target system).
 */
typedef UINT32  ADDRINT;
#define FUND_ADDRINT_SIZE 32

#elif defined(FUND_TARGET_INTEL64) || defined(FUND_TARGET_IA64)

/*!
 * Unsigned integer of the same size as a pointer on the target system.
 * The concept of "target" system makes sense only for programs that analyze or
 * generate code.  Consider a cross-compiler as an example that runs on system
 * X and generates code for system Y.  In this example, PTRINT represents a pointer
 * on system X (the host system) and ADDRINT represents a pointer on system Y (the
 * target system).
 */
typedef UINT64  ADDRINT;
#define FUND_ADDRINT_SIZE 64

#endif

} // namespace
#endif // file guard
