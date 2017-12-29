#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "SvrStatus.h"
#include "../DataTranscoding4QuoClientApi.h"


ServerStatus::ServerStatus()
 : m_bWritingTick( false ), m_bWritingMinute( false )
 , m_nFinancialUpdateDate( 0 ), m_nFinancialUpdateTime( 0 )
 , m_nWeightUpdateDate( 0 ), m_nWeightUpdateTime( 0 )
{
	::memset( m_vctMarketTime, 0, sizeof(m_vctMarketTime) );
	::memset( m_vctLastSecurity, 0, sizeof(m_vctLastSecurity) );
	::memset( m_vctsBufOccupancyRate, 0, sizeof(m_vctsBufOccupancyRate) );
}

ServerStatus& ServerStatus::GetStatusObj()
{
	static	ServerStatus	obj;

	return obj;
}

void ServerStatus::AnchorSecurity( enum XDFMarket eMkID, const char* pszCode, const char* pszName )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( NULL == pszCode || NULL == pszName || nPos >= 256 )
	{
		return;
	}

	T_SECURITY_STATUS&		refSecurity = m_vctLastSecurity[nPos];

	if( ::strlen( refSecurity.Code ) > 0 )		///< 如果代码已经存在，则不再重复锚定
	{
		return;
	}

	::strcpy( refSecurity.Code, pszCode );
	::strcpy( refSecurity.Name, pszName );
}

void ServerStatus::UpdateSecurity( enum XDFMarket eMkID, const char* pszCode, double dLastPx, double dAmount, unsigned __int64 nVolume )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( NULL == pszCode || nPos >= 256 )
	{
		return;
	}

	int						nCodeLen = ::strlen( pszCode );
	T_SECURITY_STATUS&		refSecurity = m_vctLastSecurity[nPos];

	if( ::memcmp( refSecurity.Code, pszCode, nCodeLen ) == 0 )
	{
		refSecurity.LastPx = dLastPx;
		refSecurity.Amount = dAmount;
		refSecurity.Volume = nVolume;
	}
}

T_SECURITY_STATUS& ServerStatus::FetchSecurity( enum XDFMarket eMkID )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return m_vctLastSecurity[255];
	}

	return m_vctLastSecurity[nPos];
}

void ServerStatus::UpdateMinuteWritingStatus( bool bStatus )
{
	m_bWritingMinute = bStatus;
}

bool ServerStatus::FetchMinuteWritingStatus()
{
	return m_bWritingMinute;
}

void ServerStatus::UpdateTickWritingStatus( bool bStatus )
{
	m_bWritingTick = bStatus;
}

bool ServerStatus::FetchTickWritingStatus()
{
	return m_bWritingTick;
}

void ServerStatus::UpdateMkTime( enum XDFMarket eMkID, unsigned int nMkTime )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return;
	}

	m_vctMarketTime[nPos] = nMkTime;
}

unsigned int ServerStatus::FetchMkTime( enum XDFMarket eMkID )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return 0;
	}

	return m_vctMarketTime[nPos];
}

void ServerStatus::UpdateFinancialDT()
{
	m_nFinancialUpdateDate = DateTime::Now().DateToLong();
	m_nFinancialUpdateTime = DateTime::Now().TimeToLong();
}

void ServerStatus::UpdateWeightDT()
{
	m_nWeightUpdateDate = DateTime::Now().DateToLong();
	m_nWeightUpdateTime = DateTime::Now().TimeToLong();
}

void ServerStatus::GetWeightUpdateDT( unsigned int& nDate, unsigned int& nTime )
{
	nDate = m_nWeightUpdateDate;
	nTime = m_nWeightUpdateTime;
}

void ServerStatus::GetFinancialUpdateDT( unsigned int& nDate, unsigned int& nTime )
{
	nDate = m_nFinancialUpdateDate;
	nTime = m_nFinancialUpdateTime;
}

void ServerStatus::UpdateMkOccupancyRate( enum XDFMarket eMkID, double dRate )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return;
	}

	m_vctsBufOccupancyRate[nPos] = dRate;
}

double ServerStatus::FetchMkOccupancyRate( enum XDFMarket eMkID )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return m_vctsBufOccupancyRate[255];
	}

	return m_vctsBufOccupancyRate[nPos];
}




