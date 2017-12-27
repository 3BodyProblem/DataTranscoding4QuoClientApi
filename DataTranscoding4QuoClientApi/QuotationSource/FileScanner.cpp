#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "FileScanner.h"
#include "Financial11File.h"
#include "../Infrastructure/File.h"
#include "../DataTranscoding4QuoClientApi.h"


FileScanner::FileScanner()
{
}

FileScanner::~FileScanner()
{
	Release();
}

int FileScanner::Initialize()
{
	int							nErrCode = 0;

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Initialize() : ............ FileScanner Is Activating............" );
	Release();

	if( (nErrCode = SimpleTask::Activate()) < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "FileScanner::Initialize() : failed 2 initialize file scanner obj, errorcode = %d", nErrCode );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Initialize() : ............ FileScanner Activated!.............." );

	return 0;
}

int FileScanner::Release()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Release() : ............ Destroying .............." );

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Release() : ............ Destroyed! .............." );

	return 0;
}

int FileScanner::Execute()
{
	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000*10 );

			ResaveWeightFile();					///< 解析转存权息文件
			ResaveFinancialFile();				///< 解析转存财经数据文件
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "FileScanner::Execute() : an exception occur in Execute() : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "FileScanner::Execute() : unknow exception throwed" );
		}
	}

	return NULL;
}

__inline bool PrepareFinancialDumper( enum XDFMarket eMkID, std::ofstream& oDumper )
{
	std::string				sFilePath;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };

	switch( eMkID )
	{
	case XDF_SH:		///< 上海Lv1
		::sprintf( pszFilePath, "SSE/%d/", DateTime::Now().DateToLong() );
		break;
	case XDF_SZ:		///< 深证Lv1
		::sprintf( pszFilePath, "SZSE/%d/", DateTime::Now().DateToLong() );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "FileScanner::PrepareFinancialFile() : invalid market id (%s)", eMkID );
		return false;
	}

	sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	::sprintf( pszFileName, "Financial%u.csv", DateTime::Now().TimeToLong() );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "FileScanner::PrepareFinancialFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,code,name,lotsize,contractmult,contractunit,startdate,enddate,xqdate,deliverydate,expiredate,underlyingcode,underlyingname,optiontype,callorput,exercisepx\n";
		oDumper << sTitle;
	}

	return true;
}

void FileScanner::ResaveFinancialFile()
{
	std::ofstream		oSHFinancialDumper;
	std::ofstream		oSZFinancialDumper;
	enum XDFMarket		arrayMkID[2] = { XDF_SH, XDF_SZ };
	std::string&		sFolder = Configuration::GetConfig().GetFinancialDataFolder();

	if( true == PrepareFinancialDumper( XDF_SH, oSHFinancialDumper ) )		///< 上海财经数据
	{
		//ReadDbfFile		objDbfSHL1;

	}

	if( true == PrepareFinancialDumper( XDF_SZ, oSZFinancialDumper ) )		///< 深圳财经数据
	{
		//ReadDbfFile		objDbfSZL1;

	}
}

void FileScanner::ResaveWeightFile()
{
	enum XDFMarket		arrayMkID[2] = { XDF_SH, XDF_SZ };
	std::string&		sFolder = Configuration::GetConfig().GetWeightFileFolder();

}






