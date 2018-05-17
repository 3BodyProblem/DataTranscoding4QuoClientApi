#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


#pragma pack(1)
/**
 * @class			T_MIN_LINE
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

/**
 * @class						T_TICK_LINE
 * @brief						Tick线
 */
typedef struct
{
	unsigned char				Type;					///< 类型
	char						eMarketID;				///< 市场ID
	char						Code[16];				///< 商品代码
	unsigned int				Date;					///< YYYYMMDD（如20170705）
	unsigned int				Time;					///< HHMMSSmmm（如093005000)
	double						PreClosePx;				///< 昨收价
	double						PreSettlePx;			///< 昨结价
	double						OpenPx;					///< 开盘价
	double						HighPx;					///< 最高价
	double						LowPx;					///< 最低价
	double						ClosePx;				///< 收盘价
	double						NowPx;					///< 现价
	double						SettlePx;				///< 结算价
	double						UpperPx;				///< 涨停价
	double						LowerPx;				///< 跌停价
	double						Amount;					///< 成交额
	unsigned __int64			Volume;					///< 成交量(股/张/份)
	unsigned __int64			OpenInterest;			///< 持仓量(股/张/份)
	unsigned __int64			NumTrades;				///< 成交笔数
	double						BidPx1;					///< 委托买盘一价格
	unsigned __int64			BidVol1;				///< 委托买盘一量(股/张/份)
	double						BidPx2;					///< 委托买盘二价格
	unsigned __int64			BidVol2;				///< 委托买盘二量(股/张/份)
	double						AskPx1;					///< 委托卖盘一价格
	unsigned __int64			AskVol1;				///< 委托卖盘一量(股/张/份)
	double						AskPx2;					///< 委托卖盘二价格
	unsigned __int64			AskVol2;				///< 委托卖盘二量(股/张/份)
	double						Voip;					///< 基金模净、权证溢价
	char						TradingPhaseCode[12];	///< 交易状态
	char						PreName[8];				///< 证券前缀
} T_TICK_LINE;

/**
 * @class						T_MIN_LINE
 * @brief						分钟线
 */
typedef struct
{
	unsigned char				Type;					///< 类型
	char						eMarketID;				///< 市场ID
	char						Code[16];				///< 商品代码
	unsigned int				Date;					///< YYYYMMDD（如20170705）
	unsigned int				Time;					///< HHMMSSmmm（如093005000)
	double						OpenPx;					///< 开盘价一分钟内的第一笔的nowpx
	double						HighPx;					///< 最高价一分钟内的 最高的highpx
	double						LowPx;					///< 最低价一分钟内的 最低的lowpx
	double						ClosePx;				///< 收盘价一分钟内最后一笔的Nowpx
	double						SettlePx;				///< 结算价一分钟内最后一笔的settlepx
	double						Amount;					///< 成交额一分钟最后笔减去第一笔的amount
	unsigned __int64			Volume;					///< 成交量(股/张/份)一分钟最后笔减去第一笔的volume
	unsigned __int64			OpenInterest;			///< 持仓量(股/张/份)一分钟最后一笔
	unsigned __int64			NumTrades;				///< 成交笔数一分钟最后笔减去第一笔的numtrades
	double						Voip;					///< 基金模净、权证溢价一分钟的最后一笔voip
} T_MIN_LINE;

/**
 * @class						T_LINE_PARAM
 * @brief						基础参数s
 */
typedef struct
{
	///< ---------------------- 参考基础信息 ----------------------------------
	unsigned char				Type;					///< 类型
	unsigned int				UpperPrice;				///< 涨停价
	unsigned int				LowerPrice;				///< 跌停价
	unsigned int				PreClosePx;				///< 昨收
	unsigned int				PreSettlePx;			///< 昨结
	unsigned int				PrePosition;			///< 昨持
	char						PreName[8];				///< 前缀
	double						dPriceRate;				///< 放大倍数
	///< ---------------------- 分钟线统计信息 --------------------------------
	unsigned char				Valid;					///< 是否有效数据, 1:有效，需要落盘 0:无效或已落盘
	unsigned int				MkMinute;				///< 行情时间(精确到分，HHmm)
	unsigned int				MinOpenPx1;				///< 分钟内第一笔最新价
	unsigned int				MinHighPx;				///< 分钟内最高价
	unsigned int				MinLowPx;				///< 分钟内最低价
	unsigned int				MinClosePx;				///< 分钟内最新价
	unsigned int				MinSettlePx;			///< 分钟内最新结算价
	double						MinAmount1;				///< 分钟内第一笔金额
	double						MinAmount2;				///< 分钟内第末笔金额
	unsigned __int64			MinVolume1;				///< 分钟内第一笔成交量
	unsigned __int64			MinVolume2;				///< 分钟内第末笔成交量
	unsigned __int64			MinOpenInterest;		///< 分钟内第末笔持仓量
	unsigned __int64			MinNumTrades1;			///< 分钟内第一笔成交笔数
	unsigned __int64			MinNumTrades2;			///< 分钟内第末笔成交笔数
	unsigned int				MinVoip;				///< 分钟内第末笔Voip
} T_LINE_PARAM;
#pragma pack()


typedef MLoopBufferSt<T_TICK_LINE>						T_TICKLINE_CACHE;			///< Tick线循环队列缓存
typedef MLoopBufferSt<T_MIN_LINE>						T_MINLINE_CACHE;			///< 分钟线循环队列缓存
typedef	std::map<enum XDFMarket,int>					TMAP_MKID2STATUS;			///< 各市场模块状态
const	unsigned int									MAX_WRITER_NUM = 128;		///< 最大落盘文件句柄
typedef std::map<std::string,T_LINE_PARAM>				T_MAP_QUO;					///< 行情数据缓存
extern unsigned int										s_nNumberInSection;			///< 一个市场有可以缓存多少个数据块(可配)


/**
 * @class						CacheAlloc
 * @brief						日线缓存
 */
class CacheAlloc
{
private:
	/**
	 * @brief					构造函数
	 * @param[in]				eMkID			市场ID
	 */
	CacheAlloc();

public:
	~CacheAlloc();

	/**
	 * @brief					获取单键
	 */
	static CacheAlloc&			GetObj();

	/**
	 * @brief					获取缓存地址
	 */
	char*						GetBufferPtr();

	/**
	 * @brief					获取缓存中有效数据的长度
	 */
	unsigned int				GetDataLength();

	/**
	 * @brief					获取缓存最大长度
	 */
	unsigned int				GetMaxBufLength();

	/**
	 * @brief					为一个商品代码分配缓存对应的专用缓存
	 * @param[in]				eMkID			市场ID
	 * @param[out]				nOutSize		输出为这一个商品分配的缓存的大小
	 */
	char*						GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize );

	/**
	 * @brief					释放缓存
	 */
	void						FreeCaches();

private:
	CriticalObject				m_oLock;					///< 临界区对象
	unsigned int				m_nMaxCacheSize;			///< 行情缓存总大小
	unsigned int				m_nAllocateSize;			///< 当前使用的缓存大小
	char*						m_pDataCache;				///< 行情数据缓存地址
};


/**
 * @class						QuotationData
 * @brief						行情数据存储类
 */
class QuotationData
{
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
	 * @param[in]				refParam		行情参数
	 * @return					==0				成功
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam );

	/**
	 * @brief					更新前缀
	 * @param[in]				eMarket			市场ID
	 * @param[in]				pszPreName		前缀内容
	 * @param[in]				nSize			内容长度
	 */
	int							UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize );

	/**
	 * @brief					更新前缀
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @param[in]				refParam		分钟线计算参数
	 * @param[in]				nMkDate			市场日期
	 * @param[in]				nMkTime			市场时间
	 */
	void						DispatchMinuteLine( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam, unsigned int nMkDate, unsigned int nMkTime );

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
	 * @param[in]				pSnapData		快照指针
	 * @param[in]				nSnapSize		快照大小
	 * @param[in]				nTradeDate		交易日期
	 */
	int							DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

protected:
	static void*	__stdcall	ThreadDumpTickLine( void* pSelf );			///< 日线落盘线程
	static void*	__stdcall	ThreadDumpMinuteLine( void* pSelf );		///< 分钟落盘线程
	static void*	__stdcall	ThreadOnIdle( void* pSelf );				///< 空闲线程

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< 模块状态表
	CriticalObject				m_oLock;						///< 临界区对象
	void*						m_pQuotation;					///< 行情对象
protected:
	T_MAP_QUO					m_mapSHL1;						///< 上证L1
	T_MAP_QUO					m_mapSHOPT;						///< 上证期权
	T_MAP_QUO					m_mapSZL1;						///< 深证L1
	T_MAP_QUO					m_mapSZOPT;						///< 深证期权
	T_MAP_QUO					m_mapCFF;						///< 中金期货
	T_MAP_QUO					m_mapCFFOPT;					///< 中金期权
	T_MAP_QUO					m_mapCNF;						///< 商品期货(上海/郑州/大连)
	T_MAP_QUO					m_mapCNFOPT;					///< 商品期权(上海/郑州/大连)
	T_TICKLINE_CACHE			m_arrayTickLine;				///< 全市场tick缓存
	unsigned int				m_lstMkTime[64];				///< 各市场的市场时间
protected:
	unsigned int				m_nMaxMLineBufSize;				///< 分钟线缓存长度
	char*						m_pBuf4MinuteLine;				///< 分钟线缓存地址
	T_MINLINE_CACHE				m_arrayMinuteLine;				///< 分钟线缓存管理对象
	CriticalObject				m_oMinuteLock;					///< 临界区对象
protected:
	SimpleThread				m_oThdTickDump;					///< Tick落盘数据
	SimpleThread				m_oThdMinuteDump;				///< 分钟线落盘数据
	SimpleThread				m_oThdIdle;						///< 空闲线程
};




#endif










