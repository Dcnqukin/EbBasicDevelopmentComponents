#include "EbBDCSemaphore.h"

#include <stdio.h>
#if defined(_LINUX)
#include <errno.h>
#include <time.h>       
#include <fcntl.h>        
#include <sys/stat.h> 
#include <string.h>
#include <stdlib.h>
#endif
SemaReturnCode BDCSemaphore::wait()
{
	SemaReturnCode rtn = SEMA_SUCCESSED;
#ifdef _WIN32
	unsigned int osRtn = WaitForSingleObject(m_hSemaphore,
		0xffffffff);
	switch (osRtn)
	{
	case WAIT_OBJECT_0:
		rtn = SEMA_SUCCESSED;
		break;
	case WAIT_TIMEOUT:
		rtn = SEMA_OUT_OF_TIME;
		break;
	default:
		rtn = SEMA_UNKNOWN_ERROR;
		break;
	}
	return rtn;
#elif __linux__
	if (sem_wait(m_hSemaphore) == -1)
	{
		rtn = SEMA_UNKNOWN_ERROR;
	}
#endif
	return rtn;
}

SemaReturnCode BDCSemaphore::wait(unsigned int millisec)
{
	SemaReturnCode rtn = SEMA_SUCCESSED;
#ifdef _WIN32
	unsigned int osRtn = WaitForSingleObject(m_hSemaphore, millisec);
	switch (osRtn)
	{
	case WAIT_OBJECT_0:
		rtn = SEMA_SUCCESSED;
		break;
	case WAIT_TIMEOUT:
		rtn = SEMA_OUT_OF_TIME;
		break;
	default:
		rtn = SEMA_UNKNOWN_ERROR;
		break;
	}
#elif __linux__
	struct timespec absTime;
	clock_gettime(CLOCK_REALTIME, &absTime);
	absTime.tv_nsec += 1000000 * (millisec % 1000);
	long nsecCarry = absTime.tv_nsec / 1000000000;
	absTime.tv_sec += millisec / 1000 + nsecCarry;
	absTime.tv_nsec -= nsecCarry * 1000000000;
	if (sem_timedwait(m_hSemaphore, &absTime) == -1)
	{
		rtn = (errno == ETIMEDOUT) ? SEMA_OUT_OF_TIME : SEMA_UNKNOWN_ERROR;
	}
#endif
	return rtn;
}

BDCSemaphore::BDCSemaphore(int nInitValue, int nMaxValue, const char* name)
{
#ifdef _WIN32
	m_hSemaphore = CreateSemaphoreA(NULL, nInitValue, nMaxValue, name);
	if (m_hSemaphore == NULL)
	{
		printf("CreateSemaphore failed errno(%u).\n", GetLastError());
	}
#elif __linux__
	if (name == NULL)
	{
		self->m_hSemaphore = (sem_t*)malloc(sizeof(sem_t));
		if (sem_init(m_hSemaphore, 0, (unsigned int)nInitValue) != 0)
		{
			printf("sem_init failed errno(%d).\n", errno);
		}
		m_name = NULL;
		m_created = true;
	}
	else
	{
		m_hSemaphore = sem_open(name, O_CREAT, 0644, nInitValue);
		m_created = true;
		if (m_hSemaphore == SEM_FAILED)
		{
			if (errno != EEXIST)
			{
				printf("sem_open create semaphore failed errno(%d).\n", errno);
			}
			m_hSemaphore = sem_open(name, 0);
			if (m_hSemaphore == SEM_FAILED)
			{
				printf("sem_open failed errno(%d).\n", errno);

			}
			m_created = false;
		}
		m_name = (char*)malloc(strlen(name) + 1);
		strcpy(m_name, name);
	}
#endif
}

BDCSemaphore::BDCSemaphore()
{
	BDCSemaphore(1, 1, NULL);
}

BDCSemaphore::~BDCSemaphore()
{
#ifdef _WIN32
	if (m_hSemaphore)
	{
		CloseHandle(m_hSemaphore);
	}
#elif __linux__
	if (m_name == NULL)
	{
		sem_destroy(m_hSemaphore);
		free(m_hSemaphore);
	}
	else
	{
		if (m_created)
		{
			sem_unlink(m_name);
		}
		sem_close(m_hSemaphore);
		free(m_name);
	}
#endif
}

void BDCSemaphore::signal()
{
#ifdef _WIN32
	ReleaseSemaphore(m_hSemaphore, 1, NULL);
#elif __linux__
	if (sem_post(m_hSemaphore) != 0)
	{
		printf("sem_post failed(%d).\n", errno);
	}
#endif
}

bool BDCSemaphore::tryWait()
{
#ifdef _WIN32
	int lRc = WaitForSingleObject(m_hSemaphore, 0);
	if (lRc != WAIT_OBJECT_0 && lRc != WAIT_ABANDONED)
	{
		return false;
	}
	return true;
#elif __linux__
	if (sem_trywait(m_hSemaphore) == 0)
	{
		sem_post(m_hSemaphore);
	}
	else
	{
		return false;
	}
#endif
	return true;
}