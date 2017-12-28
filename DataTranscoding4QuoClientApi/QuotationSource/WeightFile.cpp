#include "math.h"
#include "./WeightFile.h"
#include "../DataTranscoding4QuoClientApi.h"


__inline bool GetWeightFileCreateTime( std::string sFilePath, SYSTEMTIME& refFileTime )
{
	HANDLE		hFile = NULL;
	FILETIME	ftWrite = { 0 };
	FILETIME	ftCreate = { 0 };
	FILETIME	ftAccess = { 0 };
	OFSTRUCT	ofStruct = { 0 };
	FILETIME	ftLocalTime = { 0 };

	try
	{
		hFile = ::CreateFile( sFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
		if( INVALID_HANDLE_VALUE == hFile )
		{
			::memset( &refFileTime, 0, sizeof(refFileTime) );
			return false;
		}

		::GetFileTime( hFile, &ftCreate, &ftAccess, &ftWrite );		///< ��ȡ�ļ�����/��ȡ/д��ʱ��
		::FileTimeToLocalFileTime( &ftCreate, &ftLocalTime );		///< ת���ļ�ʱ�䵽�����ļ�ʱ��
		::FileTimeToSystemTime( &ftLocalTime, &refFileTime );		///< ת���ļ�����ʱ�䵽ϵͳʱ��
	}
	catch( std::exception& err )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GetMyFileTime() : an exception occur in function : %s", err.what() );
	}
	catch( ... )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GetMyFileTime() : an unknow exception" );
	}

	if( INVALID_HANDLE_VALUE != hFile )
	{
		::CloseHandle( hFile );
		return true;
	}

	return false;
}

__inline bool PrepareWeightlDumper( enum XDFMarket eMkID, std::ofstream& oDumper, SYSTEMTIME& refFileTime )
{
	std::string				sFilePath;
	std::string				sFirstLineOfCSV;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };

	switch( eMkID )
	{
	case XDF_SH:		///< �Ϻ�Lv1
		{
			::sprintf( pszFilePath, "SSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
			sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		}
		break;
	case XDF_SZ:		///< ��֤Lv1
		{
			::sprintf( pszFilePath, "SZSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
			sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : invalid market id (%s)", eMkID );
		return false;
	}

	sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );							///< ��Ŀ¼���������½�
	::sprintf( pszFileName, "Financial%u.csv", DateTime::Now().DateToLong() );
	sFilePath += pszFileName;										///< ƴ�ӳ��ļ�ȫ·��
	GetWeightFileCreateTime( sFilePath.c_str(), refFileTime );		///< ��ȡ�ļ��Ĵ���ʱ��
	if( oDumper.is_open() )		oDumper.close();
	oDumper.open( sFilePath.c_str() , std::ios::out );				///< �����ļ�

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );								///< �ļ�ָ�������ļ�β
	if( 0 == oDumper.tellp() )										///< �ж��ļ��Ƿ�Ϊȫ��
	{
		oDumper << sFirstLineOfCSV;									///< ��д���һ��(Title��)
	}

	return true;
}








