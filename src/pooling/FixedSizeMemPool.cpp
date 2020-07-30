#include "FixedSizeMemPool.h"

void* FixedSizeMemPool::Alloc()
{
	char* ptr = NULL;
	if (m_list.size() > 0)
	{
		ptr = m_list.back();
		m_list.pop_back();
	}
	else
	{
		ptr = new(std::nothrow) char[DefaultAllocMemorySize];
	}
	return ptr;
}

void FixedSizeMemPool::Dealloc(char* ptr)
{
	if (m_list.size() < MaximumAllocMemoryBlock)
	{
		m_list.push_back(ptr);
	}
	else
	{
		delete[] ptr;
	}
}

FixedSizeMemPool::FixedSizeMemPool()
{

}

FixedSizeMemPool::~FixedSizeMemPool()
{
	for (auto i = 0; i < m_list.size(); ++i)
	{
		delete[] m_list.front();
		m_list.pop_front();
	}
}
