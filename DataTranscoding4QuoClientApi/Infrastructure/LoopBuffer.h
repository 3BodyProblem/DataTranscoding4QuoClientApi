#ifndef __INFRASTRUCTURE__LOOPBUFFER_H__
#define __INFRASTRUCTURE__LOOPBUFFER_H__


#include "Lock.h"


//ע�⣺tempclass��ָ�벻�������ռ䣬���ԣ�������Ҫ����ռ��ָ�벻Ҫ��Ϊtempclass�ĳ�Ա
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
	//��ʼ��
	int  Instance( char* pszBufferPtr, unsigned long lMaxSize);
	//�ͷſռ�
	void Release(void);
public:
	//�洢����
	int  PutData(const tempclass * lpIn,unsigned long lInSize = 1);
	//ȡ������
	int  GetData(tempclass * lpOut,unsigned long lInSize = 1);
	//�鿴���ݣ������鿴����ȡ��
	int  LookData(tempclass * lpOut,unsigned long lInSize = 1);
	//�Ƴ�����
	int  MoveData(unsigned long lInSize = 1);
public:
	//�������
	void Clear(void);
public:
	//�ж��Ƿ�Ϊ�ջ���
	bool IsEmpty(void);
	bool IsFull(void);
public:
	//��ȡ��������
	int  GetRecordCount(void);
	//��ȡ��ǰ���ࣨʣ�ࣩ�ռ�����
	int  GetFreeRecordCount(void);
	//��ȡ�������ռ�
	int  GetMaxRecord(void);
	//��ȡ���ݰٷֱ�
	int  GetPercent(void);
};

//------------------------------------------------------------------------------------------------------------------------------
//����ʵ�ֲ���
//------------------------------------------------------------------------------------------------------------------------------
template<class tempclass> MLoopBufferSt<tempclass>::MLoopBufferSt(void)
{
	m_lpRecordData = NULL;
	m_lMaxRecord = 0;
	m_lFirstRecord = 0;
	m_lLastRecord = 0;
}

template<class tempclass> MLoopBufferSt<tempclass>::~MLoopBufferSt()
{
	Release();
}

template<class tempclass>int  MLoopBufferSt<tempclass>::Instance( char* pszBufferPtr, unsigned long lMaxSize )
{
	assert(lMaxSize != 0);

	Release();

	m_lpRecordData = (tempclass*)pszBufferPtr;

	if ( m_lpRecordData == NULL )
	{
		return -1;
	}

	m_lMaxRecord = lMaxSize;

	m_lFirstRecord = 0;
	m_lLastRecord = 0;

	return(1);
}

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

template<class tempclass>int  MLoopBufferSt<tempclass>::PutData(const tempclass * lpIn,unsigned long lInSize)
{
	register int				errorcode;
	register int				icopysize;

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

template<class tempclass>int  MLoopBufferSt<tempclass>::GetData(tempclass * lpOut,unsigned long lInSize)
{
	register int				errorcode;
	register int				icopysize;
	
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

	lInSize = lInSize * sizeof(tempclass);
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
		memcpy( lpOut, &m_lpRecordData[m_lFirstRecord], lInSize );
	}
	else
	{
		memcpy( lpOut, &m_lpRecordData[m_lFirstRecord], icopysize );
		memcpy( lpOut + icopysize, &m_lpRecordData[0], lInSize - icopysize );
	}

	return lInSize;
}

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

template<class tempclass>void MLoopBufferSt<tempclass>::Clear(void)
{
	m_lLastRecord = 0;
	m_lFirstRecord = 0;
}

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

template<class tempclass>int  MLoopBufferSt<tempclass>::GetRecordCount(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(0);
	}

	return((m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord);
}

template<class tempclass>int  MLoopBufferSt<tempclass>::GetFreeRecordCount(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(0);
	}

	return(m_lMaxRecord - GetRecordCount() - 1);
}

template<class tempclass>int  MLoopBufferSt<tempclass>::GetMaxRecord(void)
{
	return(m_lMaxRecord);
}

template<class tempclass>int  MLoopBufferSt<tempclass>::GetPercent(void)
{
	if ( m_lMaxRecord == 0 )
	{
		assert(0);
		return(100);
	}

	return(((m_lLastRecord + m_lMaxRecord - m_lFirstRecord) % m_lMaxRecord) * 100 / m_lMaxRecord);
}


#endif




