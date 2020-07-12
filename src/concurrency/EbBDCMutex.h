#ifndef EBBDCMUTEX_H
#define EBBDCMUTEX_H

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif

class BDCMutex
{
public:
	int tryLock();

	int lock();

	int unlock();

	BDCMutex();

	~BDCMutex();
private:
#ifdef _WIN32
	CRITICAL_SECTION m_lock;
#elif __linux__
	pthread_mutex_t m_lock;
#endif
};


#endif