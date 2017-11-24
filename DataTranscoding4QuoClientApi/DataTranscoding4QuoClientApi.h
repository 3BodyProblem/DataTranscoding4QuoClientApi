#ifndef __DATA_TABLE_PAINTER_H__
#define	__DATA_TABLE_PAINTER_H__


#include "../../DataNode/DataNode/Interface.h"
#include "Configuration.h"
#include "Infrastructure/Lock.h"
#include "QuotationSource/Quotation.h"


/**
 * @class						T_LOG_LEVEL
 * @brief						��־����
 * @author						barry
 */
enum T_LOG_LEVEL
{
	TLV_INFO = 0,
	TLV_WARN = 1,
	TLV_ERROR = 2,
	TLV_DETAIL = 3,
};


/**
 * @class						QuoCollector
 * @brief						�������ݲɼ�ģ��������
 * @date						2017/5/15
 * @author						barry
 */
class QuoCollector
{
protected:
	QuoCollector();

public:
	/**
	 * @brief					��ȡ���ݲɼ�����ĵ�������
	 */
	static QuoCollector&		GetCollector();

	/**
	 * @brief					��ʼ�����ݲɼ���
	 * @param[in]				pIDataHandle				����ص��ӿ�
	 * @return					==0							��ʼ���ɹ�
								!=0							����
	 */
	int							Initialize( I_DataHandle* pIDataHandle );

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

public:
	/**
	 * @brief					�ӱ����ļ�������˿����»ָ�����������������
	 * @note					��һ��ͬ���ĺ������������ʼ����ɺ�Ż᷵��
	 * @return					==0							�ɹ�
								!=0							����
	 */
	int							RecoverQuotation();

	/**
	 * @brief					ֹͣ
	 */
	void						Halt();

public:
	/**
	 * @brief					���ط�������ص��ӿڵ�ַ
	 * @return					����ص��ӿ�ָ���ַ
	 */
	I_DataHandle*				operator->();

	/**
	 * @brief					��ȡ�������
	 */
	Quotation&					GetQuoObj();

	/**
	 * @brief					ȡ�òɼ�ģ��ĵ�ǰ״̬
	 * @param[out]				pszStatusDesc				���س�״̬������
	 * @param[in,out]			nStrLen						�������������泤�ȣ������������Ч���ݳ���
	 * @return					����ģ�鵱ǰ״ֵ̬
	 */
	enum E_SS_Status			GetCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen );

protected:
	I_DataHandle*				m_pCbDataHandle;			///< ����(����/��־�ص��ӿ�)
	Quotation					m_oQuotationData;			///< ʵʱ�������ݻỰ����
};


/**
 * @brief						DLL�����ӿ�
 * @author						barry
 * @date						2017/4/1
 */
extern "C"
{
	/**
	 * @brief								��ʼ�����ݲɼ�ģ��
	 * @param[in]							pIDataHandle				���鹦�ܻص�
	 * @return								==0							��ʼ���ɹ�
											!=							����
	 */
	__declspec(dllexport) int __stdcall		Initialize( I_DataHandle* pIDataHandle );

	/**
	 * @brief								�ͷ����ݲɼ�ģ��
	 */
	__declspec(dllexport) void __stdcall	Release();

	/**
	 * @brief								���³�ʼ����������������
	 * @note								��һ��ͬ���ĺ������������ʼ����ɺ�Ż᷵��
 	 * @return								==0							�ɹ�
											!=0							����
	 */
	__declspec(dllexport) int __stdcall		RecoverQuotation();

	/**
	 * @brief								��ʱ���ݲɼ�
	 */
	__declspec(dllexport) void __stdcall	HaltQuotation();

	/**
	 * @brief								��ȡģ��ĵ�ǰ״̬
	 * @param[out]							pszStatusDesc				���س�״̬������
	 * @param[in,out]						nStrLen						�������������泤�ȣ������������Ч���ݳ���
	 * @return								����ģ�鵱ǰ״ֵ̬
	 */
	__declspec(dllexport) int __stdcall		GetStatus( char* pszStatusDesc, unsigned int& nStrLen );

	/**
	 * @brief								��ȡ�г����
	 * @return								�г�ID
	 */
	__declspec(dllexport) int __stdcall		GetMarketID();

	/**
	 * @brief								�Ƿ�Ϊ���鴫��Ĳɼ���
	 * @return								true						�Ǵ���ģ�������ɼ����
											false						����Դ������ɼ����
	 */
	__declspec(dllexport) bool __stdcall	IsProxy();

	/**
	 * @brief								��Ԫ���Ե�������
	 */
	__declspec(dllexport) void __stdcall	ExecuteUnitTest();

	/**
	 * @brief								�������ݷ�������
	 */
	__declspec(dllexport) void __stdcall	Echo();
}




#endif





