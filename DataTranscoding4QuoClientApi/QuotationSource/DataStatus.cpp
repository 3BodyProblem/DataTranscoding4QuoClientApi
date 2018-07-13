#include "DataStatus.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


///< �������Ʊ��ͣ�Ʊ�Ǳ� //////////////////////////////////////////////////////////////////////////////////
SecurityStatus::SecurityStatus()
{
}

int SecurityStatus::FetchAllSFlag( enum XDFMarket eMkID, QuotePrimeApi* pQuoteQueryApi, unsigned int nWareCount )
{
	int						nErrorCode = 0;
	XDFAPI_ReqFuncParam		tagQueryParam = { 0 };
	unsigned int			nFlagBufSize = sizeof(XDFAPI_StopFlag) * nWareCount + 128;
	char*					pszFlagBuf = new char[nFlagBufSize];

	m_eMarketID = eMkID;
	if( NULL == pQuoteQueryApi ) {
		return -1;
	}

	if( NULL == pszFlagBuf ) {
		return -2;
	}

	tagQueryParam.MkID = eMkID;
	tagQueryParam.nBufLen = nFlagBufSize;
	CriticalLock			section( m_oLockData );

	m_mapSFlagTable.clear();			///< �����ͣ�Ʊ�Ǳ�
	nErrorCode = pQuoteQueryApi->ReqFuncData( 102, &tagQueryParam, pszFlagBuf );
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszFlagBuf+m);
		char*				pbuf = pszFlagBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			char			pszCode[8] = { 0 };

			if( abs(pMsgHead->MsgType) == 23 )			///< stop flag
			{
				XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;

				m_mapSFlagTable[std::string( pData->Code, 6 )] = pData->SFlag;

				pbuf += sizeof(XDFAPI_StopFlag);
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	delete []pszFlagBuf;

	return m_mapSFlagTable.size();
}

/*
ͣ�Ʊ��������
	1) �Ϻ�ͣ�Ʊ�ʶ���壺
	 ' '=����
	 *=ͣ��

	2) ����ͣ�Ʊ�ʶ���壺
	 '.'=����
	'Y'=����ͣ�� 
	'H'=��ͣ����   
	'N'=�������� 
	'M'=�ָ���������
	 'J'=��˾������ 
	'X'=��Ȩ��Ϣ 
	'V'=����ͶƱ 
	'A'=��������Ҳ����ȯ 
	'C'=�������� 
	'S'=������ȯ 
	'W'=����ҪԼ�չ��� 
	'I'=��ǰ֤ȯ���ڷ����ڼ�
*/
int SecurityStatus::IsStop( std::string sCode )
{
	CriticalLock					section( m_oLockData );
	T_CODE_SFlag_Table::iterator	it = m_mapSFlagTable.find( sCode );

	if( it == m_mapSFlagTable.end() ) {
		return -1;						///< ��Ʒ�����ڣ����� -1
	}

	if( XDF_SH == m_eMarketID ) {
		if( it->second ==  ' ' ) {
			return 0;					///< �������ף�����0
		}
	} else if( XDF_SZ == m_eMarketID ) {
		if( it->second !=  'Y' ) {
			return 0;					///< ��ȫ��ͣ�ƣ�����0
		}
	} else {
		return -2;						///< �г���Ų�����
	}

	return 1;							///< �н��ף���ֻ����ʱͣ��
}






