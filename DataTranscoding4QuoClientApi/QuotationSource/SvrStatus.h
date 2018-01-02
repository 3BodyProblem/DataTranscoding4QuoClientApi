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
	unsigned int			MkDate;					///< 市场日期
	unsigned int			MkTime;					///< 市场行情时间
	double					MkBufOccupancyRate;		///< 市场缓存占用率
	char					MkStatusDesc[32];		///< 市场状态描述串
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
	 * @return				返回商品状态引用
	 */
	T_SECURITY_STATUS&		FetchSecurity( enum XDFMarket eMkID );

public:///< 市场时间更新方法
	/**
	 * @brief				更新市场时间
	 * @param[in]			eMkID					市场ID
	 * @param[in]			nMkDate					市场日期
	 * @param[in]			nMkTime					市场时间
	 */
	void					UpdateMkTime( enum XDFMarket eMkID, unsigned nMkDate, unsigned int nMkTime );

	/**
	 * @brief				获取某市场的时间
	 * @param[in]			eMkID					市场ID
	 * @return				返回市场时间
	 */
	unsigned int			FetchMkTime( enum XDFMarket eMkID );

	/**
	 * @brief				获取某市场的日期
	 * @param[in]			eMkID					市场ID
	 * @return				返回市场日期
	 */
	unsigned int			FetchMkDate( enum XDFMarket eMkID );

public:///< 资讯库和权息文件更新记录
	/**
	 * @brief				更新财经数据更新日期时间
	 */
	void					UpdateFinancialDT();

	/**
	 * @brief				更新权息文件更新日期时间
	 */
	void					UpdateWeightDT();

	/**
	 * @brief				获取权息文件更新日期时间
	 * @param[out]			nDate					日期
	 * @param[out]			nTime					时间
	 */
	void					GetWeightUpdateDT( unsigned int& nDate, unsigned int& nTime );

	/**
	 * @brief				获取资讯库更新日期时间
	 * @param[out]			nDate					日期
	 * @param[out]			nTime					时间
	 */
	void					GetFinancialUpdateDT( unsigned int& nDate, unsigned int& nTime );

public:///< 各市场的tick缓存占用率存取方法
	/**
	 * @brief				更新市场缓存占用率
	 * @param[in]			eMkID					市场ID
	 * @param[in]			dRate					tick缓存占用率
	 */
	void					UpdateMkOccupancyRate( enum XDFMarket eMkID, double dRate );

	/**
	 * @brief				获取各市场缓存占用率
	 * @param[in]			eMkID					市场ID
	 */
	double					FetchMkOccupancyRate( enum XDFMarket eMkID );

public:///< 各市场的状态
	/**
	 * @brief				获取各市场缓存占用率
	 * @param[in]			eMkID					市场ID
	 * @param[in]			pszDesc					市场状态描述串
	 */
	void					UpdateMkStatus( enum XDFMarket eMkID, const char* pszDesc );

	/**
	 * @brief				获取市场行情状态描述串
	 * @param[in]			eMkID					市场ID
	 */
	const char*				GetMkStatus( enum XDFMarket eMkID );

protected:
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< 各市场的第一个商品的状态更新表
	unsigned int			m_nFinancialUpdateDate;			///< 资讯库更新日期
	unsigned int			m_nFinancialUpdateTime;			///< 资讯库更新时间
	unsigned int			m_nWeightUpdateDate;			///< 权息库更新日期
	unsigned int			m_nWeightUpdateTime;			///< 权息库更新时间
};



#endif





