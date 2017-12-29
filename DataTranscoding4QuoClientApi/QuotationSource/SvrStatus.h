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
	 * @param[in]			pszCode					��Ʒ����
	 * @return				������Ʒ״̬����
	 */
	T_SECURITY_STATUS&		FetchSecurity( enum XDFMarket eMkID, const char* pszCode );

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

protected:
	bool					m_bWritingTick;					///< TICK�Ƿ���������
	bool					m_bWritingMinute;				///< �������Ƿ���������
	unsigned int			m_vctMarketTime[256];			///< ���г���ʱ���
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< ���г��ĵ�һ����Ʒ��״̬���±�
};



#endif





