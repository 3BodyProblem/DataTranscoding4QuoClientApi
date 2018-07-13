#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "Quotation.h"
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
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
: m_nSHL1MkDate( 0 ), m_nSZL1MkDate( 0 )
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
	int							nErrCode = 0;

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : Enter .................." );
	m_oQuoDataCenter.ClearAllMkTime();

	if( GetWorkStatus() == ET_SS_UNACTIVE )
	{
		unsigned int				nSec = 0;

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ Quotation Is Activating............" );

		if( (nErrCode = m_oQuoDataCenter.Initialize( this )) < 0 )
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
	}

		if( (nErrCode = m_oQuotPlugin.RecoverDataCollector()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 activate Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -3;
		}

		m_oWorkStatus = ET_SS_WORKING;				///< 更新Quotation会话的状态
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ Quotation Activated!.............." );
//	}

	return 0;
}

void Quotation::Halt()
{
	m_oQuotPlugin.HaltDataCollector();

	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halting .............." );

//		m_oWorkStatus = ET_SS_UNACTIVE;			///< 更新Quotation会话的状态
//		m_oQuotPlugin.Release();				///< 释放行情源插件
//		m_oQuoDataCenter.Release();				///< 释放行情数据资源

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halted! .............." );
	}
}

int Quotation::Release()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroying .............." );

		m_oQuoDataCenter.StopThreads();			///< 停止所有任务线程
		m_oWorkStatus = ET_SS_UNACTIVE;			///< 更新Quotation会话的状态
		m_oQuotPlugin.Release();				///< 释放行情源插件
		m_oQuoDataCenter.Release();				///< 释放行情数据资源
		m_nSHL1MkDate = 0;
		m_nSZL1MkDate = 0;

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

std::string& Quotation::QuoteApiVersion()
{
	return m_oQuotPlugin.GetVersion();
}

static unsigned int	s_nStaticDate = 0;
__inline bool	PrepareStaticFile( T_STATIC_LINE& refStaticLine, std::ofstream& oDumper )
{
	char				pszFilePath[512] = { 0 };

	if( s_nStaticDate == refStaticLine.Date && oDumper.is_open() ) {
		return true;
	}

	s_nStaticDate = refStaticLine.Date;
	switch( refStaticLine.eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( refStaticLine.Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : invalid market subid (Type=%d)", refStaticLine.Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : invalid market id (%s)", refStaticLine.eMarketID );
		return false;
	}

	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "STATIC%u.csv", refStaticLine.Date );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "code,name,lotsize,contractmult,contractunit,startdate,enddate,xqdate,deliverydate,expiredate,underlyingcode,underlyingname,optiontype,callorput,exercisepx\n";
		oDumper << sTitle;
	}

	return true;
}

int Quotation::SaveShLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取上海的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot fetch market infomation." );
		return -2;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 WareCount = %d", pKindHead->WareCount );
	if( true == bBuild ) {
		if( 0 != m_oQuoDataCenter.GetSHL1Min1Cache().Initialize( pKindHead->WareCount ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot initialize minute lines cache" );
			return -2;
		}
		if( 0 != m_oQuoDataCenter.GetSHL1TickCache().Initialize( pKindHead->WareCount ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot initialize tick lines cache" );
			return -2;
		}
	}

	if( m_oQuoDataCenter.GetSHL1StatusTable().FetchAllSFlag( XDF_SH, m_oQuotPlugin.GetPrimeApi(), pKindHead->WareCount ) <= 0 ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot fetch stopflag 4 shl1" );
		return -3;
	}

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

	bool						bNeedSaveStatic = false;
	char						nMkID = XDF_SH;
	XDFAPI_MarketStatusInfo		tagStatus = { 0 };
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &nMkID, &tagStatus );
	if( 1 == nErrorCode )
	{
		m_oQuoDataCenter.SetMarketTime( XDF_SH, tagStatus.MarketTime );
		ServerStatus::GetStatusObj().UpdateMkTime( XDF_SH, tagStatus.MarketDate, tagStatus.MarketTime );

		if( m_nSHL1MkDate != tagStatus.MarketDate ) {
			m_nSHL1MkDate = tagStatus.MarketDate;
			bNeedSaveStatic = true;
		}
	}
	else
	{
		m_oQuoDataCenter.SetMarketTime( XDF_SH, DateTime::Now().TimeToLong() );
		ServerStatus::GetStatusObj().UpdateMkTime( XDF_SH, DateTime::Now().DateToLong(), DateTime::Now().TimeToLong() );

		if( m_nSHL1MkDate != DateTime::Now().DateToLong() ) {
			m_nSHL1MkDate = DateTime::Now().DateToLong();
			bNeedSaveStatic = true;
		}
	}

	///< ---------------- 获取上海市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			std::ofstream			oDumper;
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
						MinGenerator::T_DATA	tagMinData = { 0 };
						TickGenerator::T_DATA	tagTickData = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;
						///< 构建商品集合
						double					dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), DateTime::Now().DateToLong(), dPriceRate, tagTickData, NULL, 0 );
						m_oQuoDataCenter.BuildSecurity4Min( XDF_SH, std::string( pData->Code, 6 ), DateTime::Now().DateToLong(), dPriceRate, tagMinData );
						nNum++;

						///< 静态数据落盘
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SH;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SH, tagStaticLine.Code, tagStaticLine.Name );
						if( true == bNeedSaveStatic ) {
							if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
							{
								int		nLen = ::sprintf( pszLine, "%s,%s,%d,,,,,,,,,,,,\n", tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize );
								oDumper.write( pszLine, nLen );
							}
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SH] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	if( true == bBuild ) {
		m_oQuoDataCenter.GetSHL1TickCache().ActivateDumper();
	}
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData), 0 );	///< 日线
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );	///< Tick线
				}
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5), 0 );	///< 日线
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );	///< Tick线
				}
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

	if( true == bBuild ) {
		m_oQuoDataCenter.GetSHL1Min1Cache().ActivateDumper();
	}
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveSzLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZ, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取深圳的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 WareCount = %d", pKindHead->WareCount );
	if( true == bBuild ) {
		if( 0 != m_oQuoDataCenter.GetSZL1Min1Cache().Initialize( pKindHead->WareCount ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot initialize minutes lines cache." );
			return -2;
		}
		if( 0 != m_oQuoDataCenter.GetSZL1TickCache().Initialize( pKindHead->WareCount ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot initialize tick lines cache" );
			return -2;
		}
	}

	if( m_oQuoDataCenter.GetSZL1StatusTable().FetchAllSFlag( XDF_SZ, m_oQuotPlugin.GetPrimeApi(), pKindHead->WareCount ) <= 0 ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot fetch stopflag 4 szl1" );
		return -3;
	}

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

	bool						bNeedSaveStatic = false;
	char						nMkID = XDF_SZ;
	XDFAPI_MarketStatusInfo		tagStatus = { 0 };
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &nMkID, &tagStatus );
	if( 1 == nErrorCode )
	{
		m_oQuoDataCenter.SetMarketTime( XDF_SZ, tagStatus.MarketTime );
		ServerStatus::GetStatusObj().UpdateMkTime( XDF_SZ, tagStatus.MarketDate, tagStatus.MarketTime );

		if( m_nSZL1MkDate != tagStatus.MarketDate ) {
			m_nSZL1MkDate = tagStatus.MarketDate;
			bNeedSaveStatic = true;
		}
	}
	else
	{
		m_oQuoDataCenter.SetMarketTime( XDF_SZ, DateTime::Now().TimeToLong() );
		ServerStatus::GetStatusObj().UpdateMkTime( XDF_SZ, DateTime::Now().DateToLong(), DateTime::Now().TimeToLong() );

		if( m_nSZL1MkDate != DateTime::Now().DateToLong() ) {
			m_nSZL1MkDate = DateTime::Now().DateToLong();
			bNeedSaveStatic = true;
		}
	}

	///< ---------------- 获取深圳市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			std::ofstream			oDumper;
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
						MinGenerator::T_DATA	tagMinData = { 0 };
						TickGenerator::T_DATA	tagTickData = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;

						///< 数据集合构建
						double					dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), DateTime::Now().DateToLong(), dPriceRate, tagTickData, pData->PreName, 4 );
						m_oQuoDataCenter.BuildSecurity4Min( XDF_SZ, std::string( pData->Code, 6 ), DateTime::Now().DateToLong(), dPriceRate, tagMinData );

						///< 静态数据落盘
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SZ;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SZ, tagStaticLine.Code, tagStaticLine.Name );
						if( true == bNeedSaveStatic ) {
							if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
							{
								int		nLen = ::sprintf( pszLine, "%s,%s,%d,,,,,,,,,,,,\n", tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize );
								oDumper.write( pszLine, nLen );
							}
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SZ] = ET_SS_WORKING;				///< 设置“可服务”状态标识
	if( true == bBuild ) {
		m_oQuoDataCenter.GetSZL1TickCache().ActivateDumper();
	}
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData), 0 );	///< 日线
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );	///< Tick线
				}
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5), 0 );	///< 日线
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );	///< Tick线
				}
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

	if( true == bBuild ) {
		m_oQuoDataCenter.GetSZL1Min1Cache().ActivateDumper();
	}
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 Snaptable Size = %d", nNum );

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	bool	bNormalStatus = false;
	char	pszDesc[128] = { 0 };

	if( true == SimpleThread::GetGlobalStopFlag() )
	{
		return true;
	}

	if( XDF_CF == cMarket && nStatus >= 2 )		///< 此处为特别处理，中金期货不会有“可服务”状态的通知(BUG)
	{
		nStatus = XRS_Normal;
		bNormalStatus = true;
	}

	switch( nStatus )
	{
	case 0:
		{
			::strcpy( pszDesc, "不可用" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 1:
		{
			::strcpy( pszDesc, "未知状态" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 2:
		{
			::strcpy( pszDesc, "初始化" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 5:
		{
			bNormalStatus = true;
			::strcpy( pszDesc, "可服务" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, "行情中" );
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

	///< 加载各市场基础信息
	if( true == bNormalStatus )
	{
		s_nStaticDate = 0;
		m_vctMkSvrStatus[cMarket] = ET_SS_INITIALIZING;			///< 设置“初始中”状态标识
		m_oQuoDataCenter.SetMarketTime( (enum XDFMarket)cMarket, 0 );

		switch( (enum XDFMarket)cMarket )
		{
		case XDF_SH:		///< 上证L1
			SaveShLv1_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZ:		///< 深证L1
			SaveSzLv1_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		default:
			return false;
		}
	}

	return true;
}

void Quotation::FlushDayLineOnCloseTime()
{
	MAP_MK_CLOSECFG&		refCloseCfg = Configuration::GetConfig().GetMkCloseCfg();
	static CriticalObject	oLock;				///< 临界区对象
	CriticalLock			section( oLock );

	for( MAP_MK_CLOSECFG::iterator it = refCloseCfg.begin(); it != refCloseCfg.end(); it++ )
	{
		short				nMkID = it->first;
		TB_MK_CLOSECFG&		refCfg = it->second;
		short				nMkStatus = m_oQuoDataCenter.GetModuleStatus( (enum XDFMarket)nMkID );

		if( m_vctMkSvrStatus[nMkID] != ET_SS_WORKING )
		{
			continue;
		}

		if( nMkStatus < 5 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : Ignore Closing Prices : Market(%d), Status=%d", nMkID, nMkStatus );
			continue;
		}

		for( TB_MK_CLOSECFG::iterator itCfg = refCfg.begin(); itCfg != refCfg.end(); itCfg++ )
		{
			CLOSE_CFG&		refCfg = *itCfg;
			unsigned int	nNowT = DateTime::Now().TimeToLong();
			unsigned int	nDate = DateTime::Now().DateToLong();
			int				nDiffTime = nNowT - refCfg.CloseTime;

			if( true == SimpleThread::GetGlobalStopFlag() )
			{
				return;
			}

			if( ( (nDiffTime>0&&nDiffTime<1500) || 0==refCfg.LastDate ) && ( refCfg.LastDate!=nDate ) && ( nNowT >= refCfg.CloseTime ) )
			{
				refCfg.LastDate = nDate;
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : Fetching Closing Prices : Market(%d), Status=%d, CloseTime=%d", nMkID, nMkStatus, refCfg.CloseTime );	

				ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)nMkID, "收盘中" );
				switch( (enum XDFMarket)nMkID )
				{
				case XDF_SH:		///< 上证L1
					{
						SaveShLv1_Static_Tick_Day( XRS_Normal, false );	///< 日线落盘
						m_oQuoDataCenter.GetSHL1Min1Cache().Release();	///< 退出1分钟线落盘线程
						QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : SHL1 Minite Line Thread released!" );	
						m_oQuoDataCenter.GetSHL1TickCache().Release();	///< 退出TICK线落盘线程
						QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : SHL1 TICK Line Thread released!" );	
					}
					break;
				case XDF_SZ:		///< 深证L1
					{
						SaveSzLv1_Static_Tick_Day( XRS_Normal, false );	///< 日线落盘
						m_oQuoDataCenter.GetSZL1Min1Cache().Release();	///< 退出1分钟线落盘线程
						QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : SZL1 Minite Line Thread released!" );	
						m_oQuoDataCenter.GetSHL1TickCache().Release();	///< 退出TICK线落盘线程
						QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : SZL1 TICK Line Thread released!" );	
					}
					break;
				default:
					break;
				}
			}

			if( nNowT >= refCfg.CloseTime )
			{
				ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)nMkID, "收盘后" );
			}
		}
	}
}

void Quotation::UpdateMarketsTime()
{
	QuotePrimeApi*			pQueryApi = m_oQuotPlugin.GetPrimeApi();
	enum XDFMarket			vctMkID[] = { XDF_SH, XDF_SHOPT, XDF_SZ, XDF_SZOPT, XDF_CF, XDF_ZJOPT, XDF_CNF, XDF_CNFOPT };

	if( NULL != pQueryApi )
	{
		for( int n = 0; n < 8; n++ )
		{
			int						nErrCode = 0;
			char					nMkID = vctMkID[n];
			XDFAPI_MarketStatusInfo	tagStatus = { 0 };

			nErrCode = pQueryApi->ReqFuncData( 101, &nMkID, &tagStatus );
			if( 1 == nErrCode )
			{
				ServerStatus::GetStatusObj().UpdateMkTime( vctMkID[n], tagStatus.MarketDate, tagStatus.MarketTime );
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
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );

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
							m_oQuoDataCenter.UpdateMarketTime( XDF_SH, ((XDFAPI_MarketStatusInfo*)pbuf)->MarketTime );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );

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

								m_oQuoDataCenter.GetSZL1TickCache().UpdatePreName( std::string( pPreNameChg->Code, 6 ), (char*)(pPreNameChg->PreName + 0), sizeof(pPreNameChg->PreName) );

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

					switch( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							m_oQuoDataCenter.UpdateMarketTime( XDF_SZ, ((XDFAPI_MarketStatusInfo*)pbuf)->MarketTime );

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
	}
}

void __stdcall	Quotation::XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel, const char * pLogBuf )
{
	QuoCollector::GetCollector()->OnLog( TLV_DETAIL, "Quotation::XDF_OnRspOutLog() : %s", pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









