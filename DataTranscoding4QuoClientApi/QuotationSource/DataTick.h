#ifndef __DATA_TICK_H__
#define __DATA_TICK_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


///< 数据结构定义 //////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
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
#pragma pack()


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


#endif










