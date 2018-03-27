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
	::memset( m_pDataCache, 0, m_nMaxLineCount );
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
		if( nDataIndex >= (m_nMaxLineCount-1) && false == s_bCloseMarket ) {
			s_bCloseMarket = true;		///< 如果有商品的市场时间为15:00，则标记为需要集体生成分钟线
		}
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

	if( true == s_bCloseMarket ) {						///< 收市，需要生成所有分钟线
		m_nDataSize = m_nMaxLineCount;
	}

	///< 从头遍历，直到最后一个收到的时间节点上
	for( int i = 0; i < m_nDataSize; i++ )
	{
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

SecurityCache::SecurityCache()
 : m_nSecurityCount( 0 ), m_pMinDataTable( NULL )
{
}

SecurityCache::~SecurityCache()
{
	Release();
}

int SecurityCache::Initialize( unsigned int nSecurityCount )
{
	unsigned int	nTotalCount = (1+nSecurityCount) * 241;

	Release();
	if( NULL == (m_pMinDataTable = new MinGenerator::T_DATA[nTotalCount]) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityCache::Initialize() : cannot allocate minutes buffer ( count:%u )", nTotalCount );
		return -1;
	}

	m_nAlloPos = 0;
	m_nSecurityCount = nTotalCount;
	::memset( m_pMinDataTable, 0, sizeof(MinGenerator::T_DATA) * nTotalCount );

	return 0;
}

void SecurityCache::Release()
{
	if( NULL != m_pMinDataTable )
	{
		delete [] m_pMinDataTable;
		m_pMinDataTable = NULL;
		m_objMapMinutes.clear();
		m_oDumpThread.StopThread();
		m_oDumpThread.Join( 5000 );
	}

	m_nAlloPos = 0;
	m_nSecurityCount = 0;
}

void SecurityCache::ActivateDumper()
{
	if( false == m_oDumpThread.IsAlive() )
	{
		if( 0 != m_oDumpThread.Create( "SecurityCache::ActivateDumper()", DumpThread, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityCache::ActivateDumper() : failed 2 create minute line thread(1)" );
		}
	}
}

int SecurityCache::NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData )
{
	CriticalLock			section( m_oLockData );

	if( (m_nAlloPos+1) >= m_nSecurityCount )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityCache::NewSecurity() : cannot grap any section from buffer ( %u:%u )", m_nAlloPos, m_nSecurityCount );
		return -1;
	}

	if( NULL == m_pMinDataTable )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityCache::NewSecurity() : invalid buffer pointer" );
		return -2;
	}

	m_objMapMinutes[sCode] = MinGenerator( eMarket, nDate, sCode, dPriceRate, objData, m_pMinDataTable + m_nAlloPos );
	m_nAlloPos += 241;

	return 0;
}

int SecurityCache::UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime )
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

int SecurityCache::UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime )
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

void* SecurityCache::DumpThread( void* pSelf )
{
	SecurityCache&		refData = *(SecurityCache*)pSelf;
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpMinuteLine() : enter..................." );

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
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpMinuteLine() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpMinuteLine() : unknow exception" );
		}
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpMinuteLine() : misson complete!............" );

	return NULL;
}

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
static T_MINLINE_CACHE		s_arrayMinuteLine;


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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::Initialize() : enter ......................" );
	Release();
	m_pQuotation = pQuotation;
	m_nMaxMLineBufSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_MIN_LINE) * 15;
	m_pBuf4MinuteLine = new char[m_nMaxMLineBufSize];
	if( NULL == m_pBuf4MinuteLine )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 allocate buffer 4 minutes lines" );
		return -1;
	}

	::memset( m_pBuf4MinuteLine, 0, m_nMaxMLineBufSize );
	nErrorCode = s_arrayMinuteLine.Instance( m_pBuf4MinuteLine, m_nMaxMLineBufSize/sizeof(T_MIN_LINE) );
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
	m_oThdTickDump.Join( 5000 );
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

	::memset( m_lstMkTime, 0, sizeof(m_lstMkTime) );
}

SecurityCache& QuotationData::GetSHL1Cache()
{
	return m_objMinCache4SHL1;
}

SecurityCache& QuotationData::GetSZL1Cache()
{
	return m_objMinCache4SZL1;
}

T_MINLINE_CACHE& QuotationData::GetMinuteCacheObj()
{
	return s_arrayMinuteLine;
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
	T_TICKLINE_CACHE&	refTickBuf = refData.m_arrayTickLine;

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000 * 2 );

			refQuotation.FlushDayLineOnCloseTime();									///< 检查是否需要落日线
			refQuotation.UpdateMarketsTime();										///< 更新各市场的日期和时间
			refStatus.UpdateTickBufOccupancyRate( refTickBuf.GetPercent() );		///< TICK缓存占用率
			refStatus.UpdateMinuteBufOccupancyRate( s_arrayMinuteLine.GetPercent() );		///< MinuteLine缓存占用率
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

			while( false == SimpleThread::GetGlobalStopFlag() )
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpTickLine() : misson complete!" );

	return NULL;
}

int QuotationData::UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize )
{
	switch( eMarket )
	{
	case XDF_SZ:	///< 深证L1
		{
			T_MAP_GEN_MLINES::iterator it = m_mapSZL1.find( sCode );
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

int QuotationData::BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam )
{
	int					nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			CriticalLock					section( m_oLockSHL1 );
			T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( sCode );
			if( it == m_mapSHL1.end() )
			{
				m_mapSHL1[sCode] = refParam;
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			CriticalLock					section( m_oLockSZL1 );
			T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				m_mapSZL1[sCode] = refParam;
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
					CriticalLock					section( m_oLockSHL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%u,%u,%f\n", nMachineDate
									, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
									, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/refParam.dPriceRate );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					CriticalLock					section( m_oLockSHL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,0.0000,%.4f,%I64d,0,0,0.0000\n"
									, nMachineDate
									, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
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
					CriticalLock					section( m_oLockSZL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%u,%u,%.4f\n", nMachineDate
									, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
									, 0.0, pStock->Amount, pStock->Volume, 0, pStock->Records, pStock->Voip/refParam.dPriceRate );
								oDumper.write( pszDayLine, nLen );
							}
						}
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					CriticalLock					section( m_oLockSZL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;

						if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
						{
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // 未在日线文件中查找出今天的日线数据，所以需要写入一条
							{
								int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,0.0000,%.4f,%I64d,0,0,0.0000\n"
									, nMachineDate
									, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
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
	int				nErrorCode = 0;
	T_TICK_LINE		refTickLine = { 0 };

	refTickLine.eMarketID = eMarket;
	refTickLine.Time = GetMarketTime( eMarket ) * 1000;
	if( 0 == refTickLine.Time )
	{
		refTickLine.Time = DateTime::Now().TimeToLong() * 1000;
	}
	if( 0 == nTradeDate )
	{
		refTickLine.Date = DateTime::Now().DateToLong();
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
					CriticalLock					section( m_oLockSHL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						refTickLine.UpperPx = pStock->HighLimit / refParam.dPriceRate;
						refTickLine.LowerPx = pStock->LowLimit / refParam.dPriceRate;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
///<if( strncmp( pStock->Code, "600000", 6 ) == 0 )::printf( "Snap, 600000, Time=%u, Open=%u, Hight=%u, Low=%u, %u, %I64d\n", refTickLine.Time/1000, pStock->Open, pStock->High, pStock->Low, pStock->Now, pStock->Volume );
						m_objMinCache4SHL1.UpdateSecurity( *pStock, refTickLine.Date, refTickLine.Time );
				}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					CriticalLock					section( m_oLockSHL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						m_objMinCache4SHL1.UpdateSecurity( *pStock, refTickLine.Date, refTickLine.Time );
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
					CriticalLock					section( m_oLockSZL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						refTickLine.UpperPx = pStock->HighLimit / refParam.dPriceRate;
						refTickLine.LowerPx = pStock->LowLimit / refParam.dPriceRate;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
						::memcpy( refTickLine.PreName, refParam.PreName, 4 );
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						m_objMinCache4SZL1.UpdateSecurity( *pStock, refTickLine.Date, refTickLine.Time );
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*				pStock = (XDFAPI_IndexData*)pSnapData;
					CriticalLock					section( m_oLockSZL1 );
					T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.dPriceRate;
						refTickLine.OpenPx = pStock->Open / refParam.dPriceRate;
						refTickLine.HighPx = pStock->High / refParam.dPriceRate;
						refTickLine.LowPx = pStock->Low / refParam.dPriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.dPriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
						///< ------------ Minute Lines -----------------------
						m_objMinCache4SZL1.UpdateSecurity( *pStock, refTickLine.Date, refTickLine.Time );
					}
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

	ServerStatus::GetStatusObj().UpdateSecurity( (enum XDFMarket)(refTickLine.eMarketID), refTickLine.Code, refTickLine.NowPx, refTickLine.Amount, refTickLine.Volume );	///< 更新各市场的商品状态

	return 0;
}




