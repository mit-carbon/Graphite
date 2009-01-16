// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: os-services
// <FILE-TYPE>: component public header

#ifndef OS_SERVICES_PROCESSES_HPP
#define OS_SERVICES_PROCESSES_HPP

#include "os-services/forward.hpp"


namespace OS_SERVICES {

/*!
 * Retrieve information about the calling process.
 */
class /*<INTERFACE>*/ IPROCESSES
{
public:
    static IPROCESSES *GetSingleton();  ///< @return The singleton object.

    /*!
     * @return  The name of the calling process, typically the initial value
     *           of argv[0] passed to main().
     */
    virtual std::string GetName() = 0;
    virtual ~IPROCESSES() {}
};

} // namespace
#endif // file guard
