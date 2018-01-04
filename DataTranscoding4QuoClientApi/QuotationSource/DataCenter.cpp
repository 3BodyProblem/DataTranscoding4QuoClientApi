#include "Quotation.h"
#include "DataCenter.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


///< -------------------- Configuration ------------------------------------------------
const int		XDF_SH_COUNT = 16000;					///< 上海Lv1
const int		XDF_SHL2_COUNT = 0;						///< 上海Lv2(QuoteClientApi内部屏蔽)
const int		XDF_SHOPT_COUNT = 500;					///< 上海期权
const int		XDF_SZ_COUNT = 8000;					///< 深证Lv1
const int		XDF_SZL2_COUNT = 0;						///< 深证Lv2(QuoteClientApi内部屏蔽)
const int		XDF_SZOPT_COUNT = 0;					///< 深圳期权
const int		XDF_CF_COUNT = 500;						///< 中金期货
const int		XDF_ZJOPT_COUNT = 500;					///< 中金期权
const int		XDF_CNF_COUNT = 500;					///< 商品期货(上海/郑州/大连)
const int		XDF_CNFOPT_COUNT = 500;					///< 商品期权(上海/郑州/大连)
unsigned int	s_nNumberInSection = 30;				///< 一个市场有可以缓存多少个数据块
///< -----------------------------------------------------------------------------------


CacheAlloc::CacheAlloc()
 : m_nMaxCacheSize( 0 ), m_nAllocateSize( 0 ), m_pDataCache( NULL )
{
	m_nMaxCacheSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_TICK_LINE) * s_nNumberInSection;
}

CacheAlloc::~CacheAlloc()
{
	CacheAlloc::GetObj().FreeCaches();
}

CacheAlloc& CacheAlloc::GetObj()
{
	static	CacheAlloc	obj;

	return obj;
}

char* CacheAlloc::GetBufferPtr()
{
	CriticalLock	section( m_oLock );

	if( NULL == m_pDataCache )
	{
		m_pDataCache = new char[m_nMaxCacheSize];
		::memset( m_pDataCache, 0, m_nMaxCacheSize );
	}

	return m_pDataCache;
}

unsigned int CacheAlloc::GetMaxBufLength()
{
	return m_nMaxCacheSize;
}

unsigned int CacheAlloc::GetDataLength()
{
	CriticalLock	section( m_oLock );

	return m_nAllocateSize;
}

char* CacheAlloc::GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize )
{
	char*					pData = NULL;
	unsigned int			nBufferSize4Market = 0;
	CriticalLock			section( m_oLock );

	try
	{
		nOutSize = 0;
		m_pDataCache = GetBufferPtr();

		if( NULL == m_pDataCache )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : invalid buffer pointer." );
			return NULL;
		}

		switch( eMkID )
		{
		case XDF_SH:		///< 上海Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHOPT:		///< 上海期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZ:		///< 深证Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZOPT:		///< 深圳期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CF:		///< 中金期货
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_ZJOPT:		///< 中金期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNF:		///< 商品期货(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		default:
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : unknow market id" );
				return NULL;
			}
		}

		if( (m_nAllocateSize + nBufferSize4Market) > m_nMaxCacheSize )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : not enough space ( %u > %u )", (m_nAllocateSize + nBufferSize4Market), m_nMaxCacheSize );
			return NULL;
		}

		nOutSize = nBufferSize4Market;
		pData = m_pDataCache + m_nAllocateSize;
		m_nAllocateSize += nBufferSize4Market;
	}
	catch( std::exception& err )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : an exceptin occur : %s", err.what() );
	}
	catch( ... )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : unknow exception" );
	}

	return pData;
}

void CacheAlloc::FreeCaches()
{
	if( NULL != m_pDataCache )
	{
		delete [] m_pDataCache;
		m_pDataCache = NULL;
	}
}


static unsigned int			s_nCNFTradingDate = 0;
static unsigned int			s_nCNFOPTTradingDate = 0;


QuotationData::QuotationData()
 : m_pQuotation( NULL ), m_pBuf4MinuteLine( NULL ), m_nMaxMLineBufSize( 0 )
{
}

QuotationData::~QuotationData()
{
}

int QuotationData::Initialize( void* pQuotation )
{
	int					nErrorCode = 0;
	CriticalLock		section( m_oMinuteLock );

	Release();
	m_pQuotation = pQuotation;

	m_nMaxMLineBufSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_MIN_LINE) * 10;
	m_pBuf4MinuteLine = new char[m_nMaxMLineBufSize];
	if( NULL == m_pBuf4MinuteLine )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 allocate buffer 4 minutes lines" );
		return -1;
	}

	::memset( m_pBuf4MinuteLine, 0, m_nMaxMLineBufSize );
	nErrorCode = m_arrayMinuteLine.Instance( m_pBuf4MinuteLine, m_nMaxMLineBufSize/sizeof(T_MIN_LINE) );
	if( 0 > nErrorCode )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 initialize minutes lines manager obj. errorcode = %d", nErrorCode );
		return -2;
	}

	nErrorCode = m_arrayTickLine.Instance( CacheAlloc::GetObj().GetBufferPtr(), CacheAlloc::GetObj().GetMaxBufLength()/sizeof(T_TICK_LINE) );
	if( 0 > nErrorCode )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 initialize tick lines manager obj. errorcode = %d", nErrorCode );
		return -3;
	}

	if( false == m_oThdTickDump.IsAlive() )
	{
		if( 0 != m_oThdTickDump.Create( "ThreadDumpTickLine()", ThreadDumpTickLine, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create tick line thread(1)" );
			return -4;
		}
	}

	if( false == m_oThdMinuteDump.IsAlive() )
	{
		if( 0 != m_oThdMinuteDump.Create( "ThreadDumpMinuteLine()", ThreadDumpMinuteLine, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create minute line thread(1)" );
			return -6;
		}
	}

	return 0;
}

void QuotationData::Release()
{
	m_mapModuleStatus.clear();
	m_mapSHL1.clear();
	m_mapSHOPT.clear();
	m_mapSZL1.clear();
	m_mapSZOPT.clear();
	m_mapCFF.clear();
	m_mapCFFOPT.clear();
	m_mapCNF.clear();
	m_mapCNFOPT.clear();

	if( NULL != m_pBuf4MinuteLine )
	{
		delete [] m_pBuf4MinuteLine;
		m_pBuf4MinuteLine = NULL;
		m_nMaxMLineBufSize = 0;
	}
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

	m_mapModuleStatus[eMarket] = nStatus;
}

__inline bool	PrepareMinuteFile( T_MIN_LINE&	refMinuteLine, std::string& sCode, std::ofstream& oDumper )
{
	char	pszFilePath[512] = { 0 };

	switch( refMinuteLine.eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( refMinuteLine.Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/MIN/%s/", refMinuteLine.Code );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/MIN/%s/", refMinuteLine.Code );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/MIN/%s/", refMinuteLine.Code );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/MIN/%s/", refMinuteLine.Code );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareMinuteFile() : invalid market subid (Type=%d)", refMinuteLine.Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareMinuteFile() : invalid market id (%s)", refMinuteLine.eMarketID );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "MIN%s_%u.csv", refMinuteLine.Code, refMinuteLine.Date/10000 );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareMinuteFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,time,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n";
		oDumper << sTitle;
	}

	return true;
}

void* QuotationData::ThreadDumpMinuteLine( void* pSelf )
{
	QuotationData&		refData = *(QuotationData*)pSelf;
	Quotation&			refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			std::string				sCode;
			std::ofstream			oDumper;

			SimpleThread::Sleep( 1000 * 3 );
			while( true )
			{
				char			pszLine[1024] = { 0 };
				T_MIN_LINE		tagMinuteLine = { 0 };
				CriticalLock	section( refData.m_oMinuteLock );

				if( refData.m_arrayMinuteLine.GetData( &tagMinuteLine ) <= 0 )
				{
					break;
				}

				if( false == PrepareMinuteFile( tagMinuteLine, std::string( tagMinuteLine.Code ), oDumper ) )
				{
					continue;
				}

				int		nLen = ::sprintf( pszLine, "%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%I64d,%.4f\n"
					, tagMinuteLine.Date, tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx
					, tagMinuteLine.SettlePx, tagMinuteLine.Amount, tagMinuteLine.Volume, tagMinuteLine.OpenInterest, tagMinuteLine.NumTrades, tagMinuteLine.Voip );
				oDumper.write( pszLine, nLen );
			}

			refQuotation.FlushDayLineOnCloseTime();															///< 检查是否需要落日线
			refQuotation.UpdateMarketsTime();																///< 更新各市场的日期和时间
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpMinuteLine() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpMinuteLine() : unknow exception" );
		}
	}

	return NULL;
}

__inline bool	PrepareTickFile( T_TICK_LINE* pTickLine, std::string& sCode, std::ofstream& oDumper )
{
	char				pszFilePath[512] = { 0 };

	switch( pTickLine->eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( pTickLine->Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : invalid market subid (Type=%d)", pTickLine->Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : invalid market id (%s)", pTickLine->eMarketID );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "TICK%s_%u.csv", pTickLine->Code, pTickLine->Date );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	sCode = pTickLine->Code;
	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,time,preclosepx,presettlepx,openpx,highpx,lowpx,closepx,nowpx,settlepx,upperpx,lowerpx,amount,volume,openinterest,numtrades,bidpx1,bidvol1,bidpx2,bidvol2,askpx1,askvol1,askpx2,askvol2,voip,tradingphasecode,prename\n";
		oDumper << sTitle;
	}

	return true;
}

typedef char	STR_TICK_LINE[512];
void* QuotationData::ThreadDumpTickLine( void* pSelf )
{
	unsigned __int64			nDumpNumber = 0;
	QuotationData&				refData = *(QuotationData*)pSelf;
	Quotation&					refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			char*					pBufPtr = CacheAlloc::GetObj().GetBufferPtr();
			unsigned int			nMaxDataNum = refData.m_arrayTickLine.GetRecordCount();
			std::string				sCode;
			std::ofstream			oDumper;
			STR_TICK_LINE			pszLine = { 0 };

			SimpleThread::Sleep( 1000 * 1 );
			if( NULL == pBufPtr || 0 == nMaxDataNum ) {
				continue;
			}

			if( nDumpNumber > 6000 ) {
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpTickLine() : (%I64d) dumped... ( Count=%u )", nDumpNumber, nMaxDataNum );
				nDumpNumber = 0;
			}

			while( true )
			{
				T_TICK_LINE		tagTickData = { 0 };

				if( refData.m_arrayTickLine.GetData( &tagTickData ) <= 0 )
				{
					break;
				}

				if( sCode != tagTickData.Code )
				{
					if( false == PrepareTickFile( &tagTickData, sCode, oDumper ) )
					{
						continue;
					}
				}

				if( !oDumper.is_open() )
				{
					QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : invalid file handle" );
					SimpleThread::Sleep( 1000 * 10 );
					continue;
				}

				int		nLen = ::sprintf( pszLine, "%u,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%s,%s\n"
					, tagTickData.Date, tagTickData.Time, tagTickData.PreClosePx, tagTickData.PreSettlePx
					, tagTickData.OpenPx, tagTickData.HighPx, tagTickData.LowPx, tagTickData.ClosePx, tagTickData.NowPx, tagTickData.SettlePx
					, tagTickData.UpperPx, tagTickData.LowerPx, tagTickData.Amount, tagTickData.Volume, tagTickData.OpenInterest, tagTickData.NumTrades
					, tagTickData.BidPx1, tagTickData.BidVol1, tagTickData.BidPx2, tagTickData.BidVol2, tagTickData.AskPx1, tagTickData.AskVol1, tagTickData.AskPx2, tagTickData.AskVol2
					, tagTickData.Voip, tagTickData.TradingPhaseCode, tagTickData.PreName );
				oDumper.write( pszLine, nLen );
				nDumpNumber++;
			}

		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : unknow exception" );
		}
	}

	return NULL;
}

int QuotationData::UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize )
{
	switch( eMarket )
	{
	case XDF_SZ:	///< 深证L1
		{
			T_MAP_QUO::iterator it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				T_LINE_PARAM&		refData = it->second;
				::memcpy( refData.PreName, pszPreName, 4 );
			}
		}
		break;
	}
	return 0;
}

int QuotationData::BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam )
{
	unsigned int		nBufSize = 0;
	char*				pDataPtr = NULL;
	int					nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			T_MAP_QUO::iterator it = m_mapSHL1.find( sCode );
			if( it == m_mapSHL1.end() )
			{
				m_mapSHL1[sCode] = refParam;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			T_MAP_QUO::iterator it = m_mapSHOPT.find( sCode );
			if( it == m_mapSHOPT.end() )
			{
				m_mapSHOPT[sCode] = refParam;
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			T_MAP_QUO::iterator it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				m_mapSZL1[sCode] = refParam;
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			T_MAP_QUO::iterator it = m_mapSZOPT.find( sCode );
			if( it == m_mapSZOPT.end() )
			{
				m_mapSZOPT[sCode] = refParam;
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			T_MAP_QUO::iterator it = m_mapCFF.find( sCode );
			if( it == m_mapCFF.end() )
			{
				m_mapCFF[sCode] = refParam;
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			T_MAP_QUO::iterator it = m_mapCFFOPT.find( sCode );
			if( it == m_mapCFFOPT.end() )
			{
				m_mapCFFOPT[sCode] = refParam;
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			T_MAP_QUO::iterator it = m_mapCNF.find( sCode );
			if( it == m_mapCNF.end() )
			{
				m_mapCNF[sCode] = refParam;
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			T_MAP_QUO::iterator it = m_mapCNFOPT.find( sCode );
			if( it == m_mapCNFOPT.end() )
			{
				m_mapCNFOPT[sCode] = refParam;
			}
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

bool PreDayFile( std::ofstream& oDumper, enum XDFMarket eMarket, std::string sCode, char cType, unsigned int nTradeDate )
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

	return true;
}

int QuotationData::DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
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
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%f\n", nMachineDate
								, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
								, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/refParam.dPriceRate );
							oDumper.write( pszDayLine, nLen );
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%f\n", nMachineDate
								, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
								, 0.0, pStock->Amount, pStock->Volume, 0, 0, 0 );
							oDumper.write( pszDayLine, nLen );
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			XDFAPI_ShOptData*		pStock = (XDFAPI_ShOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSHOPT.find( std::string(pStock->Code, 8 ) );

			if( it != m_mapSHOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 8 ), 0, nMachineDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%f\n", nMachineDate
						, pStock->OpenPx/refParam.dPriceRate, pStock->HighPx/refParam.dPriceRate, pStock->LowPx/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount*1.0, pStock->Volume, pStock->Position, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
								, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
								, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/refParam.dPriceRate );
							oDumper.write( pszDayLine, nLen );
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
								, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
								, 0.0, pStock->Amount, pStock->Volume, 0, 0, 0 );
							oDumper.write( pszDayLine, nLen );
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			XDFAPI_SzOptData*		pStock = (XDFAPI_SzOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSZOPT.find( std::string(pStock->Code,8) );

			if( it != m_mapSZOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 8 ), 0, nMachineDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
						, pStock->OpenPx/refParam.dPriceRate, pStock->HighPx/refParam.dPriceRate, pStock->LowPx/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount*1.0, pStock->Volume, pStock->Position, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			XDFAPI_CffexData*		pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
						, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			XDFAPI_ZjOptData*		pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code ), 0, nMachineDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
						, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
			}
		}
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
		{
			XDFAPI_CNFutureData*		pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );

			if( it != m_mapCNF.end() )
			{
				T_LINE_PARAM&			refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code, 6 ), refParam.Type, s_nCNFTradingDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", s_nCNFTradingDate
						, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
			}
		}
		break;
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			XDFAPI_CNFutOptData*		pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCNFOPT.end() )
			{
				T_LINE_PARAM&			refParam = it->second;

				if( true == PreDayFile( oDumper, eMarket, std::string( pStock->Code ), refParam.Type, s_nCNFOPTTradingDate ) )
				{
					int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", s_nCNFOPTTradingDate
						, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
						, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
					oDumper.write( pszDayLine, nLen );
				}
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

void QuotationData::DispatchMinuteLine( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam, unsigned int nMkDate, unsigned int nMkTime )
{
	unsigned int		nMinuteTime = nMkTime / 100;

	if( 0 != refParam.Valid && refParam.MkMinute != nMinuteTime )
	{
		T_MIN_LINE		tagMinuteLine = { 0 };

		tagMinuteLine.Date = nMkDate;
		tagMinuteLine.Time = nMkTime;
		tagMinuteLine.eMarketID = eMarket;
		tagMinuteLine.Type = refParam.Type;
		::strcpy( tagMinuteLine.Code, sCode.c_str() );
		tagMinuteLine.OpenPx = refParam.MinOpenPx1 / refParam.dPriceRate;			///< 开盘价一分钟内的第一笔的nowpx
		tagMinuteLine.HighPx = refParam.MinHighPx / refParam.dPriceRate;			///< 最高价一分钟内的 最高的highpx
		tagMinuteLine.LowPx = refParam.MinLowPx / refParam.dPriceRate;				///< 最低价一分钟内的 最低的lowpx
		tagMinuteLine.ClosePx = refParam.MinClosePx / refParam.dPriceRate;			///< 收盘价一分钟内最后一笔的Nowpx
		tagMinuteLine.SettlePx = refParam.MinSettlePx / refParam.dPriceRate;		///< 结算价一分钟内最后一笔的settlepx
		tagMinuteLine.Amount = refParam.MinAmount2 - refParam.MinAmount1;			///< 成交额一分钟最后笔减去第一笔的amount
		tagMinuteLine.Volume = refParam.MinVolume2 - refParam.MinVolume1;			///< 成交量(股/张/份)一分钟最后笔减去第一笔的volume
		tagMinuteLine.OpenInterest = refParam.MinOpenInterest;						///< 持仓量(股/张/份)一分钟最后一笔
		tagMinuteLine.NumTrades = refParam.MinNumTrades2 - refParam.MinNumTrades1;	///< 成交笔数一分钟最后笔减去第一笔的numtrades
		tagMinuteLine.Voip = refParam.MinVoip / refParam.dPriceRate;				///< 基金模净、权证溢价一分钟的最后一笔voip

		{
		CriticalLock		section( m_oMinuteLock );
		if( m_arrayMinuteLine.PutData( &tagMinuteLine ) < 0 )
		{
			ServerStatus::GetStatusObj().AddMinuteLostRef();
		}
		else
		{
			ServerStatus::GetStatusObj().UpdateMinuteBufOccupancyRate( m_arrayMinuteLine.GetPercent() );	///< MinuteLine缓存占用率
		}
		}

		refParam.Valid = 0;															///< 重置有效性标识
	}
}

int QuotationData::UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	int				nErrorCode = 0;
	T_TICK_LINE		refTickLine = { 0 };
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	refTickLine.eMarketID = eMarket;
	refTickLine.Date = nMachineDate;
	refTickLine.Time = nMachineTime;
	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						refTickLine.UpperPx = pStock->HighLimit;
						refTickLine.LowerPx = pStock->LowLimit;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
						if( 0 == refParam.Valid )							///< 取第一笔数据的部分
						{
							refParam.Valid = 1;								///< 有效数据标识
							refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
							refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
							refParam.MinHighPx = pStock->High;
							refParam.MinLowPx = pStock->Low;
							refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
							refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
							//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
						}

						refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
						refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
						refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
						refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
						//refParam.MinSettlePx = pStock->SettlePx;			///< 分钟内最新结算价
						//refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
						//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
						if( pStock->High > refParam.MinHighPx )
						{
							refParam.MinHighPx = pStock->High;				///< 分钟内最高价
						}

						if( pStock->Low < refParam.MinLowPx )
						{
							refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
						}

				}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						//refTickLine.UpperPx = 0;
						//refTickLine.LowerPx = 0;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						//refTickLine.NumTrades = pStock->Records;
						//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
						if( 0 == refParam.Valid )							///< 取第一笔数据的部分
						{
							refParam.Valid = 1;								///< 有效数据标识
							refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
							refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
							refParam.MinHighPx = pStock->High;
							refParam.MinLowPx = pStock->Low;
							refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
							refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
							//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
						}

						refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
						refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
						refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
						//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
						//refParam.MinSettlePx = pStock->SettlePx;			///< 分钟内最新结算价
						//refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
						//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
						if( pStock->High > refParam.MinHighPx )
						{
							refParam.MinHighPx = pStock->High;				///< 分钟内最高价
						}

						if( pStock->Low < refParam.MinLowPx )
						{
							refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			XDFAPI_ShOptData*		pStock = (XDFAPI_ShOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSHOPT.find( std::string(pStock->Code, 8 ) );

			if( it != m_mapSHOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 8 );
				refTickLine.PreClosePx = refParam.PreClosePx / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePx / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->OpenPx / refParam.dPriceRate;
				refTickLine.HighPx = pStock->HighPx / refParam.dPriceRate;
				refTickLine.LowPx = pStock->LowPx / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = refParam.UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = refParam.LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->Position;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code, 8 ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->HighPx;
					refParam.MinLowPx = pStock->LowPx;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->Position;		///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->HighPx > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->HighPx;			///< 分钟内最高价
				}

				if( pStock->LowPx < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->LowPx;				///< 分钟内最低价
				}
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						refTickLine.UpperPx = pStock->HighLimit;
						refTickLine.LowerPx = pStock->LowLimit;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						::memcpy( refTickLine.PreName, refParam.PreName, 4 );
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
						if( 0 == refParam.Valid )							///< 取第一笔数据的部分
						{
							refParam.Valid = 1;								///< 有效数据标识
							refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
							refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
							refParam.MinHighPx = pStock->High;
							refParam.MinLowPx = pStock->Low;
							refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
							refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
							//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
						}

						refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
						refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
						refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
						refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
						//refParam.MinSettlePx = pStock->SettlePx;			///< 分钟内最新结算价
						//refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
						//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
						if( pStock->High > refParam.MinHighPx )
						{
							refParam.MinHighPx = pStock->High;				///< 分钟内最高价
						}

						if( pStock->Low < refParam.MinLowPx )
						{
							refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						//refTickLine.UpperPx = 0;
						//refTickLine.LowerPx = 0;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						//refTickLine.NumTrades = pStock->Records;
						//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
						if( 0 == refParam.Valid )							///< 取第一笔数据的部分
						{
							refParam.Valid = 1;								///< 有效数据标识
							refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
							refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
							refParam.MinHighPx = pStock->High;
							refParam.MinLowPx = pStock->Low;
							refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
							refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
							//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
						}

						refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
						refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
						refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
						//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
						//refParam.MinSettlePx = pStock->SettlePx;			///< 分钟内最新结算价
						//refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
						//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
						if( pStock->High > refParam.MinHighPx )
						{
							refParam.MinHighPx = pStock->High;				///< 分钟内最高价
						}

						if( pStock->Low < refParam.MinLowPx )
						{
							refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			XDFAPI_SzOptData*		pStock = (XDFAPI_SzOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSZOPT.find( std::string(pStock->Code,8) );

			if( it != m_mapSZOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 8 );
				refTickLine.PreClosePx = refParam.PreClosePx / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePx / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->OpenPx / refParam.dPriceRate;
				refTickLine.HighPx = pStock->HighPx / refParam.dPriceRate;
				refTickLine.LowPx = pStock->LowPx / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = refParam.UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = refParam.LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->Position;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code, 8 ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->HighPx;
					refParam.MinLowPx = pStock->LowPx;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->Position;		///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->HighPx > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->HighPx;			///< 分钟内最高价
				}

				if( pStock->LowPx < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->LowPx;				///< 分钟内最低价
				}
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			XDFAPI_CffexData*		pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 6 );
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refTickLine.HighPx = pStock->High / refParam.dPriceRate;
				refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->High;
					refParam.MinLowPx = pStock->Low;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->High > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->High;			///< 分钟内最高价
				}

				if( pStock->Low < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
				}
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			XDFAPI_ZjOptData*		pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strcpy( refTickLine.Code, pStock->Code );
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refTickLine.HighPx = pStock->High / refParam.dPriceRate;
				refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->High;
					refParam.MinLowPx = pStock->Low;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->High > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->High;			///< 分钟内最高价
				}

				if( pStock->Low < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
				}
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			XDFAPI_CNFutureData*		pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );

			if( nTradeDate > 0 )
			{
				 s_nCNFTradingDate = nTradeDate;
			}

			if( it != m_mapCNF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				refTickLine.Type = refParam.Type;
				::strncpy( refTickLine.Code, pStock->Code, 6 );
				refTickLine.Date = s_nCNFTradingDate;
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refTickLine.HighPx = pStock->High / refParam.dPriceRate;
				refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code, 6 ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->High;
					refParam.MinLowPx = pStock->Low;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->High > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->High;			///< 分钟内最高价
				}

				if( pStock->Low < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
				}
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			XDFAPI_CNFutOptData*		pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );

			if( nTradeDate > 0 )
			{
				 s_nCNFOPTTradingDate = nTradeDate;
			}

			if( it != m_mapCNFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				refTickLine.Type = refParam.Type;
				::strcpy( refTickLine.Code, pStock->Code );
				refTickLine.Date = s_nCNFOPTTradingDate;
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refTickLine.HighPx = pStock->High / refParam.dPriceRate;
				refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				DispatchMinuteLine( eMarket, std::string( pStock->Code ), refParam, refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< 取第一笔数据的部分
				{
					refParam.Valid = 1;								///< 有效数据标识
					refParam.MkMinute = refTickLine.Time / 100;		///< 取分钟内第一笔时间
					refParam.MinOpenPx1 = pStock->Now;				///< 取分钟内第一笔现价
					refParam.MinHighPx = pStock->High;
					refParam.MinLowPx = pStock->Low;
					refParam.MinAmount1 = pStock->Amount;			///< 分钟内第一笔金额
					refParam.MinVolume1 = pStock->Volume;			///< 分钟内第一笔成交量
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< 分钟内第一笔成交笔数
				}

				refParam.MinAmount2 = pStock->Amount;				///< 分钟内第末笔金额
				refParam.MinVolume2 = pStock->Volume;				///< 分钟内第末笔成交量
				refParam.MinClosePx = pStock->Now;					///< 分钟内最新价
				//refParam.MinVoip = pStock->Voip;					///< 分钟内第末笔Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< 分钟内最新结算价
				refParam.MinOpenInterest = pStock->OpenInterest;	///< 分钟内第末笔持仓量
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< 分钟内第末笔成交笔数
				if( pStock->High > refParam.MinHighPx )
				{
					refParam.MinHighPx = pStock->High;				///< 分钟内最高价
				}

				if( pStock->Low < refParam.MinLowPx )
				{
					refParam.MinLowPx = pStock->Low;				///< 分钟内最低价
				}
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateTickLine() : invalid market id(%d)", (int)eMarket );
		break;
	}

	if( nErrorCode < 0 )
	{
		ServerStatus::GetStatusObj().AddTickLostRef();
		return -1;
	}

	ServerStatus::GetStatusObj().UpdateTickBufOccupancyRate( m_arrayTickLine.GetPercent() );	///< TICK缓存占用率
	ServerStatus::GetStatusObj().UpdateSecurity( (enum XDFMarket)(refTickLine.eMarketID), refTickLine.Code, refTickLine.NowPx, refTickLine.Amount, refTickLine.Volume );	///< 更新各市场的商品状态

	return 0;
}




