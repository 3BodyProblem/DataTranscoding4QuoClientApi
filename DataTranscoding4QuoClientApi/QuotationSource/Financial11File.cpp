#include "math.h"
#include "./Financial11File.h"


SHL1FinancialDbf::SHL1FinancialDbf() : m_ulSaveDate(0)
{
}

SHL1FinancialDbf::~SHL1FinancialDbf()
{
	Release();
}

int SHL1FinancialDbf::Instance()
{
	int							nErrCode = 0;
	std::string&				sFolder = Configuration::GetConfig().GetFinancialDataFolder();

	Release();
	if( (nErrCode = Open( sFolder.c_str() )) < 0 )
	{
		return nErrCode;
	}

	return 0;
}

void SHL1FinancialDbf::Release()
{
	Close();
}

int SHL1FinancialDbf::GetDbfDate( unsigned long * ulDate )
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

int SHL1FinancialDbf::GetShNameTableData( unsigned long RecordPos, tagSHL2Mem_NameTable * Out, unsigned long * ulClose, unsigned long * ulOpen )
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








