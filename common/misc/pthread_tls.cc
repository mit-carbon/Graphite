#include "tls.h"
#include "log.h"
#include <pthread.h>

// TLS

TLS::TLS()
{ }

TLS::~TLS()
{ }

// Pthread

class PthreadTLS : public TLS
{
public:
    PthreadTLS()
    {
        pthread_key_create(&m_key, NULL);
    }

    ~PthreadTLS()
    {
        pthread_key_delete(m_key);
    }

    void* get()
    {
        return pthread_getspecific(m_key);
    }

    const void* get() const
    {
        return pthread_getspecific(m_key);
    }

    void set(void *vp)
    {
        pthread_setspecific(m_key, vp);
    }

private:
    pthread_key_t m_key;
};

__attribute__((weak)) TLS* TLS::create()
{
    return new PthreadTLS();
}
