#include "DataCenter.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "../DataTranscoding4QuoClientApi.h"


///< -------------------- Configuration ------------------------------------------------
const int	XDF_SH_COUNT = 20000;					///< 上海Lv1
const int	XDF_SHL2_COUNT = 0;						///< 上海Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SHOPT_COUNT = 12000	;				///< 上海期权
const int	XDF_SZ_COUNT = 12000;					///< 深证Lv1
const int	XDF_SZL2_COUNT = 0;						///< 深证Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SZOPT_COUNT = 0;					///< 深圳期权
const int	XDF_CF_COUNT = 1000;					///< 中金期货
const int	XDF_ZJOPT_COUNT = 1000;					///< 中金期权
const int	XDF_CNF_COUNT = 1000;					///< 商品期货(上海/郑州/大连)
const int	XDF_CNFOPT_COUNT = 1000;				///< 商品期权(上海/郑州/大连)
static unsigned int	s_nNumberInSection = 120;		///< 一个市场有可以缓存多少个数据块
///< -----------------------------------------------------------------------------------


CacheAlloc::CacheAlloc()
 : m_nMaxCacheSize( 0 ), m_nAllocateSize( 0 ), m_pDataCache( NULL )
{
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
	return m_pDataCache;
}

unsigned int CacheAlloc::GetDataLength()
{
	return m_nAllocateSize;
}

char* CacheAlloc::GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize )
{
	char*			pData = NULL;
	unsigned int	nBufferSize4Market = 0;
	CriticalLock	section( m_oLock );

	try
	{
		nOutSize = 0;
		if( NULL == m_pDataCache )
		{
			m_nMaxCacheSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_DAY_LINE) * s_nNumberInSection;
			m_pDataCache = new char[m_nMaxCacheSize];
			::memset( m_pDataCache, 0, m_nMaxCacheSize );
		}

		if( NULL == m_pDataCache )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : invalid buffer pointer." );
			return NULL;
		}

		switch( eMkID )
		{
		case XDF_SH:		///< 上海Lv1
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_SHOPT:		///< 上海期权
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_SZ:		///< 深证Lv1
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_SZOPT:		///< 深圳期权
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_CF:		///< 中金期货
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_ZJOPT:		///< 中金期权
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_CNF:		///< 商品期货(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_DAY_LINE) * s_nNumberInSection;
			break;
		default:
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : unknow market id" );
				return NULL;
			}
		}

		if( (m_nAllocateSize + nBufferSize4Market) > m_nMaxCacheSize )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : not enough space." );
			return NULL;
		}

		nOutSize = nBufferSize4Market;
		pData = m_pDataCache + m_nAllocateSize;
		m_nAllocateSize += nBufferSize4Market;
	}
	catch( std::exception& err )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : an exceptin occur : %s", err.what() );
	}
	catch( ... )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : unknow exception" );
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


QuotationData::QuotationData()
{
}

QuotationData::~QuotationData()
{
}

int QuotationData::Initialize()
{
	Release();

	return 0;
}

void QuotationData::Release()
{
}

void QuotationData::UpdateModuleStatus( enum XDFMarket eMarket, int nStatus )
{
	m_mapModuleStatus[eMarket] = nStatus;
}

void QuotationData::BeginDumpThread( enum XDFMarket eMarket, int nStatus )
{
	if( 5 == nStatus )
	{
		switch( eMarket )
		{
		case XDF_SH:		///< 上海Lv1
			{
				if( false == m_oThdSHL1.IsAlive() )
				{
					if( 0 != m_oThdSHL1.Create( "ThreadDumpDayLine4SHL1()", ThreadDumpDayLine4SHL1, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create SHL1 day line thread" );
					}
				}
			}
			break;
		case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
			break;
		case XDF_SHOPT:		///< 上海期权
			{
/*				if( false == m_oThdSHOPT.IsAlive() )
				{
					if( 0 != m_oThdSHOPT.Create( "ThreadDumpDayLine4SHOPT()", ThreadDumpDayLine4SHOPT, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create SHOPT day line thread" );
					}
				}*/
			}
			break;
		case XDF_SZ:		///< 深证Lv1
			{
/*				if( false == m_oThdSZL1.IsAlive() )
				{
					if( 0 != m_oThdSZL1.Create( "ThreadDumpDayLine4SZL1()", ThreadDumpDayLine4SZL1, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create SZL1 day line thread" );
					}
				}*/
			}
			break;
		case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
			break;
		case XDF_SZOPT:		///< 深圳期权
			{
/*				if( false == m_oThdSZOPT.IsAlive() )
				{
					if( 0 != m_oThdSZOPT.Create( "ThreadDumpDayLine4SZOPT()", ThreadDumpDayLine4SZOPT, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create SZL1 day line thread" );
					}
				}*/
			}
			break;
		case XDF_CF:		///< 中金期货
			{
/*				if( false == m_oThdCFF.IsAlive() )
				{
					if( 0 != m_oThdCFF.Create( "ThreadDumpDayLine4CFF()", ThreadDumpDayLine4CFF, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create CFF day line thread" );
					}
				}*/
			}
			break;
		case XDF_ZJOPT:		///< 中金期权
			{
/*				if( false == m_oThdCFFOPT.IsAlive() )
				{
					if( 0 != m_oThdCFFOPT.Create( "ThreadDumpDayLine4CFFOPT()", ThreadDumpDayLine4CFFOPT, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create CFFOPT day line thread" );
					}
				}*/
			}
			break;
		case XDF_CNF:		///< 商品期货(上海/郑州/大连)
			{
/*				if( false == m_oThdCNF.IsAlive() )
				{
					if( 0 != m_oThdCNF.Create( "ThreadDumpDayLine4CNF()", ThreadDumpDayLine4CNF, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create CNF day line thread" );
					}
				}*/
			}
			break;
		case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
			{
/*				if( false == m_oThdCNFOPT.IsAlive() )
				{
					if( 0 != m_oThdCNFOPT.Create( "ThreadDumpDayLine4CNFOPT()", ThreadDumpDayLine4CNFOPT, this ) ) {
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateModuleStatus() : failed 2 create CNFOPT day line thread" );
					}
				}*/
			}
			break;
		default:
			break;
		}
	}
}

typedef char	STR_DAY_LINE[512];

void* QuotationData::ThreadDumpDayLine4SHL1( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;
	char*			pBufPtr = CacheAlloc::GetObj().GetBufferPtr();
	unsigned int	nBufLen = CacheAlloc::GetObj().GetDataLength();
	unsigned int	nMaxDataNum = nBufLen / sizeof(T_DAY_LINE);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		std::string					sCode;
		std::ofstream				oDumper;
		STR_DAY_LINE				pszLine = { 0 };

		SimpleThread::Sleep( 1000 * 1 );
		for( int n = 0; n < nMaxDataNum; n++ )
		{
			T_DAY_LINE*				pDayLine = (T_DAY_LINE*)(pBufPtr + n * sizeof(T_DAY_LINE));

			if( 0 == pDayLine->Valid )
			{
				continue;
			}
			else
			{
				if( sCode != pDayLine->Code )
				{
					std::string			sMkName;
					char				pszFile[512] = { 0 };
					unsigned int		nDate = DateTime::Now().DateToLong();

					switch( pDayLine->eMarketID )
					{
					case XDF_SH:		///< 上海Lv1
						sMkName = "SSE";
						break;
					case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
						break;
					case XDF_SHOPT:		///< 上海期权
						sMkName = "SSE";
						break;
					case XDF_SZ:		///< 深证Lv1
						sMkName = "SZSE";
						break;
					case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
						break;
					case XDF_SZOPT:		///< 深圳期权
						sMkName = "SZSE";
						break;
					case XDF_CF:		///< 中金期货
						sMkName = "CFFEX";
						break;
					case XDF_ZJOPT:		///< 中金期权
						sMkName = "CFFEX";
						break;
					case XDF_CNF:		///< 商品期货(上海/郑州/大连)
						sMkName = "DCE";
						break;
					case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)SHFE
						sMkName = "CZCE";
						break;
					}

					::sprintf( pszFile, "-%s-TICK-%s-%u-TICK%u_%u.csv", sMkName.c_str(), pDayLine->Code, nDate, pDayLine->Code, nDate );
					std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
					oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::binary|std::ios::app );

					if( !oDumper.is_open() )
					{
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4SHL1() : cannot open file (%s)", sFilePath.c_str() );
						SimpleThread::Sleep( 1000 * 30 );
						continue;
					}

					sCode = pDayLine->Code;
					oDumper.seekp( 0, std::ios::end );
					if( 0 == oDumper.tellp() )
					{
						std::string		sTitle = "date,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\r\n";
						oDumper << sTitle;
					}
				}

				if( !oDumper.is_open() )
				{
					QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4SHL1() : invalid file handle" );
					SimpleThread::Sleep( 1000 * 30 );
					continue;
				}

				int		nLen = ::sprintf( pszLine, "%u,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%I64d,%f\r\n"
					, pDayLine->Date, pDayLine->OpenPx, pDayLine->HighPx, pDayLine->LowPx, pDayLine->ClosePx, pDayLine->SettlePx
					, pDayLine->Amount, pDayLine->Volume, pDayLine->OpenInterest, pDayLine->NumTrades, pDayLine->Voip );
				oDumper.write( pszLine, nLen );
				pDayLine->Valid = 0;
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4SHOPT( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdSHOPT.IsAlive() )
	{
		T_MAP_QUO&				refMapData = refData.m_mapSHOPT;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/SSE/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4SHOPT() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}

			}

		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4SZL1( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdSZL1.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapSZL1;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/SZSE/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4SZL1() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4SZOPT( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdSZOPT.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapSZOPT;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/SZSE/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4SZOPT() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4CFF( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdCFF.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapCFF;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/CFFEX/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4CFF() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4CFFOPT( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdCFFOPT.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapCFFOPT;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/CFFEX/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4CFFOPT() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4CNF( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdCNF.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapCNF;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/SSE/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4CNF() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
}

void* QuotationData::ThreadDumpDayLine4CNFOPT( void* pSelf )
{
	QuotationData&	refData = *(QuotationData*)pSelf;

	while( true == refData.m_oThdCNFOPT.IsAlive() )
	{
		MemoDumper<T_DAY_LINE>	oDumper;
		T_MAP_QUO&				refMapData = refData.m_mapCNFOPT;

		SimpleThread::Sleep( 1000 * 1 );
		if( refMapData.empty() )
		{
			continue;
		}

		for( T_MAP_QUO::iterator it = refMapData.begin(); it != refMapData.end(); it++ )
		{
			T_DAY_LINE			tagLine = { 0 };
			T_DAYLINE_CACHE&	refDayLines = it->second.second;
			char				pszFile[512] = { 0 };
			unsigned int		nDate = DateTime::Now().DateToLong();

			::sprintf( pszFile, "/SSE/TICK/%s/%u/TICK%u_%u.CVS", it->first.c_str(), nDate, it->first, nDate );
			std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFile );
			MemoDumper<T_DAY_LINE>	oDumper( false, sFilePath.c_str() );

			if( false == oDumper.IsOpen() )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine4CNFOPT() : cannot open file (%s)", sFilePath.c_str() );
				SimpleThread::Sleep( 1000 * 30 );
				continue;
			}

			while( true )
			{
				if( refDayLines.GetData( &tagLine, sizeof(T_DAY_LINE) ) <= 0 )
				{
					break;
				}
			}
		}
	}

	return NULL;
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
			T_QUO_DATA&	refData = m_mapSHL1[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			T_QUO_DATA&	refData = m_mapSHOPT[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			T_QUO_DATA&	refData = m_mapSZL1[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			T_QUO_DATA&	refData = m_mapSZOPT[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			T_QUO_DATA&	refData = m_mapCFF[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			T_QUO_DATA&	refData = m_mapCFFOPT[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			T_QUO_DATA&	refData = m_mapCNF[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			T_QUO_DATA&	refData = m_mapCNFOPT[sCode];
			if( refData.first.dPriceRate == 0 )
			{
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
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

int QuotationData::UpdateDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize )
{
	int				nErrorCode = 0;
	T_DAY_LINE		refDayLine = { 0 };

	refDayLine.Valid = 1;
	switch( refDayLine.eMarketID )
	{
	case XDF_SH:	///< 上证L1
		{
			refDayLine.eMarketID = XDF_SH;
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second.first;
						T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

						::strncpy( refDayLine.Code, pStock->Code, 6 );
						nErrorCode = refDayLineCache.PutData( &refDayLine );
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second.first;
						T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

						::strncpy( refDayLine.Code, pStock->Code, 6 );
						nErrorCode = refDayLineCache.PutData( &refDayLine );
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			T_QUO_DATA&	refData = m_mapSHOPT[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			T_QUO_DATA&	refData = m_mapSZL1[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			T_QUO_DATA&	refData = m_mapSZOPT[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			T_QUO_DATA&	refData = m_mapCFF[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			T_QUO_DATA&	refData = m_mapCFFOPT[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			T_QUO_DATA&	refData = m_mapCNF[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			T_QUO_DATA&	refData = m_mapCNFOPT[std::string(refDayLine.Code)];
			if( refData.first.dPriceRate == 0 )
			{
				nErrorCode = refData.second.PutData( &refDayLine );
			}
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateDayLine() : an error occur while updating day line(%s), marketid=%d, errorcode=%d", refDayLine.Code, (int)refDayLine.eMarketID, nErrorCode );
		return -1;
	}

	return 0;
}




