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


//T_MAP_RATE		Quotation::m_mapRate;
//T_MAP_KIND		Quotation::m_mapKind;


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

		m_oWorkStatus = ET_SS_UNACTIVE;		///< ����Quotation�Ự��״̬

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
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
		::strcpy( pszDesc, "�ɷ���" );
		break;
	default:
		::strcpy( pszDesc, "�Ƿ�״ֵ̬" );
		break;
	}

	::printf( "�г�(%d), ״̬:%s\n", cMarket, pszDesc );		///< �г�״̬�仯֪ͨ

	return false;
}

void __stdcall	Quotation::XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes )
{
	int						nLen = nBytes;
	char*					pbuf = (char *)pszBuf;
	XDFAPI_UniMsgHead*		pMsgHead = NULL;

	if( XDF_SH == pHead->MarketID )								///< �����Ϻ�Lv1����������
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
							char				pszCode[8] = { 0 };
							XDFAPI_IndexData*	pData = (XDFAPI_IndexData*)pbuf;

							::memcpy( pszCode, pData->Code, 6 );
							::printf( "ָ��:%s, �۸�:%u\n", pszCode, pData->Now );

							pbuf += sizeof(XDFAPI_IndexData);
						}
						nLen -= pMsgHead->MsgLen;
					}
					break;
				case 22:										///< ��Ʒ����
					{
						while( nMsgCount-- > 0 )
						{
							char				pszCode[8] = { 0 };
							XDFAPI_StockData5*	pData = (XDFAPI_StockData5*)pbuf;

							if( ::strncmp( pData->Code, "600098", 6 ) == 0 )
								int a = 0;
							::memcpy( pszCode, pData->Code, 6 );
							::printf( "����:%s, �۸�:%u\n", pszCode, pData->Now );

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
						XDFAPI_MarketStatusInfo* pData = (XDFAPI_MarketStatusInfo*)pbuf;

						::printf( "�г�����=%u,ʱ��=%u\n", pData->MarketDate, pData->MarketTime );

						pbuf += sizeof(XDFAPI_MarketStatusInfo);
						nLen -= pMsgHead->MsgLen;
					}
					break;
				case 23:///< ͣ�Ʊ�ʶ
					{
						XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;

						::printf( "ͣ�Ʊ�ʶ=%c\n", pData->SFlag );

						pbuf += sizeof(XDFAPI_StopFlag);
						nLen -= pMsgHead->MsgLen;
					}
					break;
				}
			}
		}
	}
	else if( XDF_CF == pHead->MarketID )								///< �����н��ڻ�����������
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
	}
}

void __stdcall	Quotation::XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel, const char * pLogBuf )
{
	::printf( "[Plugin] Type=%u, Level=%u, Log=%s \n", nLogType, nLogLevel, pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









