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
 * @brief						��������
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


///< 1�����߼��㲢������ ///////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						MinGenerator
 * @brief						��Ʒ1������������
 * @author						barry
 */
class MinGenerator
{
public:
	typedef struct
	{
		unsigned int			Time;					///< HHMMSSmmm����093005000)
		unsigned int			OpenPx;
		unsigned int			HighPx;
		unsigned int			LowPx;
		unsigned int			ClosePx;
		unsigned int			Voip;
		double					Amount;
		unsigned __int64		Volume;
		unsigned __int64		NumTrades;
	} T_DATA;											///< ��Ʒ����
	const unsigned int			m_nMaxLineCount;		///< ���֧�ֵķ���������(241��)
public:
	MinGenerator();
	MinGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr );
	MinGenerator&				operator=( const MinGenerator& obj );

	/**
	 * @brief					��ʼ��
	 * @return					0						�ɹ�
	 */
	int							Initialize();

	/**
	 * @brief					�������е�ÿ���ӵĵ�һ�ʿ��գ����µ���Ӧ��241�����������ݲ���
	 * @param[in]				objData					�������
	 * @return					0						�ɹ�
	 */
	int							Update( T_DATA& objData );

	/**
	 * @brief					���ɷ����߲�����
	 */
	void						DumpMinutes();
protected:
	double						m_dAmountBefore930;		///< 9:30ǰ�Ľ��
	unsigned __int64			m_nVolumeBefore930;		///< 9:30ǰ����
	unsigned __int64			m_nNumTradesBefore930;	///< 9:30ǰ�ı���
protected:
	double						m_dPriceRate;			///< �Ŵ���
	enum XDFMarket				m_eMarket;				///< �г����
	unsigned int				m_nDate;				///< YYYYMMDD����20170705��
	char						m_pszCode[9];			///< ��Ʒ����
protected:
	T_DATA*						m_pDataCache;			///< 241��1���ӻ���
	int							m_nWriteSize;			///< д������ߵĳ���
	int							m_nDataSize;			///< ���ݳ���
};


/**
 * @class						SecurityMinCache
 * @brief						������Ʒ�����ߵĻ���
 * @author						barry
 */
class SecurityMinCache
{
public:
	typedef std::map<std::string,MinGenerator>	T_MAP_MINUTES;
public:
	SecurityMinCache();
	~SecurityMinCache();

	/**
	 * @brief					�����߻����ʼ��
	 * @param[in]				nSecurityCount			�г��е���Ʒ����
	 * @return					0						�ɹ�
	 */
	int							Initialize( unsigned int nSecurityCount );

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

public:
	/**
	 * @brief					��������������߳�
	 */
	void						ActivateDumper();

	/**
	 * @brief					������Ʒ�Ĳ�����Ϣ
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @param[in]				nDate			��������
	 * @param[in]				dPriceRate		�Ŵ���
	 * @param[in]				objData			����۸�
	 * @return					==0				�ɹ�
	 */
	int							NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, MinGenerator::T_DATA& objData );

	/**
	 * @brief					������Ʒ��Ϣ
	 * @param[in]				objData			����۸�
	 * @return					==0				�ɹ�
	 */
	int							UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime );
	int							UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime );

protected:
	static void*	__stdcall	DumpThread( void* pSelf );

protected:
	enum XDFMarket				m_eMarketID;			///< �г����
	SimpleThread				m_oDumpThread;			///< ��������������
	unsigned int				m_nAlloPos;				///< �����Ѿ������λ��
	unsigned int				m_nSecurityCount;		///< ��Ʒ����
	MinGenerator::T_DATA*		m_pMinDataTable;		///< �����߻���
	T_MAP_MINUTES				m_objMapMinutes;		///< ������Map
	std::vector<std::string>	m_vctCode;				///< CodeInTicks
	CriticalObject				m_oLockData;			///< ������������
};


typedef MLoopBufferSt<T_TICK_LINE>						T_TICKLINE_CACHE;			///< Tick��ѭ�����л���
typedef MLoopBufferSt<T_MIN_LINE>						T_MINLINE_CACHE;			///< ������ѭ�����л���
typedef	std::map<enum XDFMarket,int>					TMAP_MKID2STATUS;			///< ���г�ģ��״̬
const	unsigned int									MAX_WRITER_NUM = 128;		///< ��������ļ����
typedef std::map<std::string,T_LINE_PARAM>				T_MAP_GEN_MLINES;			///< ������������Map
extern unsigned int										s_nNumberInSection;			///< һ���г��п��Ի�����ٸ����ݿ�(����)


///< TICK�߼��㲢������ ///////////////////////////////////////////////////////////////////////////////////////////
/**
 * @class						TickGenerator
 * @brief						TICK��������
 * @author						barry
 */
class TickGenerator
{
public:
	typedef struct
	{
		unsigned int			Time;					///< HHMMSSmmm����093005000)
		unsigned int			HighPx;					///< ��߼�
		unsigned int			LowPx;					///< ��ͼ�
		unsigned int			ClosePx;				///< ���̼�
		unsigned int			NowPx;					///< �ּ�
		unsigned int			UpperPx;				///< ��ͣ��
		unsigned int			LowerPx;				///< ��ͣ��
		double					Amount;					///< �ɽ���
		unsigned __int64		Volume;					///< �ɽ���(��/��/��)
		unsigned __int64		NumTrades;				///< �ɽ�����
		unsigned int			BidPx1;					///< ί������һ�۸�
		unsigned __int64		BidVol1;				///< ί������һ��(��/��/��)
		unsigned int			BidPx2;					///< ί�����̶��۸�
		unsigned __int64		BidVol2;				///< ί�����̶���(��/��/��)
		unsigned int			AskPx1;					///< ί������һ�۸�
		unsigned __int64		AskVol1;				///< ί������һ��(��/��/��)
		unsigned int			AskPx2;					///< ί�����̶��۸�
		unsigned __int64		AskVol2;				///< ί�����̶���(��/��/��)
		unsigned int			Voip;					///< ����ģ����Ȩ֤���
	} T_DATA;											///< ��Ʒ����
	typedef MLoopBufferSt<T_DATA>	T_TICK_QUEUE;
	static const unsigned int		s_nMaxLineCount;	///< ���֧�ֵ�TICK������,Ԥ�����Ա���10����TICK�Ļ���(һ����20�ʿ��� * 10����)
public:
	TickGenerator();
	TickGenerator( enum XDFMarket eMkID, unsigned int nDate, const std::string& sCode, double dPriceRate, T_DATA& objData, T_DATA* pBufPtr );
	TickGenerator&				operator=( const TickGenerator& obj );

	/**
	 * @brief					��ʼ��
	 * @return					0						�ɹ�
	 */
	int							Initialize();

	/**
	 * @brief					���¿���
	 * @param[in]				objData					�������
	 * @param[in]				nPreClosePx				����
	 * @param[in]				nOpenPx					���̼�
	 * @return					0						�ɹ�
	 */
	int							Update( T_DATA& objData, unsigned int nPreClosePx, unsigned int nOpenPx );

	/** 
	 * @brief					����ǰ׺
	 */
	void						SetPreName( const char* pszPreName, unsigned int nPreNameLen );

	/**
	 * @brief					��ȡ�Ŵ���
	 */
	double						GetPriceRate();

	/**
	 * @brief					����TICK�߲�����
	 */
	void						DumpTicks();

protected:
	double						m_dPriceRate;			///< �Ŵ���
	enum XDFMarket				m_eMarket;				///< �г����
	unsigned int				m_nDate;				///< YYYYMMDD����20170705��
	unsigned int				m_nMkTime;				///< �г�ʱ��
	char						m_pszCode[9];			///< ��Ʒ����
	unsigned int				m_nPreClosePx;			///< ���ռ�
	unsigned int				m_nOpenPx;				///< ���̼�
	char						m_pszPreName[8];		///< ֤ȯǰ׺
protected:
	T_TICK_QUEUE				m_objTickQueue;			///< TICK����
	T_DATA*						m_pDataCache;			///< TICK����Buffer
protected:
	FILE*						m_pDumpFile;			///< �����ļ�
	bool						m_bHasTitle;			///< ���ļ�ͷ
	bool						m_bCloseFile;			///< �Ƿ���Ҫ�ر��ļ�
};


/**
 * @class						SecurityTickCache
 * @brief						������Ʒ�����ߵĻ���
 * @author						barry
 */
class SecurityTickCache
{
public:
	typedef std::map<std::string,TickGenerator>	T_MAP_TICKS;
public:
	SecurityTickCache();
	~SecurityTickCache();

	/**
	 * @brief					�����߻����ʼ��
	 * @param[in]				nSecurityCount			�г��е���Ʒ����
	 * @return					0						�ɹ�
	 */
	int							Initialize( unsigned int nSecurityCount );

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

public:
	/**
	 * @brief					��������������߳�
	 */
	void						ActivateDumper();

	/**
	 * @brief					������Ʒ�Ĳ�����Ϣ
	 * @param[in]				eMarket			�г�ID
	 * @param[in]				sCode			��Ʒ����
	 * @param[in]				nDate			��������
	 * @param[in]				dPriceRate		�Ŵ���
	 * @param[in]				objData			����۸�
	 * @return					==0				�ɹ�
	 */
	int							NewSecurity( enum XDFMarket eMarket, const std::string& sCode, unsigned int nDate, double dPriceRate, TickGenerator::T_DATA& objData, const char* pszPreName, unsigned int nPreNamLen );

	/**
	 * @brief					������Ʒ��Ϣ
	 * @param[in]				objData			����۸�
	 * @return					==0				�ɹ�
	 */
	int							UpdateSecurity( const XDFAPI_IndexData& refObj, unsigned int nDate, unsigned int nTime );
	int							UpdateSecurity( const XDFAPI_StockData5& refObj, unsigned int nDate, unsigned int nTime );

	/**
	 * @brief					����ǰ׺
	 * @param[in]				pszPreName		ǰ׺����
	 * @param[in]				nSize			���ݳ���
	 */
	int							UpdatePreName( std::string& sCode, char* pszPreName, unsigned int nSize );

	/**
	 * @brief					��ȡ�Ŵ���
	 */
	double						GetPriceRate( std::string& sCode );

protected:
	static void*	__stdcall	DumpThread( void* pSelf );

protected:
	enum XDFMarket				m_eMarketID;			///< �г����
	SimpleThread				m_oDumpThread;			///< TICK����������
	unsigned int				m_nAlloPos;				///< �����Ѿ������λ��
	unsigned int				m_nSecurityCount;		///< ��Ʒ����
	TickGenerator::T_DATA*		m_pTickDataTable;		///< TICK�߻���
	T_MAP_TICKS					m_objMapTicks;			///< TICK��Map
	std::vector<std::string>	m_vctCode;				///< CodeInTicks
	CriticalObject				m_oLockData;			///< TICK��������
};


///< ���� ////////////////////////////////////////////////////////////////////////////////////////////////////
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

	SecurityMinCache&			GetSHL1Cache();
	SecurityMinCache&			GetSZL1Cache();
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
	SecurityMinCache			m_objMinCache4SHL1;				///< ��֤L1�����߻���
	SecurityMinCache			m_objMinCache4SZL1;				///< ��֤L1�����߻���
	SecurityTickCache			m_objTickCache4SHL1;			///< ��֤L1TICK�߻���
	SecurityTickCache			m_objTickCache4SZL1;			///< ��֤L1TICK�߻���
	unsigned int				m_lstMkTime[64];				///< ���г����г�ʱ��
};




#endif










