#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "Quotation.h"
#include "../DataTranscoding4QuoClientApi.h"


WorkStatus::WorkStatus()
: m_eWorkStatus( ET_SS_UNACTIVE )
{
}

WorkStatus::WorkStatus( const WorkStatus& refStatus )
{
	CriticalLock	section( m_oLock );

	m_eWorkStatus = refStatus.m_eWorkStatus;
}

WorkStatus::operator enum E_SS_Status()
{
	CriticalLock	section( m_oLock );

	return m_eWorkStatus;
}

std::string& WorkStatus::CastStatusStr( enum E_SS_Status eStatus )
{
	static std::string	sUnactive = "未激活";
	static std::string	sDisconnected = "断开状态";
	static std::string	sConnected = "连通状态";
	static std::string	sLogin = "登录成功";
	static std::string	sInitialized = "初始化中";
	static std::string	sWorking = "推送行情中";
	static std::string	sUnknow = "不可识别状态";

	switch( eStatus )
	{
	case ET_SS_UNACTIVE:
		return sUnactive;
	case ET_SS_DISCONNECTED:
		return sDisconnected;
	case ET_SS_CONNECTED:
		return sConnected;
	case ET_SS_LOGIN:
		return sLogin;
	case ET_SS_INITIALIZING:
		return sInitialized;
	case ET_SS_WORKING:
		return sWorking;
	default:
		return sUnknow;
	}
}

WorkStatus&	WorkStatus::operator= ( enum E_SS_Status eWorkStatus )
{
	CriticalLock	section( m_oLock );

	if( m_eWorkStatus != eWorkStatus )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "WorkStatus::operator=() : Exchange Session Status [%s]->[%s]"
											, CastStatusStr(m_eWorkStatus).c_str(), CastStatusStr(eWorkStatus).c_str() );
				
		m_eWorkStatus = eWorkStatus;
	}

	return *this;
}


///< ----------------------------------------------------------------


Quotation::Quotation()
{
}

Quotation::~Quotation()
{
	Release();
}

WorkStatus& Quotation::GetWorkStatus()
{
	return m_oWorkStatus;
}

int Quotation::Initialize()
{
	if( GetWorkStatus() == ET_SS_UNACTIVE )
	{
		unsigned int				nSec = 0;
		int							nErrCode = 0;

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ [%s] Quotation Is Activating............" );
		Release();

		if( (nErrCode = m_oQuoDataCenter.Initialize()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize DataCenter, errorcode = %d", nErrCode );
			Release();
			return -1;
		}

		if( (nErrCode = m_oQuotPlugin.Initialize( this )) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -2;
		}

		if( (nErrCode = m_oQuotPlugin.RecoverDataCollector()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 activate Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -3;
		}

		m_oWorkStatus = ET_SS_WORKING;				///< 更新Quotation会话的状态
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ Quotation Activated!.............." );
	}

	return 0;
}

int Quotation::Release()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroying .............." );

		m_oWorkStatus = ET_SS_UNACTIVE;			///< 更新Quotation会话的状态
		m_oQuotPlugin.Release();				///< 释放行情源插件
		m_oQuoDataCenter.Release();				///< 释放行情数据资源

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

int Quotation::ReloadShLv1( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取上海的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShLv1() : cannot fetch market infomation." );
		return -2;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取上海市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableSh) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 5 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );
						nNum++;

						pbuf += sizeof(XDFAPI_NameTableSh);
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShLv1() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SH] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SH, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 21 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );
				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadShOpt( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SHOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取上海期权的基础信息 -----------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShOpt() : Loading... SHOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取上海期权的市场码表数据 ------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableShOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf + m + sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 2 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableShOpt*	pData = (XDFAPI_NameTableShOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( pData->Code, 8 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableShOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShOpt() : Loading... SHOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShOpt() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SHOPT] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ShOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SHOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 15 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );
				pbuf += sizeof(XDFAPI_ShOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSHOPT() : Loading... SHLOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadSzLv1( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZ, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取深圳的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzLv1() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取深圳市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableSz) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 6 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						::memcpy( tagParam.PreName, pData->PreName, 4 );
						m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableSz);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzLv1() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SZ] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZ, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 21 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );
				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadSzOpt( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取深圳期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzOpt() : Loading... SZOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取深圳期权市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableSzOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 9 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSzOpt*	pData = (XDFAPI_NameTableSzOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( pData->Code, 8 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableSzOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzOpt() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SZOPT] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_SzOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 35 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );
				pbuf += sizeof(XDFAPI_SzOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCFF( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CF, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取中金期货的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取中金期货市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableZjqh) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 4 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableZjqh*	pData = (XDFAPI_NameTableZjqh*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableZjqh);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFF() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CF] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CffexData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CF, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 20 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );
				pbuf += sizeof(XDFAPI_CffexData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCFFOPT( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_ZJOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取中金期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取中金期权市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableZjOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 3 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableZjOpt*	pData = (XDFAPI_NameTableZjOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_ZJOPT, std::string( pData->Code ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableZjOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFFOPT() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_ZJOPT] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ZjOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_ZJOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 18 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );
				pbuf += sizeof(XDFAPI_ZjOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCNF( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNF, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取商品期货的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取商品期货市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableCnf) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 7 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableCnf*	pData = (XDFAPI_NameTableCnf*)pbuf;

						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableCnf);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNF() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNF] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutureData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];
	char	cMkID = XDF_CNF;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNF, pszCodeBuf, noffset );			///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 26 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagStatus.MarketDate );
				pbuf += sizeof(XDFAPI_CNFutureData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCNFOPT( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNFOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取商品期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取商品期权市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableCnfOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 11 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableCnfOpt*	pData = (XDFAPI_NameTableCnfOpt*)pbuf;

						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( pData->Code ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableCnfOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNFOPT() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNFOPT] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];
	char	cMkID = XDF_CNFOPT;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNFOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 34 )			///< 指数
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagStatus.MarketDate );
				pbuf += sizeof(XDFAPI_CNFutOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT Snaptable Size = %d", nNum );

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	bool	bNormalStatus = false;
	char	pszDesc[128] = { 0 };

	if( XDF_CF == cMarket && nStatus >= 2 )		///< 此处为特别处理，中金期货不会有“可服务”状态的通知(BUG)
	{
		nStatus = XRS_Normal;
		bNormalStatus = true;
	}

	switch( nStatus )
	{
	case 0:
		::strcpy( pszDesc, "不可用" );
		break;
	case 1:
		::strcpy( pszDesc, "未知状态" );
		break;
	case 2:
		::strcpy( pszDesc, "初始化" );
		break;
	case 5:
		{
			::strcpy( pszDesc, "可服务" );
			bNormalStatus = true;
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=非法状态值", (int)cMarket, pszDesc );
		return false;
	}

	///< 更新模块状态
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=%s", (int)cMarket, pszDesc );
	m_oQuoDataCenter.UpdateModuleStatus( (enum XDFMarket)cMarket, nStatus );	///< 更新模块工作状态

	static CriticalObject	oLock;				///< 临界区对象
	CriticalLock			section( oLock );

	///< 判断是否需要重入加载过程
	unsigned int	nNowT = ::time( NULL );
	if( (nNowT - m_mapMkBuildTimeT[cMarket]) <= 3 )
	{
		return true;
	}
	else
	{
		m_mapMkBuildTimeT[cMarket] = nNowT;
	}

	///< 加载各市场基础信息
	if( true == bNormalStatus )
	{
		m_vctMkSvrStatus[cMarket] = ET_SS_INITIALIZING;			///< 设置“初始中”状态标识

		switch( (enum XDFMarket)cMarket )
		{
		case XDF_SH:		///< 上证L1
			ReloadShLv1( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SHL2:		///< 上证L2(QuoteClientApi内部屏蔽)
			break;
		case XDF_SHOPT:		///< 上证期权
			ReloadShOpt( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZ:		///< 深证L1
			ReloadSzLv1( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZOPT:		///< 深圳期权
			ReloadSzOpt( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZL2:		///< 深圳L2(QuoteClientApi内部屏蔽)
			break;
		case XDF_CF:		///< 中金期货
			ReloadCFF( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_ZJOPT:		///< 中金期权
			ReloadCFFOPT( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNF:		///< 商品期货(上海/郑州/大连)
			ReloadCNF( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNFOPT:	///< 商品期货和商品期权(上海/郑州/大连)
			ReloadCNFOPT( (enum XDFRunStat)nStatus, true );
			break;
		default:
			return false;
		}
	}

	///< 启动落盘线程
	m_oQuoDataCenter.BeginDumpThread( (enum XDFMarket)cMarket, nStatus );

	return true;
}

void Quotation::FlushSnapOnCloseTime()
{
	MAP_MK_CLOSECFG&		refCloseCfg = Configuration::GetConfig().GetMkCloseCfg();
	static CriticalObject	oLock;				///< 临界区对象
	CriticalLock			section( oLock );

	for( MAP_MK_CLOSECFG::iterator it = refCloseCfg.begin(); it != refCloseCfg.end(); it++ )
	{
		short			nMkID = it->first;
		TB_MK_CLOSECFG&	refCfg = it->second;
		short			nMkStatus = m_oQuoDataCenter.GetModuleStatus( (enum XDFMarket)nMkID );

		if( m_vctMkSvrStatus[nMkID] != ET_SS_WORKING )
		{
			continue;
		}

		if( nMkStatus < 5 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushSnapOnCloseTime() : Ignore Closing Prices : Market(%d), Status=%d", nMkID, nMkStatus );
			continue;
		}

		for( TB_MK_CLOSECFG::iterator itCfg = refCfg.begin(); itCfg != refCfg.end(); itCfg++ )
		{
			CLOSE_CFG&		refCfg = *itCfg;
			unsigned int	nNowT = DateTime::Now().TimeToLong();
			unsigned int	nDate = DateTime::Now().DateToLong();
			int				nDiffTime = nNowT - refCfg.CloseTime;

			if( ( (nDiffTime>0&&nDiffTime<1500) || 0==refCfg.LastDate ) && ( refCfg.LastDate!=nDate ) && ( nNowT >= refCfg.CloseTime ) )
			{
				refCfg.LastDate = nDate;
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushSnapOnCloseTime() : Fetching Closing Prices : Market(%d), Status=%d, CloseTime=%d", nMkID, nMkStatus, refCfg.CloseTime );	

				switch( (enum XDFMarket)nMkID )
				{
				case XDF_SH:		///< 上证L1
					ReloadShLv1( XRS_Normal, false );
					break;
				case XDF_SHL2:		///< 上证L2(QuoteClientApi内部屏蔽)
					break;
				case XDF_SHOPT:		///< 上证期权
					ReloadShOpt( XRS_Normal, false );
					break;
				case XDF_SZ:		///< 深证L1
					ReloadSzLv1( XRS_Normal, false );
					break;
				case XDF_SZOPT:		///< 深圳期权
					ReloadSzOpt( XRS_Normal, false );
					break;
				case XDF_SZL2:		///< 深圳L2(QuoteClientApi内部屏蔽)
					break;
				case XDF_CF:		///< 中金期货
					ReloadCFF( XRS_Normal, false );
					break;
				case XDF_ZJOPT:		///< 中金期权
					ReloadCFFOPT( XRS_Normal, false );
					break;
				case XDF_CNF:		///< 商品期货(上海/郑州/大连)
					ReloadCNF( XRS_Normal, false );
					break;
				case XDF_CNFOPT:	///< 商品期货和商品期权(上海/郑州/大连)
					ReloadCNFOPT( XRS_Normal, false );
					break;
				default:
					break;
				}
			}
		}
	}
}

void __stdcall	Quotation::XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes )
{
	int						nLen = nBytes;
	char*					pbuf = (char *)pszBuf;
	XDFAPI_UniMsgHead*		pMsgHead = NULL;
	unsigned char			nMarketID = pHead->MarketID;

	if( m_vctMkSvrStatus[nMarketID] != ET_SS_WORKING )				///< 如果市场未就绪，就不接收回调
	{
		return;
	}

	switch( nMarketID )
	{
		case XDF_SH:												///< 处理上海Lv1的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< 指数快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							//XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;
							//::printf( "市场日期=%u,时间=%u\n", pData->MarketDate, pData->MarketTime );
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< 停牌标识
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "停牌标识=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SHOPT:												///< 处理上海期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 15:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );

								pbuf += sizeof(XDFAPI_ShOptData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_ShOptMarketStatus);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SZ:												///< 处理深圳Lv1的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< 指数快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 24:///< XDFAPI_PreNameChg					///< 前缀变更包
						{
							while( nMsgCount-- > 0 )
							{
								XDFAPI_PreNameChg*	pPreNameChg = (XDFAPI_PreNameChg*)pbuf;

								m_oQuoDataCenter.UpdatePreName( XDF_SZ, std::string( pPreNameChg->Code, 6 ), (char*)(pPreNameChg->PreName + 0), sizeof(pPreNameChg->PreName) );

								pbuf += sizeof(XDFAPI_PreNameChg);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							//XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;
							//::printf( "市场日期=%u,时间=%u\n", pData->MarketDate, pData->MarketTime );
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< 停牌标识
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "停牌标识=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SZOPT:												///< 处理深圳期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 35:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );

								pbuf += sizeof(XDFAPI_SzOptData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CF:												///< 处理中金期货的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 20:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );

								pbuf += sizeof(XDFAPI_CffexData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_ZJOPT:												///< 处理中金期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 18:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );

								pbuf += sizeof(XDFAPI_ZjOptData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNF:												///< 处理商品期货行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 26:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData) );

								pbuf += sizeof(XDFAPI_CNFutureData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNFOPT:											///< 处理商品期权行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 34:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData) );

								pbuf += sizeof(XDFAPI_CNFutOptData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
	}
}

void __stdcall	Quotation::XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel, const char * pLogBuf )
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspOutLog() : %s", pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









