#include "EbBDCThread.h"
#include <cstdio>
BDCThread::BDCThread(const char* threadName /*= "BDCThread"*/)
	:m_threadId(0),
	m_bRun(false),
	m_suspended(false)
{
	strcpy(m_threadName, threadName);
}

BDCThread::~BDCThread()
{

}

bool BDCThread::start(bool bSuspend)
{
	if (m_bRun)
	{
		return true;
	}
	m_bRun = true;
#ifdef _WIN32
	unsigned int retCode = 0;
	if (bSuspend)
	{
		retCode = _beginthreadex(NULL, 0, StaticThreadFunc, this, CREATE_SUSPENDED, &m_threadId);
	}
	else
	{
		retCode = _beginthreadex(NULL, 0, StaticThreadFunc, this, 0, &m_threadId);
	}
	if (retCode == 0 || retCode == -1)
	{
		printf("_beginthreadex failed errno(%d).\n", errno);
		m_bRun = false;
		return false;
	}
	m_handle = (HANDLE)retCode;
#elif __linux__
	if (0 != pthread_create(&m_threadId, NULL, StaticThreadFunc, this))
	{
		printf("pthread_create failed errno(%d).\n", errno);
		m_bRun = false;
		return false;
	}
#endif
}

void BDCThread::join()
{
#ifdef _WIN32
	if (NULL == m_handle)
	{
		return;
	}
	::WaitForSingleObject(m_handle, INFINITE);
#elif __linux__
	if (m_threadId == 0)
	{
		return;
	}
	pthread_join(m_threadId, NULL);
#endif
}

void BDCThread::suspend()
{
	this->m_suspended = true;
}

bool BDCThread::stop()
{
#ifdef _WIN32
	if (NULL == m_handle || !m_bRun)
	{
		return true;
	}
	m_bRun = false;
	if (::TerminateThread(m_handle, 0))
	{
		::CloseHandle(m_handle);
		return true;
	}
	return false;
#elif __linux__
	m_bRun = false;
	return true;
#endif
}

void BDCThread::setStop(bool val)
{
	m_bRun = !val;
}

#ifdef _WIN32
unsigned int 
#elif __linux__
void*
#endif
BDCThread::StaticThreadFunc(void* arg)
{
	BDCThread* pThread = static_cast<BDCThread*>(arg);
	pThread->task();
	return 0;
}

