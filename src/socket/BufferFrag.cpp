#include "BufferFrag.h"

int BufferFragLength(Buffer& buf)
{
	// 分片需要考虑四种场景，
	// 1.buf总长度小于MTU-HeaderLen，但本身是链表
	// 2.buf总长度大于MTU-HeaderLen，但本身是链表
	// 3.buf总长度大于MTU-HeaderLen，但本身不是链表
	// 4.buf总长度小于MTU-HeaderLen，且本身不是链表，同时totalNum == MAXBUFSIZE - 1
	int bufContentSize = 0;
	int fragContentSize = 0;
	int fragSize = 0;
	int remainedSize = 1320;
	int lenIndexOffset = 0;
	int fragCount = 0;
	if (buf.totalNum < MAXBUFSIZE - 1)
	{
		for (uint32_t index = 0; index < buf.totalNum; ++index)
		{
			bufContentSize += buf.len[index];
		}
		fragSize = (bufContentSize - 1) / MAXBUFSIZE + 1;
	}
	else if (buf.totalNum > MAXBUFSIZE - 1)
	{
		Buffer* srcBuf = &buf;
		int circleLength = 0;
		while (1)
		{
			int indexCount = srcBuf->totalNum > MAXBUFSIZE ? MAXBUFSIZE : srcBuf->totalNum;
			for (uint32_t index = 0; index < indexCount; ++index)
			{
				bufContentSize += buf.len[index];
				fragContentSize += buf.len[index];
				circleLength += buf.len[index];
				lenIndexOffset++;
				if (lenIndexOffset % (MAXBUFSIZE - 1) == 0 && circleLength <= UDPMTU)
				{
					fragCount++;
					fragContentSize -= circleLength;
					circleLength = 0;
				}
				else if (circleLength >= UDPMTU)
				{
					circleLength = 0;
				}
			}
			if (srcBuf->totalNum >= MAXBUFSIZE)
			{
				srcBuf = srcBuf->nextBuf;
			}
			else
			{
				break;
			}
		}
		if (fragContentSize != 0)
		{
			fragSize = (fragContentSize - 1) / UDPMTU + 1;
		}
		fragSize += fragCount;
	}
	return fragSize;
}