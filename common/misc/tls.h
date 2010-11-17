#ifndef TLS_H
#define TLS_H

#include "fixed_types.h"
#include "log.h"

class TLS
{
public:
    virtual ~TLS();

    virtual void* get() = 0;
    virtual const void* get() const = 0;

    template<class T>
        T& get() { return *((T*)get()); }

    template<class T>
        const T& get() const { return *((const T*)get()); }

    template<class T>
        T* getPtr() { return (T*)get(); }

    template<class T>
        const T* getPtr() const { return (const T*)get(); }

    IntPtr getInt() const { return (intptr_t)get(); }

    virtual void set(void *) = 0;

    template<class T>
        void set(T *p) { set((void*)p); }

    void setInt(IntPtr i) { return set((void*)i); }

    static TLS* create();

protected:
    TLS();
};

#endif // TLS_H
