#include <string>
#include <algorithm>
#include "Configuration.h"
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


MkCfgWriter::MkCfgWriter( std::string sSubFolder )
 : m_sCfgFolder( sSubFolder )
{
}

bool MkCfgWriter::ConfigurateConnection4Mk( unsigned int nIndication, std::string sIP, unsigned short nPort, std::string sAccountID, std::string sPswd )
{
	inifile::IniFile		oIniFile;
	std::string				sPath = "./";
	char					pszKey[64] = { 0 };
	char					pszPort[64] = { 0 };

	::sprintf( pszPort, "%u", nPort );
	///< 打开某个市场的配置文件
	if( "cff_setting" == m_sCfgFolder )	{					///< 中金期货
		sPath += "cff/tran2ndcff.ini";
	}
	else if( "cffopt_setting" == m_sCfgFolder )	{			///< 中金期权
		sPath += "cffopt/tran2ndcffopt.ini";
	}
	else if( "cnf_setting" == m_sCfgFolder )	{			///< 商品期货
		sPath += "cnf/tran2ndcnf.ini";
	}
	else if( "cnfopt_setting" == m_sCfgFolder )	{			///< 商品期权
		sPath += "cnfopt/tran2ndcnfopt.ini";
	}
	else if( "sh_setting" == m_sCfgFolder )	{				///< 上海现货
		sPath += "sh/tran2ndsh.ini";
	}
	else if( "shopt_setting" == m_sCfgFolder )	{			///< 上海期权
		sPath += "shopt/tran2ndshopt.ini";
	}
	else if( "sz_setting" == m_sCfgFolder )	{				///< 深圳现货
		sPath += "sz/tran2ndsz.ini";
	}
	else if( "szopt_setting" == m_sCfgFolder )	{			///< 深圳期权
		sPath += "szopt/tran2ndszopt.ini";
	}
	else	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : invalid market subfolder name : %s", m_sCfgFolder.c_str() );
		return false;
	}

	///< 判断参数是否有效
	if( sIP == "" || nPort == 0 || sAccountID == "" || sPswd == "" )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN
											, "MkCfgWriter::ConfigurateConnection4Mk() : invalid market paramenters [%s:%u], ip:%s port:%u account:%s pswd:%s"
											, m_sCfgFolder.c_str(), nIndication, sIP.c_str(), nPort, sAccountID.c_str(), sPswd.c_str() );
		return true;
	}

	///< 加载文件 & 判断文件是否存在
	if( 0 != oIniFile.load( sPath.c_str() ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : invalid Mk configuration path = %s", sPath.c_str() );
		return false;
	}

	///< 写入配置信息到目标文件中
	::sprintf( pszKey, "ServerIP_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sIP ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save ip [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "ServerPort_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), std::string(pszPort) ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save port [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "LoginName_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sAccountID ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save account [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "LoginPWD_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sPswd ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save password [%s]", pszKey );
		return false;
	}

	if( 0 != oIniFile.save() )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save Mk configuration, path = %s", sPath.c_str() );
		return false;
	}

	return true;
}


Configuration::Configuration()
{
	///< 配置各市场的收盘时间
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

	///< 配置各市场的传输驱动所在目录名称
	m_vctMkNameCfg.push_back( "cff_setting" );
	m_vctMkNameCfg.push_back( "cffopt_setting" );
	m_vctMkNameCfg.push_back( "cnf_setting" );
	m_vctMkNameCfg.push_back( "cnfopt_setting" );
	m_vctMkNameCfg.push_back( "sh_setting" );
	m_vctMkNameCfg.push_back( "shopt_setting" );
	m_vctMkNameCfg.push_back( "sz_setting" );
	m_vctMkNameCfg.push_back( "szopt_setting" );
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

	///< 每个市场的缓存中，可以缓存的tick数据的数量
	int	nNum4OneMarket = oIniFile.getIntValue( std::string("SRV"), std::string("ItemCountInBuffer"), nErrCode );
	if( nNum4OneMarket > 0 )	{
		s_nNumberInSection = nNum4OneMarket;
	}
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Configuration::Initialize() : Setting Item Number ( = %d) In One Market Buffer ...", s_nNumberInSection );

	///< 设置财经数据 和 权息文件 的配置路径
	m_sFinancialFolder = oIniFile.getStringValue( std::string("DataSource"), std::string("FinancialData"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : financial file isn\'t exist..." );
	}

	m_sWeightFolder = oIniFile.getStringValue( std::string("DataSource"), std::string("Weight"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : weight file isn\'t exist..." );
	}

	///< 转存各市场的配置到对应的文件
	ParseAndSaveMkConfig( oIniFile );

	return 0;
}

void Configuration::ParseAndSaveMkConfig( inifile::IniFile& refIniFile )
{
	for( std::vector<std::string>::iterator it = m_vctMkNameCfg.begin(); it != m_vctMkNameCfg.end(); it++ )
	{
		int					nErrCode = 0;
		std::string&		sMkCfgName = *it;
		char				pszKey[54] = { 0 };
		std::string			sServerIP_0 = refIniFile.getStringValue( sMkCfgName, std::string("ServerIP_0"), nErrCode );
		int					nServerPort_0 = refIniFile.getIntValue( sMkCfgName, std::string("ServerPort_0"), nErrCode );
		std::string			sLoginName_0 = refIniFile.getStringValue( sMkCfgName, std::string("LoginName_0"), nErrCode );
		std::string			sLoginPWD_0 = refIniFile.getStringValue( sMkCfgName, std::string("LoginPWD_0"), nErrCode );
		std::string			sServerIP_1 = refIniFile.getStringValue( sMkCfgName, std::string("ServerIP_1"), nErrCode );
		int					nServerPort_1 = refIniFile.getIntValue( sMkCfgName, std::string("ServerPort_1"), nErrCode );
		std::string			sLoginName_1 = refIniFile.getStringValue( sMkCfgName, std::string("LoginName_1"), nErrCode );
		std::string			sLoginPWD_1 = refIniFile.getStringValue( sMkCfgName, std::string("LoginPWD_1"), nErrCode );
		MkCfgWriter			objCfgWriter( sMkCfgName );

		objCfgWriter.ConfigurateConnection4Mk( 0, sServerIP_0, nServerPort_0, sLoginName_0, sLoginPWD_0 );
		objCfgWriter.ConfigurateConnection4Mk( 1, sServerIP_1, nServerPort_1, sLoginName_1, sLoginPWD_1 );
	}
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

MAP_MK_CLOSECFG& Configuration::GetMkCloseCfg()
{
	return m_mapMkCloseCfg;
}








