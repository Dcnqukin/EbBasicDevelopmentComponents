#ifndef EBBDCTHREAD_H
#define EBBDCTHREAD_H

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#elif __linux__
#include <pthread.h>
#endif

class BDCThread
{
private:
	explicit BDCThread(const BDCThread& rhs);
public:
	BDCThread(const char* threadName = "BDCThread");

	virtual ~BDCThread();

	bool start(bool bSuspend = false);

	virtual void task() = 0;

	void join();
	
	void suspend();

	virtual bool stop();

	void setStop(bool val);
protected:
#ifdef _WIN32
	static unsigned int WINAPI StaticThreadFunc(void* arg);
	unsigned int  m_threadId;
#elif __linux__
	static void* StaticThreadFunc(void* arg);
	pthread_t m_threadId;
#endif
protected:
	char m_threadName[256];

	volatile bool m_bRun;

	volatile bool m_suspended;

#ifdef _WIN32
	HANDLE m_handle;
#endif
};


#endif