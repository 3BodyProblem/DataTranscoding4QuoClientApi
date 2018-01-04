#ifndef __CTP_QUOTATION_H__
#define __CTP_QUOTATION_H__


#pragma warning(disable:4786)
#include <set>
#include <stdexcept>
#include "DataDump.h"
#include "DataCenter.h"
#include "DataCollector.h"


/**
 * @class			WorkStatus
 * @brief			工作状态管理
 * @author			barry
 */
class WorkStatus
{
public:
	/**
	 * @brief				应状态值映射成状态字符串
	 */
	static	std::string&	CastStatusStr( enum E_SS_Status eStatus );

public:
	/**
	 * @brief			构造
	 * @param			eMkID			市场编号
	 */
	WorkStatus();
	WorkStatus( const WorkStatus& refStatus );

	/**
	 * @brief			赋值重载
						每次值变化，将记录日志
	 */
	WorkStatus&			operator= ( enum E_SS_Status eWorkStatus );

	/**
	 * @brief			重载转换符
	 */
	operator			enum E_SS_Status();

private:
	CriticalObject		m_oLock;				///< 临界区对象
	enum E_SS_Status	m_eWorkStatus;			///< 行情工作状态
};


/**
 * @class			Quotation
 * @brief			会话管理对象
 * @detail			封装了针对商品期货期权各市场的初始化、管理控制等方面的方法
 * @author			barry
 */
class Quotation : public QuoteClientSpi
{
public:
	Quotation();
	~Quotation();

	/**
	 * @brief					初始化ctp行情接口
	 * @return					>=0			成功
								<0			错误
	 * @note					整个对象的生命过程中，只会启动时真实的调用一次
	 */
	int							Initialize();

	/**
	 * @brief					释放ctp行情接口
	 */
	int							Release();

	/**
	 * @brief					停止工作
	 */
	void						Halt();

	/**
	 * @brief					获取行情插件版本号
	 */
	std::string&				QuoteApiVersion();

	/**
	 * @brief					获取会话状态信息
	 */
	WorkStatus&					GetWorkStatus();

	/**
	 * @brief					把各市场收盘后快照刷新落盘
	 * @detail					周期性调用这个函数，本函数将以配置的收盘时间信息为标准，进行全市场的收盘操作
	 */
	void						FlushDayLineOnCloseTime();

	/**
	 * @brief					更新获取所有市场的日期和时间
	 */
	void						UpdateMarketsTime();

public:///< 行情接口的回调函数
	virtual bool __stdcall		XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus );
	virtual void __stdcall		XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes );
	virtual void __stdcall		XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf );
	virtual int	__stdcall		XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam );

protected:///< 加载市场行情数据: 落盘 静态数据文件 + tick文件 或者 创建日线文件
	/**
	 * @brief					加载上海Lv1的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveShLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载上海Option的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveShOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载深圳Lv1的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveSzLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载深圳期权的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveSzOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载中金期货的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveCFF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载中金期权的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveCFFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载商品期货的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveCNF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					加载商品期权的基础信息
	 * @param[in]				eStatus		市场模块状态
	 * @param[in]				bBuild		是否为初始过程中:true,创建静态信息文件+tick信息 false,只创建日线信息
	 * @return					==0			成功
								!=0			失败
	 */
	int							SaveCNFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

private:
	CriticalObject				m_oLock;				///< 临界区对象
	WorkStatus					m_vctMkSvrStatus[64];	///< 各市场服务状态
	DataCollector				m_oQuotPlugin;			///< 行情插件
	QuotationData				m_oQuoDataCenter;		///< 行情数据集合
	WorkStatus					m_oWorkStatus;			///< 工作状态
	std::map<int,int>			m_mapMkBuildTimeT;		///< 各市场码表构建时间记录(和当前时间小于3秒的都可以再重入进行初始化)
};




#endif



