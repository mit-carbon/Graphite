// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: os-services
// <FILE-TYPE>: component public header

#ifndef OS_SERVICES_HPP
#define OS_SERVICES_HPP

/*! @mainpage OS_SERVICES library
 *
 * The OS_SERVICES library provides a generic interface to those OS system services which are
 * not already standardized by the C++ language.
 */

/*! @brief The OS_SERVICES library. */
namespace OS_SERVICES {}

#include "os-services/tcp.hpp"
#include "os-services/threads.hpp"
#include "os-services/processes.hpp"

#endif // file guard
