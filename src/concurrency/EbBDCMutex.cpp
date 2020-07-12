#include "EbBDCMutex.h"

int BDCMutex::tryLock()
{
	int result = -1;
#ifdef _WIN32
	result = TryEnterCriticalSection(&m_lock);
	return result;
#elif __linux__
	return pthread_mutex_trylock(&(m_lock)) == 0 ? 1 : 0;
#endif
	return result;
}

int BDCMutex::lock()
{
#ifdef _WIN32
	EnterCriticalSection(&m_lock);
#elif __linux__
	pthread_mutex_lock(&m_lock);
#endif
	return 0;
}

int BDCMutex::unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&m_lock);
#elif __linux__
	pthread_mutex_unlock(&m_lock);
#endif
	return 0;
}

BDCMutex::BDCMutex()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_lock);
#elif __linux__
	pthread_mutexattr_t mattr;
	// initialize an attribute to default value
	int ret = pthread_mutexattr_init(&mattr);
	if (ret == 0)
	{
		pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	}
	pthread_mutex_init(&m_lock, &mattr);
	pthread_mutexattr_destory(&mattr);
#endif
}

BDCMutex::~BDCMutex()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_lock);
#elif __linux__
	pthread_mutex_destory(&m_lock);
#endif
}