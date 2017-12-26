#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "FileScanner.h"
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
			SimpleThread::Sleep( 1000*3 );


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







