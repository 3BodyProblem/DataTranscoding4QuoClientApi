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
 * @class			T_MIN_LINE
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

/**
 * @class						T_TICK_LINE
 * @brief						Tick��
 */
typedef struct
{
	unsigned char				Type;					///< ����
	char						eMarketID;				///< �г�ID
	char						Code[16];				///< ��Ʒ����
	unsigned int				Date;					///< YYYYMMDD����20170705��
	unsigned int				Time;					///< HHMMSSmmm����093005000)
	double						PreClosePx;				///< ���ռ�
	double						PreSettlePx;			///< ����
	double						OpenPx;					///< ���̼�
	double						HighPx;					///< ��߼�
	double						LowPx;					///< ��ͼ�
	double						ClosePx;				///< ���̼�
	double						NowPx;					///< �ּ�
	double						SettlePx;				///< �����
	double						UpperPx;				///< ��ͣ��
	double						LowerPx;				///< ��ͣ��
	double						Amount;					///< �ɽ���
	unsigned __int64			Volume;					///< �ɽ���(��/��/��)
	unsigned __int64			OpenInterest;			///< �ֲ���(��/��/��)
	unsigned __int64			NumTrades;				///< �ɽ�����
	double						BidPx1;					///< ί������һ�۸�
	unsigned __int64			BidVol1;				///< ί������һ��(��/��/��)
	double						BidPx2;					///< ί�����̶��۸�
	unsigned __int64			BidVol2;				///< ί�����̶���(��/��/��)
	double						AskPx1;					///< ί������һ�۸�
	unsigned __int64			AskVol1;				///< ί������һ��(��/��/��)
	double						AskPx2;					///< ί�����̶��۸�
	unsigned __int64			AskVol2;				///< ί�����̶���(��/��/��)
	double						Voip;					///< ����ģ����Ȩ֤���
	char						TradingPhaseCode[12];	///< ����״̬
	char						PreName[8];				///< ֤ȯǰ׺
} T_TICK_LINE;

/**
 * @class						T_MIN_LINE
 * @brief						������
 */
typedef struct
{
	unsigned char				Type;					///< ����
	char						eMarketID;				///< �г�ID
	char						Code[16];				///< ��Ʒ����
	unsigned int				Date;					///< YYYYMMDD����20170705��
	unsigned int				Time;					///< HHMMSSmmm����093005000)
	double						OpenPx;					///< ���̼�һ�����ڵĵ�һ�ʵ�nowpx
	double						HighPx;					///< ��߼�һ�����ڵ� ��ߵ�highpx
	double						LowPx;					///< ��ͼ�һ�����ڵ� ��͵�lowpx
	double						ClosePx;				///< ���̼�һ���������һ�ʵ�Nowpx
	double						SettlePx;				///< �����һ���������һ�ʵ�settlepx
	double						Amount;					///< �ɽ���һ�������ʼ�ȥ��һ�ʵ�amount
	unsigned __int64			Volume;					///< �ɽ���(��/��/��)һ�������ʼ�ȥ��һ�ʵ�volume
	unsigned __int64			OpenInterest;			///< �ֲ���(��/��/��)һ�������һ��
	unsigned __int64			NumTrades;				///< �ɽ�����һ�������ʼ�ȥ��һ�ʵ�numtrades
	double						Voip;					///< ����ģ����Ȩ֤���һ���ӵ����һ��voip
} T_MIN_LINE;

/**
 * @class						T_LINE_PARAM
 * @brief						��������s
 */
typedef struct
{
	///< ---------------------- �ο�������Ϣ ----------------------------------
	unsigned char				Type;					///< ����
	unsigned int				UpperPrice;				///< ��ͣ��
	unsigned int				LowerPrice;				///< ��ͣ��
	unsigned int				PreClosePx;				///< ����
	unsigned int				PreSettlePx;			///< ���
	unsigned int				PrePosition;			///< ���
	char						PreName[8];				///< ǰ׺
	double						dPriceRate;				///< �Ŵ���
	///< ---------------------- ������ͳ����Ϣ --------------------------------
	unsigned char				Valid;					///< �Ƿ���Ч����, 1:��Ч����Ҫ���� 0:��Ч��������
	unsigned int				MkMinute;				///< ����ʱ��(��ȷ���֣�HHmm)
	unsigned int				MinOpenPx1;				///< �����ڵ�һ�����¼�
	unsigned int				MinHighPx;				///< ��������߼�
	unsigned int				MinLowPx;				///< ��������ͼ�
	unsigned int				MinClosePx;				///< ���������¼�
	unsigned int				MinSettlePx;			///< ���������½����
	double						MinAmount1;				///< �����ڵ�һ�ʽ��
	double						MinAmount2;				///< �����ڵ�ĩ�ʽ��
	unsigned __int64			MinVolume1;				///< �����ڵ�һ�ʳɽ���
	unsigned __int64			MinVolume2;				///< �����ڵ�ĩ�ʳɽ���
	unsigned __int64			MinOpenInterest;		///< �����ڵ�ĩ�ʳֲ���
	unsigned __int64			MinNumTrades1;			///< �����ڵ�һ�ʳɽ�����
	unsigned __int64			MinNumTrades2;			///< �����ڵ�ĩ�ʳɽ�����
	unsigned int				MinVoip;				///< �����ڵ�ĩ��Voip
} T_LINE_PARAM;
#pragma pack()


typedef MLoopBufferSt<T_TICK_LINE>						T_TICKLINE_CACHE;			///< Tick��ѭ�����л���
typedef MLoopBufferSt<T_MIN_LINE>						T_MINLINE_CACHE;			///< ������ѭ�����л���
typedef	std::map<enum XDFMarket,int>					TMAP_MKID2STATUS;			///< ���г�ģ��״̬
const	unsigned int									MAX_WRITER_NUM = 128;		///< ��������ļ����
typedef std::map<std::string,T_LINE_PARAM>				T_MAP_QUO;					///< �������ݻ���
extern unsigned int										s_nNumberInSection;			///< һ���г��п��Ի�����ٸ����ݿ�(����)


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
	 * @brief					��ȡ������󳤶�
	 */
	unsigned int				GetMaxBufLength();

	/**
	 * @brief					Ϊһ����Ʒ������仺���Ӧ��ר�û���
	 * @param[in]				eMkID			�г�ID
	 * @param[out]				nOutSize		���Ϊ��һ����Ʒ����Ļ���Ĵ�С
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
	 * @param[in]				refParam		�������
	 * @return					==0				�ɹ�
	 */
	int							BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam );

	/**
	 * @brief					����ǰ׺
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				pszPreName		ǰ׺����
	 * @param[in]				nSize			���ݳ���
	 */
	int							UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize );

	/**
	 * @brief					����ǰ׺
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @param[in]				refParam		�����߼������
	 * @param[in]				nMkDate			�г�����
	 * @param[in]				nMkTime			�г�ʱ��
	 */
	void						DispatchMinuteLine( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam, unsigned int nMkDate, unsigned int nMkTime );

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
	 * @param[in]				pSnapData		����ָ��
	 * @param[in]				nSnapSize		���մ�С
	 * @param[in]				nTradeDate		��������
	 */
	int							DumpDayLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate = 0 );

protected:
	static void*	__stdcall	ThreadDumpTickLine( void* pSelf );			///< ���������߳�
	static void*	__stdcall	ThreadDumpMinuteLine( void* pSelf );		///< ���������߳�
	static void*	__stdcall	ThreadOnIdle( void* pSelf );				///< �����߳�

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< ģ��״̬��
	CriticalObject				m_oLock;						///< �ٽ�������
	void*						m_pQuotation;					///< �������
protected:
	T_MAP_QUO					m_mapSHL1;						///< ��֤L1
	T_MAP_QUO					m_mapSHOPT;						///< ��֤��Ȩ
	T_MAP_QUO					m_mapSZL1;						///< ��֤L1
	T_MAP_QUO					m_mapSZOPT;						///< ��֤��Ȩ
	T_MAP_QUO					m_mapCFF;						///< �н��ڻ�
	T_MAP_QUO					m_mapCFFOPT;					///< �н���Ȩ
	T_MAP_QUO					m_mapCNF;						///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	T_MAP_QUO					m_mapCNFOPT;					///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
	T_TICKLINE_CACHE			m_arrayTickLine;				///< ȫ�г�tick����
	unsigned int				m_lstMkTime[64];				///< ���г����г�ʱ��
protected:
	unsigned int				m_nMaxMLineBufSize;				///< �����߻��泤��
	char*						m_pBuf4MinuteLine;				///< �����߻����ַ
	T_MINLINE_CACHE				m_arrayMinuteLine;				///< �����߻���������
	CriticalObject				m_oMinuteLock;					///< �ٽ�������
protected:
	SimpleThread				m_oThdTickDump;					///< Tick��������
	SimpleThread				m_oThdMinuteDump;				///< ��������������
	SimpleThread				m_oThdIdle;						///< �����߳�
};




#endif










