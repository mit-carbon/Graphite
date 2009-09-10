#include "tls.h"
#include "log.h"
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
        LOG_PRINT("%p->set(%p)", this, vp);
        LOG_ASSERT_ERROR(
            PIN_SetThreadData(m_key, vp),
            "Error setting TLS -- pin tid = %d",
            PIN_ThreadId());
    }

private:
    TLS_KEY m_key;
};

// override PthreadTLS

TLS* TLS::create()
{
    return new PinTLS();
}
