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
	unsigned int			MkDate;					///< �г�����
	unsigned int			MkTime;					///< �г�����ʱ��
	double					MkBufOccupancyRate;		///< �г�����ռ����
	char					MkStatusDesc[32];		///< �г�״̬������
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

public:///< �г�ʱ����·���
	/**
	 * @brief				�����г�ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			nMkDate					�г�����
	 * @param[in]			nMkTime					�г�ʱ��
	 */
	void					UpdateMkTime( enum XDFMarket eMkID, unsigned nMkDate, unsigned int nMkTime );

	/**
	 * @brief				��ȡĳ�г���ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @return				�����г�ʱ��
	 */
	unsigned int			FetchMkTime( enum XDFMarket eMkID );

	/**
	 * @brief				��ȡĳ�г�������
	 * @param[in]			eMkID					�г�ID
	 * @return				�����г�����
	 */
	unsigned int			FetchMkDate( enum XDFMarket eMkID );

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

public:///< ���г���״̬
	/**
	 * @brief				��ȡ���г�����ռ����
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszDesc					�г�״̬������
	 */
	void					UpdateMkStatus( enum XDFMarket eMkID, const char* pszDesc );

	/**
	 * @brief				��ȡ�г�����״̬������
	 * @param[in]			eMkID					�г�ID
	 */
	const char*				GetMkStatus( enum XDFMarket eMkID );

protected:
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< ���г��ĵ�һ����Ʒ��״̬���±�
	unsigned int			m_nFinancialUpdateDate;			///< ��Ѷ���������
	unsigned int			m_nFinancialUpdateTime;			///< ��Ѷ�����ʱ��
	unsigned int			m_nWeightUpdateDate;			///< ȨϢ���������
	unsigned int			m_nWeightUpdateTime;			///< ȨϢ�����ʱ��
};



#endif





