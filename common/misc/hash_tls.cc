#include <sys/syscall.h>

#include "tls.h"
#include "log.h"

/*
  HashTLS implements TLS via a large hash map that we assume will
  never have collisions (in order for there to be collisions, two
  thread ids within the same process would have to differ by
  HASH_SIZE).

  If PinTLS is ever fixed, then HashTLS should probably be replaced by
  PinTLS. PthreadTLS is certainly safer when Pin is not being used.
*/

class HashTLS : public TLS
{
public:
    HashTLS()
    {
    }

    ~HashTLS()
    {
    }

    void* get()
    {
        int tid = syscall(__NR_gettid);
        return m_data[tid % HASH_SIZE];
    }

    const void* get() const
    {
        return ((HashTLS*)this)->get();
    }

    void set(void *vp)
    {
        int tid = syscall(__NR_gettid);
        m_data[tid % HASH_SIZE] = vp;
    }

private:

    static const int HASH_SIZE = 10007; // prime
    void *m_data[HASH_SIZE];
};

// override PthreadTLS
TLS* TLS::create()
{
    return new HashTLS();
}
