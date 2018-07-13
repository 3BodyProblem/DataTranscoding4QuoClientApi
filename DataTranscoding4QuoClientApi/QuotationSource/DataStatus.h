#ifndef __DATA_STATUS_H__
#define __DATA_STATUS_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Infrastructure/Lock.h"


///< 沪、深股票类停牌标记表 //////////////////////////////////////////////////////////////////////////////////
/**
 * @class						SecurityStatus
 * @brief						商品状态表(停牌标记)
 * @detail						停牌标记描述：
									1) 上海停牌标识定义：
									 ' '=正常
									 *=停牌

									2) 深圳停牌标识定义：
									 '.'=正常
									'Y'=常天停牌 
									'H'=暂停交易   
									'N'=上市首日 
									'M'=恢复上市首日
									 'J'=公司再融资 
									'X'=除权除息 
									'V'=网络投票 
									'A'=即可融资也可融券 
									'C'=仅可融资 
									'S'=仅可融券 
									'W'=处于要约收购期 
									'I'=当前证券处于发行期间
 */
class SecurityStatus
{
public:
	typedef std::map<std::string,unsigned char>		T_CODE_SFlag_Table;	///< 停牌标记表
public:
	SecurityStatus();

	/**
	 * @brief					获取所有商品的停牌标记符
	 * @param[in]				eMkID				市场编号
	 * @param[in]				pQuoteQueryApi		查询接口
	 * @param[in]				nWareCount			商品总数
	 * @return					返回获取的数量；-1表示失败
	 */
	int							FetchAllSFlag( enum XDFMarket eMkID, QuotePrimeApi* pQuoteQueryApi, unsigned int nWareCount );

	/**
	 * @brief					是否为全天停牌
	 * @param[in]				sCode				商品代码
	 * @return					1					是全天停牌
								0					不是全天停牌
								-1					商品代码不存在
								<-1					其它错误
	 */
	int							IsStop( std::string sCode );

protected:
	enum XDFMarket				m_eMarketID;			///< 市场编号
	T_CODE_SFlag_Table			m_mapSFlagTable;		///< 股票停牌标记表
	CriticalObject				m_oLockData;			///< 状态表数据锁
};




#endif










