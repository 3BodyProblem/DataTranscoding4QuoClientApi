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
 * @brief						基础参数
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


///< 1分钟线计算并落盘类 ///////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						MinGenerator
 * @brief						商品1分钟线生产者
 * @author						barry
 */
class MinGenerator
{
public:
	typedef struct
	{
		unsigned int			Time;					///< HHMMSSmmm（如093005000)
		unsigned int			OpenPx;
		unsigned int			HighPx;
		unsigned int			LowPx;
		unsigned int			ClosePx;
		unsigned int			Voip;
		double					Amount;
		unsigned __int64		Volume;
		unsigned __int64		NumTrades;
	} T_DATA;											///< 商品快照
	const unsigned int			m_nMaxLineCount;		///< 最多支持的分钟线数量(241根)
public:
	MinGenerator();
	MinGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr );
	MinGenerator&				operator=( const MinGenerator& obj );

	/**
	 * @brief					初始化
	 * @return					0						成功
	 */
	int							Initialize();

	/**
	 * @brief					将行情中的每分钟的第一笔快照，更新到对应的241条分钟线数据槽中
	 * @param[in]				objData					行情快照
	 * @return					0						成功
	 */
	int							Update( T_DATA& objData );

	/**
	 * @brief					生成分钟线并存盘
	 */
	void						DumpMinutes();
protected:
	double						m_dAmountBefore930;		///< 9:30前的金额
	unsigned __int64			m_nVolumeBefore930;		///< 9:30前的量
	unsigned __int64			m_nNumTradesBefore930;	///< 9:30前的笔数
protected:
	double						m_dPriceRate;			///< 放大倍数
	enum XDFMarket				m_eMarket;				///< 市场编号
	unsigned int				m_nDate;				///< YYYYMMDD（如20170705）
	char						m_pszCode[9];			///< 商品代码
protected:
	T_DATA*						m_pDataCache;			///< 241根1分钟缓存
	int							m_nWriteSize;			///< 写入分钟线的长度
	int							m_nDataSize;			///< 数据长度
};


/**
 * @class						SecurityMinCache
 * @brief						所有商品分钟线的缓存
 * @author						barry
 */
class SecurityMinCache
{
public:
	typedef std::map<std::string,MinGenerator>	T_MAP_MINUTES;
public:
	SecurityMinCache();
	~SecurityMinCache();

	/**
	 * @brief					分钟线缓存初始化
	 * @param[in]				nSecurityCount			市场中的商品数量
	 * @return					0						成功
	 */
	int							Initialize( unsigned int nSecurityCount );

	/**
	 * @brief					释放资源
	 */
	void						Release();

public:
	/**
	 * @brief					激活分钟线落盘线程
	 */
	void						ActivateDumper();

	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @param[in]				nDate			行情日期
	 * @param[in]				dPriceRate		放大倍率
	 * @param[in]				objData			行情价格
	 * @return					==0				成功
	 */
	int							NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData );

	/**
	 * @brief					更新商品信息
	 * @param[in]				objData			行情价格
	 * @return					==0				成功
	 */
	int							UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime );
	int							UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime );

protected:
	static void*	__stdcall	DumpThread( void* pSelf );

protected:
	enum XDFMarket				m_eMarketID;			///< 市场编号
	SimpleThread				m_oDumpThread;			///< 分钟线落盘数据
	unsigned int				m_nAlloPos;				///< 缓存已经分配的位置
	unsigned int				m_nSecurityCount;		///< 商品数量
	MinGenerator::T_DATA*		m_pMinDataTable;		///< 分钟线缓存
	T_MAP_MINUTES				m_objMapMinutes;		///< 分钟线Map
	std::vector<std::string>	m_vctCode;				///< CodeInTicks
	CriticalObject				m_oLockData;			///< 分钟线数据锁
};


typedef MLoopBufferSt<T_TICK_LINE>						T_TICKLINE_CACHE;			///< Tick线循环队列缓存
typedef MLoopBufferSt<T_MIN_LINE>						T_MINLINE_CACHE;			///< 分钟线循环队列缓存
typedef	std::map<enum XDFMarket,int>					TMAP_MKID2STATUS;			///< 各市场模块状态
const	unsigned int									MAX_WRITER_NUM = 128;		///< 最大落盘文件句柄
typedef std::map<std::string,T_LINE_PARAM>				T_MAP_GEN_MLINES;			///< 分钟线生成器Map
extern unsigned int										s_nNumberInSection;			///< 一个市场有可以缓存多少个数据块(可配)


///< TICK线计算并落盘类 ///////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						TickGenerator
 * @brief						TICK线生产者
 * @author						barry
 */
class TickGenerator
{
public:
	typedef struct
	{
		unsigned int			Time;					///< HHMMSSmmm（如093005000)
		unsigned int			HighPx;					///< 最高价
		unsigned int			LowPx;					///< 最低价
		unsigned int			ClosePx;				///< 收盘价
		unsigned int			NowPx;					///< 现价
		unsigned int			UpperPx;				///< 涨停价
		unsigned int			LowerPx;				///< 跌停价
		double					Amount;					///< 成交额
		unsigned __int64		Volume;					///< 成交量(股/张/份)
		unsigned __int64		NumTrades;				///< 成交笔数
		unsigned int			BidPx1;					///< 委托买盘一价格
		unsigned __int64		BidVol1;				///< 委托买盘一量(股/张/份)
		unsigned int			BidPx2;					///< 委托买盘二价格
		unsigned __int64		BidVol2;				///< 委托买盘二量(股/张/份)
		unsigned int			AskPx1;					///< 委托卖盘一价格
		unsigned __int64		AskVol1;				///< 委托卖盘一量(股/张/份)
		unsigned int			AskPx2;					///< 委托卖盘二价格
		unsigned __int64		AskVol2;				///< 委托卖盘二量(股/张/份)
		unsigned int			Voip;					///< 基金模净、权证溢价
	} T_DATA;											///< 商品快照
	typedef MLoopBufferSt<T_DATA>	T_TICK_QUEUE;
	static const unsigned int		s_nMaxLineCount;	///< 最多支持的TICK线数量,预留可以保存10分钟TICK的缓存(一分钟20笔快照 * 10分钟)
public:
	TickGenerator();
	TickGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr );
	TickGenerator&				operator=( const TickGenerator& obj );

	/**
	 * @brief					初始化
	 * @return					0						成功
	 */
	int							Initialize();

	/**
	 * @brief					更新快照
	 * @param[in]				objData					行情快照
	 * @param[in]				nPreClosePx				昨收
	 * @param[in]				nOpenPx					开盘价
	 * @return					0						成功
	 */
	int							Update( T_DATA& objData, unsigned int nPreClosePx, unsigned int nOpenPx );

	/** 
	 * @brief					更新前缀
	 */
	void						SetPreName( const char* pszPreName, unsigned int nPreNameLen );

	/**
	 * @brief					获取放大倍数
	 */
	double						GetPriceRate();

	/**
	 * @brief					生成TICK线并存盘
	 */
	void						DumpTicks();

protected:
	double						m_dPriceRate;			///< 放大倍数
	enum XDFMarket				m_eMarket;				///< 市场编号
	unsigned int				m_nDate;				///< YYYYMMDD（如20170705）
	unsigned int				m_nMkTime;				///< 市场时间
	char						m_pszCode[9];			///< 商品代码
	unsigned int				m_nPreClosePx;			///< 昨收价
	unsigned int				m_nOpenPx;				///< 开盘价
	char						m_pszPreName[8];		///< 证券前缀
protected:
	T_TICK_QUEUE				m_objTickQueue;			///< TICK队列
	T_DATA*						m_pDataCache;			///< TICK缓存Buffer
protected:
	FILE*						m_pDumpFile;			///< 落盘文件
	bool						m_bHasTitle;			///< 有文件头
	bool						m_bCloseFile;			///< 是否需要关闭文件
};


/**
 * @class						SecurityTickCache
 * @brief						所有商品分钟线的缓存
 * @author						barry
 */
class SecurityTickCache
{
public:
	typedef std::map<std::string,TickGenerator>	T_MAP_TICKS;
public:
	SecurityTickCache();
	~SecurityTickCache();

	/**
	 * @brief					分钟线缓存初始化
	 * @param[in]				nSecurityCount			市场中的商品数量
	 * @return					0						成功
	 */
	int							Initialize( unsigned int nSecurityCount );

	/**
	 * @brief					释放资源
	 */
	void						Release();

public:
	/**
	 * @brief					激活分钟线落盘线程
	 */
	void						ActivateDumper();

	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @param[in]				nDate			行情日期
	 * @param[in]				dPriceRate		放大倍率
	 * @param[in]				objData			行情价格
	 * @return					==0				成功
	 */
	int							NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen );

	/**
	 * @brief					更新商品信息
	 * @param[in]				objData			行情价格
	 * @return					==0				成功
	 */
	int							UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime );
	int							UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime );

	/**
	 * @brief					更新前缀
	 * @param[in]				pszPreName		前缀内容
	 * @param[in]				nSize			内容长度
	 */
	int							UpdatePreName( std::string& sCode, char* pszPreName, unsigned int nSize );

	/**
	 * @brief					获取放大倍数
	 */
	double						GetPriceRate( std::string& sCode );

protected:
	static void*	__stdcall	DumpThread( void* pSelf );

protected:
	enum XDFMarket				m_eMarketID;			///< 市场编号
	SimpleThread				m_oDumpThread;			///< TICK线落盘数据
	unsigned int				m_nAlloPos;				///< 缓存已经分配的位置
	unsigned int				m_nSecurityCount;		///< 商品数量
	TickGenerator::T_DATA*		m_pTickDataTable;		///< TICK线缓存
	T_MAP_TICKS					m_objMapTicks;			///< TICK线Map
	std::vector<std::string>	m_vctCode;				///< CodeInTicks
	CriticalObject				m_oLockData;			///< TICK线数据锁
};


///< 主类 ////////////////////////////////////////////////////////////////////////////////////////////////////
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

	SecurityMinCache&			GetSHL1Cache();
	SecurityMinCache&			GetSZL1Cache();
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
	SecurityMinCache			m_objMinCache4SHL1;				///< 上证L1分钟线缓存
	SecurityMinCache			m_objMinCache4SZL1;				///< 深证L1分钟线缓存
	SecurityTickCache			m_objTickCache4SHL1;			///< 上证L1TICK线缓存
	SecurityTickCache			m_objTickCache4SZL1;			///< 深证L1TICK线缓存
	unsigned int				m_lstMkTime[64];				///< 各市场的市场时间
};




#endif










