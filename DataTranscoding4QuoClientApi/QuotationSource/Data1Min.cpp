#include "Data1Min.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "Base.h"
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


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

int MinGenerator::Initialize()
{
	m_nDataSize = 0;
	m_nWriteSize = -1;
	m_nBeginOffset = 1024;
	m_nPreClose = -1;

	return 0;
}

int MinGenerator::Update( T_DATA& objData, int nPreClose )
{
	if( NULL == m_pDataCache )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : invalid buffer pointer" );
		return -1;
	}
	if( 0 == objData.Time )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : invalid snap time" );
		return -2;
	}

	///< 判断是否需要打"收盘标记"
	unsigned int		nMKTime = objData.Time / 1000;
	unsigned int		nHH = nMKTime / 10000;
	unsigned int		nMM = nMKTime % 10000 / 100;
	int					nDataIndex = -1;

	if( m_nPreClose != nPreClose && nPreClose > 0 ) {
		m_nPreClose = nPreClose;					///< 更新昨收
	}

	///< 用当前市场时间，映射出对应的一分钟线的索引值
	if( nMKTime < 93000 ) {
		m_dAmountBefore930 = objData.Amount;		///< 9:30前的金额
		m_nVolumeBefore930 = objData.Volume;		///< 9:30前的量
		m_nNumTradesBefore930 = objData.NumTrades;	///< 9:30前的笔数
		m_nBeginOffset = 0;							///< 开始数据落盘的起始位置(说明盘前启动，且中间没有中断过)
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
	} else if( nMKTime > 130000 && nMKTime <= 151500 ) {
		nDataIndex = 120;							///< 上午共121根
		if( 13 == nHH ) {
			nDataIndex += nMM;						///< 13:01~13:59 = 59根
		} else if( 14 == nHH ) {
			nDataIndex += (60 + nMM);				///< 14:00~14:59 = 60根
		} else if( 15 == nHH ) {
			nDataIndex += (120-1);					///< 15:00~15:00 = 1根
		}
	}

	if( nDataIndex < 0 ) {
		return 0;
	}
	if( nDataIndex >= 241 ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::Update() : out of range" );
		return -4;
	}

	if( m_nBeginOffset > 1000 ) {					///< 若盘中还是初始化大值(非盘前启动)，直接跳到第1个快照的写入位置
		m_nBeginOffset = nDataIndex;
	}

	///< 除最高/低价以外，其他原样更新到对应的每分钟数据中
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
	} else {
		pData->Time = objData.Time;
		if( objData.ClosePx > pData->HighPx ) {
			pData->HighPx = objData.ClosePx;
		}
		if( objData.ClosePx < pData->LowPx ) {
			pData->LowPx = objData.ClosePx;
		}
	}

	if( nDataIndex > m_nDataSize ) {
		m_nDataSize = nDataIndex;
	}

	return 0;
}

void MinGenerator::DumpMinutes( bool bMarketClosed )
{
	if( NULL == m_pDataCache ) {
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::DumpMinutes() : invalid buffer pointer 4 code:%s", m_pszCode );
		return;
	}

	std::ofstream			oDumper;								///< 文件句柄
	unsigned int			nLastLineIndex = 0;						///< 上一笔快照是索引值
	T_MIN_LINE				tagLastLine = { 0 };					///< 上一笔快照的最后情况
	T_MIN_LINE				tagLastLastLine = { 0 };				///< 上上笔快照的最后情况

	/////////////////////< 准备好需要写入的文件句柄
	if( false == PrepareMinuteFile( m_eMarket, m_pszCode, m_nDate, oDumper ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MinGenerator::DumpMinutes() : cannot open file 4 security:%s", m_pszCode );
		return;
	}

	/////////////////////< 收盘准备： a) 全天无行情: 将全天的价格都赋为昨收 b)有情行：将最后更新数据位置赋值成241
	if( true == bMarketClosed ) {
		///< 一天内无快照的情况
		if( 0 >= m_nBeginOffset ) {
			double			dPreClosePx = m_nPreClose / m_dPriceRate;	///< 昨收价

			m_nBeginOffset = 0;		///< 起始位置为0
			///< 将一天无行情的商品，的241条1分钟线全用昨收价填充
			tagLastLine.OpenPx = dPreClosePx;
			tagLastLine.HighPx = dPreClosePx;
			tagLastLine.LowPx = dPreClosePx;
			tagLastLine.ClosePx = dPreClosePx;
			m_pDataCache[0].OpenPx = m_nPreClose;
			m_pDataCache[0].HighPx = m_nPreClose;
			m_pDataCache[0].LowPx = m_nPreClose;
			m_pDataCache[0].ClosePx = m_nPreClose;
		}

		m_nDataSize = m_nMaxLineCount;								///< 将最后更新数据位置赋值成241
	}

	//////////////////////< 计算并写分钟线：从头遍历，直到最后一个收到的时间节点上
	for( int i = m_nBeginOffset; i < m_nDataSize; i++ )	{
		///< 跳过已经落盘过的时间节点，以m_pDataCache[i].Time大于零为标识，进行"后续写入"
		if( i > m_nWriteSize ) {
			char			pszLine[1024] = { 0 };
			T_MIN_LINE		tagMinuteLine = { 0 };
			char			pszOpen[32] = { 0 };
			char			pszHigh[32] = { 0 };
			char			pszLow[32] = { 0 };
			char			pszClose[32] = { 0 };
			char			pszAmount[32] = { 0 };
			char			pszVoip[32] = { 0 };
			///< 计算每个分钟线的对应时间(单位：分)
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
				tagMinuteLine.Time = 150000;						///< 15:00~15:00 = 1根-1 (i:240-1)
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
				FormatDouble2Str( tagMinuteLine.OpenPx, pszOpen, sizeof(pszOpen), 4, false );
				FormatDouble2Str( tagMinuteLine.HighPx, pszHigh, sizeof(pszHigh), 4, false );
				FormatDouble2Str( tagMinuteLine.LowPx, pszLow, sizeof(pszLow), 4, false );
				FormatDouble2Str( tagMinuteLine.ClosePx, pszClose, sizeof(pszClose), 4, false );
				FormatDouble2Str( tagMinuteLine.Amount, pszAmount, sizeof(pszAmount), 4, false );
				FormatDouble2Str( tagMinuteLine.Voip, pszVoip, sizeof(pszVoip), 4, false );

				int		nLen = ::sprintf( pszLine, "%d,%d,%s,%s,%s,%s,,%s,%I64d,,%I64d,%s\n"
										, tagMinuteLine.Date, tagMinuteLine.Time, pszOpen, pszHigh, pszLow, pszClose
										, pszAmount, tagMinuteLine.Volume, tagMinuteLine.NumTrades, pszVoip );
				oDumper.write( pszLine, nLen );

				m_nWriteSize = i;										///< 更新最新的写盘数据位置(避免同1分钟的重复落盘)
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

				FormatDouble2Str( tagMinuteLine.OpenPx, pszOpen, sizeof(pszOpen), 4, false );
				FormatDouble2Str( tagMinuteLine.HighPx, pszHigh, sizeof(pszHigh), 4, false );
				FormatDouble2Str( tagMinuteLine.LowPx, pszLow, sizeof(pszLow), 4, false );
				FormatDouble2Str( tagMinuteLine.ClosePx, pszClose, sizeof(pszClose), 4, false );
				FormatDouble2Str( tagMinuteLine.Amount, pszAmount, sizeof(pszAmount), 4, false );
				FormatDouble2Str( tagMinuteLine.Voip, pszVoip, sizeof(pszVoip), 4, false );

				int		nLen = ::sprintf( pszLine, "%d,%d,%s,%s,%s,%s,,%s,%I64d,,%I64d,%s\n"
										, tagMinuteLine.Date, tagMinuteLine.Time, pszOpen, pszHigh, pszLow, pszClose
										, pszAmount, tagMinuteLine.Volume, tagMinuteLine.NumTrades, pszVoip );
				oDumper.write( pszLine, nLen );
				m_pDataCache[i-1].Time = 0;						///< 把时间清零，即，标记为已经落盘

				m_nWriteSize = i;									///< 更新最新的写盘数据位置(避免同1分钟的重复落盘)
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

	//////////////////< 关闭落盘文件句柄
	if( oDumper.is_open() ) {
		oDumper.close();
	}
}

SecurityMinCache::SecurityMinCache()
 : m_nSecurityCount( 0 ), m_pMinDataTable( NULL ), m_nAlloPos( 0 ), m_eMarketID( XDF_SZOPT )
{
	m_vctCode.clear();
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
	m_vctCode.reserve( nSecurityCount + 32 );

	return 0;
}

void SecurityMinCache::Release()
{
	if( NULL != m_pMinDataTable )
	{
		m_oDumpThread.StopThread();
		m_oDumpThread.Join( 6000 );

		CriticalLock			section( m_oLockData );

		delete [] m_pMinDataTable;
		m_pMinDataTable = NULL;
		m_objMapMinutes.clear();
		m_vctCode.clear();
	}

	m_nAlloPos = 0;
	m_nSecurityCount = 0;
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

	::memset( m_pMinDataTable + m_nAlloPos, 0, sizeof(MinGenerator::T_DATA) * 241 );
	m_objMapMinutes[sCode] = MinGenerator( eMarket, nDate, sCode, dPriceRate, objData, m_pMinDataTable + m_nAlloPos );
	if( 0 != m_objMapMinutes[sCode].Initialize() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "SecurityMinCache::NewSecurity() : cannot initialize tick generator class 4 code : %s", sCode.c_str() );
		return -3;
	}

	m_nAlloPos += 241;
	m_vctCode.push_back( sCode );

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

		return it->second.Update( objData, refObj.PreClose );
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

		return it->second.Update( objData, refObj.PreClose );
	}

	return -1;
}

void* SecurityMinCache::DumpThread( void* pSelf )
{
	bool					bCloseMarket = false;					///< true:需要做次全部1分钟线的落盘 false:不需要
	SecurityMinCache&		refData = *(SecurityMinCache*)pSelf;

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SecurityMinCache::DumpThread() : MarketID = %d, enter...................", (int)(refData.m_eMarketID) );
	while( true == refData.m_oDumpThread.IsAlive() )
	{
		SimpleThread::Sleep( 1000 * 3 );

		try
		{
			unsigned int	nCodeNumber = refData.m_vctCode.size();
			unsigned int	nNowTime = DateTime::Now().TimeToLong();

			if( nNowTime > 153010 && nNowTime < 155010 ) {
				bCloseMarket = true;				///< 如果有商品的市场时间为15:00，则标记为需要集体生成分钟线
			} else {
				bCloseMarket = false;				///< 标记为不在闭市阶段，不需要落余下的全部1分钟线
			}

			for( unsigned int n = 0; n < nCodeNumber && true == refData.m_oDumpThread.IsAlive(); n++ )
			{
				T_MAP_MINUTES::iterator	it;

				{
					CriticalLock			section( refData.m_oLockData );
					it = refData.m_objMapMinutes.find( refData.m_vctCode[n] );
					if( it == refData.m_objMapMinutes.end() ) {
						continue;
					}
				}

				it->second.DumpMinutes( bCloseMarket );
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

