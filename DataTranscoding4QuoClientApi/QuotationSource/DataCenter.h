#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"
#include "Data1Min.h"
#include "DataTick.h"
#include "DataStatus.h"


///< 数据结构定义 //////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
/**
 * @class			T_STATIC_LINE
 * @brief			分钟线
 * @author			barry
 */
typedef struct
{
	unsigned char				Type;					///< 类型
	char						eMarketID;				///< 市场ID
	char						Code[16];				///< 商品代码
	unsigned int				Date;					///< YYYYMMDD（如20170705）
	char						Name[64];				///< 商品名称
	unsigned int				LotSize;				///< 每手数量(股/张/份)
	unsigned int				ContractMult;			///< 合约乘数
	unsigned int				ContractUnit;			///< 合约单位
	unsigned int				StartDate;				///< 首个交易日(YYYYMMDD)
	unsigned int				EndDate;				///< 最后交易日(YYYYMMDD)
	unsigned int				XqDate;					///< 行权日(YYYYMMDD)
	unsigned int				DeliveryDate;			///< 交割日(YYYYMMDD)
	unsigned int				ExpireDate;				///< 到期日(YYYYMMDD)
	char						UnderlyingCode[16];		///< 标的代码
	char						UnderlyingName[64];		///< 标的名称
	char						OptionType;				///< 期权类型：'E'-欧式 'A'-美式
	char						CallOrPut;				///< 认沽认购：'C'认购 'P'认沽
	double						ExercisePx;				///< 行权价格
} T_STATIC_LINE;
#pragma pack()


///< 主类 ////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						QuotationData
 * @brief						行情数据存储类
 */
class QuotationData
{
public:
	typedef	std::map<enum XDFMarket,int>		TMAP_MKID2STATUS;		///< 各市场模块状态
public:
	QuotationData();
	~QuotationData();

	/**
	 * @brief					初始化
	 * @return					==0				成功
								!=				失败
	 */
	int							Initialize( void* pQuotation );

	/**
	 * @brief					释放资源
	 */
	void						Release();

	/**
	 * @brief					更新模块工作状态
	 * @param[in]				cMarket			市场ID
	 * @param[in]				nStatus			状态值
	 */
	void						UpdateModuleStatus( enum XDFMarket eMarket, int nStatus );

	/**
	 * @brief					获取市场模块状态
	 */
	short						GetModuleStatus( enum XDFMarket eMarket );

	/**
	 * @brief					停止线程
	 */
	bool						StopThreads();

	/**
	 * @brief					清理所有市场时间
	 */
	void						ClearAllMkTime();

public:
	/**
	 * @brief					设置新市场时间
	 * @param[in]				eMarket			市场ID
	 * @param[in]				nMkTime			市场时间
	 */
	void						SetMarketTime( enum XDFMarket eMarket, unsigned int nMkTime );

	/**
	 * @brief					更新市场时间
	 * @param[in]				eMarket			市场ID
	 * @param[in]				nMkTime			市场时间
	 */
	void						UpdateMarketTime( enum XDFMarket eMarket, unsigned int nMkTime );

	/**
	 * @brief					获取市场时间
	 * @param[in]				eMarket			市场ID
	 * @return					返回市场时间
	 */
	unsigned int				GetMarketTime( enum XDFMarket eMarket );

	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @return					==0				成功
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen );

	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @return					==0				成功
	 */
	int							BuildSecurity4Min( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData );

	/**
	 * @brief					更新日线信息
	 * @param[in]				pSnapData		快照指针
	 * @param[in]				nSnapSize		快照大小
	 * @param[in]				nTradeDate		交易日期
	 * @return					==0				成功
	 */
	int							UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

	/**
	 * @brief					日线落盘
	 * @param[in]				pSnapData		快照指针e
	 * @param[in]				nSnapSize		快照大小
	 * @param[in]				nTradeDate		交易日期
	 */
	int							DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

	SecurityStatus&				GetSHL1StatusTable();
	SecurityStatus&				GetSZL1StatusTable();
	SecurityMinCache&			GetSHL1Min1Cache();
	SecurityMinCache&			GetSZL1Min1Cache();
	SecurityTickCache&			GetSHL1TickCache();
	SecurityTickCache&			GetSZL1TickCache();

protected:
	static void*	__stdcall	ThreadOnIdle( void* pSelf );				///< 空闲线程

protected:
	SimpleThread				m_oThdIdle;						///< 空闲线程
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< 模块状态表
	CriticalObject				m_oLock;						///< 临界区对象
	void*						m_pQuotation;					///< 行情对象
protected:
	SecurityStatus				m_objStatus4SHL1;				///< 上证L1商品状态
	SecurityStatus				m_objStatus4SZL1;				///< 深证L1商品状态
	SecurityMinCache			m_objMinCache4SHL1;				///< 上证L1分钟线缓存
	SecurityMinCache			m_objMinCache4SZL1;				///< 深证L1分钟线缓存
	SecurityTickCache			m_objTickCache4SHL1;			///< 上证L1TICK线缓存
	SecurityTickCache			m_objTickCache4SZL1;			///< 深证L1TICK线缓存
	unsigned int				m_lstMkTime[64];				///< 各市场的市场时间
};




#endif










