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
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DayLineArray::GrabCache() : not enough space ( %u > %u )", (m_nAllocateSize + nBufferSize4Market), m_nMaxCacheSize );
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
			if( 0 != m_oThdDump.Create( "ThreadDumpDayLine1()", ThreadDumpDayLine1, this ) ) {
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::BeginDumpThread() : failed 2 create day line thread(1)" );
			}
		}
	}
}

__inline bool	PrepareFile( T_DAY_LINE* pDayLine, std::string& sCode, std::ofstream& oDumper )
{
	char				pszFilePath[512] = { 0 };

	switch( pDayLine->eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( pDayLine->Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/TICK/%s/%u/", pDayLine->Code, pDayLine->Date, pDayLine->Code, pDayLine->Date );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : invalid market subid (Type=%d)", pDayLine->Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : invalid market id (%s)", pDayLine->eMarketID );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "TICK%s_%u.csv", pDayLine->Code, pDayLine->Date );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	sCode = pDayLine->Code;
	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,time,preclosepx,presettlepx,openpx,highpx,lowpx,closepx,nowpx,settlepx,upperpx,lowerpx,amount,volume,openinterest,numtrades,bidpx1,bidvol1,bidpx2,bidvol2,askpx1,askvol1,askpx2,askvol2,voip,tradingphasecode,prename\n";
		oDumper << sTitle;
	}

	return true;
}

typedef char	STR_DAY_LINE[512];

void* QuotationData::ThreadDumpDayLine1( void* pSelf )
{
	unsigned __int64			nDumpNumber = 0;
	QuotationData&				refData = *(QuotationData*)pSelf;

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			char*					pBufPtr = CacheAlloc::GetObj().GetBufferPtr();
			unsigned int			nBufLen = CacheAlloc::GetObj().GetDataLength();
			unsigned int			nMaxDataNum = nBufLen / sizeof(T_DAY_LINE);
			std::string				sCode;
			std::ofstream			oDumper;
			STR_DAY_LINE			pszLine = { 0 };

			SimpleThread::Sleep( 1000 * 1 );
			if( nDumpNumber > 3000 ) {
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpDayLine1() : (%I64d) dumped... ( Count=%u )", nDumpNumber, nMaxDataNum );
				nDumpNumber = 0;
			}

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
						if( false == PrepareFile( pDayLine, sCode, oDumper ) )
						{
							continue;
						}
					}

					if( !oDumper.is_open() )
					{
						QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : invalid file handle" );
						SimpleThread::Sleep( 1000 * 10 );
						continue;
					}

					int		nLen = ::sprintf( pszLine, "%u,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%s,%s\n"
						, pDayLine->Date, pDayLine->Time, pDayLine->PreClosePx, pDayLine->PreSettlePx
						, pDayLine->OpenPx, pDayLine->HighPx, pDayLine->LowPx, pDayLine->ClosePx, pDayLine->NowPx, pDayLine->SettlePx
						, pDayLine->UpperPx, pDayLine->LowerPx, pDayLine->Amount, pDayLine->Volume, pDayLine->OpenInterest, pDayLine->NumTrades
						, pDayLine->BidPx1, pDayLine->BidVol1, pDayLine->BidPx2, pDayLine->BidVol2, pDayLine->AskPx1, pDayLine->AskVol1, pDayLine->AskPx2, pDayLine->AskVol2
						, pDayLine->Voip, pDayLine->TradingPhaseCode, pDayLine->PreName );
					oDumper.write( pszLine, nLen );
					pDayLine->Valid = 0;
					nDumpNumber++;
				}
			}
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpDayLine1() : unknow exception" );
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

int QuotationData::UpdateDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	int				nErrorCode = 0;
	T_DAY_LINE		refDayLine = { 0 };
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	refDayLine.Valid = 1;
	refDayLine.eMarketID = eMarket;
	refDayLine.Date = nMachineDate;
	refDayLine.Time = nMachineTime;
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
						T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

						::strncpy( refDayLine.Code, pStock->Code, 6 );
						refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refDayLine.PreSettlePx = 0;
						refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refDayLine.HighPx = pStock->High / refParam.dPriceRate;
						refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
						refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refDayLine.NowPx = refDayLine.ClosePx;
						//refDayLine.SettlePx = 0;
						refDayLine.UpperPx = pStock->HighLimit;
						refDayLine.LowerPx = pStock->LowLimit;
						refDayLine.Amount = pStock->Amount;
						refDayLine.Volume = pStock->Volume;
						//refDayLine.OpenInterest = 0;
						refDayLine.NumTrades = pStock->Records;
						refDayLine.BidPx1 = pStock->Buy[0].Price;
						refDayLine.BidVol1 = pStock->Buy[0].Volume;
						refDayLine.BidPx2 = pStock->Buy[1].Price;
						refDayLine.BidVol2 = pStock->Buy[1].Volume;
						refDayLine.AskPx1 = pStock->Sell[0].Price;
						refDayLine.AskVol1 = pStock->Sell[0].Volume;
						refDayLine.AskPx2 = pStock->Sell[1].Price;
						refDayLine.AskVol2 = pStock->Sell[1].Volume;
						refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

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
						refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refDayLine.PreSettlePx = 0;
						refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refDayLine.HighPx = pStock->High / refParam.dPriceRate;
						refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
						refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refDayLine.NowPx = refDayLine.ClosePx;
						//refDayLine.SettlePx = 0;
						//refDayLine.UpperPx = 0;
						//refDayLine.LowerPx = 0;
						refDayLine.Amount = pStock->Amount;
						refDayLine.Volume = pStock->Volume;
						//refDayLine.OpenInterest = 0;
						//refDayLine.NumTrades = pStock->Records;
						//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

						nErrorCode = refDayLineCache.PutData( &refDayLine );
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
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				::strncpy( refDayLine.Code, pStock->Code, 8 );
				refDayLine.PreClosePx = refParam.PreClosePx / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePx / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->OpenPx / refParam.dPriceRate;
				refDayLine.HighPx = pStock->HighPx / refParam.dPriceRate;
				refDayLine.LowPx = pStock->LowPx / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = refParam.UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = refParam.LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->Position;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refDayLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );

				nErrorCode = refDayLineCache.PutData( &refDayLine );
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
						T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

						::strncpy( refDayLine.Code, pStock->Code, 6 );
						refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refDayLine.PreSettlePx = 0;
						refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refDayLine.HighPx = pStock->High / refParam.dPriceRate;
						refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
						refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refDayLine.NowPx = refDayLine.ClosePx;
						//refDayLine.SettlePx = 0;
						refDayLine.UpperPx = pStock->HighLimit;
						refDayLine.LowerPx = pStock->LowLimit;
						refDayLine.Amount = pStock->Amount;
						refDayLine.Volume = pStock->Volume;
						//refDayLine.OpenInterest = 0;
						refDayLine.NumTrades = pStock->Records;
						refDayLine.BidPx1 = pStock->Buy[0].Price;
						refDayLine.BidVol1 = pStock->Buy[0].Volume;
						refDayLine.BidPx2 = pStock->Buy[1].Price;
						refDayLine.BidVol2 = pStock->Buy[1].Volume;
						refDayLine.AskPx1 = pStock->Sell[0].Price;
						refDayLine.AskVol1 = pStock->Sell[0].Volume;
						refDayLine.AskPx2 = pStock->Sell[1].Price;
						refDayLine.AskVol2 = pStock->Sell[1].Volume;
						refDayLine.Voip = pStock->Voip / refParam.dPriceRate;
						::memcpy( refDayLine.PreName, refParam.PreName, 4 );

						nErrorCode = refDayLineCache.PutData( &refDayLine );
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
						T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

						::strncpy( refDayLine.Code, pStock->Code, 6 );
						refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						//refDayLine.PreSettlePx = 0;
						refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refDayLine.HighPx = pStock->High / refParam.dPriceRate;
						refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
						refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refDayLine.NowPx = refDayLine.ClosePx;
						//refDayLine.SettlePx = 0;
						//refDayLine.UpperPx = 0;
						//refDayLine.LowerPx = 0;
						refDayLine.Amount = pStock->Amount;
						refDayLine.Volume = pStock->Volume;
						//refDayLine.OpenInterest = 0;
						//refDayLine.NumTrades = pStock->Records;
						//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

						nErrorCode = refDayLineCache.PutData( &refDayLine );
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
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				::strncpy( refDayLine.Code, pStock->Code, 6 );
				refDayLine.PreClosePx = refParam.PreClosePx / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePx / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->OpenPx / refParam.dPriceRate;
				refDayLine.HighPx = pStock->HighPx / refParam.dPriceRate;
				refDayLine.LowPx = pStock->LowPx / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = refParam.UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = refParam.LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->Position;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refDayLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );

				nErrorCode = refDayLineCache.PutData( &refDayLine );
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
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				::strncpy( refDayLine.Code, pStock->Code, 6 );
				refDayLine.Time = pStock->DataTimeStamp;
				refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refDayLine.HighPx = pStock->High / refParam.dPriceRate;
				refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->OpenInterest;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

				nErrorCode = refDayLineCache.PutData( &refDayLine );
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
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				::strcpy( refDayLine.Code, pStock->Code );
				refDayLine.Time = pStock->DataTimeStamp;
				refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refDayLine.HighPx = pStock->High / refParam.dPriceRate;
				refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->OpenInterest;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

				nErrorCode = refDayLineCache.PutData( &refDayLine );
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			XDFAPI_CNFutureData*		pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );
			static unsigned int			s_nCNFTradingDate = 0;

			if( nTradeDate > 0 )
			{
				 s_nCNFTradingDate = nTradeDate;
			}

			if( it != m_mapCNF.end() )
			{
				T_LINE_PARAM&		refParam = it->second.first;
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				refDayLine.Type = refParam.Type;
				::strncpy( refDayLine.Code, pStock->Code, 6 );
				refDayLine.Date = s_nCNFTradingDate;
				refDayLine.Time = pStock->DataTimeStamp;
				refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refDayLine.HighPx = pStock->High / refParam.dPriceRate;
				refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->OpenInterest;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

				nErrorCode = refDayLineCache.PutData( &refDayLine );
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			XDFAPI_CNFutOptData*		pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );
			static unsigned int			s_nCNFOPTTradingDate = 0;

			if( nTradeDate > 0 )
			{
				 s_nCNFOPTTradingDate = nTradeDate;
			}

			if( it != m_mapCNFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second.first;
				T_DAYLINE_CACHE&	refDayLineCache = it->second.second;

				refDayLine.Type = refParam.Type;
				::strcpy( refDayLine.Code, pStock->Code );
				refDayLine.Date = s_nCNFOPTTradingDate;
				refDayLine.Time = pStock->DataTimeStamp;
				refDayLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
				refDayLine.PreSettlePx = pStock->PreSettlePrice / refParam.dPriceRate;
				refDayLine.OpenPx = pStock->Open / refParam.dPriceRate;
				refDayLine.HighPx = pStock->High / refParam.dPriceRate;
				refDayLine.LowPx = pStock->Low / refParam.dPriceRate;
				refDayLine.ClosePx = pStock->Now / refParam.dPriceRate;
				refDayLine.NowPx = refDayLine.ClosePx;
				refDayLine.SettlePx = pStock->SettlePrice / refParam.dPriceRate;
				refDayLine.UpperPx = pStock->UpperPrice / refParam.dPriceRate;
				refDayLine.LowerPx = pStock->LowerPrice / refParam.dPriceRate;
				refDayLine.Amount = pStock->Amount;
				refDayLine.Volume = pStock->Volume;
				refDayLine.OpenInterest = pStock->OpenInterest;
				//refDayLine.NumTrades = 0;
				refDayLine.BidPx1 = pStock->Buy[0].Price;
				refDayLine.BidVol1 = pStock->Buy[0].Volume;
				refDayLine.BidPx2 = pStock->Buy[1].Price;
				refDayLine.BidVol2 = pStock->Buy[1].Volume;
				refDayLine.AskPx1 = pStock->Sell[0].Price;
				refDayLine.AskVol1 = pStock->Sell[0].Volume;
				refDayLine.AskPx2 = pStock->Sell[1].Price;
				refDayLine.AskVol2 = pStock->Sell[1].Volume;
				//refDayLine.Voip = pStock->Voip / refParam.dPriceRate;

				nErrorCode = refDayLineCache.PutData( &refDayLine );
			}
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateDayLine() : an error occur while updating day line(%s), marketid=%d, errorcode=%d", refDayLine.Code, (int)eMarket, nErrorCode );
		return -1;
	}

	return 0;
}




