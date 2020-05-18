#pragma once
#include <vector>
#include <cstdint>
#define MAXBUFSIZE 8
#define UDPMTU 1300

struct Buffer
{
	void* data[MAXBUFSIZE];
	int len[MAXBUFSIZE];
	int totalNum;
	Buffer* nextBuf;
};

struct Header
{
	uint32_t totalFrags;
	uint32_t totalLen;
	uint32_t offset;
	uint32_t frag;
	uint32_t sn;
};

int BufferFragLength(Buffer& buf);