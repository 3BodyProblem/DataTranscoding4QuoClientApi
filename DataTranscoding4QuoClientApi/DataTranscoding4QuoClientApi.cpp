#include "targetver.h"
#include <exception>
#include <algorithm>
#include <functional>
#include "Configuration.h"
#include "UnitTest/UnitTest.h"
#include "QuotationSource/SvrStatus.h"
#include "DataTranscoding4QuoClientApi.h"


const std::string		g_sVersion = "1.0.1";


QuoCollector::QuoCollector()
 : m_pCbDataHandle( NULL )
{
}

QuoCollector::~QuoCollector()
{
}

QuoCollector& QuoCollector::GetCollector()
{
	static	QuoCollector	obj;

	return obj;
}

int QuoCollector::Initialize( I_DataHandle* pIDataHandle )
{
	int		nErrorCode = 0;

	m_pCbDataHandle = pIDataHandle;
	if( NULL == m_pCbDataHandle )
	{
		::printf( "QuoCollector::Initialize() : invalid arguments (NULL), version = %s\n", g_sVersion.c_str() );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuoCollector::Initialize() : [Version] %s", g_sVersion.c_str() );

	if( 0 != (nErrorCode = Configuration::GetConfig().Initialize()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuoCollector::Initialize() : failed 2 initialize configuration obj, errorcode=%d", nErrorCode );
		return -2;
	}

	if( 0 != (nErrorCode = m_oFileScanner.Initialize()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuoCollector::Initialize() : failed 2 initialize file scanner obj, errorcode=%d", nErrorCode );
		return -3;
	}

	m_oStartTime.SetCurDateTime();

	return 0;
}

void QuoCollector::Release()
{
	m_oQuotationData.Release();
}

Quotation& QuoCollector::GetQuoObj()
{
	return m_oQuotationData;
}

I_DataHandle* QuoCollector::operator->()
{
	if( NULL == m_pCbDataHandle )
	{
		::printf( "QuoCollector::operator->() : invalid data callback interface ptr.(NULL)\n" );
	}

	return m_pCbDataHandle;
}

void QuoCollector::Halt()
{
	m_oQuotationData.Halt();
}

int QuoCollector::RecoverQuotation()
{
	unsigned int	nSec = 0;
	int				nErrorCode = 0;

	if( 0 != (nErrorCode=m_oQuotationData.Initialize()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "QuoCollector::RecoverQuotation() : failed 2 subscript quotation, errorcode=%d", nErrorCode );
		return -1;
	}

	m_pCbDataHandle->OnImage( 234, (char*)&nSec, sizeof(nSec), true );		///< 将服务的数据库状态设置成“Builed”

	return 0;
}

enum E_SS_Status QuoCollector::GetCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen )
{
	if( false == SimpleThread::GetGlobalStopFlag() )
	{
		Configuration&		refCnf = Configuration::GetConfig();
		WorkStatus&			refStatus = m_oQuotationData.GetWorkStatus();
		ServerStatus&		refSvrStatus = ServerStatus::GetStatusObj();
		std::string&		sProgramDuration = m_oStartTime.GetDurationString();
		unsigned int		nFinanDate = 0;
		unsigned int		nFinanTime = 0;
		unsigned int		nWeightDate = 0;
		unsigned int		nWeightTime = 0;
		char				pszWeightFileTime[64] = { 0 };
		char				pszFinancialFileTime[64] = { 0 };

		///< 模块基础信息
		refSvrStatus.GetFinancialUpdateDT( nFinanDate, nFinanTime );
		refSvrStatus.GetWeightUpdateDT( nWeightDate, nWeightTime );
		if( nWeightDate != 0 && nWeightTime != 0 )	{
			::sprintf( pszWeightFileTime, "%d %d", nWeightDate, nWeightTime );
		}
		if( nFinanDate != 0 && nFinanTime != 0 )	{
			::sprintf( pszFinancialFileTime, "%d %d", nFinanDate, nFinanTime );
		}
		nStrLen = ::sprintf( pszStatusDesc, "模块名=转码机,转码机版本=V%s,行情API版本=%s,运行时间=%s,Tick落盘失败数=%u,Min线落盘失败数=%u,资讯落盘时间=%s,权息落盘时间=%s,[市场信息]"
			, g_sVersion.c_str(), m_oQuotationData.QuoteApiVersion().c_str(), sProgramDuration.c_str()
			, refSvrStatus.GetTickLostRef(), refSvrStatus.GetMinuteLostRef()
			, pszFinancialFileTime, pszWeightFileTime );

		///< 各市场信息
		T_SECURITY_STATUS&	refSHL1Snap = refSvrStatus.FetchSecurity( XDF_SH );
		T_SECURITY_STATUS&	refSHOPTSnap = refSvrStatus.FetchSecurity( XDF_SHOPT );
		T_SECURITY_STATUS&	refSZL1Snap = refSvrStatus.FetchSecurity( XDF_SZ );
		T_SECURITY_STATUS&	refSZOPTSnap = refSvrStatus.FetchSecurity( XDF_SZOPT );
		T_SECURITY_STATUS&	refSHCFFSnap = refSvrStatus.FetchSecurity( XDF_CF );
		T_SECURITY_STATUS&	refSHCFFOPTSnap = refSvrStatus.FetchSecurity( XDF_ZJOPT );
		T_SECURITY_STATUS&	refCNFSnap = refSvrStatus.FetchSecurity( XDF_CNF );
		T_SECURITY_STATUS&	refCNFOPTSnap = refSvrStatus.FetchSecurity( XDF_CNFOPT );

		if( refSvrStatus.FetchMkDate( XDF_SH ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=上海L1,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_SH ), refSvrStatus.FetchMkTime( XDF_SH ), refSvrStatus.GetMkStatus(XDF_SH) );
		}
		if( refSvrStatus.FetchMkDate( XDF_SHOPT ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=上海期权,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_SHOPT ), refSvrStatus.FetchMkTime( XDF_SHOPT ), refSvrStatus.GetMkStatus(XDF_SHOPT) );
		}
		if( refSvrStatus.FetchMkDate( XDF_SZ ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=深圳L1,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_SZ ), refSvrStatus.FetchMkTime( XDF_SZ ), refSvrStatus.GetMkStatus(XDF_SZ) );
		}
		if( refSvrStatus.FetchMkDate( XDF_SZOPT ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=深圳期权,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_SZOPT ), refSvrStatus.FetchMkTime( XDF_SZOPT ), refSvrStatus.GetMkStatus(XDF_SZOPT) );
		}
		if( refSvrStatus.FetchMkDate( XDF_CF ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=中金期货,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_CF ), refSvrStatus.FetchMkTime( XDF_CF ), refSvrStatus.GetMkStatus(XDF_CF) );
		}
		if( refSvrStatus.FetchMkDate( XDF_ZJOPT ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=中金期权,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_ZJOPT ), refSvrStatus.FetchMkTime( XDF_ZJOPT ), refSvrStatus.GetMkStatus(XDF_ZJOPT) );
		}
		if( refSvrStatus.FetchMkDate( XDF_CNF ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=商品期货,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_CNF ), refSvrStatus.FetchMkTime( XDF_CNF ), refSvrStatus.GetMkStatus(XDF_CNF) );
		}
		if( refSvrStatus.FetchMkDate( XDF_CNFOPT ) > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",市场=商品期权,日期=%u,行情时间=%u,行情状态=%s"
				, refSvrStatus.FetchMkDate( XDF_CNFOPT ), refSvrStatus.FetchMkTime( XDF_CNFOPT ), refSvrStatus.GetMkStatus(XDF_CNFOPT) );
		}

		///< 各市场行情信息,
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[行情信息]" );
		if( refSHL1Snap.LastPx > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",上海L1缓存=%d%%,代码=%s,名称=%s,现价=%.2f", refSvrStatus.FetchTickBufShL1OccupancyRate(), refSHL1Snap.Code, refSHL1Snap.Name, refSHL1Snap.LastPx );
		}
		if( refSZL1Snap.LastPx > 0 )	{
			nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",深圳L1缓存=%d%%,代码=%s,名称=%s,现价=%.2f", refSvrStatus.FetchTickBufSzL1OccupancyRate(), refSZL1Snap.Code, refSZL1Snap.Name, refSZL1Snap.LastPx );
		}

		return refStatus;
	}

	return ET_SS_DISCONNECTED;
}


extern "C"
{
	__declspec(dllexport) int __stdcall	Initialize( I_DataHandle* pIDataHandle )
	{
		return QuoCollector::GetCollector().Initialize( pIDataHandle );
	}

	__declspec(dllexport) void __stdcall Release()
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "::Release() : ............ Releasing ............" );
		QuoCollector::GetCollector().Release();
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "::Release() : ............ Releaseed ............" );
	}

	__declspec(dllexport) int __stdcall	RecoverQuotation()
	{
		return QuoCollector::GetCollector().RecoverQuotation();
	}

	__declspec(dllexport) void __stdcall HaltQuotation()
	{
		QuoCollector::GetCollector().Halt();
	}

	__declspec(dllexport) int __stdcall	GetStatus( char* pszStatusDesc, unsigned int& nStrLen )
	{
		return QuoCollector::GetCollector().GetCollectorStatus( pszStatusDesc, nStrLen );
	}

	__declspec(dllexport) bool __stdcall IsProxy()
	{
		return false;
	}

	__declspec(dllexport) int __stdcall	GetMarketID()
	{
		return 255;
	}

	__declspec(dllexport) void __stdcall	ExecuteUnitTest()
	{
		::printf( "\n\n---------------------- [Begin] -------------------------\n" );
		ExecuteTestCase();
		::printf( "----------------------  [End]  -------------------------\n\n\n" );
	}

	__declspec(dllexport) void __stdcall Echo()
	{
	}
}




