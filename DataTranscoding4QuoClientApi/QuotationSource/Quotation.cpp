#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "Quotation.h"
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
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

		if( (nErrCode = m_oQuoDataCenter.Initialize( this )) < 0 )
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

void Quotation::Halt()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halting .............." );

///<		m_oWorkStatus = ET_SS_UNACTIVE;			///< ����Quotation�Ự��״̬
///<		m_oQuotPlugin.Release();				///< �ͷ�����Դ���
///<		m_oQuoDataCenter.Release();				///< �ͷ�����������Դ

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halted! .............." );
	}
}

int Quotation::Release()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroying .............." );

		m_oQuoDataCenter.StopThreads();			///< ֹͣ���������߳�
		m_oWorkStatus = ET_SS_UNACTIVE;			///< ����Quotation�Ự��״̬
		m_oQuotPlugin.Release();				///< �ͷ�����Դ���
		m_oQuoDataCenter.Release();				///< �ͷ�����������Դ

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

std::string& Quotation::QuoteApiVersion()
{
	return m_oQuotPlugin.GetVersion();
}

__inline bool	PrepareStaticFile( T_STATIC_LINE& refStaticLine, std::ofstream& oDumper )
{
	char	pszFilePath[512] = { 0 };

	switch( refStaticLine.eMarketID )
	{
	case XDF_SH:		///< �Ϻ�Lv1
	case XDF_SHOPT:		///< �Ϻ���Ȩ
	case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SSE/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_SZ:		///< ��֤Lv1
	case XDF_SZOPT:		///< ������Ȩ
	case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
		::sprintf( pszFilePath, "SZSE/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_CF:		///< �н��ڻ�
	case XDF_ZJOPT:		///< �н���Ȩ
		::sprintf( pszFilePath, "CFFEX/STATIC/%d/", refStaticLine.Date/10000 );
		break;
	case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
	case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
		{
			switch( refStaticLine.Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/STATIC/%d/", refStaticLine.Date/10000 );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : invalid market subid (Type=%d)", refStaticLine.Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : invalid market id (%s)", refStaticLine.eMarketID );
		return false;
	}

	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "STATIC%u.csv", refStaticLine.Date/10000 );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareStaticFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,code,name,lotsize,contractmult,contractunit,startdate,enddate,xqdate,deliverydate,expiredate,underlyingcode,underlyingname,optiontype,callorput,exercisepx\n";
		oDumper << sTitle;
	}

	return true;
}

int Quotation::SaveShLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�Ϻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot fetch market infomation." );
		return -2;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 WareCount = %d", pKindHead->WareCount );

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

	char						nMkID = XDF_SH;
	XDFAPI_MarketStatusInfo		tagStatus = { 0 };
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &nMkID, &tagStatus );
	if( 1 == nErrorCode )
	{
		m_oQuoDataCenter.UpdateMarketTime( XDF_SH, tagStatus.MarketTime );
	}
	else
	{
		m_oQuoDataCenter.UpdateMarketTime( XDF_SH, DateTime::Now().TimeToLong() );
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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;

						///< ������Ʒ����
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );
						nNum++;

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SH;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SH, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );	///< Tick��
				}
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< ��������
			{
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );	///< Tick��
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1_Static_Tick_Day() : Loading... SHL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveShOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SHOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�Ϻ���Ȩ�Ļ�����Ϣ -----------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShOpt_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShOpt_Static_Tick_Day() : Loading... SHOPT WareCount = %d", pKindHead->WareCount );

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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableShOpt*	pData = (XDFAPI_NameTableShOpt*)pbuf;

						///< �������ݼ���
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( pData->Code, 8 ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SHOPT;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						///<tagStaticLine.ContractMult = 0;
						tagStaticLine.ContractUnit = pData->ContractUnit;
						tagStaticLine.StartDate = pData->StartDate;
						tagStaticLine.EndDate = pData->EndDate;
						tagStaticLine.XqDate = pData->XqDate;
						tagStaticLine.DeliveryDate = pData->DeliveryDate;
						tagStaticLine.ExpireDate = pData->ExpireDate;
						::memcpy( tagStaticLine.UnderlyingCode, pData->UnderlyingCode, sizeof(pData->UnderlyingCode) );
						::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SHOPT, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShOpt_Static_Tick_Day() : Loading... SHOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShOpt_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );
				}

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

int Quotation::SaveSzLv1_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZ, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ���ڵĻ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 WareCount = %d", pKindHead->WareCount );

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

	char						nMkID = XDF_SZ;
	XDFAPI_MarketStatusInfo		tagStatus = { 0 };
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &nMkID, &tagStatus );
	if( 1 == nErrorCode )
	{
		m_oQuoDataCenter.UpdateMarketTime( XDF_SZ, tagStatus.MarketTime );
	}
	else
	{
		m_oQuoDataCenter.UpdateMarketTime( XDF_SZ, DateTime::Now().TimeToLong() );
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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;

						///< ���ݼ��Ϲ���
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						::memcpy( tagParam.PreName, pData->PreName, 4 );
						m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SZ;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SZ, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );	///< Tick��
				}
				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< ��������
			{
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );	///< Tick��
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1_Static_Tick_Day() : Loading... SZL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveSzOpt_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ������Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzOpt_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzOpt_Static_Tick_Day() : Loading... SZOPT WareCount = %d", pKindHead->WareCount );

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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableSzOpt*	pData = (XDFAPI_NameTableSzOpt*)pbuf;

						///< �������ݼ���
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						tagParam.UpperPrice = pData->UpLimit;
						tagParam.LowerPrice = pData->DownLimit;
						tagParam.PreClosePx = pData->PreClosePx;
						tagParam.PreSettlePx = pData->PreSettlePx;
						tagParam.PrePosition = pData->LeavesQty;
						m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( pData->Code, 8 ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_SZOPT;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						///<tagStaticLine.ContractMult = 0;
						tagStaticLine.ContractUnit = pData->ContractUnit;
						tagStaticLine.StartDate = pData->StartDate;
						tagStaticLine.EndDate = pData->EndDate;
						tagStaticLine.XqDate = pData->XqDate;
						tagStaticLine.DeliveryDate = pData->DeliveryDate;
						tagStaticLine.ExpireDate = pData->ExpireDate;
						::memcpy( tagStaticLine.UnderlyingCode, pData->UnderlyingCode, sizeof(pData->UnderlyingCode) );
						//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_SZOPT, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzOpt_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );
				}
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

int Quotation::SaveCFF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
{
	if( XRS_Normal != eStatus )
	{
		return -1;
	}

	int						nNum = 0;
	std::ofstream			oDumper;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CF, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�н��ڻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFF_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF_Static_Tick_Day() : Loading... CFF WareCount = %d", pKindHead->WareCount );

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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableZjqh*	pData = (XDFAPI_NameTableZjqh*)pbuf;

						///< �������ݼ���
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( pData->Code, 6 ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_CF;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						tagStaticLine.ContractMult = pData->ContractMult;
						//tagStaticLine.ContractUnit = pData->ContractUnit;
						//tagStaticLine.StartDate = pData->StartDate;
						//tagStaticLine.EndDate = pData->EndDate;
						//tagStaticLine.XqDate = pData->XqDate;
						//tagStaticLine.DeliveryDate = pData->DeliveryDate;
						//tagStaticLine.ExpireDate = pData->ExpireDate;
						::memcpy( tagStaticLine.UnderlyingCode, pData->ObjectCode, sizeof(pData->ObjectCode) );
						//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						//tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						//tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						//tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_CF, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF_Static_Tick_Day() : Loading... CFF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFF_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF_Static_Tick_Day() : Loading... CFF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCFFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_ZJOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ�н���Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFFOPT_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT_Static_Tick_Day() : Loading... CFFOPT WareCount = %d", pKindHead->WareCount );

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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableZjOpt*	pData = (XDFAPI_NameTableZjOpt*)pbuf;

						///< �������ݼ���
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_ZJOPT, std::string( pData->Code ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = 0;
						tagStaticLine.eMarketID = XDF_ZJOPT;
						tagStaticLine.Date = DateTime::Now().DateToLong();
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						//::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						tagStaticLine.ContractMult = pData->ContractMult;
						tagStaticLine.ContractUnit = pData->ContractUnit;
						tagStaticLine.StartDate = pData->StartDate;
						tagStaticLine.EndDate = pData->EndDate;
						tagStaticLine.XqDate = pData->XqDate;
						tagStaticLine.DeliveryDate = pData->DeliveryDate;
						tagStaticLine.ExpireDate = pData->ExpireDate;
						::memcpy( tagStaticLine.UnderlyingCode, pData->ObjectCode, sizeof(pData->ObjectCode) );
						//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						//tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						//tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						//tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_ZJOPT, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT_Static_Tick_Day() : Loading... CFFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFFOPT_Static_Tick_Day() : cannot fetch nametable" );
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData), 0 );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT_Static_Tick_Day() : Loading... CFFOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCNF_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNF, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ��Ʒ�ڻ��Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNF_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF_Static_Tick_Day() : Loading... CNF WareCount = %d", pKindHead->WareCount );

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
	char	cMkID = XDF_CNF;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );
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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableCnf*	pData = (XDFAPI_NameTableCnf*)pbuf;

						///< �������ݼ���
						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( pData->Code, 6 ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = tagParam.Type;
						tagStaticLine.eMarketID = XDF_CNF;
						tagStaticLine.Date = tagStatus.MarketDate;
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						//tagStaticLine.ContractMult = pData->ContractMult;
						//tagStaticLine.ContractUnit = pData->ContractUnit;
						//tagStaticLine.StartDate = pData->StartDate;
						//tagStaticLine.EndDate = pData->EndDate;
						//tagStaticLine.XqDate = pData->XqDate;
						//tagStaticLine.DeliveryDate = pData->DeliveryDate;
						//tagStaticLine.ExpireDate = pData->ExpireDate;
						//::memcpy( tagStaticLine.UnderlyingCode, pData->ObjectCode, sizeof(pData->ObjectCode) );
						//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						//tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						//tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						//tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_CNF, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF_Static_Tick_Day() : Loading... CNF Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNF_Static_Tick_Day() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNF] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutureData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];
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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagStatus.MarketDate );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagStatus.MarketDate );
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF_Static_Tick_Day() : Loading... CNF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCNFOPT_Static_Tick_Day( enum XDFRunStat eStatus, bool bBuild )
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
	CriticalLock			section( m_oLock );
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNFOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- ��ȡ��Ʒ��Ȩ�Ļ�����Ϣ --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNFOPT_Static_Tick_Day() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT_Static_Tick_Day() : Loading... CNFOPT WareCount = %d", pKindHead->WareCount );

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
	char	cMkID = XDF_CNFOPT;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );
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
						std::ofstream			oDumper;
						T_LINE_PARAM			tagParam = { 0 };
						char					pszLine[1024] = { 0 };
						T_STATIC_LINE			tagStaticLine = { 0 };
						XDFAPI_NameTableCnfOpt*	pData = (XDFAPI_NameTableCnfOpt*)pbuf;

						///< �������ݼ���
						tagParam.Type = pData->SecKind;
						tagParam.dPriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( pData->Code ), tagParam );

						///< ��̬��������
						tagStaticLine.Type = tagParam.Type;
						tagStaticLine.eMarketID = XDF_CNFOPT;
						tagStaticLine.Date = tagStatus.MarketDate;
						::memcpy( tagStaticLine.Code, pData->Code, sizeof(tagStaticLine.Code)-1 );
						::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
						tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
						tagStaticLine.ContractMult = pData->ContractMult;
						//tagStaticLine.ContractUnit = pData->ContractUnit;
						tagStaticLine.StartDate = pData->StartDate;
						tagStaticLine.EndDate = pData->EndDate;
						tagStaticLine.XqDate = pData->XqDate;
						tagStaticLine.DeliveryDate = pData->DeliveryDate;
						tagStaticLine.ExpireDate = pData->ExpireDate;
						::memcpy( tagStaticLine.UnderlyingCode, pData->UnderlyingCode, sizeof(pData->UnderlyingCode) );
						//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
						//tagStaticLine.OptionType = pData->OptionType;		///< ��Ȩ���ͣ�'E'-ŷʽ 'A'-��ʽ
						//tagStaticLine.CallOrPut = pData->CallOrPut;			///< �Ϲ��Ϲ���'C'�Ϲ� 'P'�Ϲ�
						tagStaticLine.ExercisePx = pData->XqPrice / tagParam.dPriceRate;
						ServerStatus::GetStatusObj().AnchorSecurity( XDF_CNFOPT, tagStaticLine.Code, tagStaticLine.Name );
						if( true == PrepareStaticFile( tagStaticLine, oDumper ) )
						{
							int		nLen = ::sprintf( pszLine, "%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%c,%c,%.4f\n"
								, tagStaticLine.Date, tagStaticLine.Code, tagStaticLine.Name, tagStaticLine.LotSize, tagStaticLine.ContractMult, tagStaticLine.ContractUnit
								, tagStaticLine.StartDate, tagStaticLine.EndDate, tagStaticLine.XqDate, tagStaticLine.DeliveryDate, tagStaticLine.ExpireDate
								, tagStaticLine.UnderlyingCode, tagStaticLine.UnderlyingName, tagStaticLine.OptionType, tagStaticLine.CallOrPut, tagStaticLine.ExercisePx );
							oDumper.write( pszLine, nLen );
						}

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

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT_Static_Tick_Day() : Loading... CNFOPT Nametable Size = %d", nNum );
		}
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNFOPT_Static_Tick_Day() : cannot fetch nametable" );
		return -3;
	}

	m_vctMkSvrStatus[XDF_CNFOPT] = ET_SS_WORKING;				///< ���á��ɷ���״̬��ʶ
	nNum = 0;
	///< ---------------- ��ȡ���ձ����� -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< ������Ʒ�����������ȡ���ձ���Ҫ�Ļ���
	char*	pszCodeBuf = new char[noffset];

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
				if( false == bBuild )
				{
					m_oQuoDataCenter.DumpDayLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagStatus.MarketDate );	///< ����
				}
				else
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagStatus.MarketDate );
				}
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

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT_Static_Tick_Day() : Loading... CNFOPT Snaptable Size = %d", nNum );

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	bool	bNormalStatus = false;
	char	pszDesc[128] = { 0 };

	if( true == SimpleThread::GetGlobalStopFlag() )
	{
		return true;
	}

	if( XDF_CF == cMarket && nStatus >= 2 )		///< �˴�Ϊ�ر����н��ڻ������С��ɷ���״̬��֪ͨ(BUG)
	{
		nStatus = XRS_Normal;
		bNormalStatus = true;
	}

	switch( nStatus )
	{
	case 0:
		{
			::strcpy( pszDesc, "������" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 1:
		{
			::strcpy( pszDesc, "δ֪״̬" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 2:
		{
			::strcpy( pszDesc, "��ʼ��" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 5:
		{
			bNormalStatus = true;
			::strcpy( pszDesc, "�ɷ���" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, "������" );
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

	///< ���ظ��г�������Ϣ
	if( true == bNormalStatus )
	{
		m_vctMkSvrStatus[cMarket] = ET_SS_INITIALIZING;			///< ���á���ʼ�С�״̬��ʶ

		switch( (enum XDFMarket)cMarket )
		{
		case XDF_SH:		///< ��֤L1
			SaveShLv1_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SHL2:		///< ��֤L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_SHOPT:		///< ��֤��Ȩ
			SaveShOpt_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZ:		///< ��֤L1
			SaveSzLv1_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZOPT:		///< ������Ȩ
			SaveSzOpt_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_SZL2:		///< ����L2(QuoteClientApi�ڲ�����)
			break;
		case XDF_CF:		///< �н��ڻ�
			SaveCFF_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_ZJOPT:		///< �н���Ȩ
			SaveCFFOPT_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
			SaveCNF_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		case XDF_CNFOPT:	///< ��Ʒ�ڻ�����Ʒ��Ȩ(�Ϻ�/֣��/����)
			SaveCNFOPT_Static_Tick_Day( (enum XDFRunStat)nStatus, true );
			break;
		default:
			return false;
		}
	}

	return true;
}

void Quotation::FlushDayLineOnCloseTime()
{
	MAP_MK_CLOSECFG&		refCloseCfg = Configuration::GetConfig().GetMkCloseCfg();
	static CriticalObject	oLock;				///< �ٽ�������
	CriticalLock			section( oLock );

	for( MAP_MK_CLOSECFG::iterator it = refCloseCfg.begin(); it != refCloseCfg.end(); it++ )
	{
		short				nMkID = it->first;
		TB_MK_CLOSECFG&		refCfg = it->second;
		short				nMkStatus = m_oQuoDataCenter.GetModuleStatus( (enum XDFMarket)nMkID );

		if( m_vctMkSvrStatus[nMkID] != ET_SS_WORKING )
		{
			continue;
		}

		if( nMkStatus < 5 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : Ignore Closing Prices : Market(%d), Status=%d", nMkID, nMkStatus );
			continue;
		}

		for( TB_MK_CLOSECFG::iterator itCfg = refCfg.begin(); itCfg != refCfg.end(); itCfg++ )
		{
			CLOSE_CFG&		refCfg = *itCfg;
			unsigned int	nNowT = DateTime::Now().TimeToLong();
			unsigned int	nDate = DateTime::Now().DateToLong();
			int				nDiffTime = nNowT - refCfg.CloseTime;

			if( true == SimpleThread::GetGlobalStopFlag() )
			{
				return;
			}

			if( ( (nDiffTime>0&&nDiffTime<1500) || 0==refCfg.LastDate ) && ( refCfg.LastDate!=nDate ) && ( nNowT >= refCfg.CloseTime ) )
			{
				refCfg.LastDate = nDate;
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::FlushDayLineOnCloseTime() : Fetching Closing Prices : Market(%d), Status=%d, CloseTime=%d", nMkID, nMkStatus, refCfg.CloseTime );	

				ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)nMkID, "������" );
				switch( (enum XDFMarket)nMkID )
				{
				case XDF_SH:		///< ��֤L1
					SaveShLv1_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_SHL2:		///< ��֤L2(QuoteClientApi�ڲ�����)
					break;
				case XDF_SHOPT:		///< ��֤��Ȩ
					SaveShOpt_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_SZ:		///< ��֤L1
					SaveSzLv1_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_SZOPT:		///< ������Ȩ
					SaveSzOpt_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_SZL2:		///< ����L2(QuoteClientApi�ڲ�����)
					break;
				case XDF_CF:		///< �н��ڻ�
					SaveCFF_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_ZJOPT:		///< �н���Ȩ
					SaveCFFOPT_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
					SaveCNF_Static_Tick_Day( XRS_Normal, false );
					break;
				case XDF_CNFOPT:	///< ��Ʒ�ڻ�����Ʒ��Ȩ(�Ϻ�/֣��/����)
					SaveCNFOPT_Static_Tick_Day( XRS_Normal, false );
					break;
				default:
					break;
				}
			}

			if( nNowT >= refCfg.CloseTime )
			{
				ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)nMkID, "���̺�" );
			}
		}
	}
}

void Quotation::UpdateMarketsTime()
{
	QuotePrimeApi*			pQueryApi = m_oQuotPlugin.GetPrimeApi();
	enum XDFMarket			vctMkID[] = { XDF_SH, XDF_SHOPT, XDF_SZ, XDF_SZOPT, XDF_CF, XDF_ZJOPT, XDF_CNF, XDF_CNFOPT };

	if( NULL != pQueryApi )
	{
		for( int n = 0; n < 8; n++ )
		{
			int						nErrCode = 0;
			char					nMkID = vctMkID[n];
			XDFAPI_MarketStatusInfo	tagStatus = { 0 };

			nErrCode = pQueryApi->ReqFuncData( 101, &nMkID, &tagStatus );
			if( 1 == nErrCode )
			{
				ServerStatus::GetStatusObj().UpdateMkTime( vctMkID[n], tagStatus.MarketDate, tagStatus.MarketTime );
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
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5) );

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
							m_oQuoDataCenter.UpdateMarketTime( XDF_SH, ((XDFAPI_MarketStatusInfo*)pbuf)->MarketTime );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData) );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData) );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< ��Ʒ����
						{
							while( nMsgCount-- > 0 )
							{
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5) );

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

					switch( pMsgHead->MsgType )
					{
					case 1:///< �г�״̬��Ϣ
						{
							m_oQuoDataCenter.UpdateMarketTime( XDF_SZ, ((XDFAPI_MarketStatusInfo*)pbuf)->MarketTime );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData) );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData) );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData) );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData) );

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
								m_oQuoDataCenter.UpdateTickLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData) );

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
	QuoCollector::GetCollector()->OnLog( TLV_DETAIL, "Quotation::XDF_OnRspOutLog() : %s", pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









