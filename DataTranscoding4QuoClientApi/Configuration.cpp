#include <string>
#include <algorithm>
#include "Configuration.h"
#include "Infrastructure/IniFile.h"
#include "DataTranscoding4QuoClientApi.h"


char*	__BasePath(char *in)
{
	if( !in )
		return NULL;

	int	len = strlen(in);
	for( int i = len-1; i >= 0; i-- )
	{
		if( in[i] == '\\' || in[i] == '/' )
		{
			in[i + 1] = 0;
			break;
		}
	}

	return in;
}


std::string GetModulePath( void* hModule )
{
	char					szPath[MAX_PATH] = { 0 };
#ifndef LINUXCODE
		int	iRet = ::GetModuleFileName( (HMODULE)hModule, szPath, MAX_PATH );
		if( iRet <= 0 )	{
			return "";
		} else {
			return __BasePath( szPath );
		}
#else
		if( !hModule ) {
			int iRet =  readlink( "/proc/self/exe", szPath, MAX_PATH );
			if( iRet <= 0 ) {
				return "";
			} else {
				return __BasePath( szPath );
			}
		} else {
			class MDll	*pModule = (class MDll *)hModule;
			strncpy( szPath, pModule->GetDllSelfPath(), sizeof(szPath) );
			if( strlen(szPath) == 0 ) {
				return "";
			} else {
				return __BasePath(szPath);
			}
		}
#endif
}


HMODULE						g_oModule;


Configuration::Configuration()
 : m_bBroadcastModel( false ), m_nBcBeginTime( 0 )
{
	CLOSE_CFG		objDefaultCfg = { 0, 160100 };

	m_mapMkCloseCfg[XDF_SH].push_back( objDefaultCfg );
//	m_mapMkCloseCfg[XDF_SHL2].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_SHOPT].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_SZ].push_back( objDefaultCfg );
//	m_mapMkCloseCfg[XDF_SZL2].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_SZOPT].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_CF].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_ZJOPT].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_CNF].push_back( objDefaultCfg );
	m_mapMkCloseCfg[XDF_CNFOPT].push_back( objDefaultCfg );
}

Configuration& Configuration::GetConfig()
{
	static Configuration	obj;

	return obj;
}

int Configuration::Initialize()
{
	std::string			sPath;
	inifile::IniFile	oIniFile;
	int					nErrCode = 0;
    char				pszTmp[1024] = { 0 };

    ::GetModuleFileName( g_oModule, pszTmp, sizeof(pszTmp) );
    sPath = pszTmp;
    sPath = sPath.substr( 0, sPath.find(".dll") ) + ".ini";
	if( 0 != (nErrCode=oIniFile.load( sPath )) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::Initialize() : configuration file isn\'t exist. [%s], errorcode=%d", sPath.c_str(), nErrCode );
		return -1;
	}

	///< 设置： 快照落盘目录(含文件名)
	m_sDumpFileFolder = oIniFile.getStringValue( std::string("SRV"), std::string("DumpFolder"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : shutdown dump function." );
	}

	m_sQuoPluginPath = oIniFile.getStringValue( std::string("SRV"), std::string("QuoPlugin"), nErrCode );
	if( 0 != nErrCode )	{
		m_sQuoPluginPath = "./QuoteClientApi.dll";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default quotation plugin path: %s", m_sQuoPluginPath.c_str() );
	}

	std::string	sBroadCastModel = oIniFile.getStringValue( std::string("SRV"), std::string("BroadcastModel"), nErrCode );
	if( 0 == nErrCode )	{
		if( sBroadCastModel == "1" )
		{
			m_bBroadcastModel = true;
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : ... Enter [Broadcase Model] ... !!! " );
		}
	}

	if( true == m_bBroadcastModel )
	{
		m_sBcTradeFile = oIniFile.getStringValue( std::string("SRV"), std::string("BroadcastTradeFile"), nErrCode );
		if( 0 != nErrCode )	{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : invalid broadcast (trade) file." );
		}

		m_sBcQuotationFile = oIniFile.getStringValue( std::string("SRV"), std::string("BroadcastQuotationFile"), nErrCode );
		if( 0 != nErrCode )	{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : invalid broadcast (quotation) file." );
		}

		m_nBcBeginTime = oIniFile.getIntValue( std::string("SRV"), std::string("BroadcastBeginTime"), nErrCode );
		if( 0 != nErrCode )	{
			m_nBcBeginTime = 0xffffffff;
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : Topspeed Mode...!" );
		}
	}

	m_sFinancialFolder = oIniFile.getStringValue( std::string("DataSource"), std::string("FinancialData"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : financial file isn\'t exist..." );
	}

	m_sWeightFolder = oIniFile.getStringValue( std::string("DataSource"), std::string("Weight"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : weight file isn\'t exist..." );
	}

	return 0;
}

const std::string& Configuration::GetDataCollectorPluginPath() const
{
	return m_sQuoPluginPath;
}

const std::string& Configuration::GetDumpFolder() const
{
	return m_sDumpFileFolder;
}

std::string Configuration::GetFinancialDataFolder() const
{
	return m_sFinancialFolder;
}

std::string Configuration::GetWeightFileFolder() const
{
	return m_sWeightFolder;
}

unsigned int Configuration::GetBroadcastBeginTime() const
{
	return m_nBcBeginTime;
}

std::string Configuration::GetTradeFilePath() const
{
	return m_sBcTradeFile;
}

std::string Configuration::GetQuotationFilePath() const
{
	return m_sBcQuotationFile;
}

bool Configuration::IsBroadcastModel() const
{
	return m_bBroadcastModel;
}

MAP_MK_CLOSECFG& Configuration::GetMkCloseCfg()
{
	return m_mapMkCloseCfg;
}








