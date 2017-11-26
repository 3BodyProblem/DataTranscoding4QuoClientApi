#include "DataCenter.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


QuotationData::QuotationData()
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




