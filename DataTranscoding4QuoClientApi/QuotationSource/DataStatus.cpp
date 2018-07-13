#include "DataStatus.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


///< 沪、深股票类停牌标记表 //////////////////////////////////////////////////////////////////////////////////
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

	m_mapSFlagTable.clear();			///< 先清空停牌标记表
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
停牌标记描述：
	1) 上海停牌标识定义：
	 ' '=正常
	 *=停牌

	2) 深圳停牌标识定义：
	 '.'=正常
	'Y'=常天停牌 
	'H'=暂停交易   
	'N'=上市首日 
	'M'=恢复上市首日
	 'J'=公司再融资 
	'X'=除权除息 
	'V'=网络投票 
	'A'=即可融资也可融券 
	'C'=仅可融资 
	'S'=仅可融券 
	'W'=处于要约收购期 
	'I'=当前证券处于发行期间
*/
int SecurityStatus::IsStop( std::string sCode )
{
	CriticalLock					section( m_oLockData );
	T_CODE_SFlag_Table::iterator	it = m_mapSFlagTable.find( sCode );

	if( it == m_mapSFlagTable.end() ) {
		return -1;						///< 商品不存在，返回 -1
	}

	if( XDF_SH == m_eMarketID ) {
		if( it->second ==  ' ' ) {
			return 0;					///< 正常交易，返回0
		}
	} else if( XDF_SZ == m_eMarketID ) {
		if( it->second !=  'Y' ) {
			return 0;					///< 非全天停牌，返回0
		}
	} else {
		return -2;						///< 市场编号不存在
	}

	return 1;							///< 有交易，或只是暂时停牌
}






