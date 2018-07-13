#include "DataTick.h"
#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "Base.h"
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


///< TICK线计算并落盘类 ///////////////////////////////////////////////////////////////////////////////////////////
__inline FILE*	PrepareTickFile( char cMkID, const char* pszCode, unsigned int nDate, bool& bHasTitle )
{
	FILE*				pDumpFile = NULL;
	char				pszFilePath[512] = { 0 };

	switch( cMkID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pszCode, nDate );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pszCode, nDate );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : invalid market id (%d)", (int)cMkID );
		return NULL;
	}

	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "TICK%s_%u.csv", pszCode, nDate );
	sFilePath += pszFileName;

	pDumpFile = ::fopen( sFilePath.c_str(), "a+" );
	if( NULL == pDumpFile )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : cannot open file (%s)", sFilePath.c_str() );
		return NULL;
	}

	if( false == bHasTitle ) {
		::fseek( pDumpFile, 0, SEEK_END );
		if( 0 == ftell(pDumpFile) )
		{
			static const char	pszTitle[] = "date,time,preclosepx,presettlepx,openpx,highpx,lowpx,closepx,nowpx,settlepx,upperpx,lowerpx,amount,volume,openinterest,numtrades,bidpx1,bidvol1,bidpx2,bidvol2,askpx1,askvol1,askpx2,askvol2,voip,tradingphasecode,prename\n";

			::fwrite( pszTitle, sizeof(char), sizeof(pszTitle), pDumpFile );
			bHasTitle = true;
		}
	}

	return pDumpFile;
}

const unsigned int		TickGenerator::s_nMaxLineCount = 20 * 5;
TickGenerator::TickGenerator()
 : m_nDate( 0 ), m_pDataCache( NULL ), m_dPriceRate( 0 ), m_bCloseFile( true )
 , m_nPreClosePx( 0 ), m_nOpenPx( 0 ), m_pDumpFile( NULL ), m_bHasTitle( false )
{
	::memset( m_pszCode, 0, sizeof(m_pszCode) );
	::memset( m_pszPreName, 0, sizeof(m_pszPreName) );
}

TickGenerator::TickGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr )
: m_nPreClosePx( 0 ), m_nOpenPx( 0 ), m_eMarket( eMkID ), m_pDumpFile( NULL ), m_bHasTitle( false )
, m_nDate( nDate ), m_dPriceRate( dPriceRate ), m_pDataCache( pBufPtr ), m_bCloseFile( true )
{
	::strcpy( m_pszCode, sCode.c_str() );
	::memset( m_pszPreName, 0, sizeof(m_pszPreName) );
	::memset( m_pDataCache, 0, sizeof(T_DATA) * s_nMaxLineCount );

	///< 设置，是否需要长时间打开TICK落盘文件
	unsigned int	nCode = ::atoi( m_pszCode );

	if( XDF_SH == eMkID )
	{
		/*if( nCode >= 1 && nCode <= 999 ) {
			m_bCloseFile = false;
		} else if( nCode >= 600000 && nCode <= 609999 ) {
			m_bCloseFile = false;
		}/* else if( nCode >= 510000 && nCode <= 519999 ) {
			m_bCloseFile = false;
		}*/
	}
	else if( XDF_SZ == eMkID ) {
		/*if( nCode >= 399000 && nCode <= 399999 ) {
			m_bCloseFile = false;
		} else if( nCode >= 1 && nCode <= 9999 ) {
			m_bCloseFile = false;
		}/* else if( nCode >= 300000 && nCode <= 300999 ) {
			m_bCloseFile = false;
		} else if( nCode >= 159000 && nCode <= 159999 ) {
			m_bCloseFile = false;
		}*/
	}

}

int TickGenerator::Initialize()
{
	int			nErrCode = 0;

	if( NULL == m_pDataCache )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::Initialize() : invalid tick buffer pointer 4 code : %s", m_pszCode );
		return -1;
	}

	m_objTickQueue.Release();
	if( (nErrCode = m_objTickQueue.Instance( (char*)m_pDataCache, s_nMaxLineCount ) ) < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::Initialize() : cannot initialize tick queue 4 code : %s", m_pszCode );
		return -2;
	}

	return 0;
}

TickGenerator& TickGenerator::operator=( const TickGenerator& obj )
{
	m_eMarket = obj.m_eMarket;
	m_nDate = obj.m_nDate;
	::strcpy( m_pszCode, obj.m_pszCode );
	::memcpy( m_pszPreName, obj.m_pszPreName, 4 );
	m_dPriceRate = obj.m_dPriceRate;
	m_nPreClosePx = obj.m_nPreClosePx;
	m_nOpenPx = obj.m_nOpenPx;

	m_pDataCache = obj.m_pDataCache;
	m_pDumpFile = obj.m_pDumpFile;
	m_bHasTitle = obj.m_bHasTitle;
	m_bCloseFile = obj.m_bCloseFile;

	return *this;
}

void TickGenerator::SetPreName( const char* pszPreName, unsigned int nPreNameLen )
{
	if( NULL != pszPreName && nPreNameLen > 0 ) {
		::memcpy( m_pszPreName, pszPreName, 4 );
	}
}

double TickGenerator::GetPriceRate()
{
	return m_dPriceRate;
}

int TickGenerator::Update( T_DATA& objData, unsigned int nPreClosePx, unsigned int nOpenPx )
{
	if( NULL == m_pDataCache )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::Update() : invalid buffer pointer" );
		return -1;
	}

	if( 0 == objData.Time )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::Update() : invalid snap time" );
		return -2;
	}

	m_nMkTime = objData.Time / 1000;
	m_nOpenPx = nOpenPx;
	m_nPreClosePx = nPreClosePx;
	if( m_objTickQueue.PutData( &objData ) < 0 )
	{
		ServerStatus::GetStatusObj().AddTickLostRef();
		return -1;
	}

	return 0;
}

void TickGenerator::DumpTicks()
{
	if( NULL == m_pDataCache ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::DumpTicks() : invalid buffer pointer 4 code:%s", m_pszCode );
		return;
	}

	unsigned int	nPercentage = m_objTickQueue.GetPercent();

	if( XDF_SH == m_eMarket ) {
		T_SECURITY_STATUS&	refSHL1Snap = ServerStatus::GetStatusObj().FetchSecurity( XDF_SH );
		if( ::strncmp( m_pszCode, refSHL1Snap.Code, 6 ) == 0 ) {
			ServerStatus::GetStatusObj().UpdateTickBufShL1OccupancyRate( nPercentage );			///< TICK缓存占用率
		}
	} else if( XDF_SZ == m_eMarket ) {
		T_SECURITY_STATUS&	refSZL1Snap = ServerStatus::GetStatusObj().FetchSecurity( XDF_SZ );
		if( ::strncmp( m_pszCode, refSZL1Snap.Code, 6 ) == 0 ) {
			ServerStatus::GetStatusObj().UpdateTickBufSzL1OccupancyRate( nPercentage );			///< TICK缓存占用率
		}
	}

	///< 对需要进行closefile的低交易频率的商品，在缓存使用超过30%时,才进行落盘  (注，午盘休息时间段除外)
	if( true == m_bCloseFile && nPercentage < 20 && ( (m_nMkTime>=93000&&m_nMkTime<113010)|| (m_nMkTime>125010&&m_nMkTime<=150000) ) ) {
		return;
	}

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		TickGenerator::T_DATA	tagData = { 0 };
		T_TICK_LINE				tagTickData = { 0 };
		char					pszLine[512] = { 0 };
		char					pszPreClose[32] = { 0 };
		char					pszOpen[32] = { 0 };
		char					pszHigh[32] = { 0 };
		char					pszLow[32] = { 0 };
		char					pszClose[32] = { 0 };
		char					pszNow[32] = { 0 };
		char					pszAmount[32] = { 0 };
		char					pszVoip[32] = { 0 };
		char					pszUpper[32] = { 0 };
		char					pszLower[32] = { 0 };
		char					pszBidPx1[32] = { 0 };
		char					pszBidPx2[32] = { 0 };
		char					pszAskPx1[32] = { 0 };
		char					pszAskPx2[32] = { 0 };

		if( m_objTickQueue.GetData( &tagData ) <= 0 )	{
			break;
		}

		///< 准备好需要写入的文件句柄
		if( NULL == m_pDumpFile ) {
			m_pDumpFile = PrepareTickFile( m_eMarket, m_pszCode, m_nDate, m_bHasTitle );
			if( NULL == m_pDumpFile )
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "TickGenerator::DumpTicks() : cannot open file 4 security:%s", m_pszCode );
				return;
			}
		} 

		tagTickData.Date = m_nDate;
		tagTickData.Time = tagData.Time;
		tagTickData.PreClosePx = m_nPreClosePx / m_dPriceRate;
		tagTickData.OpenPx = m_nOpenPx / m_dPriceRate;
		tagTickData.HighPx = tagData.HighPx / m_dPriceRate;
		tagTickData.LowPx = tagData.LowPx / m_dPriceRate;
		tagTickData.ClosePx = tagData.ClosePx / m_dPriceRate;
		tagTickData.NowPx = tagData.NowPx / m_dPriceRate;
		tagTickData.UpperPx = tagData.UpperPx / m_dPriceRate;
		tagTickData.LowerPx = tagData.LowerPx / m_dPriceRate;
		tagTickData.Amount = tagData.Amount;
		tagTickData.Volume = tagData.Volume;
		tagTickData.NumTrades = tagData.NumTrades;
		tagTickData.BidPx1 = tagData.BidPx1 / m_dPriceRate;
		tagTickData.BidVol1 = tagData.BidVol1;
		tagTickData.BidPx2 = tagData.BidPx2 / m_dPriceRate;
		tagTickData.BidVol2 = tagData.BidVol2;
		tagTickData.AskPx1 = tagData.AskPx1 / m_dPriceRate;
		tagTickData.AskVol1 = tagData.AskVol1;
		tagTickData.AskPx2 = tagData.AskPx2 / m_dPriceRate;
		tagTickData.AskVol2 = tagData.AskVol2;
		tagTickData.Voip = tagData.Voip / m_dPriceRate;
		FormatDouble2Str( tagTickData.PreClosePx, pszPreClose, sizeof(pszPreClose), 4, false );
		FormatDouble2Str( tagTickData.OpenPx, pszOpen, sizeof(pszOpen), 4, false );
		FormatDouble2Str( tagTickData.HighPx, pszHigh, sizeof(pszHigh), 4, false );
		FormatDouble2Str( tagTickData.LowPx, pszLow, sizeof(pszLow), 4, false );
		FormatDouble2Str( tagTickData.ClosePx, pszClose, sizeof(pszClose), 4, false );
		FormatDouble2Str( tagTickData.NowPx, pszNow, sizeof(pszNow), 4, false );
		FormatDouble2Str( tagTickData.UpperPx, pszUpper, sizeof(pszUpper), 4, false );
		FormatDouble2Str( tagTickData.LowerPx, pszLower, sizeof(pszLower), 4, false );
		FormatDouble2Str( tagTickData.Amount, pszAmount, sizeof(pszAmount), 4, false );
		FormatDouble2Str( tagTickData.BidPx1, pszBidPx1, sizeof(pszBidPx1), 4, false );
		FormatDouble2Str( tagTickData.BidPx2, pszBidPx2, sizeof(pszBidPx2), 4, false );
		FormatDouble2Str( tagTickData.AskPx1, pszAskPx1, sizeof(pszAskPx1), 4, false );
		FormatDouble2Str( tagTickData.AskPx2, pszAskPx2, sizeof(pszAskPx2), 4, false );
		FormatDouble2Str( tagTickData.Voip, pszVoip, sizeof(pszVoip), 4, false );

		int		nLen = ::sprintf( pszLine, "%u,%u,%s,,%s,%s,%s,%s,%s,,%s,%s,%s,%I64d,,%I64d,%s,%I64d,%s,%I64d,%s,%I64d,%s,%I64d,%s,,%s\n"
			, tagTickData.Date, tagTickData.Time, pszPreClose, pszOpen, pszHigh, pszLow, pszClose, pszNow
			, pszUpper, pszLower, pszAmount, tagTickData.Volume, tagTickData.NumTrades
			, pszBidPx1, tagTickData.BidVol1, pszBidPx2, tagTickData.BidVol2, pszAskPx1, tagTickData.AskVol1, pszAskPx2, tagTickData.AskVol2
			, pszVoip, m_pszPreName );

		::fwrite( pszLine, sizeof(char), nLen, m_pDumpFile );
	}

	if( true == m_bCloseFile || DateTime::Now().TimeToLong() > 160000 ) {
		if( NULL != m_pDumpFile ) {
			::fclose( m_pDumpFile );
			m_pDumpFile = NULL;
		}
	}
}

SecurityTickCache::SecurityTickCache()
 : m_nSecurityCount( 0 ), m_pTickDataTable( NULL ), m_nAlloPos( 0 ), m_eMarketID( XDF_SZOPT )
{
	m_vctCode.clear();
	m_objMapTicks.clear();
}

SecurityTickCache::~SecurityTickCache()
{
}

int SecurityTickCache::Initialize( unsigned int nSecurityCount )
{
	unsigned int	nTotalCount = (1+nSecurityCount) * TickGenerator::s_nMaxLineCount;

	Release();
	if( NULL == (m_pTickDataTable = new TickGenerator::T_DATA[nTotalCount]) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::Initialize() : cannot allocate minutes buffer ( count:%u )", nTotalCount );
		return -1;
	}

	m_nAlloPos = 0;
	m_nSecurityCount = nTotalCount;
	::memset( m_pTickDataTable, 0, sizeof(TickGenerator::T_DATA) * nTotalCount );
	m_vctCode.reserve( nSecurityCount + 32 );

	return 0;
}

void SecurityTickCache::Release()
{
	if( NULL != m_pTickDataTable )
	{
		m_oDumpThread.StopThread();
		m_oDumpThread.Join( 6000 );

		CriticalLock			section( m_oLockData );
		delete [] m_pTickDataTable;
		m_pTickDataTable = NULL;
		m_objMapTicks.clear();
		m_vctCode.clear();
	}

	m_nAlloPos = 0;
	m_nSecurityCount = 0;
}

void SecurityTickCache::ActivateDumper()
{
	if( false == m_oDumpThread.IsAlive() )
	{
		if( 0 != m_oDumpThread.Create( "SecurityTickCache::ActivateDumper()", DumpThread, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::ActivateDumper() : failed 2 create ticks line thread(1)" );
		}
	}
}

int SecurityTickCache::NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen )
{
	CriticalLock			section( m_oLockData );

	m_eMarketID = eMarket;
	if( (m_nAlloPos+1) >= m_nSecurityCount )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::NewSecurity() : cannot grap any section from buffer ( %u:%u )", m_nAlloPos, m_nSecurityCount );
		return -1;
	}

	if( NULL == m_pTickDataTable )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::NewSecurity() : invalid buffer pointer" );
		return -2;
	}

	m_objMapTicks[sCode] = TickGenerator( eMarket, nDate, sCode, dPriceRate, objData, m_pTickDataTable + m_nAlloPos );
	if( 0 != m_objMapTicks[sCode].Initialize() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::NewSecurity() : cannot initialize tick generator class 4 code : %s", sCode.c_str() );
		return -3;
	}

	m_nAlloPos += TickGenerator::s_nMaxLineCount;
	m_vctCode.push_back( sCode );

	if( NULL != pszPreName && nPreNamLen > 0 ) {
		m_objMapTicks[sCode].SetPreName( pszPreName, nPreNamLen );
	}

	return 0;
}

double SecurityTickCache::GetPriceRate( std::string& sCode )
{
	CriticalLock				section( m_oLockData );
	T_MAP_TICKS::iterator		it = m_objMapTicks.find( sCode );

	if( it != m_objMapTicks.end() )
	{
		return it->second.GetPriceRate();
	}

	return 1.0;
}

int SecurityTickCache::UpdatePreName( std::string& sCode, char* pszPreName, unsigned int nSize )
{
	CriticalLock				section( m_oLockData );
	T_MAP_TICKS::iterator		it = m_objMapTicks.find( sCode );

	if( it != m_objMapTicks.end() )
	{
		it->second.SetPreName( pszPreName, nSize );

		return 0;
	}

	return -1;
}

int SecurityTickCache::UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime )
{
	std::string					sCode( refObj.Code, 6 );
	CriticalLock				section( m_oLockData );
	T_MAP_TICKS::iterator		it = m_objMapTicks.find( sCode );

	if( it != m_objMapTicks.end() )
	{
		TickGenerator::T_DATA	objData = { 0 };

		objData.Time = nTime;
		objData.HighPx = refObj.High;
		objData.LowPx = refObj.Low;
		objData.NowPx = refObj.Now;
		objData.ClosePx = refObj.Now;
		objData.Amount = refObj.Amount;
		objData.Volume = refObj.Volume;

		return it->second.Update( objData, refObj.PreClose, refObj.Open );
	}

	return -1;
}

int SecurityTickCache::UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime )
{
	std::string					sCode( refObj.Code, 6 );
	CriticalLock				section( m_oLockData );
	T_MAP_TICKS::iterator		it = m_objMapTicks.find( sCode );

	if( it != m_objMapTicks.end() )
	{
		TickGenerator::T_DATA		objData = { 0 };

		objData.Time = nTime;
		objData.HighPx = refObj.High;
		objData.LowPx = refObj.Low;
		objData.NowPx = refObj.Now;
		objData.ClosePx = refObj.Now;
		objData.Amount = refObj.Amount;
		objData.Volume = refObj.Volume;
		objData.NumTrades = refObj.Records;
		objData.UpperPx = refObj.HighLimit;
		objData.LowerPx = refObj.LowLimit;
		objData.Voip = refObj.Voip;
		objData.BidPx1 = refObj.Buy[0].Price;
		objData.BidVol1 = refObj.Buy[0].Volume;
		objData.BidPx2 = refObj.Buy[1].Price;
		objData.BidVol2 = refObj.Buy[1].Volume;
		objData.AskPx1 = refObj.Sell[0].Price;
		objData.AskVol1 = refObj.Sell[0].Volume;
		objData.AskPx2 = refObj.Sell[1].Price;
		objData.AskVol2 = refObj.Sell[1].Volume;

		return it->second.Update( objData, refObj.PreClose, refObj.Open );
	}

	return -1;
}

void* SecurityTickCache::DumpThread( void* pSelf )
{
	SecurityTickCache&		refData = *(SecurityTickCache*)pSelf;
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SecurityTickCache::DumpThread() : MarketID = %d, enter...................", (int)(refData.m_eMarketID) );

	while( true == refData.m_oDumpThread.IsAlive() )
	{
		SimpleThread::Sleep( 1000 * 3 );

		try
		{
			unsigned int	nCodeNumber = refData.m_vctCode.size();

			for( unsigned int n = 0; n < nCodeNumber && true == refData.m_oDumpThread.IsAlive(); n++ )
			{
				T_MAP_TICKS::iterator	it;

				{
					CriticalLock			section( refData.m_oLockData );
					it = refData.m_objMapTicks.find( refData.m_vctCode[n] );
					if( it == refData.m_objMapTicks.end() ) {
						continue;
					}
				}

				it->second.DumpTicks();
			}
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::DumpThread() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityTickCache::DumpThread() : unknow exception" );
		}
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SecurityTickCache::DumpThread() : MarketID = %d, misson complete!............", (int)(refData.m_eMarketID) );

	return NULL;
}




