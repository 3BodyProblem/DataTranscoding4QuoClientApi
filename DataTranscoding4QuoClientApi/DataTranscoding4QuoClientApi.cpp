#include "targetver.h"
#include <exception>
#include <algorithm>
#include <functional>
#include "Configuration.h"
#include "UnitTest/UnitTest.h"
#include "QuotationSource/SvrStatus.h"
#include "DataTranscoding4QuoClientApi.h"


const std::string		g_sVersion = "1.3.4";


QuoCollector::QuoCollector()
 : m_pCbDataHandle( NULL )
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
	m_oQuotationData.Release();
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
	nStrLen = ::sprintf( pszStatusDesc, "模块名=转码机,转码机版本=V%s,运行时间=%s,资讯落盘时间=%s,权息落盘时间=%s,行情API版本=%s,[市场信息]"
		, g_sVersion.c_str(), sProgramDuration.c_str(), pszFinancialFileTime, pszWeightFileTime, m_oQuotationData.QuoteApiVersion().c_str() );

	///< 各市场信息
	T_SECURITY_STATUS&	refSHL1Snap = refSvrStatus.FetchSecurity( XDF_SH );
	T_SECURITY_STATUS&	refSHOPTSnap = refSvrStatus.FetchSecurity( XDF_SHOPT );
	T_SECURITY_STATUS&	refSZL1Snap = refSvrStatus.FetchSecurity( XDF_SZ );
	T_SECURITY_STATUS&	refSZOPTSnap = refSvrStatus.FetchSecurity( XDF_SZOPT );
	T_SECURITY_STATUS&	refSHCFFSnap = refSvrStatus.FetchSecurity( XDF_CF );
	T_SECURITY_STATUS&	refSHCFFOPTSnap = refSvrStatus.FetchSecurity( XDF_ZJOPT );
	T_SECURITY_STATUS&	refCNFSnap = refSvrStatus.FetchSecurity( XDF_CNF );
	T_SECURITY_STATUS&	refCNFOPTSnap = refSvrStatus.FetchSecurity( XDF_CNFOPT );
	double				dSHL1Rate = refSvrStatus.FetchMkOccupancyRate( XDF_SH );
	double				dSHOPTRate = refSvrStatus.FetchMkOccupancyRate( XDF_SHOPT );
	double				dSZL1Rate = refSvrStatus.FetchMkOccupancyRate( XDF_SZ );
	double				dSZOPTRate = refSvrStatus.FetchMkOccupancyRate( XDF_SZOPT );
	double				dCFFRate = refSvrStatus.FetchMkOccupancyRate( XDF_CF );
	double				dCFFOPTRate = refSvrStatus.FetchMkOccupancyRate( XDF_ZJOPT );
	double				dCNFRate = refSvrStatus.FetchMkOccupancyRate( XDF_CNF );
	double				dCNFOPTRate = refSvrStatus.FetchMkOccupancyRate( XDF_CNFOPT );

	if( refSvrStatus.FetchMkDate( XDF_SH ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",上海L1日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_SH ), refSvrStatus.FetchMkTime( XDF_SH ), refSvrStatus.GetMkStatus(XDF_SH), dSHL1Rate );
	}
	if( refSvrStatus.FetchMkDate( XDF_SHOPT ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",上海期权日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_SHOPT ), refSvrStatus.FetchMkTime( XDF_SHOPT ), refSvrStatus.GetMkStatus(XDF_SHOPT), dSHOPTRate );
	}
	if( refSvrStatus.FetchMkDate( XDF_SZ ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",深圳L1日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_SZ ), refSvrStatus.FetchMkTime( XDF_SZ ), refSvrStatus.GetMkStatus(XDF_SZ), dSZL1Rate );
	}
	if( refSvrStatus.FetchMkDate( XDF_SZOPT ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",深圳期权日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_SZOPT ), refSvrStatus.FetchMkTime( XDF_SZOPT ), refSvrStatus.GetMkStatus(XDF_SZOPT), dSZOPTRate );
	}
	if( refSvrStatus.FetchMkDate( XDF_CF ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",中金期货日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_CF ), refSvrStatus.FetchMkTime( XDF_CF ), refSvrStatus.GetMkStatus(XDF_CF), dCFFRate );
	}
	if( refSvrStatus.FetchMkDate( XDF_ZJOPT ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",中金期权日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_ZJOPT ), refSvrStatus.FetchMkTime( XDF_ZJOPT ), refSvrStatus.GetMkStatus(XDF_ZJOPT), dCFFOPTRate );
	}
	if( refSvrStatus.FetchMkDate( XDF_CNF ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",商品期货日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_CNF ), refSvrStatus.FetchMkTime( XDF_CNF ), refSvrStatus.GetMkStatus(XDF_CNF), dCNFRate );
	}
	if( refSvrStatus.FetchMkDate( XDF_CNFOPT ) > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",商品期权日期=%u,行情时间=%u,行情状态=%s,缓存应用率=%0.3f"
			, refSvrStatus.FetchMkDate( XDF_CNFOPT ), refSvrStatus.FetchMkTime( XDF_CNFOPT ), refSvrStatus.GetMkStatus(XDF_CNFOPT), dCNFOPTRate );
	}

	///< 各市场行情信息
	nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[行情信息]" );
	if( refSHL1Snap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SHL1 %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSHL1Snap.Name, refSHL1Snap.Code, refSHL1Snap.LastPx, refSHL1Snap.Amount, refSHL1Snap.Volume );
	}
	if( refSHOPTSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SH OPTION %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSHOPTSnap.Name, refSHOPTSnap.Code, refSHOPTSnap.LastPx, refSHOPTSnap.Amount, refSHOPTSnap.Volume );
	}
	if( refSZL1Snap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SZL1 %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSZL1Snap.Name, refSZL1Snap.Code, refSZL1Snap.LastPx, refSZL1Snap.Amount, refSZL1Snap.Volume );
	}
	if( refSZOPTSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SZ OPTION %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSZOPTSnap.Name, refSZOPTSnap.Code, refSZOPTSnap.LastPx, refSZOPTSnap.Amount, refSZOPTSnap.Volume );
	}
	if( refSHCFFSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CFF %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSHCFFSnap.Name, refSHCFFSnap.Code, refSHCFFSnap.LastPx, refSHCFFSnap.Amount, refSHCFFSnap.Volume );
	}
	if( refSHCFFOPTSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CFF OPTION %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refSHCFFOPTSnap.Name, refSHCFFOPTSnap.Code, refSHCFFOPTSnap.LastPx, refSHCFFOPTSnap.Amount, refSHCFFOPTSnap.Volume );
	}
	if( refCNFSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CNF %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refCNFSnap.Name, refCNFSnap.Code, refCNFSnap.LastPx, refCNFSnap.Amount, refCNFSnap.Volume );
	}
	if( refSHCFFSnap.Volume > 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CNF OPTION %s],代码=%s,现价=%.4f,金额=%.4f,量=%I64d", refCNFOPTSnap.Name, refCNFOPTSnap.Code, refCNFOPTSnap.LastPx, refCNFOPTSnap.Amount, refCNFOPTSnap.Volume );
	}

	return refStatus;
}


extern "C"
{
	__declspec(dllexport) int __stdcall	Initialize( I_DataHandle* pIDataHandle )
	{
		return QuoCollector::GetCollector().Initialize( pIDataHandle );
	}

	__declspec(dllexport) void __stdcall Release()
	{
		QuoCollector::GetCollector().Release();
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




