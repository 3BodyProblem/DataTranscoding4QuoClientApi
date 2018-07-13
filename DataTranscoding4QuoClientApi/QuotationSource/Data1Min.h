#ifndef __DATA_1MINUTE_H__
#define __DATA_1MINUTE_H__


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
	 * @param[in]				nPreClose				����
	 * @return					0						�ɹ�
	 */
	int							Update( T_DATA& objData, int nPreClose );

	/**
	 * @brief					���ɷ����߲�����
	 * @param[in]				bMarketClosed			�г����б�ʶ��true����Ҫ�������µ�ȫ��1�����ߣ�
	 */
	void						DumpMinutes( bool bMarketClosed );
protected:
	double						m_dAmountBefore930;		///< 9:30ǰ�Ľ��
	unsigned __int64			m_nVolumeBefore930;		///< 9:30ǰ����
	unsigned __int64			m_nNumTradesBefore930;	///< 9:30ǰ�ı���
	int							m_nPreClose;			///< ����
protected:
	double						m_dPriceRate;			///< �Ŵ���
	enum XDFMarket				m_eMarket;				///< �г����
	unsigned int				m_nDate;				///< YYYYMMDD����20170705��
	char						m_pszCode[9];			///< ��Ʒ����
protected:
	T_DATA*						m_pDataCache;			///< 241��1���ӻ���
	short						m_nBeginOffset;			///< ��һ����ʵд�����ݵ�����(������в���������֮ǰ�ķ����߲�Ӧ�����̣���ȻΪȫ���¼)
	short						m_nWriteSize;			///< �Ѿ�����̵ķ�����λ��(�����ظ�����)
	short						m_nDataSize;			///< ���ݳ���
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




#endif










