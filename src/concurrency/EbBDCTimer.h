#ifndef EBBDCTIMER_H
#define EBBDCTIMER_H

struct BDCTimerUserEvent
{
	unsigned short id;
	bool del;
	unsigned long long duration;
	unsigned long trigger;
	void* param;
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
	BDCTimer* timer;
	unsigned long long skipDuration;
	unsigned long long nextTrigTime;
	unsigned long long lastTrigTime;

};

#endif