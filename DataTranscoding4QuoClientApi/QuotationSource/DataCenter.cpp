#include "DataCenter.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


static char*		s_pDataCache = NULL;
static unsigned int	s_nMaxCacheSize = 0;
static unsigned int	s_nAllocateSize = 0;
static unsigned int	s_nSectionNumber = 60;

DayLineArray::DayLineArray( enum XDFMarket eMkID )
 : m_eMarketID( eMkID )
{
}

char* DayLineArray::GrabCache()
{
	if( NULL == s_pDataCache )
	{
		///<				结构大小	单市场k线数	市场预留数
		s_nMaxCacheSize = sizeof(T_DAY_LINE) * 15000 * 10 * s_nSectionNumber + 128;
		s_pDataCache = new char[s_nMaxCacheSize];
	}

	return NULL;
}

void DayLineArray::FreeCaches()
{
	if( NULL != s_pDataCache )
	{
		delete [] s_pDataCache;
		s_pDataCache = NULL;
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




