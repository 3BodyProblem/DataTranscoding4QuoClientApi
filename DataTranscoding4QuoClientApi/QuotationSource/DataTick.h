#ifndef __DATA_TICK_H__
#define __DATA_TICK_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


///< ���ݽṹ���� //////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
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
#pragma pack()


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


#endif










