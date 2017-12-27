#include "./L1DbfFileIO.h"
#include "./QzGzFile.h"
#include "../GlobalIO/MGlobalIO.h"
#include "math.h"


SHShow2003Dbf::SHShow2003Dbf() : m_ulSaveDate(0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SHShow2003Dbf::~SHShow2003Dbf()
{
	Release();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SHShow2003Dbf::Instance( bool bFirst )
{
	int							nErrCode = -1;
	tagSHL2Mem_MarketHead		tagMarkHead = {0};

	if( "" == Global_Option.GetL1DbfFilePath() )
	{
		return 1;
	}

	Global_Process.SetSvcStatus( -1 );
	Global_Process.SetWorkStatus( STATUS_INITIAL );
	Global_DataIO.ClearData();
	Release();

	if( Global_Option.BeUseL1NameTable() )
	{
		nErrCode = PreProcessFile();
		if( nErrCode < 0 )
		{
			return nErrCode;
		}
		nErrCode = ExLoadQzxx();
		if( nErrCode < 0 )
		{
			Global_Process.WriteWarning( 0, "上海L2fast源驱动", "读取权证信息出错[Err:%d]", nErrCode );
		}
		nErrCode = ExLoadGzlx();
		if( nErrCode < 0 )
		{
			Global_Process.WriteWarning( 0, "上海L2fast源驱动", "读取国债利息出错[Err:%d]", nErrCode );
		}
	}

	Global_DataIO.GetShl2FastMarket().GetMarketHead( &tagMarkHead );
	tagMarkHead.warecount = Global_DataIO.GetShl2FastNameTb().GetCount();
	tagMarkHead.checkcode = Global_DataIO.GetShl2FastNameTb().GetNameTableCheckCode();
	Global_DataIO.GetShl2FastMarket().SetMarketHead( &tagMarkHead );

	if( bFirst )
	{
		if( (nErrCode = m_thread.StartThread( "L1初始化监测线程", ThreadFunc, this ) ) < 0 )
		{
			Global_Process.WriteError( 0, "L1码表初始化", "初始化监测线程启动失败" );
			return nErrCode;
		}
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SHShow2003Dbf::Release()
{
	if( "" == Global_Option.GetL1DbfFilePath() )
	{
		return;
	}

	m_thread.StopThread();
	Close();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//从库中装入码表
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SHShow2003Dbf::PreProcessFile()
{
	int							nErrCode = -1;
	unsigned long				ulTime;
	unsigned long				ulDate;
	unsigned long				ulOpen;
	int							nSerial;
	int							KindNum[32] = {0};
	tagSHL2Mem_NameTable		tempName;
	tagQuickNameTableInfo		tempQName = {0};
	tagSHL2Mem_IndexTable		tempIndex = {0};
	tagSHL2Mem_StockData		tempStock = {0};
	tagSHL2Mem_TranData			tempTranData = {0};
	tagSHL2Mem_VirtualPrice		tempVPrice = {0};
	tagShL2FastMem_TdPhase		tempTdPha = {0};
	StockPriceLevel	tempOData;
	char						tempbuf[32];

	Release();

	nErrCode = Open( Global_Option.GetL1DbfFilePath().c_str() );
	if( nErrCode < 0 )
	{
		return nErrCode;
	}

	if( m_RecordCount <= 0 )
	{
		return -1;
	}
	try
	{
		nErrCode = First();
		if( nErrCode < 0 )
		{
			return -2;
		}

		if( ReadInteger( "S2", (int *)&ulTime ) < 0 )
		{
			return -3;
		}
		if( ReadInteger( "S6", (int *)&ulDate ) < 0 )
		{
			return -4;
		}
		if( ulDate != MDateTime::Now().DateToLong() )
		{
			Global_Process.WriteWarning( 0, "上海L2源驱动", "L1 Dbf库(%s)日期(%d)不对", Global_Option.GetL1DbfFilePath().c_str(), ulDate );
		}
		m_ulSaveDate = ulDate;
		nSerial = -1;
		for( int i = 1; i < GetRecordCount(); ++i )
		{
			if( ( nErrCode = Goto( i ) < 0 ) )
			{
				Global_Process.WriteError( 0, "ShL2扫库", "<PreProcessFile>Goto(%d)[Err:%d]\n", i, nErrCode );
				return -5;
			}
			nErrCode = GetShNameTableData( i, &tempName, &ulDate, &ulOpen );
			if( nErrCode < 0 )
			{
				Global_Process.WriteWarning( 0, "ShL2扫库", "<PreProcessFile>GetShNameTableData(%d)[Err:%d]\n", i, nErrCode );
				continue;
			}
			++nSerial;
			tempName.close = ulDate;
			tempQName.serial = tempQName.aserial = Global_DataIO.GetShl2FastNameTb().GetCount();
			memcpy( tempbuf, tempName.code, 6 );
			tempbuf[6] = 0;
			tempQName.ulcode = atol( tempbuf );	//此处没关系
			tempQName.type = tempName.type;
			if( !Global_DataIO.GetShl2FastNameTb().PushOne( &tempName )||!Global_DataIO.GetShl2FastQuickNameTb().PushOne(&tempQName) )
			{
				Global_Process.WriteError( 0, "ShL2扫库", "<PreProcessFile>码表PushOne(%d)出错\n", nSerial );
				return -6;
			}

			if( tempName.type == 0 )
			{
				tempIndex.serial = nSerial;
				tempIndex.open = ulOpen;
				Global_DataIO.GetShl2FastIndex().PushOne( &tempIndex );
			}
			else
			{
				tempStock.serial = nSerial;
				tempStock.open = ulOpen;
				Global_DataIO.GetShl2FastStock().PushOne( &tempStock );
				tempTranData.serial = nSerial;
				Global_DataIO.GetShl2FastTran().PushOne( &tempTranData );
				tempVPrice.serial = nSerial;
				Global_DataIO.GetShl2FastVirPrice().PushOne( &tempVPrice );
				tempOData.Serial = nSerial;
				Global_DataIO.GetShl2FastOrderData().PushOne( &tempOData );
			}
			tempTdPha.serial = nSerial;
			Global_DataIO.GetShl2FastTdPhase().PushOne(&tempTdPha);
			KindNum[tempName.type]++;
		}
	}
	catch( ... )
	{}
	
 	Global_DataIO.StatKind( KindNum, sizeof(KindNum)/sizeof(KindNum[0]) );
	Global_DataIO.AdjustNameTb();
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SHShow2003Dbf::GetDbfDate( unsigned long * ulDate )
{
	*ulDate = 0;
	if( !m_FilePtr.IsOpen() )
	{
		return -1;
	}
	if( First() < 0 )
	{
		return -2;
	}
	if( ReadInteger( "S6", (int *)ulDate ) < 0 )
	{
		return -4;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SHShow2003Dbf::GetShNameTableData( unsigned long RecordPos, tagSHL2Mem_NameTable * Out, unsigned long * ulClose, unsigned long * ulOpen )
{
	double				dTemp = 0.f;
	char				tempbuf[32];

	if( Out == NULL )
	{
		return 0;
	}

	if( m_RecordCount == 0 )
	{
		Global_Process.WriteError( 0, "ShL2扫库", "<GetShNameTableData>dbf库还未装入" );
		return -1;
	}

	try
	{
		if( Goto( RecordPos ) < 0 )
		{
			return -2;
		}
		memset( Out, 0, sizeof( tagSHL2Mem_NameTable ) );
		if( ReadString( "S1", Out->code, 6+1 ) < 0 )
		{
			return -3;
		}
		if( ReadString( "S2", Out->name, 8+1 ) < 0 )
		{
			return -4;
		}
		memcpy( tempbuf, Out->code, 6 );
		tempbuf[6] = 0;

		if( IsIndex( Out->code ) )		//和市场信息头部一致，序号和其顺序一致。
		{
			Out->type = 0;
		}
		else if( IsAGu( Out->code) )
		{
			Out->type = 1;
		}
		else if( IsBGu( Out->code ) )
		{
			Out->type = 2;
		}
		else if( IsJijin( Out->code ) )
		{
			Out->type = 3;
		}
		else if( IsZhaiQuan( Out->code ) )
		{
			Out->type = 4;
		}
		else if( IsZhuanZhai( Out->code ) )
		{
			Out->type = 5;
		}
		else if( IsHuig( Out->code ) )
		{
			Out->type = 6;
		}
		else if( IsETF( Out->code ) )
		{
			Out->type = 7;
		}
		else if( IsJiJinTong( Out->code ) )
		{
			Out->type = 8;
		}
		else if( IsQzxx( Out->code ) )
		{
			Out->type = 9;
		}
		else//都算其他
		{
			Out->type = 10;
		}
		ReadFloat( "S3", &dTemp );
		*ulClose = (long)( dTemp*pow( 10, Global_DataIO.GetShl2FastMarket().GetTypeRate(Out->type))+0.5 );
		ReadFloat( "S4", &dTemp );
		*ulOpen = (long)( dTemp*pow( 10, Global_DataIO.GetShl2FastMarket().GetTypeRate(Out->type))+0.5 );

	}
	catch( ... )
	{
		Global_Process.WriteError( 0, "ShL2扫库", "GetShNameTableData发生异常" );
		return -5;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsIndex( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	p = Global_Option.rpst_GetKindMask(0);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else if( !memcmp(code,"000",3) && code[3]!='6' && code[3]!='5' )
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsAGu( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(1);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else if(  code[0]=='6' ) 
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsBGu( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(2);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else if( code[0]=='9') 
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsZhaiQuan( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(4);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else if ( (code[0]=='0'&&code[1]=='0')||
			(code[0]=='0'&&code[1]=='1')||
			(code[0]=='1'&&code[1]=='2') ||
			(code[0]=='1'&&code[1]=='3' && code[2] =='0')||
			(code[0]=='0'&&code[1]=='2') )
	{
			return true;
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsZhuanZhai( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(5);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else if ( (code[0]=='1'&&code[1]=='0')||
			(code[0]=='1'&&code[1]=='1') ||
			(code[0]=='1'&&code[1]=='0' && code[2] =='5') )
	{
			return true;
	}
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsHuig( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(6);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else
	if ((code[0]=='2'&&code[1]=='0' &&code[2]=='1') ||
			(code[0]=='2'&&code[1]=='0' &&code[2]=='2') ||
			(code[0]=='2'&&code[1]=='0' &&code[2]=='3') ||
			(code[0]=='2'&&code[1]=='0' &&code[2]=='4') ||
			(code[0]=='1'&&code[1]=='0' &&code[2]=='6')  )
	{
			return true;
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsETF( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(8);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else		
	if(  code[0]=='5'&&code[1]=='1'&&code[2]=='0' ) 
	{
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsJiJinTong( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(9);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else
	if ( (code[0]=='5'&&code[1]=='1'&&code[2] == '9')||
			(code[0]=='5'&&code[1]=='2'&&code[2] == '1')||
			(code[0]=='5'&&code[1]=='2'&&code[2] == '2')||
			(code[0]=='5'&&code[1]=='2'&&code[2] == '3')  )
	{
			return true;
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsGzlx( const char (&code)[6] )
{
	if( !IsIndex( code )&&( code[0]=='0'||code[0]=='1' ) )
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsQzxx( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(11);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else		
	if( code[0]=='5'&&code[1]=='8' )
	{
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SHShow2003Dbf::IsJijin( const char (&code)[6] )
{
	const TKindMask * p;
	int i;
	if( IsIndex( code ))
		return false;
	p = Global_Option.rpst_GetKindMask(3);
	if(NULL != p && 0 < p->uchCount)
	{
		//使用3X中的定义
		for(i=0; i<p->uchCount; i++)
		{
			if(0 == memcmp(code, &p->uchMask[i][0], strlen((const char*)p->uchMask[i])))
				return true;
		}
	}
	else		
	if( code[0]=='5'&&code[1]=='0' )
	{
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void * __stdcall SHShow2003Dbf::ThreadFunc( void * Param )
{
	SHShow2003Dbf		*			pSelf = NULL;
	unsigned long					ulDate = 0;
	bool							bSuspend = false;

	pSelf = reinterpret_cast<SHShow2003Dbf	*>(Param);
	if( pSelf == NULL )
	{
		return 0;
	}

	while( !pSelf->m_thread.GetThreadStopFlag() )
	{
		try
		{
			MThread::Sleep( 2000 );
			pSelf->GetDbfDate( &ulDate );
			if( ulDate > 0 && ulDate != pSelf->m_ulSaveDate )
			{
				MThread::Sleep( 5000 );
				Global_Process.WriteInfo( 0, "L2fast L1码表监测", "开始重新初始化,日期(%d)", ulDate );
				pSelf->Instance( false );
			}

			Global_DataIO.GetData8102File().Flush();
		}
		catch( ... )
		{
			Global_Process.WriteError( 0, "shl2fast连接vde线程", "发生未知异常" );			
		}
	}

	return 0;
}
