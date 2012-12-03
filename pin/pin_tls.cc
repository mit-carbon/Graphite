#include <cstdio>
#include <cstdlib>
#include "tls.h"
#include <pin.H>

class PinTLS : public TLS
{
public:
    PinTLS()
    {
        _key = PIN_CreateThreadDataKey(NULL);
    }

    ~PinTLS()
    {
        PIN_DeleteThreadDataKey(_key);
    }

    void* get()
    {
        return PIN_GetThreadData(_key);
    }

    const void* get() const
    {
        return PIN_GetThreadData(_key);
    }

    void set(void *vp)
    {
        if (!PIN_SetThreadData(_key, vp))
        {
            fprintf(stderr, "Error setting TLS -- pin tid = %d", PIN_ThreadId());
            exit(EXIT_FAILURE);
        }
    }

    void insert(void *vp)
    {
       set(vp);
    }

    void erase()
    {
    }

private:
    TLS_KEY _key;
};

#if 0
// override HashTLS
TLS* TLS::create()
{
    return new PinTLS();
}
#endif
