#ifndef EBBDCTIMER_H
#define EBBDCTIMER_H
#include "EbBDCThread.h"
#include "EbBDCSemaphore.h"
#include "EbBDCMutex.h"
#include <map>
#include <set>
struct BDCTimerUserEvent
{
	unsigned short id;
	bool del;
	unsigned long long duration;
	void* param;
	unsigned totalSkipTimes;
	unsigned int totalTrigs;
	unsigned int triggered;
};

enum BDCTIMER_TRIGER_TYPE
{
	RUNNING,
	DELETING,
};

typedef int(*BDCTimerCallback)(BDCTIMER_TRIGER_TYPE, BDCTimerUserEvent*);

class BDCTimer;
struct BDCTimerEvent
{
	BDCTimerUserEvent userEvent;
	BDCTimerCallback callback;
	unsigned int totalTrigNum;
	unsigned int trigCounter;
	unsigned int skipNext;
	BDCTimer* timer;
	unsigned long long skipDuration;
	unsigned long long nextTrigTime;
	unsigned long long lastTrigTime;
	unsigned long long id;
	bool activeFlag;
	bool delFlag;
	bool resetFlag;
};

class BDCTimer : public BDCThread
{
public:
	BDCTimer();
	~BDCTimer();
	int startTimer();
	int stopTimer();
	BDCTimerUserEvent* createEvent(unsigned long long duration, unsigned int trigNum, BDCTimerCallback callback, unsigned int skipNum, void* param);
	int removeEvent(BDCTimerUserEvent* userEvent);
	int setSkip(BDCTimerUserEvent* userEvent, unsigned int skipNum);
	int resetEventTrigCount(BDCTimerUserEvent* userEvent, unsigned int trigNum, unsigned int skipNum);
	int resetEventDuration(BDCTimerUserEvent* userEvent);
	int resetEventDuration(BDCTimerUserEvent* userEvent, unsigned long long duration);
	virtual void run();
private:
	typedef std::map<const unsigned long long, BDCTimerEvent*> Event_Map;

	typedef std::set<unsigned short> ID_SET;

	static const unsigned long long TIMEMASK = 0xffffffffffff0000LL;
	static const unsigned long long IDMASK = 0x000000000000ffffLL;
	static const unsigned int TIMESHIFT = 16;
	static const unsigned long long MINTHREAD_SUSPENDDURATION = 16;

	BDCSemaphore m_sem;
	BDCMutex m_lock;
	Event_Map m_events;
	ID_SET m_idSet;
	int destroyAllEvents();
};

#endif