#ifndef __DATA_1MINUTE_H__
#define __DATA_1MINUTE_H__


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
	 * @param[in]				nPreClose				昨收
	 * @return					0						成功
	 */
	int							Update( T_DATA& objData, int nPreClose );

	/**
	 * @brief					生成分钟线并存盘
	 * @param[in]				bMarketClosed			市场闭市标识（true，需要落盘余下的全部1分钟线）
	 */
	void						DumpMinutes( bool bMarketClosed );
protected:
	double						m_dAmountBefore930;		///< 9:30前的金额
	unsigned __int64			m_nVolumeBefore930;		///< 9:30前的量
	unsigned __int64			m_nNumTradesBefore930;	///< 9:30前的笔数
	int							m_nPreClose;			///< 昨收
protected:
	double						m_dPriceRate;			///< 放大倍数
	enum XDFMarket				m_eMarket;				///< 市场编号
	unsigned int				m_nDate;				///< YYYYMMDD（如20170705）
	char						m_pszCode[9];			///< 商品代码
protected:
	T_DATA*						m_pDataCache;			///< 241根1分钟缓存
	short						m_nBeginOffset;			///< 第一个真实写入数据的索引(如果盘中才启动，那之前的分钟线不应该落盘，不然为全零记录)
	short						m_nWriteSize;			///< 已经落过盘的分钟线位置(避免重复落盘)
	short						m_nDataSize;			///< 数据长度
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




#endif










