#pragma once
#define CYQ_MAXSIZE (0x003FFFFF)
#define CYQ_SECTOR_SIZE (CYQ_MAXSIZE>>8)
#define CYQ_PAGE_SIZE (0x000000FF)
#define CYQ_X(nID) ((unsigned short)(nID>>8))
#define CYQ_Y(nID) ((unsigned short)(nID&0x000000FF))
#include <mutex>

using namespace std;
template <class T>
class CCycleQueue
{
protected:
	unsigned int m_nBitMaskTbl;
	unsigned int m_nWriteIdx;
	unsigned int m_nReadIdx;
	mutex m_lock;
	T m_stDefault;
	T** m_ppData;
public:
	CCycleQueue()
	{
		m_ppData = NULL;
		Init(CYQ_PAGE_SIZE); //默认一页
	}
	CCycleQueue(unsigned int nMaxSize)
	{
		m_ppData = NULL;
		Init(nMaxSize);
	}
	bool Init(unsigned int nMaxSize) 
	{
		if (m_ppData) { Release(); }
		if (m_ppData) { return false; }
		memset(&m_stDefault, 0, sizeof(T));
		m_nWriteIdx = m_nReadIdx = 0;
		m_nBitMaskTbl = CYQ_PAGE_SIZE;
		while ((m_nBitMaskTbl & nMaxSize) != nMaxSize)
		{
			m_nBitMaskTbl = (m_nBitMaskTbl << 1) | 0x00000001;
		}
		m_nBitMaskTbl &= CYQ_MAXSIZE;
		m_ppData = new T* [CYQ_X(m_nBitMaskTbl)+0x0001]; //最小一页
		memset(m_ppData, NULL, sizeof(T*) * (CYQ_X(m_nBitMaskTbl) + 0x0001));
		for (unsigned short i = 0; i <= CYQ_X(m_nBitMaskTbl); i++)
		{
			*(m_ppData+i) = new T[CYQ_PAGE_SIZE+2]; //实际会索引到CYQ_PAGE_SIZE=255，所以需要new255+2个
		}
		return true;
	}
	void Release()
	{
		m_lock.lock();
		for (unsigned short i = 0; i <= CYQ_X(m_nBitMaskTbl); i++)
		{
			if (*(m_ppData + i))
			{
				delete[] * (m_ppData + i);
			}
		}
		delete[] m_ppData;
		m_ppData = NULL;
		m_lock.unlock();
	}
	~CCycleQueue()
	{
		Release();
	}
	unsigned int Size()
	{
		unique_lock<mutex> lock(m_lock);
		return (m_nWriteIdx >= m_nReadIdx) ? (m_nWriteIdx - m_nReadIdx) : (m_nBitMaskTbl - m_nReadIdx + m_nWriteIdx + 1);
	}
	void Clear()
	{
		unique_lock<mutex> lock(m_lock);
		m_nWriteIdx = m_nReadIdx = 0;
	}
	T* Push(T const& tValue)
	{
		unique_lock<mutex> lock(m_lock);
		m_nWriteIdx &= m_nBitMaskTbl;
		T* pTmp = (*(m_ppData+CYQ_X(m_nWriteIdx))+CYQ_Y(m_nWriteIdx));
		*pTmp = tValue;
		m_nWriteIdx++;
		m_nWriteIdx &= m_nBitMaskTbl;
		if (m_nWriteIdx == m_nReadIdx)//环形队列满
		{
			m_nReadIdx +=0x0F;
			m_nReadIdx &= m_nBitMaskTbl;
		}
		return pTmp;
	}
	bool Pop()
	{
		unique_lock<mutex> lock(m_lock);
		if (m_nWriteIdx == m_nReadIdx) //无数据
		{
			return false;
		}
		m_nReadIdx++;
		m_nReadIdx &= m_nBitMaskTbl;
		return true;
	}
	bool Front(T& tValue) //
	{
		unique_lock<mutex> lock(m_lock);
		if (m_nWriteIdx == m_nReadIdx) //无数据
		{
			return false;
		}
		tValue = *(*(m_ppData+CYQ_X(m_nReadIdx)) + CYQ_Y(m_nReadIdx));
		return true;
	}
	T& operator[](unsigned int nPos) //POS = 0-SIZE
	{
		unique_lock<mutex> lock(m_lock);
		nPos = ((m_nReadIdx+ nPos) & m_nBitMaskTbl);
		return *(*(m_ppData + CYQ_X(nPos)) + CYQ_Y(nPos));;
	}
	bool Back(T& tValue)
	{
		unique_lock<mutex> lock(m_lock);
		if (m_nWriteIdx == m_nReadIdx) //无数据
		{
			return false;
		}
		unsigned int nBackIDX = ((m_nWriteIdx - 1) & m_nBitMaskTbl);
		tValue = *(*(m_ppData + CYQ_X(nBackIDX)) + CYQ_Y(nBackIDX));
		return true;
	}
};

