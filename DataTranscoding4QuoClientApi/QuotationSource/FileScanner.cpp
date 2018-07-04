#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "FileScanner.h"
#include "WeightFile.h"
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
			SimpleThread::Sleep( 1000*60*3 );

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

void FileScanner::ResaveFinancialFile()
{
	int						nErrCode = 0;			///< 错误码
	SHL1FinancialDbf		objDbfSHL1;				///< 上海财经数据转存对象
	SZL1FinancialDbf		objDbfSZL1;				///< 深圳财经数据转存对象

	if( (nErrCode=objDbfSHL1.Redirect2File()) != 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "FileScanner::ResaveFinancialFile() : an error occur while saving financial data for SHL1, errorcode = %d", nErrCode );
	}

	if( (nErrCode=objDbfSZL1.Redirect2File()) != 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "FileScanner::ResaveFinancialFile() : an error occur while saving financial data for SZL1, errorcode = %d", nErrCode );
	}
}

void FileScanner::ResaveWeightFile()
{
	WeightFile				objWeightFiles;			///< 所以市场的权息文件转存对象

	objWeightFiles.ScanWeightFiles();				///< 开始转存操作

}






