#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


#pragma pack(1)
/**
 * @class						T_DAY_LINE
 * @brief						����
 */
typedef struct
{
	unsigned char				Valid;			///< �Ƿ���Ч����, 1:��Ч����Ҫ���� 0:��Ч��������
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

/**
 * @class						T_LINE_PARAM
 * @brief						��������s
 */
typedef struct
{
	double						dPriceRate;		///< �Ŵ���
} T_LINE_PARAM;
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
	char*						GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize );

	/**
	 * @brief					�ͷŻ���
	 */
	void						FreeCaches();

private:
	CriticalObject				m_oLock;					///< �ٽ�������
	unsigned int				m_nMaxCacheSize;			///< ���黺���ܴ�С
	unsigned int				m_nAllocateSize;			///< ��ǰʹ�õĻ����С
	char*						m_pDataCache;				///< �������ݻ����ַ
};


typedef	std::pair<T_LINE_PARAM,T_DAYLINE_CACHE>	T_QUO_DATA;	///< ���ݻ���
typedef std::map<std::string,T_QUO_DATA>		T_MAP_QUO;	///< �������ݻ���


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
	void						UpdateModuleStatus( enum XDFMarket eMarket, int nStatus );

	/**
	 * @brief					Ϊ�������г����������߳�
	 */
	void						BeginDumpThread( enum XDFMarket eMarket, int nStatus );

public:
	/**
	 * @brief					������Ʒ�Ĳ�����Ϣ
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @param[in]				refParam		�������
	 * @return					==0				�ɹ�
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam );

	/**
	 * @brief					����������Ϣ
	 * @param[in]				refDayLine		������Ϣ
	 * @param[in]				pSnapData		����ָ��
	 * @param[in]				nSnapSize		���մ�С
	 * @return					==0				�ɹ�
	 */
	int							UpdateDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize );

protected:///< ���������߳�
	static void*	__stdcall	ThreadDumpDayLine4SHL1( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4SHOPT( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4SZL1( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4SZOPT( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4CFF( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4CFFOPT( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4CNF( void* pSelf );
	static void*	__stdcall	ThreadDumpDayLine4CNFOPT( void* pSelf );

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< ģ��״̬��
	std::ofstream				m_vctDataDump[MAX_WRITER_NUM];	///< �����ļ��������
protected:
	T_MAP_QUO					m_mapSHL1;						///< ��֤L1
	T_MAP_QUO					m_mapSHOPT;						///< ��֤��Ȩ
	T_MAP_QUO					m_mapSZL1;						///< ��֤L1
	T_MAP_QUO					m_mapSZOPT;						///< ��֤��Ȩ
	T_MAP_QUO					m_mapCFF;						///< �н��ڻ�
	T_MAP_QUO					m_mapCFFOPT;					///< �н���Ȩ
	T_MAP_QUO					m_mapCNF;						///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	T_MAP_QUO					m_mapCNFOPT;					///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
protected:
	SimpleThread				m_oThdSHL1;						///< ��֤L1
	SimpleThread				m_oThdSHOPT;					///< ��֤��Ȩ
	SimpleThread				m_oThdSZL1;						///< ��֤L1
	SimpleThread				m_oThdSZOPT;					///< ��֤��Ȩ
	SimpleThread				m_oThdCFF;						///< �н��ڻ�
	SimpleThread				m_oThdCFFOPT;					///< �н���Ȩ
	SimpleThread				m_oThdCNF;						///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	SimpleThread				m_oThdCNFOPT;					///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
};


#endif










