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

int Quotation::ReloadShLv1( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�Ϻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShLv1() : cannot fetch market infomation." );
		return -2;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 WareCount = %d", pKindHead->WareCount );

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
		if( true == bBuild )
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
					if( abs(pMsgHead->MsgType) == 5 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );
						nNum++;

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShLv1() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SH] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SH, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 21 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< ��������
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );
				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShLv1() : Loading... SHL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadShOpt( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SHOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�Ϻ���Ȩ�Ļ�����Ϣ -----------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShOpt() : Loading... SHOPT WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ�Ϻ���Ȩ���г�������� ------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, NULL, NULL, nCodeCount );				///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableShOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf + m + sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 2 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableShOpt*	pData = (XDFAPI_NameTableShOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( pData->Code, 8 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableShOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadShOpt() : Loading... SHOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadShOpt() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SHOPT] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ShOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SHOPT, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 15 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );
				pbuf += sizeof(XDFAPI_ShOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSHOPT() : Loading... SHLOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadSzLv1( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
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
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 WareCount = %d", pKindHead->WareCount );

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
		if( true == bBuild )
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
					if( abs(pMsgHead->MsgType) == 6 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						::memcpy( tagParam.PreName, pData->PreName, 4 );
						m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableSz);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzLv1() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SZ] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZ, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 21 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< ��������
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );
				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzLv1() : Loading... SZL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadSzOpt( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ������Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSzOpt() : Loading... SZOPT WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ������Ȩ�г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, NULL, NULL, nCodeCount );					///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableSzOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 9 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableSzOpt*	pData = (XDFAPI_NameTableSzOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( pData->Code, 8 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableSzOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadSzOpt() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_SZOPT] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_SzOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZOPT, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 35 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );
				pbuf += sizeof(XDFAPI_SzOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCFF( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CF, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�н��ڻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ�н��ڻ��г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, NULL, NULL, nCodeCount );					///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableZjqh) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 4 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableZjqh*	pData = (XDFAPI_NameTableZjqh*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableZjqh);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFF() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CF] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CffexData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CF, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 20 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );
				pbuf += sizeof(XDFAPI_CffexData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFF() : Loading... CFF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCFFOPT( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_ZJOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�н���Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ�н���Ȩ�г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, NULL, NULL, nCodeCount );				///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableZjOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 3 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableZjOpt*	pData = (XDFAPI_NameTableZjOpt*)pbuf;

						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_ZJOPT, std::string( pData->Code ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableZjOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCFFOPT() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_ZJOPT] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ZjOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_ZJOPT, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 18 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );
				pbuf += sizeof(XDFAPI_ZjOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCFFOPT() : Loading... CFFOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCNF( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNF, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ��Ʒ�ڻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ��Ʒ�ڻ��г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, NULL, NULL, nCodeCount );				///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableCnf) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 7 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableCnf*	pData = (XDFAPI_NameTableCnf*)pbuf;

						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( pData->Code, 6 ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableCnf);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNF() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNF] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutureData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];
	char	cMkID = XDF_CNF;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNF, pszCodeBuf, noffset );			///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 26 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagStatus.MarketDate );
				pbuf += sizeof(XDFAPI_CNFutureData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNF() : Loading... CNF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::ReloadCNFOPT( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNFOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ��Ʒ��Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT WareCount = %d", pKindHead->WareCount );

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

	///< ---------------- ��ȡ��Ʒ��Ȩ�г�������� ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, NULL, NULL, nCodeCount );					///< �Ȼ�ȡһ����Ʒ����
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		if( true == bBuild )
		{
			int		noffset = (sizeof(XDFAPI_NameTableCnfOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
			char*	pszCodeBuf = new char[noffset];

			nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, pszCodeBuf, noffset, nCodeCount );	///< ��ȡ���
			for( int m = 0; m < nErrorCode; )
			{
				XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
				char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
				int					MsgCount = pMsgHead->MsgCount;

				for( int i = 0; i < MsgCount; i++ )
				{
					if( abs(pMsgHead->MsgType) == 11 )
					{
						T_LINE_PARAM			tagParam = { 0 };
						XDFAPI_NameTableCnfOpt*	pData = (XDFAPI_NameTableCnfOpt*)pbuf;

						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( pData->Code ), tagParam );

						pbuf += sizeof(XDFAPI_NameTableCnfOpt);
						nNum++;
					}
				}

				m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
			}

			if( NULL != pszCodeBuf )
			{
				delete []pszCodeBuf;
				pszCodeBuf = NULL;
			}

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::ReloadCNFOPT() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNFOPT] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];
	char	cMkID = XDF_CNFOPT;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNFOPT, pszCodeBuf, noffset );		///< ��ȡ����
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			if( abs(pMsgHead->MsgType) == 34 )			///< ָ��
			{
				m_oQuoDataCenter.UpdateDayLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagStatus.MarketDate );
				pbuf += sizeof(XDFAPI_CNFutOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadCNFOPT() : Loading... CNFOPT Snaptable Size = %d", nNum );

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	bool	bNormalStatus = false;
	char	pszDesc[128] = { 0 };

	if( XDF_CF == cMarket && nStatus >= 2 )		///< �˴�Ϊ�ر����н��ڻ������С��ɷ���״̬��֪ͨ(BUG)
	{
		nStatus = XRS_Normal;
		bNormalStatus = true;
	}

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

	static CriticalObject	oLock;				///< �ٽ�������
	CriticalLock			section( oLock );

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
			ReloadShLv1( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SHL2:		///< ��֤L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_SHOPT:		///< ��֤��Ȩ
			ReloadShOpt( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZ:		///< ��֤L1
			ReloadSzLv1( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZOPT:		///< ������Ȩ
			ReloadSzOpt( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZL2:		///< ����L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_CF:		///< �н��ڻ�
			ReloadCFF( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_ZJOPT:		///< �н���Ȩ
			ReloadCFFOPT( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
			ReloadCNF( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNFOPT:	///< ��Ʒ�ڻ�����Ʒ��Ȩ(�Ϻ�/֣��/����)
			ReloadCNFOPT( (enum XDFRunStat)nStatus, true );
			break;
		default:
			return false;
		}
	}

	///< ���������߳�
	m_oQuoDataCenter.BeginDumpThread( (enum XDFMarket)cMarket, nStatus );

	return true;
}

void Quotation::FlushSnapOnCloseTime()
{
	MAP_MK_CLOSECFG&		refCloseCfg = Configuration::GetConfig().GetMkCloseCfg();
	static CriticalObject	oLock;				///< �ٽ�������
	CriticalLock			section( oLock );

	for( MAP_MK_CLOSECFG::iterator it = refCloseCfg.begin(); it != refCloseCfg.end(); it++ )
	{
		short			nMkID = it->first;
		TB_MK_CLOSECFG&	refCfg = it->second;
		short			nMkStatus = m_oQuoDataCenter.GetModuleStatus( (enum XDFMarket)nMkID );

		if( m_vctMkSvrStatus[nMkID] != ET_SS_WORKING )
		{
			continue;
		}

		if( nMkStatus < 5 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushSnapOnCloseTime() : Ignore Closing Prices : Market(%d), Status=%d", nMkID, nMkStatus );
			continue;
		}

		for( TB_MK_CLOSECFG::iterator itCfg = refCfg.begin(); itCfg != refCfg.end(); itCfg++ )
		{
			CLOSE_CFG&		refCfg = *itCfg;
			unsigned int	nNowT = DateTime::Now().TimeToLong();
			unsigned int	nDate = DateTime::Now().DateToLong();
			int				nDiffTime = nNowT - refCfg.CloseTime;

			if( ( (nDiffTime>0&&nDiffTime<1500) || 0==refCfg.LastDate ) && ( refCfg.LastDate!=nDate ) && ( nNowT >= refCfg.CloseTime ) )
			{
				refCfg.LastDate = nDate;
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushSnapOnCloseTime() : Fetching Closing Prices : Market(%d), Status=%d, CloseTime=%d", nMkID, nMkStatus, refCfg.CloseTime );	

				switch( (enum XDFMarket)nMkID )
				{
				case XDF_SH:		///< ��֤L1
					ReloadShLv1( XRS_Normal, false );
					break;
				case XDF_SHL2:		///< ��֤L2(QuoteClientApi�ڲ�����)
					break;
				case XDF_SHOPT:		///< ��֤��Ȩ
					ReloadShOpt( XRS_Normal, false );
					break;
				case XDF_SZ:		///< ��֤L1
					ReloadSzLv1( XRS_Normal, false );
					break;
				case XDF_SZOPT:		///< ������Ȩ
					ReloadSzOpt( XRS_Normal, false );
					break;
				case XDF_SZL2:		///< ����L2(QuoteClientApi�ڲ�����)
					break;
				case XDF_CF:		///< �н��ڻ�
					ReloadCFF( XRS_Normal, false );
					break;
				case XDF_ZJOPT:		///< �н���Ȩ
					ReloadCFFOPT( XRS_Normal, false );
					break;
				case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
					ReloadCNF( XRS_Normal, false );
					break;
				case XDF_CNFOPT:	///< ��Ʒ�ڻ�����Ʒ��Ȩ(�Ϻ�/֣��/����)
					ReloadCNFOPT( XRS_Normal, false );
					break;
				default:
					break;
				}
			}
		}
	}
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
		break;
		case XDF_SHOPT:												///< �����Ϻ���Ȩ����������
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
					case 15:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );

								pbuf += sizeof(XDFAPI_ShOptData);
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
							pbuf += sizeof(XDFAPI_ShOptMarketStatus);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
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
						{
							while( nMsgCount-- > 0 )
							{
								XDFAPI_PreNameChg*	pPreNameChg = (XDFAPI_PreNameChg*)pbuf;

								m_oQuoDataCenter.UpdatePreName( XDF_SZ, std::string( pPreNameChg->Code, 6 ), (char*)(pPreNameChg->PreName + 0), sizeof(pPreNameChg->PreName) );

								pbuf += sizeof(XDFAPI_PreNameChg);
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
		break;
		case XDF_SZOPT:												///< ����������Ȩ����������
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
					case 35:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );

								pbuf += sizeof(XDFAPI_SzOptData);
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
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
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
								m_oQuoDataCenter.UpdateDayLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );

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
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_ZJOPT:												///< �����н���Ȩ����������
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
					case 18:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );

								pbuf += sizeof(XDFAPI_ZjOptData);
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
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNF:												///< ������Ʒ�ڻ���������
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
					case 26:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData) );

								pbuf += sizeof(XDFAPI_CNFutureData);
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
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNFOPT:											///< ������Ʒ��Ȩ��������
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
					case 34:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateDayLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData) );

								pbuf += sizeof(XDFAPI_CNFutOptData);
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
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
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









