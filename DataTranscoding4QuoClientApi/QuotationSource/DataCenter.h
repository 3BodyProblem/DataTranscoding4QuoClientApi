#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


#pragma pack(1)
/**
 * @class						T_DAY_LINE
 * @brief						日线
 */
typedef struct
{
	char						eMarketID;		///< 市场ID
	char						Code[16];		///< 商品代码
	unsigned int				Date;			///< 日期(YYYYMMDD)
	double						OpenPx;			///< 开盘价
	double						HighPx;			///< 最高价
	double						LowPx;			///< 最低价
	double						ClosePx;		///< 收盘价
	double						SettlePx;		///< 结算价
	double						Amount;			///< 成交额
	unsigned __int64			Volume;			///< 成交量
	unsigned __int64			OpenInterest;	///< 持仓量
	unsigned __int64			NumTrades;		///< 成交笔数
	double						Voip;			///< 基金模净、权证溢价
} T_DAY_LINE;
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
	char*						GrabCache( enum XDFMarket eMkID );

	/**
	 * @brief					释放缓存
	 */
	void						FreeCaches();

private:
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
	void						UpdateModuleStatus( enum XDFMarket nMarket, int nStatus );

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< 模块状态表
	std::ofstream				m_vctDataDump[MAX_WRITER_NUM];	///< 落盘文件句柄数组
};



#endif















