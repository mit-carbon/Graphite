// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: os-services
// <FILE-TYPE>: component public header

#ifndef OS_SERVICES_THREADS_HPP
#define OS_SERVICES_THREADS_HPP

#include "os-services/forward.hpp"


namespace OS_SERVICES {

/*!
 * Low-level services for creating and managing threads.  This functionality
 * is layered directly on the underlying O/S services, bypassing higher level
 * libraries like pthreads.
 */
class /*<INTERFACE>*/ ITHREADS
{
public:
    static ITHREADS *GetSingleton();    ///< @return The singleton object.

    /*!
     * Create a new low-level thread.
     *
     *  @param[in] stackSize    Size (bytes) of the stack for the thread's stack.
     *  @param[in] obj          Object which holds client data for the new thread.  The new
     *                           thread starts running on the object's RunThread() method.
     *
     * @return  On success, an object representing the new thread.  NULL on failure.
     */
    virtual ITHREAD *Spawn(size_t stackSize, ITHREAD_RUNNER *obj) = 0;

    /*!
     * Attempt to yield the processor to some other thread.
     */
    virtual void YieldMe() = 0;

    /*!
     * @return  The O/S ID for the calling thread.
     */
    virtual unsigned long GetSysId() = 0;

    /*!
     * Terminate the calling thread, even if it was not created via Spawn().  Threads
     * created via Spawn() should terminate with ITHREAD::Exit() instead, because that
     * method also marks the ITHREAD object as "not active".
     */
    virtual void Exit() = 0;

    virtual ~ITHREADS() {}
};


/*!
 * Interface that represents a thread created via ITHREADS::Spawn().
 */
class /*<INTERFACE>*/ ITHREAD
{
public:
    /*!
     * This method may only be called by the thread itself.  It causes
     * that thread to terminate, thus this method does not return.  This
     * method does NOT destroy the ITHREAD object nor clean up associated
     * data structures.
     */
    virtual void Exit() = 0;

    /*!
     * This method is typically called by some other thread in order to tell
     * if the created thread has exitted.
     *
     * @return  TRUE if the created thread is still running, FALSE if it has
     *           exitted.
     */
    virtual bool IsActive() = 0;

    /*!
     * The destructor cleans up any resources for the thread.  It must NOT
     * be called until after the thread has exitted.  Therefore, it must not
     * be called by the thread itself.
     */
    virtual ~ITHREAD() {}
};


/*!
 * Client code should implement this interface in order to use ITHREADS::Spawn().
 */
class /*<INTERFACE>*/ ITHREAD_RUNNER
{
public:
    /*!
     * The new thread starts running at this method.  The thread may terminate either
     * by returning from this method or by calling me->Exit().
     *
     *  @param[in] me   An object representing this running thread.
     */
    virtual void RunThread(ITHREAD *me) = 0;
    virtual ~ITHREAD_RUNNER() {}
};

} // namespace
#endif // file guard
