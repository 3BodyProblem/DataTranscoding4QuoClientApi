#include "Quotation.h"
#include "DataCenter.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "Base.h"
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


///< 主数据管理类 ///////////////////////////////////////////////////////////////////////////////////////////
QuotationData::QuotationData()
 : m_pQuotation( NULL )
{
}

QuotationData::~QuotationData()
{
}

int QuotationData::Initialize( void* pQuotation )
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::Initialize() : enter ......................" );

	Release();
	m_pQuotation = pQuotation;
	if( false == m_oThdIdle.IsAlive() )
	{
		if( 0 != m_oThdIdle.Create( "ThreadOnIdle()", ThreadOnIdle, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create onidle line thread" );
			return -1;
		}
	}

	return 0;
}

bool QuotationData::StopThreads()
{
	SimpleThread::StopAllThread();
	SimpleThread::Sleep( 3000 );
	m_oThdIdle.Join( 5000 );

	return true;
}

void QuotationData::ClearAllMkTime()
{
	::memset( m_lstMkTime, 0, sizeof(m_lstMkTime) );
}

void QuotationData::Release()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::Release() : enter ......................" );

	CriticalLock				section( m_oLock );

	m_mapModuleStatus.clear();
	::memset( m_lstMkTime, 0, sizeof(m_lstMkTime) );
}

SecurityStatus& QuotationData::GetSHL1StatusTable()
{
	return m_objStatus4SHL1;
}

SecurityStatus& QuotationData::GetSZL1StatusTable()
{
	return m_objStatus4SZL1;
}

SecurityMinCache& QuotationData::GetSHL1Min1Cache()
{
	return m_objMinCache4SHL1;
}

SecurityMinCache& QuotationData::GetSZL1Min1Cache()
{
	return m_objMinCache4SZL1;
}

SecurityTickCache& QuotationData::GetSHL1TickCache()
{
	return m_objTickCache4SHL1;
}

SecurityTickCache& QuotationData::GetSZL1TickCache()
{
	return m_objTickCache4SZL1;
}

short QuotationData::GetModuleStatus( enum XDFMarket eMarket )
{
	CriticalLock				section( m_oLock );
	TMAP_MKID2STATUS::iterator	it = m_mapModuleStatus.find( eMarket );

	if( m_mapModuleStatus.end() == it )
	{
		return -1;
	}

	return it->second;
}

void QuotationData::UpdateModuleStatus( enum XDFMarket eMarket, int nStatus )
{
	CriticalLock	section( m_oLock );

	m_lstMkTime[eMarket] = 0;
	m_mapModuleStatus[eMarket] = nStatus;
}

void* QuotationData::ThreadOnIdle( void* pSelf )
{
	ServerStatus&		refStatus = ServerStatus::GetStatusObj();
	QuotationData&		refData = *(QuotationData*)pSelf;
	Quotation&			refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000 * 2 );

			refQuotation.FlushDayLineOnCloseTime();				///< 检查是否需要落日线
			refQuotation.UpdateMarketsTime();					///< 更新各市场的日期和时间
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadOnIdle() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadOnIdle() : unknow exception" );
		}
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadOnIdle() : mission complete!" );

	return NULL;
}

void QuotationData::SetMarketTime( enum XDFMarket eMarket, unsigned int nMkTime )
{
	if( (unsigned int)eMarket < 64 )
	{
		m_lstMkTime[(short)eMarket] = nMkTime;
	}
}

void QuotationData::UpdateMarketTime( enum XDFMarket eMarket, unsigned int nMkTime )
{
	if( (unsigned int)eMarket < 64 )
	{
		unsigned int	&refMkTime = m_lstMkTime[(short)eMarket];

		if( nMkTime > refMkTime )
		{
			refMkTime = nMkTime;
		}
	}
}

unsigned int QuotationData::GetMarketTime( enum XDFMarket eMarket )
{
	if( (unsigned int)eMarket < 64 )
	{
		return m_lstMkTime[(short)eMarket];
	}

	return 0;
}

int QuotationData::BuildSecurity4Min( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData )
{
	int				nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:
		if( 0 == m_objStatus4SHL1.IsStop( sCode ) ) {
			nErrorCode = m_objMinCache4SHL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData );
		} else {
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::BuildSecurity4Min() : [MkID=%d] min1: code = %s stopflag = true", (int)eMarket, sCode.c_str() );
		}
		break;
	case XDF_SZ:
		if( 0 == m_objStatus4SZL1.IsStop( sCode ) ) {
			nErrorCode = m_objMinCache4SZL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData );
		} else {
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::BuildSecurity4Min() : [MkID=%d] min1: code = %s stopflag = true", (int)eMarket, sCode.c_str() );
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::BuildSecurity4Min() : an error occur while building security table, marketid=%d, errorcode=%d", (int)eMarket, nErrorCode );
	}

	return nErrorCode;
}

int QuotationData::BuildSecurity( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen )
{
	int					nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		if( 0 == m_objStatus4SHL1.IsStop( sCode ) ) {
			nErrorCode = m_objTickCache4SHL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData, pszPreName, nPreNamLen );
		} else {
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::BuildSecurity() : [MkID=%d] tick & day1: code = %s stopflag = true", (int)eMarket, sCode.c_str() );
		}
		break;
	case XDF_SZ:	///< 深证L1
		if( 0 == m_objStatus4SZL1.IsStop( sCode ) ) {
			nErrorCode = m_objTickCache4SZL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData, pszPreName, nPreNamLen );
		} else {
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::BuildSecurity() : [MkID=%d] tick & day1: code = %s stopflag = true", (int)eMarket, sCode.c_str() );
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::BuildSecurity() : an error occur while building security table, marketid=%d, errorcode=%d", (int)eMarket, nErrorCode );
	}

	return nErrorCode;
}

bool PreDayFile( std::ifstream& oLoader, std::ofstream& oDumper, enum XDFMarket eMarket, std::string sCode, char cType, unsigned int nTradeDate )
{
	char		pszFilePath[512] = { 0 };

	switch( eMarket )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::strcpy( pszFilePath, "SSE/DAY/" );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::strcpy( pszFilePath, "SZSE/DAY/" );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::strcpy( pszFilePath, "CFFEX/DAY/" );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( cType )
			{
			case 1:
			case 4:
				::strcpy( pszFilePath, "CZCE/DAY/" );
				break;
			case 2:
			case 5:
				::strcpy( pszFilePath, "DCE/DAY/" );
				break;
			case 3:
			case 6:
				::strcpy( pszFilePath, "SHFE/DAY/" );
				break;
			case 7:
				::strcpy( pszFilePath, "INE/DAY/" );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : invalid market subid (Type=%d)", (int)cType );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : invalid market id (%s)", eMarket );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "DAY%s.csv", sCode.c_str() );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n";
		oDumper << sTitle;
	}

	if( oLoader.is_open() )	oLoader.close();
	oLoader.open( sFilePath.c_str() );

	return true;
}

bool CheckDateInDayFile( std::ifstream& oLoader, unsigned int nDate )
{
	if( !oLoader.is_open() )
	{
		return false;		///< 不存在该日期的日线的情况
	}

	char		pszDate[32] = { 0 };
	char		pszTail[1024*2] = { 0 };

	::sprintf( pszDate, "%u,", nDate );
	oLoader.seekg( -1024, std::ios::end );
	oLoader.read( pszTail, sizeof(pszTail) );

	char*		pPos = ::strstr( pszTail, pszDate );
	if( NULL != pPos )
	{
		return true;		///< 已经存在该日期的日线的情况
	}

	return false;			///< 不存在该日期的日线的情况
}

int QuotationData::DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	std::ifstream	oLoader;
	std::ofstream	oDumper;
	int				nErrorCode = 0;
	char			pszDayLine[1024] = { 0 };
	char			pszFilePath[512] = { 0 };
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*				pStock = (XDFAPI_StockData5*)pSnapData;
					double							dPriceRate = m_objTickCache4SHL1.GetPriceRate( std::string(pStock->Code, 6 ) );

					if( dPriceRate > 1 )
					{
						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								char	pszOpen[32] = { 0 };
								char	pszHigh[32] = { 0 };
								char	pszLow[32] = { 0 };
								char	pszNow[32] = { 0 };
								char	pszAmount[32] = { 0 };
								char	pszVoip[32] = { 0 };

								FormatDouble2Str( pStock->Open/dPriceRate, pszOpen, sizeof(pszOpen), 4, false );
								FormatDouble2Str( pStock->High/dPriceRate, pszHigh, sizeof(pszHigh), 4, false );
								FormatDouble2Str( pStock->Low/dPriceRate, pszLow, sizeof(pszLow), 4, false );
								FormatDouble2Str( pStock->Now/dPriceRate, pszNow, sizeof(pszNow), 4, false );
								FormatDouble2Str( pStock->Amount, pszAmount, sizeof(pszAmount), 4, false );
								FormatDouble2Str( pStock->Voip/dPriceRate, pszVoip, sizeof(pszVoip), 4, false );

								int	nLen = ::sprintf( pszDayLine, "%u,%s,%s,%s,%s,,%s,%I64d,,%u,%s\n"
									, nMachineDate, pszOpen, pszHigh, pszLow, pszNow
									, pszAmount, pStock->Volume, pStock->Records, pszVoip );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					double							dPriceRate = m_objTickCache4SHL1.GetPriceRate( std::string(pStock->Code, 6 ) );

					if( dPriceRate > 1 )
					{
						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								char	pszOpen[32] = { 0 };
								char	pszHigh[32] = { 0 };
								char	pszLow[32] = { 0 };
								char	pszNow[32] = { 0 };
								char	pszAmount[32] = { 0 };
								char	pszVoip[32] = { 0 };

								FormatDouble2Str( pStock->Open/dPriceRate, pszOpen, sizeof(pszOpen), 4, false );
								FormatDouble2Str( pStock->High/dPriceRate, pszHigh, sizeof(pszHigh), 4, false );
								FormatDouble2Str( pStock->Low/dPriceRate, pszLow, sizeof(pszLow), 4, false );
								FormatDouble2Str( pStock->Now/dPriceRate, pszNow, sizeof(pszNow), 4, false );
								FormatDouble2Str( pStock->Amount, pszAmount, sizeof(pszAmount), 4, false );

								int	nLen = ::sprintf( pszDayLine, "%u,%s,%s,%s,%s,,%s,%I64d,,,\n"
									, nMachineDate, pszOpen, pszHigh, pszLow, pszNow, pszAmount, pStock->Volume );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*				pStock = (XDFAPI_StockData5*)pSnapData;
					double							dPriceRate = m_objTickCache4SZL1.GetPriceRate( std::string(pStock->Code, 6 ) );

					if( dPriceRate > 1 )
					{
						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								char	pszOpen[32] = { 0 };
								char	pszHigh[32] = { 0 };
								char	pszLow[32] = { 0 };
								char	pszNow[32] = { 0 };
								char	pszAmount[32] = { 0 };
								char	pszVoip[32] = { 0 };

								FormatDouble2Str( pStock->Open/dPriceRate, pszOpen, sizeof(pszOpen), 4, false );
								FormatDouble2Str( pStock->High/dPriceRate, pszHigh, sizeof(pszHigh), 4, false );
								FormatDouble2Str( pStock->Low/dPriceRate, pszLow, sizeof(pszLow), 4, false );
								FormatDouble2Str( pStock->Now/dPriceRate, pszNow, sizeof(pszNow), 4, false );
								FormatDouble2Str( pStock->Amount, pszAmount, sizeof(pszAmount), 4, false );
								FormatDouble2Str( pStock->Voip/dPriceRate, pszVoip, sizeof(pszVoip), 4, false );

								int	nLen = ::sprintf( pszDayLine, "%u,%s,%s,%s,%s,,%s,%I64d,,%u,%s\n"
									, nMachineDate, pszOpen, pszHigh, pszLow, pszNow
									, pszAmount, pStock->Volume, pStock->Records, pszVoip );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					double							dPriceRate = m_objTickCache4SZL1.GetPriceRate( std::string(pStock->Code, 6 ) );

					if( dPriceRate > 1 )
					{
						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								char	pszOpen[32] = { 0 };
								char	pszHigh[32] = { 0 };
								char	pszLow[32] = { 0 };
								char	pszNow[32] = { 0 };
								char	pszAmount[32] = { 0 };
								char	pszVoip[32] = { 0 };

								FormatDouble2Str( pStock->Open/dPriceRate, pszOpen, sizeof(pszOpen), 4, false );
								FormatDouble2Str( pStock->High/dPriceRate, pszHigh, sizeof(pszHigh), 4, false );
								FormatDouble2Str( pStock->Low/dPriceRate, pszLow, sizeof(pszLow), 4, false );
								FormatDouble2Str( pStock->Now/dPriceRate, pszNow, sizeof(pszNow), 4, false );
								FormatDouble2Str( pStock->Amount, pszAmount, sizeof(pszAmount), 4, false );

								int	nLen = ::sprintf( pszDayLine, "%u,%s,%s,%s,%s,,%s,%I64d,,,\n"
									, nMachineDate, pszOpen, pszHigh, pszLow, pszNow, pszAmount, pStock->Volume );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			}
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( oDumper.is_open() )
	{
		oDumper.close();
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::DumpDayLine() : an error occur while dumping day line, marketid=%d, errorcode=%d", (int)eMarket, nErrorCode );
		return -1;
	}

	return 0;
}

int QuotationData::UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	char				pszCode[18] = { 0 };
	unsigned int		nLastPrice = 0;
	double				dAmount = 0;
	unsigned __int64	dVolume = 0;
	int					nErrorCode = 0;
	unsigned int		nMkTime = GetMarketTime( eMarket ) * 1000;

	if( 0 == nMkTime )	{
		nMkTime = DateTime::Now().TimeToLong() * 1000;
	}

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*				pStock = (XDFAPI_StockData5*)pSnapData;

					nLastPrice = pStock->Now;
					dAmount = pStock->Amount;
					dVolume = pStock->Volume;
					::memcpy( pszCode, pStock->Code, 6 );
					///< ------------ Tick Lines -------------------------
					nErrorCode = m_objTickCache4SHL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
					///< ------------ Minute Lines -----------------------
///<if( strncmp( pStock->Code, "600000", 6 ) == 0 )::printf( "Snap, 600000, Time=%u, Open=%u, Hight=%u, Low=%u, %u, %I64d\n", refTickLine.Time/1000, pStock->Open, pStock->High, pStock->Low, pStock->Now, pStock->Volume );
					m_objMinCache4SHL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;

					nLastPrice = pStock->Now;
					dAmount = pStock->Amount;
					dVolume = pStock->Volume;
					::memcpy( pszCode, pStock->Code, 6 );
					///< ------------ Tick Lines -------------------------
					nErrorCode = m_objTickCache4SHL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
					///< ------------ Minute Lines -----------------------
					m_objMinCache4SHL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
				}
				break;
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*				pStock = (XDFAPI_StockData5*)pSnapData;

					nLastPrice = pStock->Now;
					dAmount = pStock->Amount;
					dVolume = pStock->Volume;
					::memcpy( pszCode, pStock->Code, 6 );
					///< ------------ Tick Lines -------------------------
					nErrorCode = m_objTickCache4SZL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
					///< ------------ Minute Lines -----------------------
					m_objMinCache4SZL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;

					nLastPrice = pStock->Now;
					dAmount = pStock->Amount;
					dVolume = pStock->Volume;
					::memcpy( pszCode, pStock->Code, 6 );
					///< ------------ Tick Lines -------------------------
					nErrorCode = m_objTickCache4SZL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
					///< ------------ Minute Lines -----------------------
					m_objMinCache4SZL1.UpdateSecurity( *pStock, nTradeDate, nMkTime );
				}
				break;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateTickLine() : invalid market id(%d)", (int)eMarket );
		break;
	}

	if( nErrorCode < 0 )	{
		ServerStatus::GetStatusObj().AddTickLostRef();
		return -1;
	}

	ServerStatus::GetStatusObj().UpdateSecurity( eMarket, pszCode, nLastPrice, dAmount, dVolume );	///< 更新各市场的商品状态

	return 0;
}




