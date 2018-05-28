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
unsigned int	s_nNumberInSection = 50;				///< 一个市场有可以缓存多少个数据块
///< -----------------------------------------------------------------------------------


///< 1分钟线计算并落盘类 ///////////////////////////////////////////////////////////////////////////////////////////
__inline bool	PrepareMinuteFile( enum XDFMarket eMarket, const char* pszCode, unsigned int nDate, std::ofstream& oDumper )
{
	char	pszFilePath[512] = { 0 };

	switch( eMarket )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/MIN/%s/", pszCode );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/MIN/%s/", pszCode );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareMinuteFile() : invalid market id (%d)", (int)eMarket );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "MIN%s_%u.csv", pszCode, nDate/10000 );
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

MinGenerator::MinGenerator()
 : m_nDate( 0 ), m_nMaxLineCount( 241 ), m_pDataCache( NULL ), m_dPriceRate( 0 )
 , m_nWriteSize( -1 ), m_nDataSize( 0 ), m_dAmountBefore930( 0. ), m_nVolumeBefore930( 0 ), m_nNumTradesBefore930( 0 )
{
	::memset( m_pszCode, 0, sizeof(m_pszCode) );
}

MinGenerator::MinGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr )
: m_nMaxLineCount( 241 ), m_nWriteSize( -1 ), m_nDataSize( 0 ), m_dAmountBefore930( 0. ), m_nVolumeBefore930( 0 ), m_nNumTradesBefore930( 0 )
{
	m_eMarket = eMkID;
	m_nDate = nDate;
	::strcpy( m_pszCode, sCode.c_str() );
	m_dPriceRate = dPriceRate;
	m_pDataCache = pBufPtr;
	::memset( m_pDataCache, 0, sizeof(T_DATA) * m_nMaxLineCount );
}

MinGenerator& MinGenerator::operator=( const MinGenerator& obj )
{
	m_eMarket = obj.m_eMarket;
	m_nDate = obj.m_nDate;
	::strcpy( m_pszCode, obj.m_pszCode );

	m_dPriceRate = obj.m_dPriceRate;
	m_dAmountBefore930 = obj.m_dAmountBefore930;
	m_nVolumeBefore930 = obj.m_nVolumeBefore930;
	m_nNumTradesBefore930 = obj.m_nNumTradesBefore930;

	m_pDataCache = obj.m_pDataCache;
	m_nWriteSize = obj.m_nWriteSize;
	m_nDataSize = obj.m_nDataSize;

	return *this;
}

static bool s_bCloseMarket = false;

int MinGenerator::Update( T_DATA& objData )
{
	if( NULL == m_pDataCache )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : invalid buffer pointer" );
		return -1;
	}

	if( 0 == objData.Time )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : invalid snap time" );
		return -2;
	}

	unsigned int		nMKTime = objData.Time / 1000;
	unsigned int		nHH = nMKTime / 10000;
	unsigned int		nMM = nMKTime % 10000 / 100;
	int					nDataIndex = -1;

	if( nMKTime >= 150000 ) {
		s_bCloseMarket = true;		///< 如果有商品的市场时间为15:00，则标记为需要集体生成分钟线
	}

	if( nMKTime < 93000 ) {
		m_dAmountBefore930 = objData.Amount;		///< 9:30前的金额
		m_nVolumeBefore930 = objData.Volume;		///< 9:30前的量
		m_nNumTradesBefore930 = objData.NumTrades;	///< 9:30前的笔数
	} else if( nMKTime >= 93000 && nMKTime <= 130000 ) {
		nDataIndex = 0;								///< 需要包含9:30这根线
		if( 9 == nHH ) {
			nDataIndex += (nMM - 30);				///< 9:30~9:59 = 30根
		} else if( 10 == nHH ) {
			nDataIndex += (30 + nMM);				///< 10:00~10:59 = 60根
		} else if( 11 == nHH ) {
			nDataIndex += (90 + nMM);				///< 11:00~11:30 = 31根
		} else if( 13 == nHH ) {
			nDataIndex += 120;						///< 13:01(即13:00)的数据存放到11:30的位置
		}
	} else if( nMKTime > 130000 && nMKTime <= 150000 ) {
		nDataIndex = 120;							///< 上午共121根
		if( 13 == nHH ) {
			nDataIndex += nMM;						///< 13:01~13:59 = 59根
		} else if( 14 == nHH ) {
			nDataIndex += (60 + nMM);				///< 14:00~14:59 = 60根
		} else if( 15 == nHH ) {
			nDataIndex += 120;						///< 15:00~15:00 = 1根
		}
	}

	if( nDataIndex < 0 ) {
		return 0;
	}

	if( nDataIndex >= 241 ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : out of range" );
		return -4;
	}

	T_DATA*		pData = m_pDataCache + nDataIndex;

	pData->Amount = objData.Amount;
	pData->Volume = objData.Volume;
	pData->NumTrades = objData.NumTrades;
	pData->Voip = objData.Voip;
	pData->ClosePx = objData.ClosePx;
	if( pData->Time == 0 ) {
		pData->Time = objData.Time;
		pData->OpenPx = objData.ClosePx;
		pData->HighPx = objData.ClosePx;
		pData->LowPx = objData.ClosePx;
//if( ::strncmp( m_pszCode, "600000", 6 ) == 0 )::printf( "First, 600000, Time=%u, Open=%u, Hight=%u, Low=%u, %u, %I64d\n", pData->Time, pData->OpenPx, pData->HighPx, pData->LowPx, pData->ClosePx, pData->Volume );
	} else {
		pData->Time = objData.Time;
		if( objData.ClosePx > pData->HighPx ) {
			pData->HighPx = objData.ClosePx;
		}
		if( objData.ClosePx < pData->LowPx ) {
			pData->LowPx = objData.ClosePx;
		}
//if( ::strncmp( m_pszCode, "600000", 6 ) == 0 )::printf( "Then, 600000, Time=%u, Open=%u, Hight=%u, Low=%u, %u, %I64d\n", pData->Time, pData->OpenPx, pData->HighPx, pData->LowPx, pData->ClosePx, pData->Volume );
	}

	if( nDataIndex > m_nDataSize ) {
		m_nDataSize = nDataIndex;
	}

	return 0;
}

void MinGenerator::DumpMinutes()
{
	std::ofstream			oDumper;						///< 文件句柄
	unsigned int			nLastLineIndex = 0;				///< 上一笔快照是索引值
	T_MIN_LINE				tagLastLine = { 0 };			///< 上一笔快照的最后情况
	T_MIN_LINE				tagLastLastLine = { 0 };		///< 上上笔快照的最后情况
	///< 准备好需要写入的文件句柄
	if( false == PrepareMinuteFile( m_eMarket, m_pszCode, m_nDate, oDumper ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::DumpMinutes() : cannot open file 4 security:%s", m_pszCode );
		return;
	}

	if( true == s_bCloseMarket ) m_nDataSize = m_nMaxLineCount;
	///< 从头遍历，直到最后一个收到的时间节点上
	for( int i = 0; i < m_nDataSize; i++ )
	{
		///< 收市，需要生成所有分钟线
		if( true == s_bCloseMarket ) {
			m_nDataSize = m_nMaxLineCount;
		}
		///< 跳过已经落盘过的时间节点，以m_pDataCache[i].Time大于零为标识，进行"后续写入"
		if( i > m_nWriteSize ) {
			char			pszLine[1024] = { 0 };
			T_MIN_LINE		tagMinuteLine = { 0 };

			tagMinuteLine.Date = m_nDate;
			if( i == 0 ) {											///< [ 上午121根分钟线，下午120根 ]
				tagMinuteLine.Time = 93000;							///< 9:30~9:30 = 1根 (i:0)
			} else if( i >= 1 && i <= 29 ) {
				tagMinuteLine.Time = (930 + i) * 100;				///< 9:31~9:59 = 29根 (i:1--29)
			} else if( i >= 30 && i <= 89 ) {
				tagMinuteLine.Time = (1000 + (i-30)) * 100;			///< 10:00~10:59 = 60根 (i:30--89)
			} else if( i >= 90 && i <= 120 ) {
				tagMinuteLine.Time = (1100 + (i-90)) * 100;			///< 11:00~11:30 = 31根 (i:90--120)
			} else if( i > 120 && i <= 179 ) {
				tagMinuteLine.Time = (1300 + (i-120)) * 100;		///< 13:01~13:59 = 59根 (i:121--179)
			} else if( i > 179 && i <= 239 ) {
				tagMinuteLine.Time = (1400 + (i-180)) * 100;		///< 14:00~14:59 = 60根 (i:180--239)
			} else if( i == 240 ) {
				tagMinuteLine.Time = 150000;						///< 15:00~15:00 = 1根 (i:240)
			}

			if( 0 == i ) {	////////////////////////< 第一个节点是9:30，此时只需要将9:30分的第一个快照落盘
				tagMinuteLine.Amount = m_pDataCache[i].Amount;
				tagMinuteLine.Volume = m_pDataCache[i].Volume;
				tagMinuteLine.NumTrades = m_pDataCache[i].NumTrades;
				tagMinuteLine.OpenPx = m_pDataCache[i].OpenPx / m_dPriceRate;
				tagMinuteLine.HighPx = m_pDataCache[i].HighPx / m_dPriceRate;
				tagMinuteLine.LowPx = m_pDataCache[i].LowPx / m_dPriceRate;
				tagMinuteLine.ClosePx = m_pDataCache[i].ClosePx / m_dPriceRate;
				tagMinuteLine.Voip = m_pDataCache[i].Voip / m_dPriceRate;
				int		nLen = ::sprintf( pszLine, "%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%I64d,%.4f\n"
										, tagMinuteLine.Date, tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx
										, tagMinuteLine.SettlePx, tagMinuteLine.Amount, tagMinuteLine.Volume, tagMinuteLine.OpenInterest, tagMinuteLine.NumTrades, tagMinuteLine.Voip );
				oDumper.write( pszLine, nLen );
				m_pDataCache[i].Time = 0;								///< 把时间清零，即，标记为已经落盘
				m_nWriteSize = i;										///< 更新最新的写盘数据位置
//if( ::strncmp( m_pszCode, "600000", 6 ) == 0 )::printf( "[WRITE], 600000, Time=%u, Open=%f, High=%f, Low=%f, %f, %I64d\n", tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx, tagMinuteLine.Volume );
			} else {		////////////////////////< 处理9:30后的分钟线计算与落盘的情况 [1. 前面无成交的情况 2.前面是连续成交的情况]
				if( i - nLastLineIndex > 1 ) {	///< 如果前面n分钟内无成交，则开盘最高最低等于ClosePx
					tagMinuteLine.OpenPx = tagLastLine.ClosePx;
					tagMinuteLine.HighPx = tagLastLine.ClosePx;
					tagMinuteLine.LowPx = tagLastLine.ClosePx;
					tagMinuteLine.ClosePx = tagLastLine.ClosePx;
					tagMinuteLine.Voip = tagLastLine.Voip;
				} else {						///< 最近一分钟内有连续成交
					tagMinuteLine.OpenPx = tagLastLine.OpenPx;
					tagMinuteLine.HighPx = tagLastLine.HighPx;
					tagMinuteLine.LowPx = tagLastLine.LowPx;
					tagMinuteLine.ClosePx = tagLastLine.ClosePx;
					tagMinuteLine.Voip = tagLastLine.Voip;
					tagMinuteLine.Amount = tagLastLine.Amount - tagLastLastLine.Amount;
					tagMinuteLine.Volume = tagLastLine.Volume - tagLastLastLine.Volume;
					tagMinuteLine.NumTrades = tagLastLine.NumTrades - tagLastLastLine.NumTrades;
				}

				int		nLen = ::sprintf( pszLine, "%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%I64d,%.4f\n"
										, tagMinuteLine.Date, tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx
										, tagMinuteLine.SettlePx, tagMinuteLine.Amount, tagMinuteLine.Volume, tagMinuteLine.OpenInterest, tagMinuteLine.NumTrades, tagMinuteLine.Voip );
				oDumper.write( pszLine, nLen );
				m_nWriteSize = i;										///< 更新最新的写盘数据位置
//if( ::strncmp( m_pszCode, "600000", 6 ) == 0 )::printf( "[WRITE], 600000, Time=%u, Open=%f, High=%f, Low=%f, %f, %I64d\n", tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx, tagMinuteLine.Volume );
			}
		}

		///< 记录： 本次的金额，量等信息，供用于后一笔的差值计算
		if( m_pDataCache[i].Volume > 0 ) {
			if( i == 0 ) {	///< 将9:30前最后一记赋值，用于计算9:31那笔的差值
				tagLastLastLine.Amount = m_dAmountBefore930;
				tagLastLastLine.Volume = m_nVolumeBefore930;
				tagLastLastLine.NumTrades = m_nNumTradesBefore930;
			} else {		///< 用于计算9:31以后的分钟线差值
				::memcpy( &tagLastLastLine, &tagLastLine, sizeof tagLastLine );
			}
			///< 记录上一次快照的最后状态值
			nLastLineIndex = i;
			tagLastLine.Amount = m_pDataCache[i].Amount;
			tagLastLine.Volume = m_pDataCache[i].Volume;
			tagLastLine.NumTrades = m_pDataCache[i].NumTrades;
			tagLastLine.OpenPx = m_pDataCache[i].OpenPx / m_dPriceRate;
			tagLastLine.HighPx = m_pDataCache[i].HighPx / m_dPriceRate;
			tagLastLine.LowPx = m_pDataCache[i].LowPx / m_dPriceRate;
			tagLastLine.ClosePx = m_pDataCache[i].ClosePx / m_dPriceRate;
			tagLastLine.Voip = m_pDataCache[i].Voip / m_dPriceRate;
		}
	}

	if( oDumper.is_open() ) {
		oDumper.close();
	}
}

SecurityMinCache::SecurityMinCache()
 : m_nSecurityCount( 0 ), m_pMinDataTable( NULL ), m_nAlloPos( 0 ), m_eMarketID( XDF_SZOPT )
{
	m_objMapMinutes.clear();
}

SecurityMinCache::~SecurityMinCache()
{
}

int SecurityMinCache::Initialize( unsigned int nSecurityCount )
{
	unsigned int	nTotalCount = (1+nSecurityCount) * 241;

	Release();
	if( NULL == (m_pMinDataTable = new MinGenerator::T_DATA[nTotalCount]) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::Initialize() : cannot allocate minutes buffer ( count:%u )", nTotalCount );
		return -1;
	}

	m_nAlloPos = 0;
	m_nSecurityCount = nTotalCount;
	::memset( m_pMinDataTable, 0, sizeof(MinGenerator::T_DATA) * nTotalCount );

	return 0;
}

void SecurityMinCache::Release()
{
	if( NULL != m_pMinDataTable )
	{
		CriticalLock			section( m_oLockData );

		delete [] m_pMinDataTable;
		m_pMinDataTable = NULL;
		m_objMapMinutes.clear();
		m_oDumpThread.StopThread();
		m_oDumpThread.Join( 5000 );
	}

	m_nAlloPos = 0;
	m_nSecurityCount = 0;
	m_objMapMinutes.clear();
}

void SecurityMinCache::ActivateDumper()
{
	if( false == m_oDumpThread.IsAlive() )
	{
		if( 0 != m_oDumpThread.Create( "SecurityMinCache::ActivateDumper()", DumpThread, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::ActivateDumper() : failed 2 create minute line thread(1)" );
		}
	}
}

int SecurityMinCache::NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData )
{
	CriticalLock			section( m_oLockData );

	m_eMarketID = eMarket;
	if( (m_nAlloPos+1) >= m_nSecurityCount )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::NewSecurity() : cannot grap any section from buffer ( %u:%u )", m_nAlloPos, m_nSecurityCount );
		return -1;
	}

	if( NULL == m_pMinDataTable )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::NewSecurity() : invalid buffer pointer" );
		return -2;
	}

	m_objMapMinutes[sCode] = MinGenerator( eMarket, nDate, sCode, dPriceRate, objData, m_pMinDataTable + m_nAlloPos );
	m_nAlloPos += 241;

	return 0;
}

int SecurityMinCache::UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime )
{
	std::string					sCode( refObj.Code, 6 );
	CriticalLock				section( m_oLockData );
	T_MAP_MINUTES::iterator		it = m_objMapMinutes.find( sCode );

	if( it != m_objMapMinutes.end() )
	{
		MinGenerator::T_DATA	objData = { 0 };

		objData.Time = nTime;
		objData.OpenPx = refObj.Open;
		objData.HighPx = refObj.High;
		objData.LowPx = refObj.Low;
		objData.ClosePx = refObj.Now;
		objData.Amount = refObj.Amount;
		objData.Volume = refObj.Volume;

		return it->second.Update( objData );
	}

	return -1;
}

int SecurityMinCache::UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime )
{
	std::string					sCode( refObj.Code, 6 );
	CriticalLock				section( m_oLockData );
	T_MAP_MINUTES::iterator		it = m_objMapMinutes.find( sCode );

	if( it != m_objMapMinutes.end() )
	{
		MinGenerator::T_DATA		objData = { 0 };

		objData.Time = nTime;
		objData.OpenPx = refObj.Open;
		objData.HighPx = refObj.High;
		objData.LowPx = refObj.Low;
		objData.ClosePx = refObj.Now;
		objData.Amount = refObj.Amount;
		objData.Volume = refObj.Volume;
		objData.NumTrades = refObj.Records;
		objData.Voip = refObj.Voip;

		return it->second.Update( objData );
	}

	return -1;
}

void* SecurityMinCache::DumpThread( void* pSelf )
{
	SecurityMinCache&		refData = *(SecurityMinCache*)pSelf;
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SecurityMinCache::DumpThread() : MarketID = %d, enter...................", (int)(refData.m_eMarketID) );

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		SimpleThread::Sleep( 1000 * 60 * 3 );

		try
		{
			for( T_MAP_MINUTES::iterator it = refData.m_objMapMinutes.begin()
				; it != refData.m_objMapMinutes.end() && false == SimpleThread::GetGlobalStopFlag()
				; it++ )
			{
				it->second.DumpMinutes();
			}
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::DumpThread() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::DumpThread() : unknow exception" );
		}
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SecurityMinCache::DumpThread() : MarketID = %d, misson complete!............", (int)(refData.m_eMarketID) );

	return NULL;
}


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
		tagTickData.LowerPx = tagData.LowerPx / m_dPriceRate;
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

		int		nLen = ::sprintf( pszLine, "%u,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,,%s\n"
			, tagTickData.Date, tagTickData.Time, tagTickData.PreClosePx, tagTickData.PreSettlePx
			, tagTickData.OpenPx, tagTickData.HighPx, tagTickData.LowPx, tagTickData.ClosePx, tagTickData.NowPx, tagTickData.SettlePx
			, tagTickData.UpperPx, tagTickData.LowerPx, tagTickData.Amount, tagTickData.Volume, tagTickData.OpenInterest, tagTickData.NumTrades
			, tagTickData.BidPx1, tagTickData.BidVol1, tagTickData.BidPx2, tagTickData.BidVol2, tagTickData.AskPx1, tagTickData.AskVol1, tagTickData.AskPx2, tagTickData.AskVol2
			, tagTickData.Voip, m_pszPreName );

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
		CriticalLock			section( m_oLockData );

		delete [] m_pTickDataTable;
		m_pTickDataTable = NULL;
		m_objMapTicks.clear();
		m_vctCode.clear();
		m_oDumpThread.StopThread();
		m_oDumpThread.Join( 5000 );
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

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		SimpleThread::Sleep( 1000 * 3 );

		try
		{
			unsigned int	nCodeNumber = refData.m_vctCode.size();

			for( unsigned int n = 0; n < nCodeNumber && false == SimpleThread::GetGlobalStopFlag(); n++ )
			{
				T_MAP_TICKS::iterator	it = NULL;

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


///< 主数据管理类 ///////////////////////////////////////////////////////////////////////////////////////////
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
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::Initialize() : enter ......................" );
	int					nErrorCode = 0;

	Release();
	m_pQuotation = pQuotation;
	if( false == m_oThdIdle.IsAlive() )
	{
		if( 0 != m_oThdIdle.Create( "ThreadOnIdle()", ThreadOnIdle, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create onidle line thread" );
			return -6;
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

	m_mapModuleStatus.clear();
	::memset( m_lstMkTime, 0, sizeof(m_lstMkTime) );
}

SecurityMinCache& QuotationData::GetSHL1Cache()
{
	return m_objMinCache4SHL1;
}

SecurityMinCache& QuotationData::GetSZL1Cache()
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
		nErrorCode = m_objMinCache4SHL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData );
		break;
	case XDF_SZ:
		nErrorCode = m_objMinCache4SZL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData );
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
		nErrorCode = m_objTickCache4SHL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData, pszPreName, nPreNamLen );
		break;
	case XDF_SZ:	///< 深证L1
		nErrorCode = m_objTickCache4SZL1.NewSecurity( eMarket, sCode, nDate, dPriceRate, objData, pszPreName, nPreNamLen );
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
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%u,%u,%f\n", nMachineDate
									, pStock->Open/dPriceRate, pStock->High/dPriceRate, pStock->Low/dPriceRate, pStock->Now/dPriceRate
									, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/dPriceRate );
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
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,0.0000,%.4f,%I64d,0,0,0.0000\n"
									, nMachineDate
									, pStock->Open/dPriceRate, pStock->High/dPriceRate, pStock->Low/dPriceRate, pStock->Now/dPriceRate
									, pStock->Amount, pStock->Volume );
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
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%u,%u,%.4f\n", nMachineDate
									, pStock->Open/dPriceRate, pStock->High/dPriceRate, pStock->Low/dPriceRate, pStock->Now/dPriceRate
									, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/dPriceRate );
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
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,0.0000,%.4f,%I64d,0,0,0.0000\n"
									, nMachineDate
									, pStock->Open/dPriceRate, pStock->High/dPriceRate, pStock->Low/dPriceRate, pStock->Now/dPriceRate
									, pStock->Amount, pStock->Volume );
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




