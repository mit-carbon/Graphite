// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: os-services
// <FILE-TYPE>: component public header

#ifndef OS_SERVICES_TCP_HPP
#define OS_SERVICES_TCP_HPP

#include "util/data.hpp"


namespace OS_SERVICES {

/*!
 * A class that provides TCP/IP services.
 */
class /*<INTERFACE>*/ ITCP
{
public:
    static ITCP *GetSingleton();    ///< @return The singleton object.

    /*!
     * This must be called at least once, before using any other TCP service.
     *
     * @return  FALSE if there was some error initilizing the TCP services.
     *           TRUE on success or if the TCP services have already been initialized.
     */
    virtual bool Initialize() = 0;

    /*!
     * This should be called after the client has completed all TCP services.
     */
    virtual void Cleanup() = 0;
    virtual ~ITCP();
};


/*!
 * A TCP socket, either connected or unconnected.
 *
 * On Unix, these sockets are configured to automatically close (ungracefully) if the calling
 * process does an exec().
 */
class /*<INTERFACE>*/ ISOCK
{
public:
    /*!
     * Create a server-like TCP socket that will block waiting for another socket to
     * connect to it.  The system binds the socket to an arbitrary free local port,
     * which the client can get via GetLocalPort().
     *
     * @return  An unconnected socket or NULL on error.
     */
    static ISOCK *CreateServer();

    /*!
     * Create a client-like TCP socket that will connect to a specific remote address.
     * The system binds the socket to an arbitrary free local port, which the client
     * can get via GetLocalPort().
     *
     *  @param[in] ip       The remote IP address.
     *  @param[in] port     The remote port number to connect to.
     *
     * @return  An unconnected socket or NULL on error.
     */
    static ISOCK *CreateClient(const char *ip, int port);

    virtual int GetLocalPort() = 0;     ///< @return The local port number for the socket.

    /*!
     * Connect a client-like socket if it is not already connected.  This method
     * blocks until a server socket accepts the connection.
     *
     * @return  TRUE on success or if the socket is already connected, FALSE if the
     *           socket cannot be connected or if it is a server-like socket.
     */
    virtual bool ConnectToServer() = 0;

    /*!
     * Accept a connection on a server-like socket.  This method blocks until a
     * client attempts to connect to this server.
     *
     * @return  On success, a new socket representing the new connection to the
     *           client.  The original server socket remains valid and can be
     *           re-connected to a new client.
     */
    virtual ISOCK *AcceptConnection() = 0;

    /*!
     * Send a packet of data on a connected socket.
     *
     *  @param[in] data     The data to send.
     *
     * @return  TRUE if the data is sent, FALSE if not or if the socket is unconnected.
     */
    virtual bool Send(const UTIL::DATA &data) = 0;

    /*!
     * Receive data on a connected socket.
     *
     *  @param[in,out] data     On input, provides a buffer to recieve the data.
     *                           The size of \a data tells how much to attempt to read.
     *                           On output, \a data is resized to the actual amount
     *                           of data read.
     *
     * @return  TRUE on success, FALSE on error or if the socket is unconnected.
     */
    virtual bool Receive(UTIL::DATA *data) = 0;

    /*!
     * Close down and disconnect a socket.
     *
     * @return  FALSE if there is an error while closing the socket.
     */
    virtual bool Close() = 0;

    /*!
     * On Unix after a fork, both parent and child inherit a copy of the socket.
     * One of the two processes must call CloseAfterFork() to close the socket.
     * The other can continue to use the socket, and should eventually call Close()
     * to close down the connection (or call the ISOCK destructor, which implicitly
     * calls Close()).
     */
    virtual bool CloseAfterFork() = 0;

    /*!
     * The socket is automatically closed when ISOCK is destroyed.
     */
    virtual ~ISOCK() {}
};


/*!
 * An unordered container of sockets.
 */
class /*<INTERFACE>*/ ISOCKSET
{
public:
    static ISOCKSET *Create();    ///< @return  An empty socket container.

    /*!
     * Add a new socket to the container.
     *
     *  @param[in] sock     The socket to add.
     */
    virtual void Add(ISOCK *sock) = 0;

    /*!
     * Add all elements from a socket container to this container.
     *
     *  @param[in] socks    Elements are read from this container.
     */
    virtual void AddSet(ISOCKSET *socks) = 0;

    virtual void Clear() = 0;           ///< Remove all sockets in the container, making it empty.
    virtual unsigned GetCount() = 0;    ///< @return The number of sockets in the container.

    /*!
     * Retrieves a socket from the container.
     *
     *  @param[in] index    The index of the socket to return, in the range [0, n-1] where
     *                       \i n is from GetCount().
     *
     * @return  The indexed socket or NULL if \a index is out of range.
     */
    virtual ISOCK *GetIndexedItem(unsigned index) = 0;

    /*!
     * Destroying the container does not destroy or close the contained sockets.
     */
    virtual ~ISOCKSET() {}
};


/*!
 * An object that allows a calling thread to block until one (or more) of several sockets
 * is ready to read or write data.
 */
class /*<INTERFACE>*/ ISOCK_WAITER
{
public:
    /*!
     * Create a new wait object.
     *
     * @return  A new wait object.
     */
    static ISOCK_WAITER *Create();

    /*!
     * Change the set of sockets that this wait object will wait for, replacing any previous
     * set.
     *
     *  @param[in] readers  The Wait() method will return when any socket in this list has
     *                       data available to read.  (I.e. ISOCK::Receive() or
     *                       ISOCK::AcceptConnection() would not block.)  May be NULL.
     *  @param[in] writers  The Wait() method will return when any socket in this list can
     *                       be written to.  (I.e. ISOCK::Send() or ISOCK::ConnectToServer()
     *                       would not block.)  May be NULL.
     */
    virtual void SetSockets(ISOCKSET *readers, ISOCKSET *writers) = 0;

    /*!
     * Change the set of sockets that this wait object will wait for, replacing any previous
     * set.  This has the same effect as SetSockets(), except that at most only one reader and
     * one writer socket can be specified.
     *
     *  @param[in] reader   The Wait() method will return if this socket has data available
     *                       to read.  May be NULL.
     *  @param[in] writer   The Wait() method will return if this socket can be written to.
     *                       May be NULL.
     */
    virtual void SetSingleSockets(ISOCK *reader, ISOCK *writer) = 0;

    /*!
     * On Unix after a fork, both parent and child inherit the ISOCK_WAITER object.
     * One of the two processes must call CloseAfterFork() to clean up internal
     * resources.  After this call, the wait object becomes unusable and should
     * be destroyed via the ~ISOCK_WAITER() destructor.
     */
    virtual void CloseAfterFork() = 0;

    /*!
     * Block the calling thread until an action can be performed on one of the "reader"
     * or "writer" sockets.  This method returns immediately if the sticky "interrupt
     * flag" is set.
     */
    virtual void Wait() = 0;

    /*!
     * After Wait() returns, this method tells the set of sockets that have data
     * available to read.
     *
     *  @param[out] sockets     If not NULL, receives the list of readable sockets.
     *
     * @return  TRUE if there is at least one readable socket.
     */
    virtual bool GetReadable(ISOCKSET *sockets) = 0;

    /*!
     * After Wait() returns, this method tells the set of sockets that can be
     * written to.
     *
     *  @param[out] sockets     If not NULL, receives the list of writeable sockets.
     *
     * @return  TRUE if there is at least one writeable socket.
     */
    virtual bool GetWriteable(ISOCKSET *sockets) = 0;

    /*!
     * After Wait() returns, this method tells if Wait() returned because another
     * thread called Interrupt().
     *
     * @return  TRUE if Interrupt() was called on this wait object.
     */
    virtual bool WasInterrupted() = 0;

    /*!
     * This method may be called by a second thread while a different thread is blocked
     * in Wait().  Doing so causes the blocked thread to return from Wait() with no
     * readable or writeable sockets.
     *
     * This method also sets a sticky "interrupt flag".  If a call is made to Wait()
     * while this flag is set, the call immediately returns.
     *
     * @return  TRUE on success.  FALSE if there is an internal error.
     */
    virtual bool Interrupt() = 0;

    virtual void ClearInterruptFlag() = 0;  ///< Clears the sticky "interrupt flag".
    virtual ~ISOCK_WAITER() {}              ///< Destroys the wait object.
};

} // namespace
#endif // file guard
