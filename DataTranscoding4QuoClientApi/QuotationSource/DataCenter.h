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
 * @class						T_DAY_LINE
 * @brief						日线
 */
typedef struct
{
	unsigned char				Valid;			///< 是否有效数据, 1:有效，需要落盘 0:无效或已落盘
	unsigned char				Type;			///< 类型
	char						eMarketID;		///< 市场ID
	char						Code[16];		///< 商品代码
	unsigned int				Date;			///< YYYYMMDD（如20170705）
	unsigned int				Time;			///< HHMMSSmmm（如093005000)
	double						PreClosePx;		///< 昨收价
	double						PreSettlePx;	///< 昨结价
	double						OpenPx;			///< 开盘价
	double						HighPx;			///< 最高价
	double						LowPx;			///< 最低价
	double						ClosePx;		///< 收盘价
	double						NowPx;			///< 现价
	double						SettlePx;		///< 结算价
	double						UpperPx;		///< 涨停价
	double						LowerPx;		///< 跌停价
	double						Amount;			///< 成交额
	unsigned __int64			Volume;			///< 成交量(股/张/份)
	unsigned __int64			OpenInterest;	///< 持仓量(股/张/份)
	unsigned __int64			NumTrades;		///< 成交笔数
	double						BidPx1;			///< 委托买盘一价格
	unsigned __int64			BidVol1;		///< 委托买盘一量(股/张/份)
	double						BidPx2;			///< 委托买盘二价格
	unsigned __int64			BidVol2;		///< 委托买盘二量(股/张/份)
	double						AskPx1;			///< 委托卖盘一价格
	unsigned __int64			AskVol1;		///< 委托卖盘一量(股/张/份)
	double						AskPx2;			///< 委托卖盘二价格
	unsigned __int64			AskVol2;		///< 委托卖盘二量(股/张/份)
	double						Voip;			///< 基金模净、权证溢价
	char						TradingPhaseCode[12];///< 交易状态
	char						PreName[8];		///< 证券前缀
} T_DAY_LINE;

/**
 * @class						T_LINE_PARAM
 * @brief						基础参数s
 */
typedef struct
{
	unsigned char				Type;			///< 类型
	double						dPriceRate;		///< 放大倍数
} T_LINE_PARAM;
#pragma pack()


typedef MLoopBufferSt<T_DAY_LINE>				T_DAYLINE_CACHE;			///< 循环队列缓存
typedef	std::map<enum XDFMarket,int>			TMAP_MKID2STATUS;			///< 各市场模块状态
const	unsigned int							MAX_WRITER_NUM = 128;		///< 最大落盘文件句柄


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
	 * @brief					为一个商品代码分配缓存对应的专用缓存
	 * @param[in]				eMkID			市场ID
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


typedef	std::pair<T_LINE_PARAM,T_DAYLINE_CACHE>	T_QUO_DATA;	///< 数据缓存
typedef std::map<std::string,T_QUO_DATA>		T_MAP_QUO;	///< 行情数据缓存


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
	int							Initialize();

	/**
	 * @brief					释放资源
	 */
	void						Release();

public:
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
	 * @brief					为就绪的市场启动落盘线程
	 */
	void						BeginDumpThread( enum XDFMarket eMarket, int nStatus );

public:
	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @param[in]				refParam		行情参数
	 * @return					==0				成功
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam );

	/**
	 * @brief					更新日线信息
	 * @param[in]				refDayLine		日线信息
	 * @param[in]				pSnapData		快照指针
	 * @param[in]				nSnapSize		快照大小
	 * @return					==0				成功
	 */
	int							UpdateDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize );

protected:///< 日线落盘线程
	static void*	__stdcall	ThreadDumpDayLine1( void* pSelf );

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< 模块状态表
	CriticalObject				m_oLock;						///< 临界区对象
	static unsigned int			s_nDumpCount;					///< 落盘次数统计
protected:
	T_MAP_QUO					m_mapSHL1;						///< 上证L1
	T_MAP_QUO					m_mapSHOPT;						///< 上证期权
	T_MAP_QUO					m_mapSZL1;						///< 深证L1
	T_MAP_QUO					m_mapSZOPT;						///< 深证期权
	T_MAP_QUO					m_mapCFF;						///< 中金期货
	T_MAP_QUO					m_mapCFFOPT;					///< 中金期权
	T_MAP_QUO					m_mapCNF;						///< 商品期货(上海/郑州/大连)
	T_MAP_QUO					m_mapCNFOPT;					///< 商品期权(上海/郑州/大连)
protected:
	SimpleThread				m_oThdDump;						///< 上证L1
};




#endif










