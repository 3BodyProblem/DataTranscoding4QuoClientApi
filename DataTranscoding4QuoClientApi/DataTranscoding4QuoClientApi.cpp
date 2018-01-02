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
	nStrLen = ::sprintf( pszStatusDesc, "模块名=转码机,转码机版本=V%s,运行时间=%s,资讯落盘时间=%s,权息落盘时间=%s,行情API版本=%s"
		, g_sVersion.c_str(), sProgramDuration.c_str(), pszFinancialFileTime, pszWeightFileTime, m_oQuotationData.QuoteApiVersion().c_str() );

	///< 各市场信息

	///< 各市场商品行情 & 缓存使用信息
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

	if( dSHL1Rate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SHL1],缓存占用率=%.4f", dSHL1Rate );
	}
	if( dSHOPTRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SH OPTION],缓存占用率=%.4f", dSHOPTRate );
	}
	if( dSZL1Rate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SZL1],缓存占用率=%.4f", dSZL1Rate );
	}
	if( dSZOPTRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[SZ OPTION],缓存占用率=%.4f", dSZOPTRate );
	}
	if( dCFFRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CFF],缓存占用率=%.4f", dCFFRate );
	}
	if( dCFFOPTRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CFF OPTION],缓存占用率=%.4f", dCFFOPTRate );
	}
	if( dCNFRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CNF],缓存占用率=%.4f", dCNFRate );
	}
	if( dCNFOPTRate >= 0 )	{
		nStrLen += ::sprintf( pszStatusDesc + nStrLen, ",[CNF OPTION],缓存占用率=%.4f", dCNFOPTRate );
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




