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


typedef MLoopBufferSt<T_DAY_LINE>				T_DAYLINE_CACHE;			///< ѭ�����л���
typedef	std::map<enum XDFMarket,int>			TMAP_MKID2STATUS;			///< ���г�ģ��״̬
const	unsigned int							MAX_WRITER_NUM = 128;		///< ��������ļ����


/**
 * @class						CacheAlloc
 * @brief						���߻���
 */
class CacheAlloc
{
private:
	/**
	 * @brief					���캯��
	 * @param[in]				eMkID			�г�ID
	 */
	CacheAlloc();

public:
	~CacheAlloc();

	/**
	 * @brief					��ȡ����
	 */
	static CacheAlloc&			GetObj();

	/**
	 * @brief					��ȡ�����ַ
	 */
	char*						GetBufferPtr();

	/**
	 * @brief					��ȡ��������Ч���ݵĳ���
	 */
	unsigned int				GetDataLength();

	/**
	 * @brief					Ϊһ����Ʒ������仺���Ӧ��ר�û���
	 * @param[in]				eMkID			�г�ID
	 */
	char*						GrabCache( enum XDFMarket eMkID );

	/**
	 * @brief					�ͷŻ���
	 */
	void						FreeCaches();

private:
	unsigned int				m_nMaxCacheSize;			///< ���黺���ܴ�С
	unsigned int				m_nAllocateSize;			///< ��ǰʹ�õĻ����С
	char*						m_pDataCache;				///< �������ݻ����ַ
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















