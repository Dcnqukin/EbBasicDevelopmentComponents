#ifndef FixedSizeMemPool_h
#define FixedSizeMemPool_h
#include <list>
#define MaximumAllocMemoryBlock 64
#define DefaultAllocMemorySize 65536

class FixedSizeMemPool
{
public:
	void* Alloc();
	void Dealloc(char* ptr);
	FixedSizeMemPool();
	~FixedSizeMemPool();
private:
	std::list<char*> m_list;
};
#endif // FixedSizeMemPool_h