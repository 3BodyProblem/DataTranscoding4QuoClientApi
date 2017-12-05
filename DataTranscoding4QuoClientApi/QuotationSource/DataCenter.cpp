#include "Quotation.h"
#include "DataCenter.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


///< -------------------- Configuration ------------------------------------------------
const int	XDF_SH_COUNT = 16000;					///< 上海Lv1
const int	XDF_SHL2_COUNT = 0;						///< 上海Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SHOPT_COUNT = 500;					///< 上海期权
const int	XDF_SZ_COUNT = 8000;					///< 深证Lv1
const int	XDF_SZL2_COUNT = 0;						///< 深证Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SZOPT_COUNT = 0;					///< 深圳期权
const int	XDF_CF_COUNT = 500;					///< 中金期货
const int	XDF_ZJOPT_COUNT = 500;					///< 中金期权
const int	XDF_CNF_COUNT = 500;					///< 商品期货(上海/郑州/大连)
const int	XDF_CNFOPT_COUNT = 500;				///< 商品期权(上海/郑州/大连)
static unsigned int	s_nNumberInSection = 60;		///< 一个市场有可以缓存多少个数据块
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
	CriticalLock	section( m_oLock );

	return m_pDataCache;
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
		if( NULL == m_pDataCache )
		{
			m_nMaxCacheSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_TICK_LINE) * s_nNumberInSection;
			m_pDataCache = new char[m_nMaxCacheSize];
			::memset( m_pDataCache, 0, m_nMaxCacheSize );
		}

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
 : m_pQuotation( NULL )
{
}

QuotationData::~QuotationData()
{
}

int QuotationData::Initialize( void* pQuotation )
{
	m_pQuotation = pQuotation;
	Release();

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

void QuotationData::BeginDumpThread( enum XDFMarket eMarket, int nStatus )
{
	if( 5 == nStatus )
	{
		if( false == m_oThdDump.IsAlive() )
		{
			if( 0 != m_oThdDump.Create( "ThreadDumpTickLine()", ThreadDumpTickLine, this ) ) {
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::BeginDumpThread() : failed 2 create tick line thread(1)" );
			}
		}
	}
}

__inline bool	PrepareTickFile( T_TICK_LINE* pTickLine, std::string& sCode, std::ofstream& oDumper )
{
	char				pszFilePath[512] = { 0 };

	switch( pTickLine->eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( pTickLine->Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date, pTickLine->Code, pTickLine->Date );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : invalid market subid (Type=%d)", pTickLine->Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : invalid market id (%s)", pTickLine->eMarketID );
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
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : cannot open file (%s)", sFilePath.c_str() );
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
			unsigned int			nBufLen = CacheAlloc::GetObj().GetDataLength();
			unsigned int			nMaxDataNum = nBufLen / sizeof(T_TICK_LINE);
			std::string				sCode;
			std::ofstream			oDumper;
			STR_TICK_LINE			pszLine = { 0 };

			SimpleThread::Sleep( 1000 * 1 );
			if( nDumpNumber > 3000 ) {
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpTickLine() : (%I64d) dumped... ( Count=%u )", nDumpNumber, nMaxDataNum );
				nDumpNumber = 0;
			}

			for( int n = 0; n < nMaxDataNum; n++ )
			{
				T_TICK_LINE*				pTickLine = (T_TICK_LINE*)(pBufPtr + n * sizeof(T_TICK_LINE));

				if( 0 == pTickLine->Valid )
				{
					continue;
				}
				else
				{
					if( sCode != pTickLine->Code )
					{
						if( false == PrepareTickFile( pTickLine, sCode, oDumper ) )
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
						, pTickLine->Date, pTickLine->Time, pTickLine->PreClosePx, pTickLine->PreSettlePx
						, pTickLine->OpenPx, pTickLine->HighPx, pTickLine->LowPx, pTickLine->ClosePx, pTickLine->NowPx, pTickLine->SettlePx
						, pTickLine->UpperPx, pTickLine->LowerPx, pTickLine->Amount, pTickLine->Volume, pTickLine->OpenInterest, pTickLine->NumTrades
						, pTickLine->BidPx1, pTickLine->BidVol1, pTickLine->BidPx2, pTickLine->BidVol2, pTickLine->AskPx1, pTickLine->AskVol1, pTickLine->AskPx2, pTickLine->AskVol2
						, pTickLine->Voip, pTickLine->TradingPhaseCode, pTickLine->PreName );
					oDumper.write( pszLine, nLen );
					pTickLine->Valid = 0;
					nDumpNumber++;
				}
			}

			refQuotation.FlushDayLineOnCloseTime();		///< 检查是否需要落日线
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
				T_QUO_DATA&		refData = it->second;
				::memcpy( refData.first.PreName, pszPreName, 4 );
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
				T_QUO_DATA&		refData = m_mapSHL1[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			T_MAP_QUO::iterator it = m_mapSHOPT.find( sCode );
			if( it == m_mapSHOPT.end() )
			{
				T_QUO_DATA&		refData = m_mapSHOPT[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			T_MAP_QUO::iterator it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				T_QUO_DATA&		refData = m_mapSZL1[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			T_MAP_QUO::iterator it = m_mapSZOPT.find( sCode );
			if( it == m_mapSZOPT.end() )
			{
				T_QUO_DATA&		refData = m_mapSZOPT[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			T_MAP_QUO::iterator it = m_mapCFF.find( sCode );
			if( it == m_mapCFF.end() )
			{
				T_QUO_DATA&		refData = m_mapCFF[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			T_MAP_QUO::iterator it = m_mapCFFOPT.find( sCode );
			if( it == m_mapCFFOPT.end() )
			{
				T_QUO_DATA&		refData = m_mapCFFOPT[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			T_MAP_QUO::iterator it = m_mapCNF.find( sCode );
			if( it == m_mapCNF.end() )
			{
				T_QUO_DATA&		refData = m_mapCNF[sCode];
				pDataPtr = CacheAlloc::GetObj().GrabCache( eMarket, nBufSize );
				refData.first = refParam;
				nErrorCode = refData.second.Instance( pDataPtr, nBufSize );
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			T_MAP_QUO::iterator it = m_mapCNFOPT.find( sCode );
			if( it == m_mapCNFOPT.end() )
			{
				T_QUO_DATA&		refData = m_mapCNFOPT[sCode];
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
	char			pszDayLine[512] = { 0 };
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
						T_LINE_PARAM&		refParam = it->second.first;

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
						T_LINE_PARAM&		refParam = it->second.first;

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
				T_LINE_PARAM&		refParam = it->second.first;

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
						T_LINE_PARAM&		refParam = it->second.first;

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
						T_LINE_PARAM&		refParam = it->second.first;

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
				T_LINE_PARAM&		refParam = it->second.first;

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
				T_LINE_PARAM&		refParam = it->second.first;

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
				T_LINE_PARAM&		refParam = it->second.first;

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
				T_LINE_PARAM&			refParam = it->second.first;

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
				T_LINE_PARAM&			refParam = it->second.first;

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

int QuotationData::UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	int				nErrorCode = 0;
	T_TICK_LINE		refTickLine = { 0 };
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	refTickLine.Valid = 1;
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
						T_LINE_PARAM&		refParam = it->second.first;
						T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

						nErrorCode = refTickLineCache.PutData( &refTickLine );
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
						T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

						nErrorCode = refTickLineCache.PutData( &refTickLine );
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
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
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
						T_LINE_PARAM&		refParam = it->second.first;
						T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

						nErrorCode = refTickLineCache.PutData( &refTickLine );
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second.first;
						T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

						nErrorCode = refTickLineCache.PutData( &refTickLine );
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
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

				::strncpy( refTickLine.Code, pStock->Code, 6 );
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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			XDFAPI_CffexData*		pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			XDFAPI_ZjOptData*		pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
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
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
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
				T_LINE_PARAM&		refParam = it->second.first;
				T_TICKLINE_CACHE&	refTickLineCache = it->second.second;

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

				nErrorCode = refTickLineCache.PutData( &refTickLine );
			}
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateTickLine() : an error occur while updating tick line(%s), marketid=%d, errorcode=%d", refTickLine.Code, (int)eMarket, nErrorCode );
		return -1;
	}

	return 0;
}




