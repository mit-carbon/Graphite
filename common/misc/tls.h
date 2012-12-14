#ifndef TLS_H
#define TLS_H

class TLS
{
public:
    virtual ~TLS();

    // Get entry
    virtual void* get() = 0;
    virtual const void* get() const = 0;

    template<class T>
        T* get() { return (T*) get(); }

    template<class T>
        const T* get() const { return (const T*) get(); }

    long int getInt() const { return (long int) get(); }

    // Set entry
    virtual void set(void *) = 0;

    template<class T>
        void set(T *p) { set((void*)p); }

    void setInt(long int i) { return set((void*)i); }

    // Insert entry
    virtual void insert(void *) = 0;

    template<class T>
        void insert(T *p) { insert((void*)p); }

    void insertInt(long int i) { return insert((void*)i); }

    // Erase entry
    virtual void erase() = 0;
    
    static TLS* create();

protected:
    TLS();
};

#endif // TLS_H
