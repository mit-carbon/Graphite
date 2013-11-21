// Build up / consume unstructured packets through a simple interface

#ifndef PACKETIZE_H
#define PACKETIZE_H

//#define DEBUG_UNSTRUCTURED_BUFFER
#include <assert.h>
#include <string>
#include <utility>

class UnstructuredBuffer
{

private:
    std::string m_chars;

public:

    UnstructuredBuffer();
    const void* getBuffer();
    void clear();
    int size();

    // These put / get scalars
    template<class T> void put(const T & data);
    template<class T> bool get(T& data);

    // These put / get arrays
    // Note: these are shallow copy only
    template<class T> void put(const T * data, int num);
    template<class T> bool get(T* data, int num);

    // Wrappers
    template<class T>
    UnstructuredBuffer& operator<<(const T & data);
    template<class T>
    UnstructuredBuffer& operator>>(T & data);

    template<class T, class I>
    UnstructuredBuffer& operator<<(std::pair<T*, I> buffer);
    template<class T, class I>
    UnstructuredBuffer& operator>>(std::pair<T*, I> buffer);
    
    UnstructuredBuffer& operator<<(std::pair<const void*, int> buffer);
    UnstructuredBuffer& operator>>(std::pair<void*, int> buffer);
};

template<class T> void UnstructuredBuffer::put(const T* data, int num)
{
    assert(num >= 0);
    m_chars.append((const char *) data, num * sizeof(T));
}

template<class T> bool UnstructuredBuffer::get(T* data, int num)
{
    assert(num >= 0);
    if (m_chars.size() < (num * sizeof(T)))
        return false;

    m_chars.copy((char *) data, num * sizeof(T));
    m_chars.erase(0, num * sizeof(T));

    return true;
}

template<class T> void UnstructuredBuffer::put(const T& data)
{
    put<T>(&data, 1);
}

template<class T> bool UnstructuredBuffer::get(T& data)
{
    return get<T>(&data, 1);
}

template<class T>
UnstructuredBuffer& UnstructuredBuffer::operator<<(const T & data)
{
    put<T>(data);
    return *this;
}

template<class T>
UnstructuredBuffer& UnstructuredBuffer::operator>>(T & data)
{
    __attribute__((unused)) bool res = get<T>(data);
    assert(res);
    return *this;
}

template<class T, class I>
UnstructuredBuffer& UnstructuredBuffer::operator<<(std::pair<T*, I> buffer)
{
    return (*this) << std::make_pair((const void *) buffer.first, (int) buffer.second);
}

template<class T, class I>
UnstructuredBuffer& UnstructuredBuffer::operator>>(std::pair<T*, I> buffer)
{
    return (*this) >> std::make_pair((void*) buffer.first, (int) buffer.second);
}

#endif
