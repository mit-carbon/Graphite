// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: fund
// <FILE-TYPE>: component public header

#ifndef FUND_CONFIG_H
#define FUND_CONFIG_H

// Possible CPU types.
//
#define FUND_CPU_NONE       0
#define FUND_CPU_IA32       1
#define FUND_CPU_INTEL64    2
#define FUND_CPU_IA64       3

// Possible OS types.
//
#define FUND_OS_NONE        0
#define FUND_OS_LINUX       1
#define FUND_OS_WINDOWS     2
#define FUND_OS_MAC         3


// If the build system doesn't pre-define these, look for a header with
// the definition.  This allows an installation to hard-code a configuration,
// thus eliminating the need for these macros to be defined on the compiler
// command line.
//
#if !defined(FUND_TC_HOSTCPU)
#   include "fund/config-hostcpu.h"
#endif
#if !defined(FUND_TC_HOSTOS)
#   include "fund/config-hostos.h"
#endif
#if !defined(FUND_TC_TARGETCPU)
#   include "fund/config-targetcpu.h"
#endif
#if !defined(FUND_TC_TARGETOS)
#   include "fund/config-targetos.h"
#endif


#if (FUND_TC_HOSTCPU == FUND_CPU_IA32) && (FUND_TC_HOSTOS == FUND_OS_LINUX)
#   define FUND_HOST_IA32
#   define FUND_HOST_X86
#   define FUND_HOST_LINUX
#   define FUND_HOST_UNIX
#   define FUND_HOST_IA32_LINUX
#   define FUND_HOST_IA32_UNIX
#   define FUND_HOST_X86_LINUX
#   define FUND_HOST_X86_UNIX
#elif (FUND_TC_HOSTCPU == FUND_CPU_INTEL64) && (FUND_TC_HOSTOS == FUND_OS_LINUX)
#   define FUND_HOST_INTEL64
#   define FUND_HOST_X86
#   define FUND_HOST_LINUX
#   define FUND_HOST_UNIX
#   define FUND_HOST_INTEL64_LINUX
#   define FUND_HOST_INTEL64_UNIX
#   define FUND_HOST_X86_LINUX
#   define FUND_HOST_X86_UNIX
#elif (FUND_TC_HOSTCPU == FUND_CPU_IA64) && (FUND_TC_HOSTOS == FUND_OS_LINUX)
#   define FUND_HOST_IA64
#   define FUND_HOST_LINUX
#   define FUND_HOST_UNIX
#   define FUND_HOST_IA64_LINUX
#   define FUND_HOST_IA64_UNIX
#elif (FUND_TC_HOSTCPU == FUND_CPU_IA32) && (FUND_TC_HOSTOS == FUND_OS_WINDOWS)
#   define FUND_HOST_IA32
#   define FUND_HOST_X86
#   define FUND_HOST_WINDOWS
#   define FUND_HOST_IA32_WINDOWS
#   define FUND_HOST_X86_WINDOWS
#elif (FUND_TC_HOSTCPU == FUND_CPU_INTEL64) && (FUND_TC_HOSTOS == FUND_OS_WINDOWS)
#   define FUND_HOST_INTEL64
#   define FUND_HOST_X86
#   define FUND_HOST_WINDOWS
#   define FUND_HOST_INTEL64_WINDOWS
#   define FUND_HOST_X86_WINDOWS
#elif (FUND_TC_HOSTCPU == FUND_CPU_IA32) && (FUND_TC_HOSTOS == FUND_OS_MAC)
#   define FUND_HOST_IA32
#   define FUND_HOST_X86
#   define FUND_HOST_MAC
#   define FUND_HOST_UNIX
#   define FUND_HOST_IA32_MAC
#   define FUND_HOST_IA32_UNIX
#   define FUND_HOST_X86_MAC
#   define FUND_HOST_X86_UNIX
#else
#   error "Illegal host CPU / OS combination"
#endif

#if (FUND_TC_TARGETCPU == FUND_CPU_IA32) && (FUND_TC_TARGETOS == FUND_OS_LINUX)
#   define FUND_TARGET_IA32
#   define FUND_TARGET_X86
#   define FUND_TARGET_LINUX
#   define FUND_TARGET_UNIX
#   define FUND_TARGET_IA32_LINUX
#   define FUND_TARGET_IA32_UNIX
#   define FUND_TARGET_X86_LINUX
#   define FUND_TARGET_X86_UNIX
#elif (FUND_TC_TARGETCPU == FUND_CPU_INTEL64) && (FUND_TC_TARGETOS == FUND_OS_LINUX)
#   define FUND_TARGET_INTEL64
#   define FUND_TARGET_X86
#   define FUND_TARGET_LINUX
#   define FUND_TARGET_UNIX
#   define FUND_TARGET_INTEL64_LINUX
#   define FUND_TARGET_INTEL64_UNIX
#   define FUND_TARGET_X86_LINUX
#   define FUND_TARGET_X86_UNIX
#elif (FUND_TC_TARGETCPU == FUND_CPU_IA64) && (FUND_TC_TARGETOS == FUND_OS_LINUX)
#   define FUND_TARGET_IA64
#   define FUND_TARGET_LINUX
#   define FUND_TARGET_UNIX
#   define FUND_TARGET_IA64_LINUX
#   define FUND_TARGET_IA64_UNIX
#elif (FUND_TC_TARGETCPU == FUND_CPU_IA32) && (FUND_TC_TARGETOS == FUND_OS_WINDOWS)
#   define FUND_TARGET_IA32
#   define FUND_TARGET_X86
#   define FUND_TARGET_WINDOWS
#   define FUND_TARGET_IA32_WINDOWS
#   define FUND_TARGET_X86_WINDOWS
#elif (FUND_TC_TARGETCPU == FUND_CPU_INTEL64) && (FUND_TC_TARGETOS == FUND_OS_WINDOWS)
#   define FUND_TARGET_INTEL64
#   define FUND_TARGET_X86
#   define FUND_TARGET_WINDOWS
#   define FUND_TARGET_INTEL64_WINDOWS
#   define FUND_TARGET_X86_WINDOWS
#elif (FUND_TC_TARGETCPU == FUND_CPU_IA32) && (FUND_TC_TARGETOS == FUND_OS_MAC)
#   define FUND_TARGET_IA32
#   define FUND_TARGET_X86
#   define FUND_TARGET_MAC
#   define FUND_TARGET_UNIX
#   define FUND_TARGET_IA32_MAC
#   define FUND_TARGET_IA32_UNIX
#   define FUND_TARGET_X86_MAC
#   define FUND_TARGET_X86_UNIX
#elif (FUND_TC_TARGETCPU == FUND_CPU_NONE) && (FUND_TC_TARGETOS == FUND_OS_NONE)
#else
#   error "Illegal target CPU / OS combination"
#endif

#endif // file guard
