#ifndef __SERVER_STATUS_H__
#define __SERVER_STATUS_H__


#pragma warning(disable:4786)
#include <set>
#include <stdexcept>
#include "../QuoteCltDef.h"


/**
 * @class					T_SECURITY_STATUS
 * @brief					商品状态更新描述
 */
typedef struct
{
	char					Code[32];				///< 商品代码
	char					Name[64];				///< 商品名称
	double					LastPx;					///< 最新价格
	double					Amount;					///< 成交金额
	unsigned __int64		Volume;					///< 成交量
} T_SECURITY_STATUS;


/**
 * @class					ServerStatus
 * @brief					服务状态维护类
 * @author					barry
 */
class ServerStatus
{
private:
	ServerStatus();

public:
	static ServerStatus&	GetStatusObj();

public:///< 商品状态更新方法
	/**
	 * @brief				锚定一个商品（提供代码和名称)
	 * @param[in]			eMkID					市场ID
	 * @param[in]			pszCode					商品代码
	 * @param[in]			pszName					商品名称
	 */
	void					AnchorSecurity( enum XDFMarket eMkID, const char* pszCode, const char* pszName );

	/**
	 * @brief				更新一个商品交易状态
	 * @param[in]			eMkID					市场ID
	 * @param[in]			pszCode					商品代码
	 * @param[in]			dLastPx					最新价格
	 * @param[in]			dAmount					成交金额
	 * @param[in]			nVolume					成交量
	 */
	void					UpdateSecurity( enum XDFMarket eMkID, const char* pszCode, double dLastPx, double dAmount, unsigned __int64 nVolume );

	/**
	 * @brief				获取商品状态引用
	 * @param[in]			eMkID					市场ID
	 * @param[in]			pszCode					商品代码
	 * @return				返回商品状态引用
	 */
	T_SECURITY_STATUS&		FetchSecurity( enum XDFMarket eMkID, const char* pszCode );

public:///< 行情写盘状态方法
	/**
	 * @brief				更新Tick行情写盘状态
	 * @param[in]			bStatus					是否在写盘中
	 */
	void					UpdateTickWritingStatus( bool bStatus );

	/**
	 * @brief				获取Tick行情写盘状态
	 * @return				true					写盘中...
	 */
	bool					FetchTickWritingStatus();

	/**
	 * @brief				更新Minute行情写盘状态
	 * @param[in]			bStatus					是否在写盘中
	 */
	void					UpdateMinuteWritingStatus( bool bStatus );

	/**
	 * @brief				获取Minute行情写盘状态
	 * @return				true					写盘中...
	 */
	bool					FetchMinuteWritingStatus();

public:///< 市场时间更新方法
	/**
	 * @brief				更新市场时间
	 * @param[in]			eMkID					市场ID
	 * @param[in]			nMkTime					市场时间
	 */
	void					UpdateMkTime( enum XDFMarket eMkID, unsigned int nMkTime );

	/**
	 * @brief				获取某市场的时间
	 * @param[in]			eMkID					市场ID
	 * @return				返回市场时间
	 */
	unsigned int			FetchMkTime( enum XDFMarket eMkID );

protected:
	bool					m_bWritingTick;					///< TICK是否在落盘中
	bool					m_bWritingMinute;				///< 分钟线是否在落盘中
	unsigned int			m_vctMarketTime[256];			///< 各市场的时间表
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< 各市场的第一个商品的状态更新表
};



#endif





