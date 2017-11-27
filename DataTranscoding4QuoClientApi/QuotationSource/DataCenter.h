#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


#pragma pack(1)
/**
 * @class						T_DAY_LINE
 * @brief						����
 */
typedef struct
{
	char						eMarketID;		///< �г�ID
	char						Code[16];		///< ��Ʒ����
	unsigned int				Date;			///< ����(YYYYMMDD)
	double						OpenPx;			///< ���̼�
	double						HighPx;			///< ��߼�
	double						LowPx;			///< ��ͼ�
	double						ClosePx;		///< ���̼�
	double						SettlePx;		///< �����
	double						Amount;			///< �ɽ���
	unsigned __int64			Volume;			///< �ɽ���
	unsigned __int64			OpenInterest;	///< �ֲ���
	unsigned __int64			NumTrades;		///< �ɽ�����
	double						Voip;			///< ����ģ����Ȩ֤���
} T_DAY_LINE;
#pragma pack()


typedef MLoopBufferSt<T_DAY_LINE>				T_DAYLINE_CACHE;
typedef	std::map<enum XDFMarket,int>			TMAP_MKID2STATUS;
const	unsigned int							MAX_WRITER_NUM = 128;


/**
 * @class						DayLineArray
 * @brief						���߻���
 */
class DayLineArray
{
public:
	/**
	 * @brief					���캯��
	 * @param[in]				eMkID			�г�ID
	 */
	DayLineArray( enum XDFMarket eMkID );

protected:
	/**
	 * @brief					Ϊһ����Ʒ������仺���Ӧ��ר�û���
	 */
	static char*				GrabCache();

	/**
	 * @brief					�ͷŻ���
	 */
	static void					FreeCaches();

protected:
	enum XDFMarket				m_eMarketID;	///< �г����
	T_DAYLINE_CACHE				m_oKLineBuffer;	///< ���߻���
	bool						m_bWrited;		///< �Ƿ���Ҫ����������
};


/**
 * @class						QuotationData
 * @brief						�������ݴ洢��
 */
class QuotationData
{
public:
	QuotationData();
	~QuotationData();

	/**
	 * @brief					��ʼ��
	 * @return					==0				�ɹ�
								!=				ʧ��
	 */
	int							Initialize();

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

public:
	/**
	 * @brief					����ģ�鹤��״̬
	 * @param[in]				cMarket			�г�ID
	 * @param[in]				nStatus			״ֵ̬
	 */
	void						UpdateModuleStatus( enum XDFMarket nMarket, int nStatus );

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< ģ��״̬��
	std::ofstream				m_vctDataDump[MAX_WRITER_NUM];	///< �����ļ��������
};



#endif















