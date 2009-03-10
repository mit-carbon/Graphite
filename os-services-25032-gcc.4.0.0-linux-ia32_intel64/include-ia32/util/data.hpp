// <ORIGINAL-AUTHOR>: Greg Lueck
// <COMPONENT>: util
// <FILE-TYPE>: component public header

#ifndef UTIL_DATA_HPP
#define UTIL_DATA_HPP

#include <string>
#include <string.h>
#include "fund.hpp"


namespace UTIL {

/*!
 * Utility that holds a raw data buffer.  The internal implementation uses reference
 * counting, so the various copy and "slice" operations are fast.
 *
 * None of the operations are thread safe, so the caller must provide any necessary
 * synchronization.  Since the implementation uses reference counting, two distinct
 * DATA objects may actually share a buffer and require mutual synchronization.  To
 * avoid this, use DATA::MakeUnique() if necessary.
 */
class /*<UTILITY>*/ DATA
{
public:
    /*!
     * Construct a new empty buffer.
     */
    DATA() : _sbuf(0), _start(0), _size(0) {}

    /*!
     * Construct a new buffer.  The contents of the buffer are initially unspecified.
     *
     *  @param[in] size     Size (bytes) of the buffer.
     */
    DATA(size_t size) : _sbuf(new SHARED_BUF(size)), _start(_sbuf->_buf), _size(size) {}

    /*!
     * Construct a new buffer that is a copy of some existing data.
     *
     *  @param[in] buf      Points to the data to copy.
     *  @param[in] size     Size (bytes) of data in \a buf.
     */
    template<typename T> DATA(const T *buf, size_t size)
        : _sbuf(new SHARED_BUF(size)), _start(_sbuf->_buf), _size(size)
    {
        memcpy(_sbuf->_buf, static_cast<const void *>(buf), size);
    }

    /*!
     * Construct a new buffer that is a copy of a string (not including its terminating NUL).
     *
     *  @param[in] str  NUL-terminated string, which is copied.
     */
    DATA(const char *str) : _sbuf(new SHARED_BUF(strlen(str))), _start(_sbuf->_buf), _size(_sbuf->_size)
    {
        memcpy(_sbuf->_buf, str, _size);
    }

    /*!
     * Construct a new buffer that is a copy of a string (not including its terminating NUL).
     *
     *  @param[in] str  The string, which is copied.
     */
    DATA(const std::string &str)
        : _sbuf(new SHARED_BUF(str.size())), _start(_sbuf->_buf), _size(_sbuf->_size)
    {
        memcpy(_sbuf->_buf, str.c_str(), _size);
    }

    /*!
     * Construct a new buffer that is a copy of a subrange of an existing buffer.
     *
     *  @param[in] other    The new buffer are a copy of the contents of \a other.
     *  @param[in] off      The new buffer starts at \a off bytes from the start of \a other.
     *                       If \a off is larger than \a other, the new buffer is empty.
     */
    DATA(const DATA &other, size_t off=0)
    {
        if (off >= other._size)
        {
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }

        _sbuf = other._sbuf;
        _sbuf->_refCount++;
        _start = other._start + off;
        _size = other._size - off;
    }

    /*!
     * Construct a new buffer that is a copy of a subrange of an existing buffer.
     *
     *  @param[in] other    The new buffer are a copy of the contents of \a other.
     *  @param[in] off      The new buffer starts at \a off bytes from the start of \a other.
     *                       If \a off is larger than \a other, the new buffer is empty.
     *  @param[in] len      The new buffer is at most \a len bytes long.  If \a off + \a len
     *                       is greater than the length of \a other, the new buffer is a copy
     *                       of the data up to the end of \a other.
     */
    DATA(const DATA &other, size_t off, size_t len)
    {
        if (off >= other._size)
        {
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }

        _sbuf = other._sbuf;
        _sbuf->_refCount++;
        _start = other._start + off;

        if (len > other._size - off)
            _size = other._size - off;
        else
            _size = len;
    }

    ~DATA() {DetachBuf();}

    /*!
     * Reconstruct the buffer to be a copy of another buffer.
     *
     *  @param[in] other    The contents of this buffer are copied.
     *
     * @return  Reference to the new data buffer.
     */
    DATA &operator =(const DATA &other)
    {
        Assign(other);
        return *this;
    }

    /*!
     * Reconstruct the buffer to a new size.  The contents of the buffer are unspecified.
     *
     *  @param[in] size     Size (bytes) of the buffer.
     */
    void Assign(size_t size)
    {
        DetachBuf();
        _sbuf = new SHARED_BUF(size);
        _start = _sbuf->_buf;
        _size = size;
    }

    /*!
     * Reconstruct the buffer to be a copy of some existing data.
     *
     *  @param[in] buf      Points to the data to copy.
     *  @param[in] size     Size (bytes) of data in \a buf.
     */
    template<typename T> void Assign(const T *buf, size_t size)
    {
        DetachBuf();
        _sbuf = new SHARED_BUF(size);
        _start = _sbuf->_buf;
        _size = size;
        memcpy(_sbuf->_buf, static_cast<const void *>(buf), size);
    }

    /*!
     * Reconstruct the buffer to be a copy of a string (not including its terminating NUL).
     *
     *  @param[in] str  NUL-terminated string, which is copied.
     */
    void Assign(const char *str)
    {
        DetachBuf();
        _size = strlen(str);
        _sbuf = new SHARED_BUF(_size);
        _start = _sbuf->_buf;
        memcpy(_sbuf->_buf, str, _size);
    }

    /*!
     * Reconstruct the buffer to be a copy of a string (not including its terminating NUL).
     *
     *  @param[in] str  The string, which is copied.
     */
    void Assign(const std::string &str)
    {
        DetachBuf();
        _size = str.size();
        _sbuf = new SHARED_BUF(_size);
        _start = _sbuf->_buf;
        memcpy(_sbuf->_buf, str.c_str(), _size);
    }

    /*!
     * Reconstruct the buffer to be a copy of a subrange of an existing buffer.
     *
     *  @param[in] other    The contents of this buffer are copied.
     *  @param[in] off      This buffer starts at \a off bytes from the start of \a other.
     *                       If \a off is larger than \a other, the new buffer is empty.
     */
    void Assign(const DATA &other, size_t off=0)
    {
        DetachBuf();
        if (off >= other._size)
        {
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }

        _sbuf = other._sbuf;
        _sbuf->_refCount++;
        _start = other._start + off;
        _size = other._size - off;
    }

    /*!
     * Reconstruct the buffer to be a copy of a subrange of an existing buffer.
     *
     *  @param[in] other    The contents of this buffer are a copy of the contents of \a other.
     *  @param[in] off      This buffer starts at \a off bytes from the start of \a other.
     *                       If \a off is larger than \a other, the new buffer is empty.
     *  @param[in] len      This buffer is at most \a len bytes long.  If \a off + \a len
     *                       is greater than the length of \a other, the new buffer is a copy
     *                       of the data up to the end of \a other.
     */
    void Assign(const DATA &other, size_t off, size_t len)
    {
        DetachBuf();
        if (off >= other._size)
        {
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }

        _sbuf = other._sbuf;
        _sbuf->_refCount++;
        _start = other._start + off;

        if (len > other._size - off)
            _size = other._size - off;
        else
            _size = len;
    }

    /*!
     * Remove initial bytes from the start of the buffer, making it shorter.
     *
     *  @param[in] num  This many bytes are removed from the buffer.  If \a num
     *                   is larger than the length of the buffer, the buffer becomes empty.
     */
    void PopFront(size_t num)
    {
        if (num >= _size)
        {
            DetachBuf();
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }
        _start += num;
        _size -= num;
    }

    /*!
     * Remove trailing bytes from the end of the buffer, making it shorter.
     *
     *  @param[in] num  This many bytes are removed from the buffer.  If \a num
     *                   is larger than the length of the buffer, the buffer becomes empty.
     */
    void PopBack(size_t num)
    {
        if (num >= _size)
        {
            DetachBuf();
            _sbuf = 0;
            _start = 0;
            _size = 0;
            return;
        }
        _size -= num;
    }

    /*!
     * Change the size of the buffer, retaining it's current content.  If the new size is
     * smaller than the previous size, trailing bytes in the buffer are lost.  If the new
     * size is greater than the previous size, the newly allocated bytes have an unspecified
     * value.
     *
     *  @param[in] newSize  The new buffer size (bytes).
     */
    void Resize(size_t newSize)
    {
        if (newSize <= _size)
        {
            if (!newSize)
            {
                DetachBuf();
                _sbuf = 0;
                _start = 0;
                _size = 0;
                return;
            }
            _size = newSize;
        }
        else
        {
            SHARED_BUF *sbuf = new SHARED_BUF(newSize);
            memcpy(sbuf->_buf, _start, _size);
            DetachBuf();
            _sbuf = sbuf;
            _start = sbuf->_buf;
            _size = newSize;
        }
    }

    /*!
     * Calling this function ensures that the buffer does not share any data with any other
     * DATA object, copying any shared buffer if necessary.  This could be useful, for example,
     * to ensure that two DATA objects can be safely used by different threads.
     */
    void MakeUnique()
    {
        if (!_sbuf || _sbuf->_refCount == 1)
            return;
        _sbuf->_refCount--;
        _sbuf = new SHARED_BUF(_size);
        memcpy(_sbuf->_buf, _start, _size);
        _start = _sbuf->_buf;
    }

    /*!
     * @return  Size (bytes) of the buffer
     */
    size_t GetSize() const {return _size;}

    /*!
     * @return  Pointer to the buffer's data
     */
    template<typename T> const T *GetBuf() const {return reinterpret_cast<const T *>(_start);}

    /*!
     * @return  Pointer to the buffer's data
     */
    template<typename T> T *GetBuf() {return reinterpret_cast<T *>(_start);}

private:
    void DetachBuf()
    {
        if (_sbuf && --(_sbuf->_refCount) == 0)
        {
            delete [] _sbuf->_buf;
            delete _sbuf;
        }
    }

private:
    // This is potentially shared by many DATA instances.
    //
    struct SHARED_BUF
    {
        SHARED_BUF(size_t sz) : _refCount(1), _size(sz), _buf(new FUND::UINT8[_size]) {}

        FUND::UINT32 _refCount;
        size_t _size;           // allocated size of _buf
        FUND::UINT8 *_buf;
    };

    SHARED_BUF *_sbuf;
    FUND::UINT8 *_start;    // start of my instance's data in _buf
    size_t _size;           // size of my instance's data
};

} // namespace
#endif // file guard
