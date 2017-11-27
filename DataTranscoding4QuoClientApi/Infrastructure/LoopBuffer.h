#ifndef __INFRASTRUCTURE__LOOPBUFFER_H__
#define __INFRASTRUCTURE__LOOPBUFFER_H__


#include "Lock.h"


//注意：tempclass对指针不另外分配空间，所以，对于需要分配空间的指针不要作为tempclass的成员
template<class tempclass>class MLoopBufferSt
{
protected:
	tempclass					*	m_lpRecordData;
	unsigned long					m_lMaxRecord;
	unsigned long					m_lFirstRecord;
	unsigned long					m_lLastRecord;
public:
	MLoopBufferSt(void);
	virtual ~MLoopBufferSt();
public:
	//初始化
	int  Instance(unsigned long lMaxSize);
	//释放空间
	void Release(void);
public:
	//存储数据
	int  PutData(const tempclass * lpIn,unsigned long lInSize = 1);
	//取出数据
	int  GetData(tempclass * lpOut,unsigned long lInSize = 1);
	//查看数据（仅仅查看，不取出
	int  LookData(tempclass * lpOut,unsigned long lInSize = 1);
	//移除数据
	int  MoveData(unsigned long lInSize = 1);
public:
	//清除数据
	void Clear(void);
public:
	//判断是否为空或满
	bool IsEmpty(void);
	bool IsFull(void);
public:
	//获取数据数量
	int  GetRecordCount(void);
	//获取当前空余（剩余）空间数量
	int  GetFreeRecordCount(void);
	//获取数据最大空间
	int  GetMaxRecord(void);
	//获取数据百分比
	int  GetPercent(void);
};

template<class tempclass>class MLoopBufferMt : public MLoopBufferSt<tempclass>
{
protected:
	CriticalObject				m_mSection;
public:
	MLoopBufferMt(void);
	virtual ~MLoopBufferMt();
public:
	//初始化
	int  Instance(unsigned long lMaxSize);
	//释放空间
	void Release(void);
public:
	//存储数据
	int  PutData(const tempclass * lpIn,unsigned long lInSize = 1);
	//取出数据
	int  GetData(tempclass * lpOut,unsigned long lInSize = 1);
	//查看数据（仅仅查看，不取出
	int  LookData(tempclass * lpOut,unsigned long lInSize = 1);
	//移除数据
	int  MoveData(unsigned long lInSize = 1);
public:
	//清除数据
	void Clear(void);
public:
	//判断是否为空或满
	bool IsEmpty(void);
	bool IsFull(void);
public:
	//获取数据数量
	int  GetRecordCount(void);
	//获取当前空余（剩余）空间数量
	int  GetFreeRecordCount(void);
	//获取数据最大空间
	int  GetMaxRecord(void);
	//获取数据百分比
	int  GetPercent(void);
};
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//代码实现部分
//------------------------------------------------------------------------------------------------------------------------------
template<class tempclass> MLoopBufferSt<tempclass>::MLoopBufferSt(void)
{
	m_lpRecordData = NULL;
	m_lMaxRecord = 0;
	m_lFirstRecord = 0;
	m_lLastRecord = 0;
}
//..............................................................................................................................
template<class tempclass> MLoopBufferSt<tempclass>::~MLoopBufferSt()
{
	Release();
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::Instance(unsigned long lMaxSize)
{
	assert(lMaxSize != 0);

	Release();
#ifndef	_LINUXTRYOFF
	try{
#endif
		m_lpRecordData = new tempclass[lMaxSize];
#ifndef	_LINUXTRYOFF
	}catch(...)
	{
		m_lpRecordData = NULL;
	}
#endif
	if ( m_lpRecordData == NULL )
	{
		return -1;
	}
	m_lMaxRecord = lMaxSize;

	m_lFirstRecord = 0;
	m_lLastRecord = 0;

	return(1);
}
//..............................................................................................................................
template<class tempclass>void MLoopBufferSt<tempclass>::Release(void)
{
	if ( m_lpRecordData != NULL )
	{
		delete [] m_lpRecordData;
		m_lpRecordData = NULL;
	}

	m_lMaxRecord = 0;
	m_lFirstRecord = 0;
	m_lLastRecord = 0;
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::PutData(const tempclass * lpIn,unsigned long lInSize)
{
	register int				errorcode;
	register int				icopysize;

	assert(lpIn != NULL);
	
	if ( lInSize == 0 )
	{
		return(0);
	}

	if ( m_lpRecordData == NULL || m_lMaxRecord == 0 )
	{
		assert(0);
		return -1;
	}

	errorcode = (m_lFirstRecord + m_lMaxRecord - m_lLastRecord - 1) % m_lMaxRecord;
	if ( lInSize > errorcode )
	{
#ifdef _DEBUG	//	GUOGUO TEMP
//printf("FAIL:%d,%d, %d, %d, %d, %d\n", m_lFirstRecord, m_lMaxRecord, m_lLastRecord, m_lMaxRecord, errorcode, lInSize);fflush(stdout);
#endif
		return -2;
	}
	
	icopysize = m_lMaxRecord - m_lLastRecord;
	if ( icopysize >= lInSize )
	{
		memcpy(&m_lpRecordData[m_lLastRecord],(char *)lpIn,sizeof(tempclass) * lInSize);
	}
	else
	{
		memcpy(&m_lpRecordData[m_lLastRecord],lpIn,sizeof(tempclass) * icopysize);
		memcpy(&m_lpRecordData[0],lpIn + icopysize,sizeof(tempclass) * (lInSize - icopysize));
	}
	
	m_lLastRecord = (m_lLastRecord + lInSize) % m_lMaxRecord;
#ifdef _DEBUG
//	printf("SUCC:%d\n", lInSize);
#endif
	return(lInSize);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::GetData(tempclass * lpOut,unsigned long lInSize)
{
	register int				errorcode;
	register int				icopysize;
	
	assert(lpOut != NULL);

	if ( m_lpRecordData == NULL || m_lMaxRecord == 0 )
	{
		assert(0);
		return -1;
	}

	errorcode = (m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord;
	if ( errorcode == 0 )
	{
		return -2;
	}
	else if ( lInSize > errorcode )
	{
		lInSize = errorcode;
	}
	
	icopysize = m_lMaxRecord - m_lFirstRecord;
	if ( icopysize >= lInSize )
	{
		memcpy(lpOut,&m_lpRecordData[m_lFirstRecord],sizeof(tempclass) * lInSize);
	}
	else
	{
		memcpy(lpOut,&m_lpRecordData[m_lFirstRecord],sizeof(tempclass) * icopysize);
		memcpy(lpOut + icopysize,&m_lpRecordData[0],sizeof(tempclass) * (lInSize - icopysize));
	}
	
	m_lFirstRecord = (m_lFirstRecord + lInSize) % m_lMaxRecord;
	
	return(lInSize);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::LookData(tempclass * lpOut,unsigned long lInSize)
{
	register int				errorcode;
	register int				icopysize;
	
	assert(lpOut != NULL);
	
	if ( m_lpRecordData == NULL || m_lMaxRecord == 0 )
	{
		assert(0);
		return -1;
	}
	
	errorcode = (m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord;
	if ( errorcode <= 0 )
	{
		return -2;
	}
	else if ( lInSize > errorcode )
	{
		lInSize = errorcode;
	}
	
	icopysize = m_lMaxRecord - m_lFirstRecord;
	if ( icopysize >= lInSize )
	{
		memcpy(lpOut,&m_lpRecordData[m_lFirstRecord],sizeof(tempclass) * lInSize);
	}
	else
	{
		memcpy(lpOut,&m_lpRecordData[m_lFirstRecord],sizeof(tempclass) * icopysize);
		memcpy(lpOut + icopysize,&m_lpRecordData[0],sizeof(tempclass) * (lInSize - icopysize));
	}
	
	return(lInSize);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::MoveData(unsigned long lInSize)
{
	register unsigned long				errorcode;
	
	if ( m_lpRecordData == NULL || m_lMaxRecord == 0 )
	{
		assert(0);
		return(ERR_PUBLIC_NOINSTANCE);
	}
	
	errorcode = (m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord;
	if ( lInSize > errorcode )
	{
		lInSize = errorcode;
	}
	
	m_lFirstRecord = (m_lFirstRecord + lInSize) % m_lMaxRecord;
	
	return(lInSize);
}
//..............................................................................................................................
template<class tempclass>void MLoopBufferSt<tempclass>::Clear(void)
{
	m_lLastRecord = 0;
	m_lFirstRecord = 0;
}
//..............................................................................................................................
template<class tempclass>bool MLoopBufferSt<tempclass>::IsEmpty(void)
{
	if ( m_lLastRecord == m_lFirstRecord )
	{
		return(true);
	}
	else
	{
		return(false);
	}
}
//..............................................................................................................................
template<class tempclass>bool MLoopBufferSt<tempclass>::IsFull(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(true);
	}
	else if ( ((m_lLastRecord + 1) % m_lMaxRecord) == m_lFirstRecord )
	{
		return(true);
	}
	else
	{
		return(false);
	}
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::GetRecordCount(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(0);
	}

	return((m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::GetFreeRecordCount(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(0);
	}

	return(m_lMaxRecord - GetRecordCount() - 1);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::GetMaxRecord(void)
{
	return(m_lMaxRecord);
}
//..............................................................................................................................
template<class tempclass>int  MLoopBufferSt<tempclass>::GetPercent(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(100);
	}

	return(((m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord) * 100 / m_lMaxRecord);
}
//------------------------------------------------------------------------------------------------------------------------------
template<class tempclass> MLoopBufferMt<tempclass>::MLoopBufferMt(void)
{
}
//..............................................................................................................................
template<class tempclass> MLoopBufferMt<tempclass>::~MLoopBufferMt()
{
	Release();
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::Instance(unsigned long lMaxSize)
{
	CriticalLock				section(m_mSection);

	return(MLoopBufferSt<tempclass>::Instance(lMaxSize));
}
//..............................................................................................................................
template<class tempclass> void MLoopBufferMt<tempclass>::Release(void)
{
	CriticalLock				section(m_mSection);
	
	MLoopBufferSt<tempclass>::Release();
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::PutData(const tempclass * lpIn,unsigned long lInSize)
{
	CriticalLock				section(m_mSection);

	return(MLoopBufferSt<tempclass>::PutData(lpIn,lInSize));
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::GetData(tempclass * lpOut,unsigned long lInSize)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::GetData(lpOut,lInSize));
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::LookData(tempclass * lpOut,unsigned long lInSize)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::LookData(lpOut,lInSize));
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::MoveData(unsigned long lInSize)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::MoveData(lInSize));
}
//..............................................................................................................................
template<class tempclass> void MLoopBufferMt<tempclass>::Clear(void)
{
	CriticalLock				section(m_mSection);
	
	MLoopBufferSt<tempclass>::Clear();
}
//..............................................................................................................................
template<class tempclass> bool MLoopBufferMt<tempclass>::IsEmpty(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::IsEmpty());
}
//..............................................................................................................................
template<class tempclass> bool MLoopBufferMt<tempclass>::IsFull(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::IsFull());
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::GetRecordCount(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::GetRecordCount());
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::GetFreeRecordCount(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::GetFreeRecordCount());
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::GetMaxRecord(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::GetMaxRecord());
}
//..............................................................................................................................
template<class tempclass> int  MLoopBufferMt<tempclass>::GetPercent(void)
{
	CriticalLock				section(m_mSection);
	
	return(MLoopBufferSt<tempclass>::GetPercent());
}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------------------------------------------------------
