#include "EbBDCTimer.h"
#include <new>
#define MILLSEC 1000
#define MICROSEC 1000000
#define NANOSEC 1000000000
unsigned long long GetNowTime()
{
#if defined(_WIN32)
	FILETIME current;
	static unsigned long long s_firstTime = 0;
	if (s_firstTime == 0)
	{
		GetSystemTimeAsFileTime(&current);
		s_firstTime = ((unsigned long long)current.dwHighDateTime << 32) + current.dwLowDateTime;
	}
	GetSystemTimeAsFileTime(&current);
	unsigned long long curTime = ((unsigned long long)current.dwHighDateTime << 32) + current.dwLowDateTime;
	curTime -= (s_firstTime);
	curTime = curTime / 10000;
	return curTime;
#elif defined(__linux__)
	struct timeval current;
	gettimeofday(&current, NULL);
	unsigned long long nowTime = 0;
	nowTime = (unsigned long long)current.tv_sec * MILLSEC;
	nowTime += (unsigned long long)current.tv_usec * MILLSEC / MICROSEC;
	return nowTime;
#endif
}

BDCTimer::BDCTimer()
{
	m_sem = BDCSemaphore(0, 2, NULL);
}

BDCTimer::~BDCTimer()
{
	this->stopTimer();
	m_sem.~BDCSemaphore();
	m_lock.~BDCMutex();
}

int BDCTimer::startTimer()
{
	if (m_bRun)
	{
		return 0;
	}
	if (this->start(false))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int BDCTimer::stopTimer()
{
	if (!m_bRun)
	{
		return 0;
	}
	this->setStop(true);
	m_sem.signal();
	this->join();
	m_lock.lock();
	int rst = destroyAllEvents();
	m_lock.unlock();
	return rst;
}

BDCTimerUserEvent* BDCTimer::createEvent(unsigned long long duration, unsigned int trigNum, BDCTimerCallback callback, unsigned int skipNum, void* param)
{
	if (callback == NULL || trigNum == 0 || duration == 0)
	{
		return NULL;
	}
	m_lock.lock();
	unsigned short idCounter = 0;
	if (m_idSet.size() == 0xffff)
	{
		// too many events and we do not have property space to contain it
		m_lock.unlock();
		return NULL;
	}
	for (auto iter = m_idSet.begin();
		iter != m_idSet.end();
		++iter)
	{
		if (*iter > idCounter)
		{
			break;
		}
		else
		{
			idCounter = *iter + 1;
		}
	}
	m_idSet.insert(idCounter);
	unsigned long long now = GetNowTime();
	void* tptr = malloc(sizeof(BDCTimerEvent));
	if (tptr == NULL)
	{
		m_lock.lock();
		return NULL;
	}
	BDCTimerEvent* event = new(tptr)BDCTimerEvent;
	event->callback = callback;
	event->trigCounter = 0;
	event->totalTrigNum = trigNum;
	event->skipNext = skipNum;
	event->timer = this;
	event->activeFlag = true;
	event->userEvent.del = false;
	event->userEvent.totalSkipTimes = 0;
	event->userEvent.totalTrigs = trigNum;
	event->userEvent.triggered = 0;
	event->userEvent.duration = duration;
	event->userEvent.id = idCounter;
	event->userEvent.param = param;
	event->skipDuration = 0;
	event->nextTrigTime = duration + now;
	event->lastTrigTime = now;
	event->resetFlag = false;
	unsigned long long mapId = (event->nextTrigTime) << 16;
	m_sem.signal();
	m_lock.unlock();
	return &(event->userEvent);
}

int BDCTimer::removeEvent(BDCTimerUserEvent* userEvent)
{
	if (userEvent == NULL)
	{
		return 1;
	}
	BDCTimerEvent* event = (BDCTimerEvent*)userEvent;
	if (event->timer != this)
	{
		return -1;
	}
	m_lock.lock();
	event->delFlag = true;
	event->userEvent.duration = 0;
	event->userEvent.del = true;
	auto iter = m_events.find(event->id);
	if (iter != m_events.end())
	{
		event->nextTrigTime = GetNowTime();
		unsigned long long id = (event->nextTrigTime) << TIMESHIFT;
		id |= event->userEvent.id;
		m_events.erase(iter);
		m_events.insert({ id, event });
		m_sem.signal();
	}
	m_lock.unlock();
	return 0;
}

int BDCTimer::setSkip(BDCTimerUserEvent* userEvent, unsigned int skipNum)
{
	if (userEvent == NULL)
	{
		return 1;
	}
	BDCTimerEvent* event = (BDCTimerEvent*)userEvent;
	if (event->timer != this)
	{
		return -1;
	}

	m_lock.lock();
	event->skipNext = skipNum;
	m_lock.unlock();
	return 0;
}

int BDCTimer::resetEventTrigCount(BDCTimerUserEvent* userEvent, unsigned int trigNum, unsigned int skipNum)
{
	if (userEvent == NULL)
	{
		return 1;
	}
	BDCTimerEvent* event = (BDCTimerEvent*)userEvent;
	if (event->timer != this)
	{
		return -1;
	}

	m_lock.lock();
	event->activeFlag = true;
	event->skipNext = skipNum;
	event->trigCounter = 0;
	event->userEvent.totalSkipTimes = 0;
	event->userEvent.triggered = 0;
	event->delFlag = false;
	if (trigNum != 0)
	{
		event->totalTrigNum = trigNum;
	}
	event->userEvent.totalTrigs = event->totalTrigNum;
	m_lock.unlock();
	return 0;
}

int BDCTimer::resetEventDuration(BDCTimerUserEvent* userEvent)
{
	if (userEvent == NULL)
	{
		return 1;
	}
	BDCTimerEvent& event = *(BDCTimerEvent*)userEvent;
	if (event.timer != this)
	{
		return -1;
	}
	m_lock.lock();
	if (!event.resetFlag)
	{
		event.resetFlag = true;
	}
	unsigned long long now = GetNowTime();
	event.skipDuration = event.userEvent.duration - (event.nextTrigTime - now);
	event.nextTrigTime = now + event.userEvent.duration;
	m_lock.unlock();
	return 0;
}

int BDCTimer::resetEventDuration(BDCTimerUserEvent* userEvent, unsigned long long duration)
{
	if (userEvent == NULL)
	{
		return 1;
	}
	BDCTimerEvent& event = *(BDCTimerEvent*)userEvent;
	if (event.timer != this)
	{
		return -2;
	}
	m_lock.lock();
	auto iter = m_events.find(event.id);
	if (iter == m_events.end())
	{
		m_lock.unlock();
		return -3;
	}
	m_events.erase(iter);
	unsigned long long now = GetNowTime();
	event.userEvent.duration = duration;
	event.nextTrigTime = now + duration;
	unsigned long long newMapId = ((event.nextTrigTime) << TIMESHIFT) | event.userEvent.id;
	event.id = newMapId;
	m_events.insert({ newMapId, &event });
	m_sem.signal();
	m_lock.unlock();
	return 0;
}

void BDCTimer::run()
{
	// 重置所有事件状态
	m_lock.lock();
	Event_Map tmpMap = m_events;
	m_events.clear();
	unsigned long long now = GetNowTime();
	for (auto iter = tmpMap.begin();
		iter != tmpMap.end();
		++iter)
	{
		iter->second->nextTrigTime = (now + iter->second->userEvent.duration);
		unsigned long long newMapId = (iter->second->nextTrigTime << 16) | iter->second->userEvent.id;
		iter->second->lastTrigTime = now;
		iter->second->id = newMapId;
		m_events.insert({ newMapId, iter->second });
	}
	m_lock.unlock();

	unsigned long long waitTime = 0;
	while (m_bRun)
	{
		if (m_events.size() == 0)
		{
			waitTime = 0xfffffff;
		}
		m_sem.wait(waitTime);
		if (!m_bRun)
		{
			break;
		}
		m_lock.lock();
		auto iter = m_events.begin();
		now = GetNowTime();
		unsigned long long nowMaxMapId = now << TIMESHIFT | IDMASK;
		if (iter->first > nowMaxMapId)
		{
			waitTime = (iter->first >> TIMESHIFT) - now;
			m_lock.unlock();
			continue;
		}
		BDCTimerEvent* curEvent = NULL;
		unsigned long long newMapId = 0;
		while (iter != m_events.end() && iter->first <= nowMaxMapId)
		{
			curEvent = iter->second;
			m_events.erase(iter);
			bool delFlag = curEvent->delFlag;
			if (!curEvent->resetFlag)
			{
				if (curEvent->skipNext > 0)
				{
					if (curEvent->skipNext < 0xffffffff)
					{
						--curEvent->skipNext;
					}
					++curEvent->userEvent.totalSkipTimes;
				}
				else if (delFlag || curEvent->activeFlag)
				{
					++curEvent->trigCounter;
					curEvent->userEvent.triggered = curEvent->trigCounter;
					m_lock.lock();
					curEvent->callback(delFlag ? DELETING : RUNNING,
						&(curEvent->userEvent));
					m_lock.unlock();
					if (curEvent->trigCounter == curEvent->totalTrigNum)
					{
						curEvent->activeFlag = false;
					}
					if (curEvent->userEvent.duration == 0 && !curEvent->delFlag)
					{
						curEvent->userEvent.duration = MINTHREAD_SUSPENDDURATION;
					}
				}
			}
			if (delFlag)
			{
				m_idSet.erase(curEvent->userEvent.id);
				curEvent->~BDCTimerEvent();
				delete curEvent;
			}
			else
			{
				curEvent->lastTrigTime = now;
				if (curEvent->resetFlag)
				{
					curEvent->resetFlag = false;
				}
				else
				{
					curEvent->nextTrigTime = now + curEvent->userEvent.duration;
				}
				newMapId = ((curEvent->nextTrigTime << TIMESHIFT) | curEvent->userEvent.id);
				curEvent->id = newMapId;
				m_events.insert({ newMapId, curEvent });
			}
			iter = m_events.begin();
		}
		if (iter == m_events.end())
		{
			waitTime = 0xffffffff;
		}
		else
		{
			waitTime = (iter->first >> TIMESHIFT) - now;
		}
		m_lock.unlock();
	}
}

int BDCTimer::destroyAllEvents()
{
	for (auto iter = m_events.begin();
		iter != m_events.end();
		++iter)
	{
		m_idSet.erase(iter->second->userEvent.id);
		iter->second->callback(DELETING, &(iter->second->userEvent));
		iter->second->~BDCTimerEvent();
		delete iter->second;
	}
	m_events.clear();
	return 0;
}

