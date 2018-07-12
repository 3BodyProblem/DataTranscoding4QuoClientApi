#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"
#include "Data1Min.h"
#include "DataTick.h"
#include "DataStatus.h"


///< ���ݽṹ���� //////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
/**
 * @class			T_STATIC_LINE
 * @brief			������
 * @author			barry
 */
typedef struct
{
	unsigned char				Type;					///< ����
	char						eMarketID;				///< �г�ID
	char						Code[16];				///< ��Ʒ����
	unsigned int				Date;					///< YYYYMMDD����20170705��
	char						Name[64];				///< ��Ʒ����
	unsigned int				LotSize;				///< ÿ������(��/��/��)
	unsigned int				ContractMult;			///< ��Լ����
	unsigned int				ContractUnit;			///< ��Լ��λ
	unsigned int				StartDate;				///< �׸�������(YYYYMMDD)
	unsigned int				EndDate;				///< �������(YYYYMMDD)
	unsigned int				XqDate;					///< ��Ȩ��(YYYYMMDD)
	unsigned int				DeliveryDate;			///< ������(YYYYMMDD)
	unsigned int				ExpireDate;				///< ������(YYYYMMDD)
	char						UnderlyingCode[16];		///< ��Ĵ���
	char						UnderlyingName[64];		///< �������
	char						OptionType;				///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
	char						CallOrPut;				///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
	double						ExercisePx;				///< ��Ȩ�۸�
} T_STATIC_LINE;
#pragma pack()


///< ���� ////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						QuotationData
 * @brief						�������ݴ洢��
 */
class QuotationData
{
public:
	typedef	std::map<enum XDFMarket,int>		TMAP_MKID2STATUS;		///< ���г�ģ��״̬
public:
	QuotationData();
	~QuotationData();

	/**
	 * @brief					��ʼ��
	 * @return					==0				�ɹ�
								!=				ʧ��
	 */
	int							Initialize( void* pQuotation );

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

	/**
	 * @brief					����ģ�鹤��״̬
	 * @param[in]				cMarket			�г�ID
	 * @param[in]				nStatus			״ֵ̬
	 */
	void						UpdateModuleStatus( enum XDFMarket eMarket, int nStatus );

	/**
	 * @brief					��ȡ�г�ģ��״̬
	 */
	short						GetModuleStatus( enum XDFMarket eMarket );

	/**
	 * @brief					ֹͣ�߳�
	 */
	bool						StopThreads();

	/**
	 * @brief					���������г�ʱ��
	 */
	void						ClearAllMkTime();

public:
	/**
	 * @brief					�������г�ʱ��
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				nMkTime			�г�ʱ��
	 */
	void						SetMarketTime( enum XDFMarket eMarket, unsigned int nMkTime );

	/**
	 * @brief					�����г�ʱ��
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				nMkTime			�г�ʱ��
	 */
	void						UpdateMarketTime( enum XDFMarket eMarket, unsigned int nMkTime );

	/**
	 * @brief					��ȡ�г�ʱ��
	 * @param[in]				eMarket			�г�ID
	 * @return					�����г�ʱ��
	 */
	unsigned int				GetMarketTime( enum XDFMarket eMarket );

	/**
	 * @brief					������Ʒ�Ĳ�����Ϣ
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @return					==0				�ɹ�
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen );

	/**
	 * @brief					������Ʒ�Ĳ�����Ϣ
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @return					==0				�ɹ�
	 */
	int							BuildSecurity4Min( enum XDFMarket eMarket, std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData );

	/**
	 * @brief					����������Ϣ
	 * @param[in]				pSnapData		����ָ��
	 * @param[in]				nSnapSize		���մ�С
	 * @param[in]				nTradeDate		��������
	 * @return					==0				�ɹ�
	 */
	int							UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

	/**
	 * @brief					��������
	 * @param[in]				pSnapData		����ָ��e
	 * @param[in]				nSnapSize		���մ�С
	 * @param[in]				nTradeDate		��������
	 */
	int							DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

	SecurityStatus&				GetSHL1StatusTable();
	SecurityStatus&				GetSZL1StatusTable();
	SecurityMinCache&			GetSHL1Min1Cache();
	SecurityMinCache&			GetSZL1Min1Cache();
	SecurityTickCache&			GetSHL1TickCache();
	SecurityTickCache&			GetSZL1TickCache();

protected:
	static void*	__stdcall	ThreadOnIdle( void* pSelf );				///< �����߳�

protected:
	SimpleThread				m_oThdIdle;						///< �����߳�
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< ģ��״̬��
	CriticalObject				m_oLock;						///< �ٽ�������
	void*						m_pQuotation;					///< �������
protected:
	SecurityStatus				m_objStatus4SHL1;				///< ��֤L1��Ʒ״̬
	SecurityStatus				m_objStatus4SZL1;				///< ��֤L1��Ʒ״̬
	SecurityMinCache			m_objMinCache4SHL1;				///< ��֤L1�����߻���
	SecurityMinCache			m_objMinCache4SZL1;				///< ��֤L1�����߻���
	SecurityTickCache			m_objTickCache4SHL1;			///< ��֤L1TICK�߻���
	SecurityTickCache			m_objTickCache4SZL1;			///< ��֤L1TICK�߻���
	unsigned int				m_lstMkTime[64];				///< ���г����г�ʱ��
};




#endif










