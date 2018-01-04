#ifndef __CTP_QUOTATION_H__
#define __CTP_QUOTATION_H__


#pragma warning(disable:4786)
#include <set>
#include <stdexcept>
#include "DataDump.h"
#include "DataCenter.h"
#include "DataCollector.h"


/**
 * @class			WorkStatus
 * @brief			����״̬����
 * @author			barry
 */
class WorkStatus
{
public:
	/**
	 * @brief				Ӧ״ֵ̬ӳ���״̬�ַ���
	 */
	static	std::string&	CastStatusStr( enum E_SS_Status eStatus );

public:
	/**
	 * @brief			����
	 * @param			eMkID			�г����
	 */
	WorkStatus();
	WorkStatus( const WorkStatus& refStatus );

	/**
	 * @brief			��ֵ����
						ÿ��ֵ�仯������¼��־
	 */
	WorkStatus&			operator= ( enum E_SS_Status eWorkStatus );

	/**
	 * @brief			����ת����
	 */
	operator			enum E_SS_Status();

private:
	CriticalObject		m_oLock;				///< �ٽ�������
	enum E_SS_Status	m_eWorkStatus;			///< ���鹤��״̬
};


/**
 * @class			Quotation
 * @brief			�Ự�������
 * @detail			��װ�������Ʒ�ڻ���Ȩ���г��ĳ�ʼ����������Ƶȷ���ķ���
 * @author			barry
 */
class Quotation : public QuoteClientSpi
{
public:
	Quotation();
	~Quotation();

	/**
	 * @brief					��ʼ��ctp����ӿ�
	 * @return					>=0			�ɹ�
								<0			����
	 * @note					������������������У�ֻ������ʱ��ʵ�ĵ���һ��
	 */
	int							Initialize();

	/**
	 * @brief					�ͷ�ctp����ӿ�
	 */
	int							Release();

	/**
	 * @brief					ֹͣ����
	 */
	void						Halt();

	/**
	 * @brief					��ȡ�������汾��
	 */
	std::string&				QuoteApiVersion();

	/**
	 * @brief					��ȡ�Ự״̬��Ϣ
	 */
	WorkStatus&					GetWorkStatus();

	/**
	 * @brief					�Ѹ��г����̺����ˢ������
	 * @detail					�����Ե�������������������������õ�����ʱ����ϢΪ��׼������ȫ�г������̲���
	 */
	void						FlushDayLineOnCloseTime();

	/**
	 * @brief					���»�ȡ�����г������ں�ʱ��
	 */
	void						UpdateMarketsTime();

public:///< ����ӿڵĻص�����
	virtual bool __stdcall		XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus );
	virtual void __stdcall		XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes );
	virtual void __stdcall		XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf );
	virtual int	__stdcall		XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam );

protected:///< �����г���������: ���� ��̬�����ļ� + tick�ļ� ���� ���������ļ�
	/**
	 * @brief					�����Ϻ�Lv1�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveShLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					�����Ϻ�Option�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveShOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					��������Lv1�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveSzLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					����������Ȩ�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveSzOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					�����н��ڻ��Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCFF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					�����н���Ȩ�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCFFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					������Ʒ�ڻ��Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCNF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

	/**
	 * @brief					������Ʒ��Ȩ�Ļ�����Ϣ
	 * @param[in]				eStatus		�г�ģ��״̬
	 * @param[in]				bBuild		�Ƿ�Ϊ��ʼ������:true,������̬��Ϣ�ļ�+tick��Ϣ false,ֻ����������Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCNFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild );

private:
	CriticalObject				m_oLock;				///< �ٽ�������
	WorkStatus					m_vctMkSvrStatus[64];	///< ���г�����״̬
	DataCollector				m_oQuotPlugin;			///< ������
	QuotationData				m_oQuoDataCenter;		///< �������ݼ���
	WorkStatus					m_oWorkStatus;			///< ����״̬
	std::map<int,int>			m_mapMkBuildTimeT;		///< ���г������ʱ���¼(�͵�ǰʱ��С��3��Ķ�������������г�ʼ��)
};




#endif



