#ifndef __DATA_COLLECTOR_H__
#define	__DATA_COLLECTOR_H__


#include <string>
#include "../QuoteClientApi.h"
#include "../Infrastructure/Dll.h"
#include "../Infrastructure/Lock.h"
#include "../../../DataNode/DataNode/Interface.h"


/**
 * @class				CollectorStatus
 * @brief				��ǰ����Ự��״̬
 * @detail				��������Ҫͨ������жϣ���ϳ�ʼ������ʵ�������ж��Ƿ���Ҫ���³�ʼ���ȶ���
 * @author				barry
 */
class CollectorStatus
{
public:
	CollectorStatus();

	CollectorStatus( const CollectorStatus& obj );

public:
	enum E_SS_Status		Get() const;

	bool					Set( enum E_SS_Status eNewStatus );

private:
	mutable CriticalObject	m_oCSLock;			///< �ٽ���
	enum E_SS_Status		m_eStatus;			///< ��ǰ�����߼�״̬�������жϵ�ǰ����ʲô������
	int						m_nMarketID;		///< ���ݲɼ�����Ӧ���г�ID
};


/**
 * @class					DataCollector
 * @brief					���ݲɼ�ģ�����ע��ӿ�
 * @note					�ɼ�ģ��ֻ�ṩ������ʽ�Ļص�֪ͨ( I_DataHandle: ��ʼ��ӳ�����ݣ� ʵʱ�������ݣ� ��ʼ����ɱ�ʶ ) + ���³�ʼ����������
 * @date					2017/5/3
 * @author					barry
 */
class DataCollector
{
public:
	DataCollector();

	/**
	 * @brief				���ݲɼ�ģ���ʼ��
	 * @param[in]			pIQuotationSpi				����ص��ӿ�
	 * @return				==0							�ɹ�
							!=0							����
	 */
	int						Initialize( QuoteClientSpi* pIQuotationSpi );

	/**
	 * @breif				���ݲɼ�ģ���ͷ��˳�
	 */
	void					Release();

public:///< ���ݲɼ�ģ���¼�����
	/**
 	 * @brief				��ʼ��/���³�ʼ���ص�
	 * @note				ͬ�����������������غ󣬼���ʼ�������Ѿ����꣬�����ж�ִ�н���Ƿ�Ϊ���ɹ���
	 * @return				==0							�ɹ�
							!=0							����
	 */
	int						RecoverDataCollector();

	/**
	 * @brief				��ͣ���ݲɼ���
	 */
	void					HaltDataCollector();

	/**
	 * @biref				ȡ�õ�ǰ���ݲɼ�ģ��״̬
	 * @param[out]			pszStatusDesc				���س�״̬������
	 * @param[in,out]		nStrLen						�������������泤�ȣ������������Ч���ݳ���
	 * @return				E_SS_Status״ֵ̬
	 */
	enum E_SS_Status		InquireDataCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen );

	/**
	 * @brief				������ת��
	 */
	QuoteClientApi*			operator->();

	/**
	 * @brief				��ȡ��ѯ�ӿ�
	 */
	QuotePrimeApi*			GetPrimeApi();

	/**
	 * @brief				��ȡ�������汾��
	 */
	std::string&			GetVersion();

private:
	Dll						m_oDllPlugin;					///< ���������
	CollectorStatus			m_oCollectorStatus;				///< ���ݲɼ�ģ���״̬
	QuoteClientApi*			m_pQuoteClientApi;				///< ��������Դ������
	QuotePrimeApi*			m_pQuotePrimeApi;				///< �������ݲ�ѯ�ӿ�
	std::string				m_sPluginVersion;				///< �汾��
};







#endif









