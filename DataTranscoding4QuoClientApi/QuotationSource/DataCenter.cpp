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
const int		XDF_SH_COUNT = 16000;					///< �Ϻ�Lv1
const int		XDF_SHL2_COUNT = 0;						///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
const int		XDF_SHOPT_COUNT = 500;					///< �Ϻ���Ȩ
const int		XDF_SZ_COUNT = 8000;					///< ��֤Lv1
const int		XDF_SZL2_COUNT = 0;						///< ��֤Lv2(QuoteClientApi�ڲ�����)
const int		XDF_SZOPT_COUNT = 0;					///< ������Ȩ
const int		XDF_CF_COUNT = 500;						///< �н��ڻ�
const int		XDF_ZJOPT_COUNT = 500;					///< �н���Ȩ
const int		XDF_CNF_COUNT = 500;					///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
const int		XDF_CNFOPT_COUNT = 500;					///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
unsigned int	s_nNumberInSection = 50;				///< һ���г��п��Ի�����ٸ����ݿ�
///< -----------------------------------------------------------------------------------


///< �ۼ�һ�����ߵ�ʱ�� (HH:mm)
__inline unsigned int IncM1Time( unsigned int nM1Time )
{
	unsigned int	nHour = nM1Time / 100;
	unsigned int	nMinute = nM1Time % 100 + 1;

	if( nMinute >= 60 )
	{
		nMinute = 0;
		nHour += 1;
	}

	if( nHour > 23 )
	{
		nHour = 0;
	}

	return nHour * 100 + nMinute;
}

static CriticalObject	s_oMinuteLock;					///< �ٽ�������

MinuteGenerator::MinuteGenerator()
{
	::memset( &m_tagBasicParam, 0, sizeof(m_tagBasicParam) );
}

MinuteGenerator::operator T_LINE_PARAM&()
{
	return m_tagBasicParam;
}

void MinuteGenerator::SetCode( enum XDFMarket eMarket, const char* pszCode )
{
	m_eMarket = eMarket;
	::strcpy( m_pszCode, pszCode );
}

MinuteGenerator& MinuteGenerator::operator=( T_LINE_PARAM& refBasicParam )
{
	::memcpy( &m_tagBasicParam, &refBasicParam, sizeof T_LINE_PARAM );

	return *this;
}

void MinuteGenerator::Update( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime )
{
	Dispatch( m_eMarket, std::string( refObj.Code, 6 ), nDate, nTime );
	if( 0 == m_tagBasicParam.Valid )								///< ȡ��һ�����ݵĲ���
	{
		m_tagBasicParam.Valid = 1;									///< ��Ч���ݱ�ʶ
		m_tagBasicParam.MkMinute = IncM1Time(nTime/1000/100) * 100;///< ȡ�����ڵ�һ��ʱ��
		m_tagBasicParam.MinOpenPx1 = refObj.Now;					///< ȡ�����ڵ�һ���ּ�
		m_tagBasicParam.MinHighPx = refObj.Now;					///< �����ߵ�һ����߼�Ϊ��һ���ּ�
		m_tagBasicParam.MinLowPx = refObj.Now;						///< �����ߵ�һ����ͼ�Ϊ��һ���ּ�
		//m_tagBasicParam.MinAmount1 = refObj.Amount;				///< �����ڵ�һ�ʽ��
		m_tagBasicParam.MinAmount1 = m_tagBasicParam.MinAmount2;	///< �����ڵ�һ�ʽ��
		//m_tagBasicParam.MinVolume1 = refObj.Volume;				///< �����ڵ�һ�ʳɽ���
		m_tagBasicParam.MinVolume1 = m_tagBasicParam.MinVolume2;	///< �����ڵ�һ�ʳɽ���
		//m_tagBasicParam.MinNumTrades1 = pStock->NumTrades;		///< �����ڵ�һ�ʳɽ�����
	}
	if( refObj.Now > m_tagBasicParam.MinHighPx )	{
		m_tagBasicParam.MinHighPx = refObj.Now;					///< ���������Now��
	}
	if( refObj.Now < m_tagBasicParam.MinLowPx )	{
		m_tagBasicParam.MinLowPx = refObj.Now;						///< ���������Now��
	}
	m_tagBasicParam.MinAmount2 = refObj.Amount;					///< �����ڵ�ĩ�ʽ��
	m_tagBasicParam.MinVolume2 = refObj.Volume;					///< �����ڵ�ĩ�ʳɽ���
	m_tagBasicParam.MinClosePx = refObj.Now;						///< ���������¼�
	m_tagBasicParam.MinVoip = refObj.Voip;							///< �����ڵ�ĩ��Voip
}

void MinuteGenerator::Update( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime )
{
	Dispatch( m_eMarket, std::string( refObj.Code, 6 ), nDate, nTime );
	if( 0 == m_tagBasicParam.Valid )								///< ȡ��һ�����ݵĲ���
	{
		m_tagBasicParam.Valid = 1;									///< ��Ч���ݱ�ʶ
		m_tagBasicParam.MkMinute = IncM1Time(nTime/1000/100)*100;	///< ȡ�����ڵ�һ��ʱ��
		m_tagBasicParam.MinOpenPx1 = refObj.Now;					///< ȡ�����ڵ�һ���ּ�
		m_tagBasicParam.MinHighPx = refObj.Now;					///< �����ߵ�һ����߼�Ϊ��һ���ּ�
		m_tagBasicParam.MinLowPx = refObj.Now;						///< �����ߵ�һ����ͼ�Ϊ��һ���ּ�
		//m_tagBasicParam.MinAmount1 = refObj.Amount;				///< �����ڵ�һ�ʽ��
		m_tagBasicParam.MinAmount1 = m_tagBasicParam.MinAmount2;	///< �����ڵ�һ�ʽ��
		//m_tagBasicParam.MinVolume1 = refObj.Volume;				///< �����ڵ�һ�ʳɽ���
		m_tagBasicParam.MinVolume1 = m_tagBasicParam.MinVolume2;	///< �����ڵ�һ�ʳɽ���
		//m_tagBasicParam.MinNumTrades1 = refObj.NumTrades;		///< �����ڵ�һ�ʳɽ�����
	}
	if( refObj.Now > m_tagBasicParam.MinHighPx )	{
		m_tagBasicParam.MinHighPx = refObj.Now;					///< ���������Now��
	}
	if( refObj.Now < m_tagBasicParam.MinLowPx )	{
		m_tagBasicParam.MinLowPx = refObj.Now;						///< ���������Now��
	}
	m_tagBasicParam.MinAmount2 = refObj.Amount;					///< �����ڵ�ĩ�ʽ��
	m_tagBasicParam.MinVolume2 = refObj.Volume;					///< �����ڵ�ĩ�ʳɽ���
	m_tagBasicParam.MinClosePx = refObj.Now;						///< ���������¼�
}

void MinuteGenerator::Dispatch( enum XDFMarket eMarket, std::string& sCode, unsigned int nMkDate, unsigned int nMkTime )
{
	if( 1 == m_tagBasicParam.Valid && (m_tagBasicParam.MkMinute/100) <= (nMkTime / (1000*100)) )
	{
		T_MIN_LINE		tagMinuteLine = { 0 };

		tagMinuteLine.Date = nMkDate;
		tagMinuteLine.Time = m_tagBasicParam.MkMinute;
		tagMinuteLine.eMarketID = eMarket;
		tagMinuteLine.Type = m_tagBasicParam.Type;
		::strcpy( tagMinuteLine.Code, sCode.c_str() );
		tagMinuteLine.OpenPx = m_tagBasicParam.MinOpenPx1 / m_tagBasicParam.dPriceRate;					///< ���̼�һ�����ڵĵ�һ�ʵ�nowpx
		tagMinuteLine.HighPx = m_tagBasicParam.MinHighPx / m_tagBasicParam.dPriceRate;					///< ��߼�һ�����ڵ� ��ߵ�highpx
		tagMinuteLine.LowPx = m_tagBasicParam.MinLowPx / m_tagBasicParam.dPriceRate;					///< ��ͼ�һ�����ڵ� ��͵�lowpx
		tagMinuteLine.ClosePx = m_tagBasicParam.MinClosePx / m_tagBasicParam.dPriceRate;				///< ���̼�һ���������һ�ʵ�Nowpx
		tagMinuteLine.SettlePx = m_tagBasicParam.MinSettlePx / m_tagBasicParam.dPriceRate;				///< �����һ���������һ�ʵ�settlepx
		tagMinuteLine.Amount = m_tagBasicParam.MinAmount2 - m_tagBasicParam.MinAmount1;					///< �ɽ���һ�������ʼ�ȥ��һ�ʵ�amount
		tagMinuteLine.Volume = m_tagBasicParam.MinVolume2 - m_tagBasicParam.MinVolume1;					///< �ɽ���(��/��/��)һ�������ʼ�ȥ��һ�ʵ�volume
		tagMinuteLine.OpenInterest = m_tagBasicParam.MinOpenInterest;									///< �ֲ���(��/��/��)һ�������һ��
		tagMinuteLine.NumTrades = m_tagBasicParam.MinNumTrades2 - m_tagBasicParam.MinNumTrades1;		///< �ɽ�����һ�������ʼ�ȥ��һ�ʵ�numtrades
		tagMinuteLine.Voip = m_tagBasicParam.MinVoip / m_tagBasicParam.dPriceRate;						///< ����ģ����Ȩ֤���һ���ӵ����һ��voip
/*
		if( strncmp( tagMinuteLine.Code, "600000", 6 ) == 0 ) {
			char pszLine[1024] = { 0 };
			::printf( "Date=%d,Time=%d,Open=%.4f,High=%.4f,Low=%.4f,Close=%.4f,Settle=%.4f,Amt=%.4f,Vol=%I64d,OI=%I64d,TrdNum=%I64d,IOPV=%.4f\n"
				, tagMinuteLine.Date, tagMinuteLine.Time, tagMinuteLine.OpenPx, tagMinuteLine.HighPx, tagMinuteLine.LowPx, tagMinuteLine.ClosePx
				, tagMinuteLine.SettlePx, tagMinuteLine.Amount, tagMinuteLine.Volume, tagMinuteLine.OpenInterest, tagMinuteLine.NumTrades, tagMinuteLine.Voip );
		}
*/
		{
		CriticalLock		section( s_oMinuteLock );
		if( QuotationData::GetMinuteCacheObj().PutData( &tagMinuteLine ) < 0 )
		{
			ServerStatus::GetStatusObj ().AddMinuteLostRef();
		}
		}

		m_tagBasicParam.Valid = 0;															///< ������Ч�Ա�ʶ
	}
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
		case XDF_SH:		///< �Ϻ�Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHOPT:		///< �Ϻ���Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZ:		///< ��֤Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZOPT:		///< ������Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CF:		///< �н��ڻ�
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_ZJOPT:		///< �н���Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::Initialize() : enter ......................" );

	int					nErrorCode = 0;
	CriticalLock		section( s_oMinuteLock );

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

	if( false == m_oThdMinuteDump.IsAlive() )
	{
		if( 0 != m_oThdMinuteDump.Create( "ThreadDumpMinuteLine()", ThreadDumpMinuteLine, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create minute line thread(1)" );
			return -6;
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
	m_oThdMinuteDump.Join( 5000 );
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

__inline bool	PrepareMinuteFile( T_MIN_LINE&	refMinuteLine, std::string& sCode, std::ofstream& oDumper )
{
	char	pszFilePath[512] = { 0 };

	switch( refMinuteLine.eMarketID )
	{
	case XDF_SH:		///< �Ϻ�Lv1
	case XDF_SHOPT:		///< �Ϻ���Ȩ
	case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SSE/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_SZ:		///< ��֤Lv1
	case XDF_SZOPT:		///< ������Ȩ
	case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SZSE/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_CF:		///< �н��ڻ�
	case XDF_ZJOPT:		///< �н���Ȩ
		::sprintf( pszFilePath, "CFFEX/MIN/%s/", refMinuteLine.Code );
		break;
	case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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
			while( false == SimpleThread::GetGlobalStopFlag() )
			{
				char			pszLine[1024] = { 0 };
				T_MIN_LINE		tagMinuteLine = { 0 };
				CriticalLock	section( s_oMinuteLock );

				if( s_arrayMinuteLine.GetData( &tagMinuteLine ) <= 0 )
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpMinuteLine() : misson complete!" );

	return NULL;
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

			refQuotation.FlushDayLineOnCloseTime();									///< ����Ƿ���Ҫ������
			refQuotation.UpdateMarketsTime();										///< ���¸��г������ں�ʱ��
			refStatus.UpdateTickBufOccupancyRate( refTickBuf.GetPercent() );		///< TICK����ռ����
			refStatus.UpdateMinuteBufOccupancyRate( s_arrayMinuteLine.GetPercent() );		///< MinuteLine����ռ����
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadOnIdle() : misson complete!" );

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
	case XDF_SH:		///< �Ϻ�Lv1
	case XDF_SHOPT:		///< �Ϻ���Ȩ
	case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_SZ:		///< ��֤Lv1
	case XDF_SZOPT:		///< ������Ȩ
	case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CF:		///< �н��ڻ�
	case XDF_ZJOPT:		///< �н���Ȩ
		::sprintf( pszFilePath, "CFFEX/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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
	case XDF_SZ:	///< ��֤L1
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

int QuotationData::BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam )
{
	unsigned int		nBufSize = 0;
	char*				pDataPtr = NULL;
	int					nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:	///< ��֤L1
		{
			CriticalLock					section( m_oLockSHL1 );
			T_MAP_GEN_MLINES::iterator		it = m_mapSHL1.find( sCode );
			if( it == m_mapSHL1.end() )
			{
				m_mapSHL1[sCode] = refParam;
				m_mapSHL1[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_SHOPT:	///< ����
		{
			T_MAP_GEN_MLINES::iterator it = m_mapSHOPT.find( sCode );
			if( it == m_mapSHOPT.end() )
			{
				m_mapSHOPT[sCode] = refParam;
				m_mapSHOPT[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_SZ:	///< ��֤L1
		{
			CriticalLock					section( m_oLockSZL1 );
			T_MAP_GEN_MLINES::iterator		it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				m_mapSZL1[sCode] = refParam;
				m_mapSZL1[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_SZOPT:	///< ����
		{
			T_MAP_GEN_MLINES::iterator it = m_mapSZOPT.find( sCode );
			if( it == m_mapSZOPT.end() )
			{
				m_mapSZOPT[sCode] = refParam;
				m_mapSZOPT[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_CF:	///< �н��ڻ�
		{
			T_MAP_GEN_MLINES::iterator it = m_mapCFF.find( sCode );
			if( it == m_mapCFF.end() )
			{
				m_mapCFF[sCode] = refParam;
				m_mapCFF[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_ZJOPT:	///< �н���Ȩ
		{
			T_MAP_GEN_MLINES::iterator it = m_mapCFFOPT.find( sCode );
			if( it == m_mapCFFOPT.end() )
			{
				m_mapCFFOPT[sCode] = refParam;
				m_mapCFFOPT[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_CNF:	///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
		{
			T_MAP_GEN_MLINES::iterator it = m_mapCNF.find( sCode );
			if( it == m_mapCNF.end() )
			{
				m_mapCNF[sCode] = refParam;
				m_mapCNF[sCode].SetCode( eMarket, sCode.c_str() );
			}
		}
		break;
	case XDF_CNFOPT:///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
		{
			T_MAP_GEN_MLINES::iterator it = m_mapCNFOPT.find( sCode );
			if( it == m_mapCNFOPT.end() )
			{
				m_mapCNFOPT[sCode] = refParam;
				m_mapCNFOPT[sCode].SetCode( eMarket, sCode.c_str() );
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
	case XDF_SH:		///< �Ϻ�Lv1
	case XDF_SHOPT:		///< �Ϻ���Ȩ
	case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
		::strcpy( pszFilePath, "SSE/DAY/" );
		break;
	case XDF_SZ:		///< ��֤Lv1
	case XDF_SZOPT:		///< ������Ȩ
	case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
		::strcpy( pszFilePath, "SZSE/DAY/" );
		break;
	case XDF_CF:		///< �н��ڻ�
	case XDF_ZJOPT:		///< �н���Ȩ
		::strcpy( pszFilePath, "CFFEX/DAY/" );
		break;
	case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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
		return false;		///< �����ڸ����ڵ����ߵ����
	}

	char		pszDate[32] = { 0 };
	char		pszTail[1024*2] = { 0 };

	::sprintf( pszDate, "%u,", nDate );
	oLoader.seekg( -1024, std::ios::end );
	oLoader.read( pszTail, sizeof(pszTail) );

	char*		pPos = ::strstr( pszTail, pszDate );
	if( NULL != pPos )
	{
		return true;		///< �Ѿ����ڸ����ڵ����ߵ����
	}

	return false;			///< �����ڸ����ڵ����ߵ����
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
	case XDF_SH:	///< ��֤L1
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
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
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
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
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
	case XDF_SHOPT:	///< ����
		{
			XDFAPI_ShOptData*				pStock = (XDFAPI_ShOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapSHOPT.find( std::string(pStock->Code, 8 ) );

			if( it != m_mapSHOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 8 ), 0, nMachineDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%f\n", nMachineDate
							, pStock->OpenPx/refParam.dPriceRate, pStock->HighPx/refParam.dPriceRate, pStock->LowPx/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount*1.0, pStock->Volume, pStock->Position, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
				}
			}
		}
		break;
	case XDF_SZ:	///< ��֤L1
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
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
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
							if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
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
	case XDF_SZOPT:	///< ����
		{
			XDFAPI_SzOptData*				pStock = (XDFAPI_SzOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapSZOPT.find( std::string(pStock->Code,8) );

			if( it != m_mapSZOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 8 ), 0, nMachineDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
							, pStock->OpenPx/refParam.dPriceRate, pStock->HighPx/refParam.dPriceRate, pStock->LowPx/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount*1.0, pStock->Volume, pStock->Position, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
				}
			}
		}
		break;
	case XDF_CF:	///< �н��ڻ�
		{
			XDFAPI_CffexData*				pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), 0, nMachineDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
							, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
				}
			}
		}
		break;
	case XDF_ZJOPT:	///< �н���Ȩ
		{
			XDFAPI_ZjOptData*				pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code ), 0, nMachineDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", nMachineDate
							, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
				}
			}
		}
		break;
	case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
		{
			XDFAPI_CNFutureData*				pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_GEN_MLINES::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );

			if( it != m_mapCNF.end() )
			{
				T_LINE_PARAM&			refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code, 6 ), refParam.Type, s_nCNFTradingDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", s_nCNFTradingDate
							, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
				}
			}
		}
		break;
	case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
		{
			XDFAPI_CNFutOptData*				pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCNFOPT.end() )
			{
				T_LINE_PARAM&			refParam = it->second;

				if( true == PreDayFile( oLoader, oDumper, eMarket, std::string( pStock->Code ), refParam.Type, s_nCNFOPTTradingDate ) )
				{
					if( false == CheckDateInDayFile( oLoader, nMachineDate ) ) // δ�������ļ��в��ҳ�������������ݣ�������Ҫд��һ��
					{
						int	nLen = ::sprintf( pszDayLine, "%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%I64d,%I64d,%u,%.4f\n", s_nCNFOPTTradingDate
							, pStock->Open/refParam.dPriceRate, pStock->High/refParam.dPriceRate, pStock->Low/refParam.dPriceRate, pStock->Now/refParam.dPriceRate
							, pStock->SettlePrice/refParam.dPriceRate, pStock->Amount, pStock->Volume, pStock->OpenInterest, 0, 0 );
						oDumper.write( pszDayLine, nLen );
					}
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
	case XDF_SH:	///< ��֤L1
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
						MinuteGenerator&	refMinuGen = it->second;
						T_LINE_PARAM&		refParam = refMinuGen;
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
						refTickLine.UpperPx = pStock->HighLimit / refParam.dPriceRate;
						refTickLine.LowerPx = pStock->LowLimit / refParam.dPriceRate;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
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
/*						if( strncmp( pStock->Code, "600000", 6 ) == 0 ) {
							::printf( "600000, (%u)Time=%u + 1, Now=%u, Amt=%f, Vol=%I64d\n", nMkTime, refTickLine.Time/1000/100, pStock->Now, pStock->Amount, pStock->Volume );
						}*/
						if( refTickLine.Time > 0 ) {
							refMinuGen.Update( *pStock, refTickLine.Date, refTickLine.Time );
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
						MinuteGenerator&	refMinuGen = it->second;
						T_LINE_PARAM&		refParam = refMinuGen;
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
						if( refTickLine.Time > 0 ) {
							refMinuGen.Update( *pStock, refTickLine.Date, refTickLine.Time );
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< ����
		{
			XDFAPI_ShOptData*				pStock = (XDFAPI_ShOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapSHOPT.find( std::string(pStock->Code, 8 ) );

			if( it != m_mapSHOPT.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				refMinuGen.Dispatch( eMarket, std::string( pStock->Code, 8 ), refTickLine.Date, refTickLine.Time );
				if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
				{
					refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
					refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
					refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
					refParam.MinHighPx = pStock->Now;
					refParam.MinLowPx = pStock->Now;
					//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
					refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
					//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
					refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
					//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
				}
				if( pStock->Now > refParam.MinHighPx )	{
					refParam.MinHighPx = pStock->Now;			///< ��������߼�
				}
				if( pStock->Now < refParam.MinLowPx )	{
					refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
				}
				refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
				refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
				refParam.MinClosePx = pStock->Now;					///< ���������¼�
				//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
				refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
				refParam.MinOpenInterest = pStock->Position;		///< �����ڵ�ĩ�ʳֲ���
				//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
			}
		}
		break;
	case XDF_SZ:	///< ��֤L1
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
						MinuteGenerator&	refMinuGen = it->second;
						T_LINE_PARAM&		refParam = refMinuGen;
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
						refTickLine.UpperPx = pStock->HighLimit / refParam.dPriceRate;
						refTickLine.LowerPx = pStock->LowLimit / refParam.dPriceRate;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
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
						if( refTickLine.Time > 0 ) {
							refMinuGen.Update( *pStock, refTickLine.Date, refTickLine.Time );
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
						MinuteGenerator&	refMinuGen = it->second;
						T_LINE_PARAM&		refParam = refMinuGen;
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
						if( refTickLine.Time > 0 ) {
							refMinuGen.Update( *pStock, refTickLine.Date, refTickLine.Time );
						}
					}
				}
				break;
			}
		}
		break;
	case XDF_SZOPT:	///< ����
		{
			XDFAPI_SzOptData*				pStock = (XDFAPI_SzOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapSZOPT.find( std::string(pStock->Code,8) );

			if( it != m_mapSZOPT.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				if( refTickLine.Time > 0 ) {
					refMinuGen.Dispatch( eMarket, std::string( pStock->Code, 8 ), refTickLine.Date, refTickLine.Time );
					if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
					{
						refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
						refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
						refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
						refParam.MinHighPx = pStock->Now;
						refParam.MinLowPx = pStock->Now;
						//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
						refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
						//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
						refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
						//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
					}
					if( pStock->Now > refParam.MinHighPx )	{
						refParam.MinHighPx = pStock->Now;			///< ��������߼�
					}
					if( pStock->Now < refParam.MinLowPx )	{
						refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
					}
					refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
					refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
					refParam.MinClosePx = pStock->Now;					///< ���������¼�
					//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
					refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
					refParam.MinOpenInterest = pStock->Position;		///< �����ڵ�ĩ�ʳֲ���
					//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
				}
			}
		}
		break;
	case XDF_CF:	///< �н��ڻ�
		{
			XDFAPI_CffexData*				pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				if( refTickLine.Time > 0 ) {
					refMinuGen.Dispatch( eMarket, std::string( pStock->Code, 6 ), refTickLine.Date, refTickLine.Time );
					if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
					{
						refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
						refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
						refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
						refParam.MinHighPx = pStock->Now;
						refParam.MinLowPx = pStock->Now;
						//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
						refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
						//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
						refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
						//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
					}
					if( pStock->Now > refParam.MinHighPx )	{
						refParam.MinHighPx = pStock->Now;			///< ��������߼�
					}
					if( pStock->Now < refParam.MinLowPx )	{
						refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
					}
					refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
					refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
					refParam.MinClosePx = pStock->Now;					///< ���������¼�
					//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
					refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
					refParam.MinOpenInterest = pStock->OpenInterest;	///< �����ڵ�ĩ�ʳֲ���
					//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
				}
			}
		}
		break;
	case XDF_ZJOPT:	///< �н���Ȩ
		{
			XDFAPI_ZjOptData*				pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				if( refTickLine.Time > 0 ) {
					refMinuGen.Dispatch( eMarket, std::string( pStock->Code ), refTickLine.Date, refTickLine.Time );
					if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
					{
						refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
						refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
						refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
						refParam.MinHighPx = pStock->Now;
						refParam.MinLowPx = pStock->Now;
						//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
						refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
						//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
						refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
						//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
					}
					if( pStock->Now > refParam.MinHighPx )	{
						refParam.MinHighPx = pStock->Now;			///< ��������߼�
					}
					if( pStock->Now < refParam.MinLowPx )	{
						refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
					}
					refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
					refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
					refParam.MinClosePx = pStock->Now;					///< ���������¼�
					//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
					refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
					refParam.MinOpenInterest = pStock->OpenInterest;	///< �����ڵ�ĩ�ʳֲ���
					//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
				}
			}
		}
		break;
	case XDF_CNF:	///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
		{
			XDFAPI_CNFutureData*				pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_GEN_MLINES::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );

			if( nTradeDate > 0 )
			{
				 s_nCNFTradingDate = nTradeDate;
			}

			if( it != m_mapCNF.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				if( refTickLine.Time > 0 ) {
					refMinuGen.Dispatch( eMarket, std::string( pStock->Code, 6 ), refTickLine.Date, refTickLine.Time );
					if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
					{
						refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
						refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
						refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
						refParam.MinHighPx = pStock->Now;
						refParam.MinLowPx = pStock->Now;
						//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
						refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
						//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
						refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
						//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
					}
					if( pStock->Now > refParam.MinHighPx )	{
						refParam.MinHighPx = pStock->Now;			///< ��������߼�
					}
					if( pStock->Now < refParam.MinLowPx )	{
						refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
					}
					refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
					refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
					refParam.MinClosePx = pStock->Now;					///< ���������¼�
					//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
					refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
					refParam.MinOpenInterest = pStock->OpenInterest;	///< �����ڵ�ĩ�ʳֲ���
					//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
				}
			}
		}
		break;
	case XDF_CNFOPT:///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
		{
			XDFAPI_CNFutOptData*				pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_GEN_MLINES::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );

			if( nTradeDate > 0 )
			{
				 s_nCNFOPTTradingDate = nTradeDate;
			}

			if( it != m_mapCNFOPT.end() )
			{
				MinuteGenerator&	refMinuGen = it->second;
				T_LINE_PARAM&		refParam = refMinuGen;
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
				refTickLine.BidPx1 = pStock->Buy[0].Price / refParam.dPriceRate;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price / refParam.dPriceRate;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price / refParam.dPriceRate;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price / refParam.dPriceRate;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.dPriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				///< ------------ Minute Lines -----------------------
				if( refTickLine.Time > 0 ) {
					refMinuGen.Dispatch( eMarket, std::string( pStock->Code ), refTickLine.Date, refTickLine.Time );
					if( 0 == refParam.Valid )							///< ȡ��һ�����ݵĲ���
					{
						refParam.Valid = 1;								///< ��Ч���ݱ�ʶ
						refParam.MkMinute = IncM1Time(refTickLine.Time/1000/100)*100;///< ȡ�����ڵ�һ��ʱ��
						refParam.MinOpenPx1 = pStock->Now;				///< ȡ�����ڵ�һ���ּ�
						refParam.MinHighPx = pStock->Now;
						refParam.MinLowPx = pStock->Now;
						//refParam.MinAmount1 = pStock->Amount;			///< �����ڵ�һ�ʽ��
						refParam.MinAmount1 = refParam.MinAmount2;		///< �����ڵ�һ�ʽ��
						//refParam.MinVolume1 = pStock->Volume;			///< �����ڵ�һ�ʳɽ���
						refParam.MinVolume1 = refParam.MinVolume2;		///< �����ڵ�һ�ʳɽ���
						//refParam.MinNumTrades1 = pStock->NumTrades;	///< �����ڵ�һ�ʳɽ�����
					}
					if( pStock->Now > refParam.MinHighPx )	{
						refParam.MinHighPx = pStock->Now;				///< ��������߼�
					}
					if( pStock->Now < refParam.MinLowPx )	{
						refParam.MinLowPx = pStock->Now;				///< ��������ͼ�
					}
					refParam.MinAmount2 = pStock->Amount;				///< �����ڵ�ĩ�ʽ��
					refParam.MinVolume2 = pStock->Volume;				///< �����ڵ�ĩ�ʳɽ���
					refParam.MinClosePx = pStock->Now;					///< ���������¼�
					//refParam.MinVoip = pStock->Voip;					///< �����ڵ�ĩ��Voip
					refParam.MinSettlePx = pStock->SettlePrice;			///< ���������½����
					refParam.MinOpenInterest = pStock->OpenInterest;	///< �����ڵ�ĩ�ʳֲ���
					//refParam.MinNumTrades2 = pStock->NumTrades;		///< �����ڵ�ĩ�ʳɽ�����
				}
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

	ServerStatus::GetStatusObj().UpdateSecurity( (enum XDFMarket)(refTickLine.eMarketID), refTickLine.Code, refTickLine.NowPx, refTickLine.Amount, refTickLine.Volume );	///< ���¸��г�����Ʒ״̬

	return 0;
}




