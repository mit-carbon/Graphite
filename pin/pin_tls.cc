#include "tls.h"
#include <pin.H>

class PinTLS : public TLS
{
public:
    PinTLS()
    {
        m_key = PIN_CreateThreadDataKey(NULL);
    }

    ~PinTLS()
    {
        PIN_DeleteThreadDataKey(m_key);
    }

    void* get()
    {
        return PIN_GetThreadData(m_key);
    }

    const void* get() const
    {
        return PIN_GetThreadData(m_key);
    }

    void set(void *vp)
    {
        PIN_SetThreadData(m_key, vp);
    }

private:
    TLS_KEY m_key;
};

// override PthreadTLS

TLS* TLS::create()
{
    return new PinTLS();
}
