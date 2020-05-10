#ifndef EBBDCSEMAPHORE_H
#define EBBDCSEMAPHORE_H
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <semaphore.h>
#include <pthread.h>
#endif
enum SemaReturnCode
{
	SEMA_OUT_OF_TIME = 0x00000001,
	SEMA_SUCCESSED = 0x00000001 << 1,
	SEMA_UNKNOWN_ERROR = 0x00000001 << 2
};

class BDCSemaphore
{
public:
	SemaReturnCode wait();

	void signal();

	bool tryWait();

	SemaReturnCode wait(unsigned int millisec);

	BDCSemaphore(int nInitValue, int nMaxValue, const char* name);

	BDCSemaphore();

	~BDCSemaphore();
private:
#ifdef _WIN32
	HANDLE m_hSemaphore;
#elif __linux__
	sem_t* m_hSemaphore;
	char* m_name;
	bool m_created;
#endif
};

#endif