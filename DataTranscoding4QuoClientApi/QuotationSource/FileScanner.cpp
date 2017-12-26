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
	unsigned int				nSec = 0;
	int							nErrCode = 0;

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Initialize() : ............ FileScanner Is Activating............" );
	Release();
/*
	if( (nErrCode = m_oQuoDataCenter.Initialize( this )) < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize DataCenter, errorcode = %d", nErrCode );
		Release();
		return -1;
	}

	if( (nErrCode = m_oQuotPlugin.Initialize( this )) < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize Quotation Plugin, errorcode = %d", nErrCode );
		Release();
		return -2;
	}

	if( (nErrCode = m_oQuotPlugin.RecoverDataCollector()) < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 activate Quotation Plugin, errorcode = %d", nErrCode );
		Release();
		return -3;
	}
*/

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Initialize() : ............ FileScanner Activated!.............." );

	return 0;
}

int FileScanner::Release()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Release() : ............ Destroying .............." );

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "FileScanner::Release() : ............ Destroyed! .............." );

	return 0;
}







