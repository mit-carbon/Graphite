#include "tls.h"
#include <pthread.h>

// TLS

TLS::TLS()
{}

TLS::~TLS()
{}

// Pthread

class PthreadTLS : public TLS
{
public:
    PthreadTLS()
    {
        pthread_key_create(&_key, NULL);
    }

    ~PthreadTLS()
    {
        pthread_key_delete(_key);
    }

    void* get()
    {
        return pthread_getspecific(_key);
    }

    const void* get() const
    {
        return pthread_getspecific(_key);
    }

    void set(void *vp)
    {
        pthread_setspecific(_key, vp);
    }

    void insert(void *vp)
    {
       pthread_setspecific(_key, vp);
    }

    void erase()
    {
    }

private:
    pthread_key_t _key;
};

__attribute__((weak)) TLS* TLS::create()
{
    return new PthreadTLS();
}
