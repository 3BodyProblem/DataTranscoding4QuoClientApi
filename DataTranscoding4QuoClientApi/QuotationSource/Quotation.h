#ifndef __CTP_QUOTATION_H__
#define __CTP_QUOTATION_H__


#pragma warning(disable:4786)
#include <set>
#include <map>
#include <string>
#include <stdexcept>
#include "DataDump.h"
#include "DataCollector.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"


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


//typedef			std::map<unsigned int,unsigned int>					T_MAP_RATE;			///< 放大倍数映射表[商品分类,商品放大倍数]
//typedef			std::map<std::string,tagSHL1KindDetail_LF150>		T_MAP_KIND;			///< 根据标的代码映射到对应的分类信息


/**
 * @class			Quotation
 * @brief			会话管理对象
 * @detail			封装了针对商品期货期权各市场的初始化、管理控制等方面的方法
 * @author			barry
 */
class Quotation : public SimpleTask, public QuoteClientSpi
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

public:
	virtual bool __stdcall		XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus );
	virtual void __stdcall		XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes );
	virtual void __stdcall		XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf );
	virtual int	__stdcall		XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam );

public:
	/**
	 * @brief					任务函数(内循环)
	 * @return					==0					成功
								!=0					失败
	 */
	int							Execute();

	/**
	 * @brief					获取会话状态信息
	 */
	WorkStatus&					GetWorkStatus();

private:
	DataCollector				m_oQuotPlugin;			///< 行情插件
	WorkStatus					m_oWorkStatus;			///< 工作状态
//	static T_MAP_RATE			m_mapRate;				///< 各分类的放大倍数
//	static T_MAP_KIND			m_mapKind;				///< 分类信息集合
};




#endif




