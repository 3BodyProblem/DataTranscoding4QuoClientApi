#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "Quotation.h"
#include "../DataTranscoding4QuoClientApi.h"


WorkStatus::WorkStatus()
: m_eWorkStatus( ET_SS_UNACTIVE )
{
}

WorkStatus::WorkStatus( const WorkStatus& refStatus )
{
	CriticalLock	section( m_oLock );

	m_eWorkStatus = refStatus.m_eWorkStatus;
}

WorkStatus::operator enum E_SS_Status()
{
	CriticalLock	section( m_oLock );

	return m_eWorkStatus;
}

std::string& WorkStatus::CastStatusStr( enum E_SS_Status eStatus )
{
	static std::string	sUnactive = "δ����";
	static std::string	sDisconnected = "�Ͽ�״̬";
	static std::string	sConnected = "��ͨ״̬";
	static std::string	sLogin = "��¼�ɹ�";
	static std::string	sInitialized = "��ʼ����";
	static std::string	sWorking = "����������";
	static std::string	sUnknow = "����ʶ��״̬";

	switch( eStatus )
	{
	case ET_SS_UNACTIVE:
		return sUnactive;
	case ET_SS_DISCONNECTED:
		return sDisconnected;
	case ET_SS_CONNECTED:
		return sConnected;
	case ET_SS_LOGIN:
		return sLogin;
	case ET_SS_INITIALIZING:
		return sInitialized;
	case ET_SS_WORKING:
		return sWorking;
	default:
		return sUnknow;
	}
}

WorkStatus&	WorkStatus::operator= ( enum E_SS_Status eWorkStatus )
{
	CriticalLock	section( m_oLock );

	if( m_eWorkStatus != eWorkStatus )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "WorkStatus::operator=() : Exchange Session Status [%s]->[%s]"
											, CastStatusStr(m_eWorkStatus).c_str(), CastStatusStr(eWorkStatus).c_str() );
				
		m_eWorkStatus = eWorkStatus;
	}

	return *this;
}


///< ----------------------------------------------------------------


Quotation::Quotation()
{
}

Quotation::~Quotation()
{
	Release();
}

WorkStatus& Quotation::GetWorkStatus()
{
	return m_oWorkStatus;
}

int Quotation::Initialize()
{
	if( GetWorkStatus() == ET_SS_UNACTIVE )
	{
		unsigned int				nSec = 0;
		int							nErrCode = 0;

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ [%s] Quotation Is Activating............" );
		Release();

		if( (nErrCode = m_oQuoDataCenter.Initialize()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize DataCenter, errorcode = %d", nErrCode );
			Release();
			return -1;
		}

		if( (nErrCode = m_oQuotPlugin.Initialize( this )) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -2;
		}

		if( (nErrCode = m_oQuotPlugin.RecoverDataCollector()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 activate Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -3;
		}

		m_oWorkStatus = ET_SS_WORKING;				///< ����Quotation�Ự��״̬
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ Quotation Activated!.............." );
	}

	return 0;
}

int Quotation::Execute()
{
	return 0;
}

int Quotation::Release()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroying .............." );

		m_oWorkStatus = ET_SS_UNACTIVE;			///< ����Quotation�Ự��״̬
		m_oQuotPlugin.Release();				///< �ͷ�����Դ���
		m_oQuoDataCenter.Release();				///< �ͷ�����������Դ

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

int Quotation::ReloadShLv1( enum XDFRunStat eStatus )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�Ϻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShLv1() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : SHL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- ��ȡ�Ϻ��г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, NULL, NULL, nCodeCount );					///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int		noffset = (sizeof(XDFAPI_NameTableSh) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
		char*	pszCodeBuf = new char[noffset];

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				char			pszCode[8] = { 0 };

				if( abs(pMsgHead->MsgType) == 5 )
				{
					T_LINE_PARAM			tagParam = { 0 };
					XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;

					tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
					m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );

					pbuf += sizeof(XDFAPI_NameTableSh);
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}
	}

	return 0;
}

int Quotation::ReloadSzLv1( enum XDFRunStat eStatus )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZ, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ���ڵĻ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzLv1() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : SZL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- ��ȡ�����г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, NULL, NULL, nCodeCount );					///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int		noffset = (sizeof(XDFAPI_NameTableSz) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
		char*	pszCodeBuf = new char[noffset];

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				char			pszCode[8] = { 0 };

				if( abs(pMsgHead->MsgType) == 6 )
				{
					T_LINE_PARAM			tagParam = { 0 };
					XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;

					tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
					m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

					pbuf += sizeof(XDFAPI_NameTableSz);
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}
	}

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	bool	bNormalStatus = false;
	char	pszDesc[128] = { 0 };

	switch( nStatus )
	{
	case 0:
		::strcpy( pszDesc, "������" );
		break;
	case 1:
		::strcpy( pszDesc, "δ֪״̬" );
		break;
	case 2:
		::strcpy( pszDesc, "��ʼ��" );
		break;
	case 5:
		{
			::strcpy( pszDesc, "�ɷ���" );
			bNormalStatus = true;
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=�Ƿ�״ֵ̬", (int)cMarket, pszDesc );
		return false;
	}

	///< ����ģ��״̬
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=%s", (int)cMarket, pszDesc );
	m_oQuoDataCenter.UpdateModuleStatus( (enum XDFMarket)cMarket, nStatus );	///< ����ģ�鹤��״̬

	///< �ж��Ƿ���Ҫ������ع���
	unsigned int	nNowT = ::time( NULL );
	if( (nNowT - m_mapMkBuildTimeT[cMarket]) <= 3 )
	{
		return true;
	}
	else
	{
		m_mapMkBuildTimeT[cMarket] = nNowT;
	}

	///< ���ظ��г�������Ϣ
	if( true == bNormalStatus )
	{
		m_vctMkSvrStatus[cMarket] = ET_SS_INITIALIZING;			///< ���á���ʼ�С�״̬��ʶ

		switch( (enum XDFMarket)cMarket )
		{
		case XDF_SH:		///< ��֤L1
			ReloadShLv1( (enum XDFRunStat)nStatus );
			break;
		case XDF_SHL2:		///< ��֤L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_SHOPT:		///< ��֤��Ȩ
			break;
		case XDF_SZ:		///< ��֤L1
			ReloadSzLv1( (enum XDFRunStat)nStatus );
			break;
		case XDF_SZOPT:		///< ������Ȩ
			break;
		case XDF_SZL2:		///< ����L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_CF:		///< �н��ڻ�
			break;
		case XDF_ZJOPT:		///< �н���Ȩ
			break;
		case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
			break;
		case XDF_CNFOPT:	///< ��Ʒ�ڻ�����Ʒ��Ȩ(�Ϻ�/֣��/����)
			break;
		default:
			return false;
		}

		m_vctMkSvrStatus[cMarket] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	}

	return true;
}

void __stdcall	Quotation::XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes )
{
	int						nLen = nBytes;
	char*					pbuf = (char *)pszBuf;
	XDFAPI_UniMsgHead*		pMsgHead = NULL;
	unsigned char			nMarketID = pHead->MarketID;

	if( m_vctMkSvrStatus[nMarketID] != ET_SS_WORKING )				///< ����г�δ�������Ͳ����ջص�
	{
		return;
	}

	switch( nMarketID )
	{
		case XDF_SH:												///< �����Ϻ�Lv1����������
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< �������ݰ�
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< ָ������
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< ��һ���ݰ�
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< �г�״̬��Ϣ
						{
							//XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;
							//::printf( "�г�����=%u,ʱ��=%u\n", pData->MarketDate, pData->MarketTime );
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< ͣ�Ʊ�ʶ
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "ͣ�Ʊ�ʶ=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		case XDF_SZ:												///< ��������Lv1����������
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< �������ݰ�
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< ָ������
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 24:///< XDFAPI_PreNameChg					///< ǰ׺�����
						nLen -= pMsgHead->MsgLen;
						break;
					}
				}
				else												///< ��һ���ݰ�
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< �г�״̬��Ϣ
						{
							//XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;
							//::printf( "�г�����=%u,ʱ��=%u\n", pData->MarketDate, pData->MarketTime );
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< ͣ�Ʊ�ʶ
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "ͣ�Ʊ�ʶ=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		case XDF_CF:												///< �����н��ڻ�����������
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< �������ݰ�
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 20:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								char				pszCode[8] = { 0 };
								XDFAPI_CffexData*	pData = (XDFAPI_CffexData*)pbuf;

								if( ::strncmp( pData->Code, "600098", 6 ) == 0 )
									int a = 0;
								::memcpy( pszCode, pData->Code, 6 );
								::printf( "����:%s, �۸�:%u\n", pszCode, pData->Now );

								pbuf += sizeof(XDFAPI_CffexData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< ��һ���ݰ�
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< �г�״̬��Ϣ
						{
							XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;

							::printf( "�г�����=%u,ʱ��=%u\n", pData->MarketDate, pData->MarketTime );

							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}///< end case
	}
}

void __stdcall	Quotation::XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel, const char * pLogBuf )
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspOutLog() : %s", pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









