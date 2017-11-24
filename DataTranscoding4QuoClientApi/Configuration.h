#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <string>
#include "Infrastructure/Lock.h"


extern	HMODULE					g_oModule;							///< 当前dll模块变量
extern	std::string				GetModulePath( void* hModule );		///< 获取当前模块路径

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
	 * @brief					是否为播放模式
	 * @return					true				行情自动播放模式
	 */
	bool						IsBroadcastModel() const;

	/**
	 * @brief					获取请求文件的路径
	 */
	std::string					GetTradeFilePath() const;

	/**
	 * @brief					获取行情文件的路径
	 */
	std::string					GetQuotationFilePath() const;

	/**
	 * @brief					获取正常速度播放的开始时间
	 */
	unsigned int				GetBroadcastBeginTime() const;

private:
	std::string					m_sQuoPluginPath;		///< 行情插件路径
	std::string					m_sDumpFileFolder;		///< 快照落盘路径(需要有文件名)
	bool						m_bBroadcastModel;		///< 数据自动播放模式
	std::string					m_sBcTradeFile;			///< 播放的请求文件路径
	std::string					m_sBcQuotationFile;		///< 播放的实时文件路径
	unsigned int				m_nBcBeginTime;			///< 正常速度的播放时间
};


#endif







