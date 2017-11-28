#include "DataCenter.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


///< -------------------- Configuration ------------------------------------------------
const int	XDF_SH_COUNT = 20000;					///< 上海Lv1
const int	XDF_SHL2_COUNT = 20000;					///< 上海Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SHOPT_COUNT = 12000	;				///< 上海期权
const int	XDF_SZ_COUNT = 12000;					///< 深证Lv1
const int	XDF_SZL2_COUNT = 12000;					///< 深证Lv2(QuoteClientApi内部屏蔽)
const int	XDF_SZOPT_COUNT = 12000;				///< 深圳期权
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

char* CacheAlloc::GrabCache( enum XDFMarket eMkID )
{
	char*			pData = NULL;
	unsigned int	nBufferSize4Market = 0;

	if( NULL == m_pDataCache )
	{
		m_nMaxCacheSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_DAY_LINE) * s_nNumberInSection;
		m_pDataCache = new char[m_nMaxCacheSize];
	}

	if( NULL == m_pDataCache )
	{
		throw std::runtime_error( "DayLineArray::GrabCache() : invalid buffer pointer." );
	}

	switch( eMkID )
	{
	case XDF_SH:		///< 上海Lv1
		nBufferSize4Market = XDF_SH_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		nBufferSize4Market = XDF_SHL2_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_SHOPT:		///< 上海期权
		nBufferSize4Market = XDF_SHOPT_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_SZ:		///< 深证Lv1
		nBufferSize4Market = XDF_SZ_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		nBufferSize4Market = XDF_SZL2_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_SZOPT:		///< 深圳期权
		nBufferSize4Market = XDF_SZOPT_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_CF:		///< 中金期货
		nBufferSize4Market = XDF_CF_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_ZJOPT:		///< 中金期权
		nBufferSize4Market = XDF_ZJOPT_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
		nBufferSize4Market = XDF_CNF_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		nBufferSize4Market = XDF_CNFOPT_COUNT * sizeof(T_DAY_LINE) * s_nNumberInSection;
		break;
	default:
		throw std::runtime_error( "DayLineArray::GrabCache() : unknow market id" );
	}

	if( (m_nAllocateSize + nBufferSize4Market) > m_nMaxCacheSize )
	{
		throw std::runtime_error( "DayLineArray::GrabCache() : not enough space." );
	}

	pData = m_pDataCache + m_nAllocateSize;
	m_nAllocateSize += nBufferSize4Market;

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

void QuotationData::UpdateModuleStatus( enum XDFMarket nMarket, int nStatus )
{
	m_mapModuleStatus[nMarket] = nStatus;
}




