#ifndef __SERVER_STATUS_H__
#define __SERVER_STATUS_H__


#pragma warning(disable:4786)
#include <set>
#include <stdexcept>
#include "../QuoteCltDef.h"


/**
 * @class					T_SECURITY_STATUS
 * @brief					��Ʒ״̬��������
 */
typedef struct
{
	char					Code[32];				///< ��Ʒ����
	char					Name[64];				///< ��Ʒ����
	double					LastPx;					///< ���¼۸�
	double					Amount;					///< �ɽ����
	unsigned __int64		Volume;					///< �ɽ���
} T_SECURITY_STATUS;


/**
 * @class					ServerStatus
 * @brief					����״̬ά����
 * @author					barry
 */
class ServerStatus
{
private:
	ServerStatus();

public:
	static ServerStatus&	GetStatusObj();

public:///< ��Ʒ״̬���·���
	/**
	 * @brief				ê��һ����Ʒ���ṩ���������)
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszCode					��Ʒ����
	 * @param[in]			pszName					��Ʒ����
	 */
	void					AnchorSecurity( enum XDFMarket eMkID, const char* pszCode, const char* pszName );

	/**
	 * @brief				����һ����Ʒ����״̬
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszCode					��Ʒ����
	 * @param[in]			dLastPx					���¼۸�
	 * @param[in]			dAmount					�ɽ����
	 * @param[in]			nVolume					�ɽ���
	 */
	void					UpdateSecurity( enum XDFMarket eMkID, const char* pszCode, double dLastPx, double dAmount, unsigned __int64 nVolume );

	/**
	 * @brief				��ȡ��Ʒ״̬����
	 * @param[in]			eMkID					�г�ID
	 * @return				������Ʒ״̬����
	 */
	T_SECURITY_STATUS&		FetchSecurity( enum XDFMarket eMkID );

public:///< ����д��״̬����
	/**
	 * @brief				����Tick����д��״̬
	 * @param[in]			bStatus					�Ƿ���д����
	 */
	void					UpdateTickWritingStatus( bool bStatus );

	/**
	 * @brief				��ȡTick����д��״̬
	 * @return				true					д����...
	 */
	bool					FetchTickWritingStatus();

	/**
	 * @brief				����Minute����д��״̬
	 * @param[in]			bStatus					�Ƿ���д����
	 */
	void					UpdateMinuteWritingStatus( bool bStatus );

	/**
	 * @brief				��ȡMinute����д��״̬
	 * @return				true					д����...
	 */
	bool					FetchMinuteWritingStatus();

public:///< �г�ʱ����·���
	/**
	 * @brief				�����г�ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			nMkTime					�г�ʱ��
	 */
	void					UpdateMkTime( enum XDFMarket eMkID, unsigned int nMkTime );

	/**
	 * @brief				��ȡĳ�г���ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @return				�����г�ʱ��
	 */
	unsigned int			FetchMkTime( enum XDFMarket eMkID );

public:///< ��Ѷ���ȨϢ�ļ����¼�¼
	/**
	 * @brief				���²ƾ����ݸ�������ʱ��
	 */
	void					UpdateFinancialDT();

	/**
	 * @brief				����ȨϢ�ļ���������ʱ��
	 */
	void					UpdateWeightDT();

	/**
	 * @brief				��ȡȨϢ�ļ���������ʱ��
	 * @param[out]			nDate					����
	 * @param[out]			nTime					ʱ��
	 */
	void					GetWeightUpdateDT( unsigned int& nDate, unsigned int& nTime );

	/**
	 * @brief				��ȡ��Ѷ���������ʱ��
	 * @param[out]			nDate					����
	 * @param[out]			nTime					ʱ��
	 */
	void					GetFinancialUpdateDT( unsigned int& nDate, unsigned int& nTime );

public:///< ���г���tick����ռ���ʴ�ȡ����
	/**
	 * @brief				�����г�����ռ����
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			dRate					tick����ռ����
	 */
	void					UpdateMkOccupancyRate( enum XDFMarket eMkID, double dRate );

	/**
	 * @brief				��ȡ���г�����ռ����
	 * @param[in]			eMkID					�г�ID
	 */
	double					FetchMkOccupancyRate( enum XDFMarket eMkID );

protected:
	bool					m_bWritingTick;					///< TICK�Ƿ���������
	bool					m_bWritingMinute;				///< �������Ƿ���������
	unsigned int			m_vctMarketTime[256];			///< ���г���ʱ���
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< ���г��ĵ�һ����Ʒ��״̬���±�
	unsigned int			m_nFinancialUpdateDate;			///< ��Ѷ���������
	unsigned int			m_nFinancialUpdateTime;			///< ��Ѷ�����ʱ��
	unsigned int			m_nWeightUpdateDate;			///< ȨϢ���������
	unsigned int			m_nWeightUpdateTime;			///< ȨϢ�����ʱ��
	double					m_vctsBufOccupancyRate[256];	///< ���г�����ռ����ͳ��
};



#endif





