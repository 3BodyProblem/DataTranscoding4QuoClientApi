#include "math.h"
#include "./Financial11File.h"
#include "../DataTranscoding4QuoClientApi.h"


SHL1FinancialDbf::SHL1FinancialDbf() : m_ulSaveDate(0)
{
}

SHL1FinancialDbf::~SHL1FinancialDbf()
{
	Release();
}

int SHL1FinancialDbf::Instance()
{
	int					nErrCode = 0;
	std::string&		sFolder = Configuration::GetConfig().GetFinancialDataFolder();
	std::string			sFilePath = JoinPath( sFolder, "SSE/shbase.dbf" );

	Release();
	if( (nErrCode = Open( sFilePath.c_str() )) < 0 )
	{
		return nErrCode;
	}

	return 0;
}

void SHL1FinancialDbf::Release()
{
	Close();
}

int SHL1FinancialDbf::Redirect2File( std::ofstream& oDumper )
{
	int					nErrCode = 0;
	int					nRecordCount = GetRecordCount();

	for( int n = 0; n < nRecordCount; n++ )
	{
		char			STOCK_CODE[20] = { 0 };
		char			STOCK_NAME[64] = { 0 };
		int				START_DATE = 0;
		char			VOCATION[32] = { 0 };
		char			FIELD[32] = { 0 };
		double			ZGB = 0.;
		double			AG = 0.;
		double			BG = 0.;
		double			KZQ = 0.;
		double			ZZC = 0.;
		double			LDZC = 0.;
		double			GDZC = 0.;
		double			WXDYZC = 0.;
		double			QTZC = 0.;
		double			ZFZ = 0.;
		double			CQFZ = 0.;
		double			LDFZ = 0.;
		double			QTFZ = 0.;
		double			GDQY = 0.;
		double			ZBGJJ = 0.;
		double			WFPLR = 0.;
		double			MGJZC = 0.;
		double			LRZE = 0.;
		double			JLR = 0.;
		double			ZYSR = 0.;
		double			ZYYWLR = 0.;
		double			ZQMGSY = 0.;
		double			NDMGSY = 0.;
		double			SYL = 0.;
		double			ZCFZB = 0.;
		double			LDBL = 0.;
		double			SDBL = 0.;
		double			GDQYB = 0.;
		int				FP_DATE = 0;
		double			M10G_SG = 0.;
		double			M10G_PG = 0.;
		double			PGJ_HIGH = 0.;
		double			PGJ_LOW = 0.;
		double			MGHL = 0.;
		char			NEWS[1024] = { 0 };
		int				UPDATEDATE = 0;
		double			HG = 0.;
		double			YFPSZ = 0.;
		double			YFPHL = 0.;
		char			UPDATETIME[32] = { 0 };

		if( ( nErrCode = Goto( n ) < 0 ) )	{
			return -1;
		}

		if( ReadString( "STOCK_CODE", STOCK_CODE, sizeof(STOCK_CODE) ) < 0 )	{
			return -2;
		}
		if( ReadString( "STOCK_NAME", STOCK_NAME, sizeof(STOCK_NAME) ) < 0 )	{
			return -3;
		}
		if( ReadInteger( "START_DATE", &START_DATE ) < 0 )	{
			return -4;
		}
		if( ReadString( "VOCATION", VOCATION, sizeof(VOCATION) ) < 0 )	{
			return -5;
		}
		if( ReadString( "FIELD", FIELD, sizeof(FIELD) ) < 0 )	{
			return -6;
		}
		if( ReadFloat( "ZGB", &ZGB ) < 0 )	{
			return -7;
		}
		if( ReadFloat( "AG", &AG ) < 0 )	{
			return -8;
		}
		if( ReadFloat( "BG", &BG ) < 0 )	{
			return -9;
		}
		if( ReadFloat( "KZQ", &KZQ ) < 0 )	{
			return -10;
		}
		if( ReadFloat( "ZZC", &ZZC ) < 0 )	{
			return -11;
		}
		if( ReadFloat( "LDZC", &LDZC ) < 0 )	{
			return -12;
		}
		if( ReadFloat( "GDZC", &GDZC ) < 0 )	{
			return -13;
		}
		if( ReadFloat( "WXDYZC", &WXDYZC ) < 0 )	{
			return -14;
		}
		if( ReadFloat( "QTZC", &QTZC ) < 0 )	{
			return -15;
		}
		if( ReadFloat( "ZFZ", &ZFZ ) < 0 )	{
			return -16;
		}
		if( ReadFloat( "CQFZ", &CQFZ ) < 0 )	{
			return -17;
		}
		if( ReadFloat( "LDFZ", &LDFZ ) < 0 )	{
			return -18;
		}
		if( ReadFloat( "QTFZ", &QTFZ ) < 0 )	{
			return -19;
		}
		if( ReadFloat( "GDQY", &GDQY ) < 0 )	{
			return -20;
		}
		if( ReadFloat( "ZBGJJ", &ZBGJJ ) < 0 )	{
			return -21;
		}
		if( ReadFloat( "WFPLR", &WFPLR ) < 0 )	{
			return -22;
		}
		if( ReadFloat( "MGJZC", &MGJZC ) < 0 )	{
			return -23;
		}
		if( ReadFloat( "LRZE", &LRZE ) < 0 )	{
			return -24;
		}
		if( ReadFloat( "JLR", &JLR ) < 0 )	{
			return -25;
		}
		if( ReadFloat( "ZYSR", &ZYSR ) < 0 )	{
			return -26;
		}
		if( ReadFloat( "ZYYWLR", &ZYYWLR ) < 0 )	{
			return -27;
		}
		if( ReadFloat( "ZQMGSY", &ZQMGSY ) < 0 )	{
			return -28;
		}
		if( ReadFloat( "NDMGSY", &NDMGSY ) < 0 )	{
			return -29;
		}
		if( ReadFloat( "SYL", &SYL ) < 0 )	{
			return -30;
		}
		if( ReadFloat( "ZCFZB", &ZCFZB ) < 0 )	{
			return -31;
		}
		if( ReadFloat( "LDBL", &LDBL ) < 0 )	{
			return -32;
		}
		if( ReadFloat( "SDBL", &SDBL ) < 0 )	{
			return -33;
		}
		if( ReadFloat( "GDQYB", &GDQYB ) < 0 )	{
			return -34;
		}
		if( ReadInteger( "FP_DATE", &FP_DATE ) < 0 )	{
			return -35;
		}
		if( ReadFloat( "M10G_SG", &M10G_SG ) < 0 )	{
			return -36;
		}
		if( ReadFloat( "M10G_PG", &M10G_PG ) < 0 )	{
			return -37;
		}
		if( ReadFloat( "PGJ_HIGH", &PGJ_HIGH ) < 0 )	{
			return -38;
		}
		if( ReadFloat( "PGJ_LOW", &PGJ_LOW ) < 0 )	{
			return -39;
		}
		if( ReadFloat( "MGHL", &MGHL ) < 0 )	{
			return -40;
		}
		if( ReadString( "NEWS", NEWS, sizeof(NEWS) ) < 0 )	{
			return -41;
		}
		if( ReadInteger( "UPDATEDATE", &UPDATEDATE ) < 0 )	{
			return -42;
		}
		if( ReadFloat( "HG", &HG ) < 0 )	{
			return -43;
		}
		if( ReadFloat( "YFPSZ", &YFPSZ ) < 0 )	{
			return -44;
		}
		if( ReadFloat( "YFPHL", &YFPHL ) < 0 )	{
			return -45;
		}
		if( ReadString( "UPDATETIME", UPDATETIME, sizeof(UPDATETIME) ) < 0 )	{
			return -46;
		}

		int a = 0;

	}

	return 0;
}

/*
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
*/







