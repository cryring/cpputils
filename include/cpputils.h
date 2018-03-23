#ifndef _CPP_UTILS_H_
#define _CPP_UTILS_H_

#include <pthread.h>

namespace cap
{

class NonCopyable
{
protected:
    NonCopyable()
    {
    }

private:
    NonCopyable(const NonCopyable&);
    NonCopyable& operator=(const NonCopyable&);
};

template<class T>
class Singleton : NonCopyable
{
public:
    static T* getInstance()
    {
        static T m_instance;
        return &m_instance;
    }
};

class MutexLock : NonCopyable
{
public:
    MutexLock()
    {
        pthread_mutex_init(&m_lock, NULL);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&m_lock);
    }

public:
    void Lock()
    {
        pthread_mutex_lock(&m_lock);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&m_lock);
    }

private:
    pthread_mutex_t m_lock;
};

template<typename Lock>
class AutoLock : NonCopyable
{
public:
    inline AutoLock(Lock& lock)
    {
        m_lock = &lock;
        m_lock->Lock();
    }

    inline ~AutoLock()
    {
        m_lock->Unlock();
    }

private:
    Lock* m_lock;
};

} // namespace cap

#endif // _CPP_UTILS_H_