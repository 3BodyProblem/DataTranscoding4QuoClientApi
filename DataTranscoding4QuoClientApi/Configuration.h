#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "Infrastructure/Lock.h"
#include "Infrastructure/IniFile.h"


extern	HMODULE					g_oModule;							///< 当前dll模块变量
extern	std::string				GetModulePath( void* hModule );		///< 获取当前模块路径


/**
 * @class						MkCfgWriter
 * @brief						市场配置写入类
 * @author						barry
 */
class MkCfgWriter
{
public:
	MkCfgWriter( std::string sSubFolder );

	/**
	 * @brief					更新新连接配置到指定的市场行情配置文件中
	 * @param[in]				nIndication							配置项指向标识号
	 * @param[in]				sIP									行情地址
	 * @param[in]				nPort								行情端口
	 * @param[in]				sAccountID							用户帐号
	 * @param[in]				sPswd								用户密码
	 */
	virtual bool				ConfigurateConnection4Mk( unsigned int nIndication, std::string sIP, unsigned short nPort, std::string sAccountID, std::string sPswd );

protected:
	std::string					m_sCfgFolder;						///< 市场驱动所在目录路径
};


/**
 * @class						T_CLOSE_CFG
 * @brief						收盘配置信息
 * @author						barry
 */
typedef struct
{
	unsigned int						LastDate;					///< 最后落盘日期
	unsigned int						CloseTime;					///< 收盘时间
} CLOSE_CFG;


typedef std::vector<CLOSE_CFG>			TB_MK_CLOSECFG;				///< 市场收盘配置表
typedef std::map<short,TB_MK_CLOSECFG>	MAP_MK_CLOSECFG;			///< 各市场收盘配置


/**
 * @class						Configuration
 * @brief						配置信息
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					获取配置对象的单键引用对象
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					加载配置项
	 * @return					==0					成功
								!=					出错
	 */
    int							Initialize();

public:
	/**
	 * @brief					取得快照落盘目录(含文件名)
	 */
	const std::string&			GetDumpFolder() const;

	/**
	 * @brief					获取插件加载路径(含文件名)
	 */
	const std::string&			GetDataCollectorPluginPath() const;

	/**
	 * @brief					获取市场收盘信息配置
	 */
	MAP_MK_CLOSECFG&			GetMkCloseCfg();

	/**
	 * @brief					获取请求文件的路径
	 */
	std::string					GetFinancialDataFolder() const;

	/**
	 * @brief					获取行情文件的路径
	 */
	std::string					GetWeightFileFolder() const;

protected:
	/**
	 * @brief					解析各市场的行情配置并转存到对应目录中
	 * @param[in]				refIniFile					配置文件
	 */
	void						ParseAndSaveMkConfig( inifile::IniFile& refIniFile );

protected:
	std::vector<std::string>	m_vctMkNameCfg;				///< 市场名称列表

private:
	std::string					m_sFinancialFolder;			///< 财经文件路径
	std::string					m_sWeightFolder;			///< 权息文件路径
	std::string					m_sQuoPluginPath;			///< 行情插件路径
	std::string					m_sDumpFileFolder;			///< 快照落盘路径(需要有文件名)
	MAP_MK_CLOSECFG				m_mapMkCloseCfg;			///< 市场收盘时间配置
};





#endif








