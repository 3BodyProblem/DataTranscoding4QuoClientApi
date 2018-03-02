#include "math.h"
#include "SvrStatus.h"
#include "./Financial11File.h"
#include "../DataTranscoding4QuoClientApi.h"


__inline bool GetMyFileCreateTime( std::string sFilePath, SYSTEMTIME& refFileTime )
{
	HANDLE		hFile = NULL;
	FILETIME	ftWrite = { 0 };
	FILETIME	ftCreate = { 0 };
	FILETIME	ftAccess = { 0 };
	OFSTRUCT	ofStruct = { 0 };
	FILETIME	ftLocalTime = { 0 };

	try
	{
		hFile = ::CreateFile( sFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
		if( INVALID_HANDLE_VALUE == hFile )
		{
			::memset( &refFileTime, 0, sizeof(refFileTime) );
			return false;
		}

		::GetFileTime( hFile, &ftCreate, &ftAccess, &ftWrite );		///< 获取文件创建/存取/写入时间
		::FileTimeToLocalFileTime( &ftCreate, &ftLocalTime );		///< 转换文件时间到本地文件时间
		::FileTimeToSystemTime( &ftLocalTime, &refFileTime );		///< 转换文件本地时间到系统时间
	}
	catch( std::exception& err )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GetMyFileTime() : an exception occur in function : %s", err.what() );
	}
	catch( ... )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GetMyFileTime() : an unknow exception" );
	}

	if( INVALID_HANDLE_VALUE != hFile )
	{
		::CloseHandle( hFile );
		return true;
	}

	return false;
}

__inline std::string GenFinancialFilePath( enum XDFMarket eMkID )
{
	std::string				sFilePath;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };


	switch( eMkID )
	{
	case XDF_SH:		///< 上海Lv1
		::sprintf( pszFilePath, "SSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
		break;
	case XDF_SZ:		///< 深证Lv1
		::sprintf( pszFilePath, "SZSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : invalid market id (%s)", eMkID );
		return "";
	}

	sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );							///< 若目录不存在则新建
	::sprintf( pszFileName, "Financial%u.csv", DateTime::Now().DateToLong() );
	sFilePath += pszFileName;										///< 拼接出文件全路径

	return sFilePath;
}

__inline bool PrepareFinancialDumper( enum XDFMarket eMkID, std::ofstream& oDumper )
{
	std::string				sFirstLineOfCSV;
	std::string				sFilePath = GenFinancialFilePath( eMkID );

	switch( eMkID )
	{
	case XDF_SH:		///< 上海Lv1
		sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		break;
	case XDF_SZ:		///< 深证Lv1
		sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : invalid market id (%s)", eMkID );
		return false;
	}

	if( oDumper.is_open() )		oDumper.close();
	oDumper.open( sFilePath.c_str() , std::ios::out );				///< 创建文件

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );								///< 文件指针移至文件尾
	if( 0 == oDumper.tellp() )										///< 判断文件是否为全空
	{
		oDumper << sFirstLineOfCSV;									///< 先写入第一行(Title列)
	}

	return true;
}


///< -----------------------------------------------------------------------------------------------------------


SHL1FinancialDbf::SHL1FinancialDbf()
{
	::memset( &m_oDumpFileTime, 0, sizeof(m_oDumpFileTime) );
}

SHL1FinancialDbf::~SHL1FinancialDbf()
{
}

int SHL1FinancialDbf::Redirect2File()
{
	int					nErrCode = 0;
	int					nRecordCount = 0;
	SYSTEMTIME			tagDBFSystemTime = { 0 };
	std::string&		sFolder = Configuration::GetConfig().GetFinancialDataFolder();
	std::string			sFilePath = JoinPath( sFolder, "SSE/shbase.dbf" );
	std::string			sCSVPath = GenFinancialFilePath( XDF_SH );

	Close();
	if( false == GetMyFileCreateTime( sFilePath, tagDBFSystemTime ) )			///< 获取DBF文件时间
	{
		return -1024;
	}

	if( (nErrCode = Open( sFilePath.c_str() )) < 0 )							///< 打开DBF文件
	{
		return nErrCode;
	}

	GetMyFileCreateTime( sCSVPath.c_str(), m_oDumpFileTime );					///< 获取文件的创建时间

	unsigned __int64	nDBFDateTime = tagDBFSystemTime.wYear * 10000000000000 + tagDBFSystemTime.wMonth * 100000000000 + (__int64)tagDBFSystemTime.wDay * 1000000000 + tagDBFSystemTime.wHour * 10000000 + tagDBFSystemTime.wMinute * 100000 + tagDBFSystemTime.wSecond * 1000 + tagDBFSystemTime.wMilliseconds;
	unsigned __int64	nFileDateTime = m_oDumpFileTime.wYear * 10000000000000 + m_oDumpFileTime.wMonth * 100000000000 + (__int64)m_oDumpFileTime.wDay * 1000000000 + m_oDumpFileTime.wHour * 10000000 + m_oDumpFileTime.wMinute * 100000 + m_oDumpFileTime.wSecond * 1000 + m_oDumpFileTime.wMilliseconds;

	///< 判断dbf是否比较落盘转存文件的创建时间旧，则直接返回
	if( nDBFDateTime <= nFileDateTime && 0 != nFileDateTime )
	{
		return 0;
	}

	if( false == PrepareFinancialDumper( XDF_SH, m_oCSVDumper ) )				///< 打开上海财经数据落盘文件&获取文件时间
	{
		return -2048;
	}

	ServerStatus::GetStatusObj().UpdateFinancialDT();
	nRecordCount = GetRecordCount();
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

		char	pszRecord[1024*2] = { 0 };
		int		nSize = ::sprintf( pszRecord, "%s,%s,%d,%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%d,%.3f,%.3f,%.4f,%.4f,%.4f,%s,%d,%.2f,%.3f,%.3f,%s\n"
									, STOCK_CODE, STOCK_NAME, START_DATE, VOCATION, FIELD, ZGB, AG, BG, KZQ, ZZC, LDZC, GDZC, WXDYZC, QTZC, ZFZ, CQFZ, LDFZ, QTFZ, GDQY, ZBGJJ
									, WFPLR, MGJZC, LRZE, JLR, ZYSR, ZYYWLR, ZQMGSY, NDMGSY, SYL, ZCFZB, LDBL, SDBL, GDQYB, FP_DATE, M10G_SG, M10G_PG, PGJ_HIGH, PGJ_LOW, MGHL
									, NEWS, UPDATEDATE, HG, YFPSZ, YFPHL, UPDATETIME );

		m_oCSVDumper.write( pszRecord, nSize );
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SHL1FinancialDbf::Redirect2File() : %d Records dumped ....... ", nRecordCount );

	return 0;
}


///< -----------------------------------------------------------------------------------------------------------


SZL1FinancialDbf::SZL1FinancialDbf()
{
	::memset( &m_oDumpFileTime, 0, sizeof(m_oDumpFileTime) );
}

SZL1FinancialDbf::~SZL1FinancialDbf()
{
}

int SZL1FinancialDbf::Redirect2File()
{
	int					nErrCode = 0;
	int					nRecordCount = 0;
	SYSTEMTIME			tagDBFSystemTime = { 0 };
	std::string&		sFolder = Configuration::GetConfig().GetFinancialDataFolder();
	std::string			sFilePath = JoinPath( sFolder, "SZSE/szbase.dbf" );
	std::string			sCSVPath = GenFinancialFilePath( XDF_SH );

	Close();
	if( false == GetMyFileCreateTime( sFilePath, tagDBFSystemTime ) )			///< 获取DBF文件时间
	{
		return -1024;
	}

	if( (nErrCode = Open( sFilePath.c_str() )) < 0 )
	{
		return nErrCode;
	}

	GetMyFileCreateTime( sCSVPath.c_str(), m_oDumpFileTime );					///< 获取文件的创建时间

	unsigned __int64	nDBFDateTime = tagDBFSystemTime.wYear * 10000000000000 + tagDBFSystemTime.wMonth * 100000000000 + (__int64)tagDBFSystemTime.wDay * 1000000000 + tagDBFSystemTime.wHour * 10000000 + tagDBFSystemTime.wMinute * 100000 + tagDBFSystemTime.wSecond * 1000 + tagDBFSystemTime.wMilliseconds;
	unsigned __int64	nFileDateTime = m_oDumpFileTime.wYear * 10000000000000 + m_oDumpFileTime.wMonth * 100000000000 + (__int64)m_oDumpFileTime.wDay * 1000000000 + m_oDumpFileTime.wHour * 10000000 + m_oDumpFileTime.wMinute * 100000 + m_oDumpFileTime.wSecond * 1000 + m_oDumpFileTime.wMilliseconds;

	///< 判断dbf是否比较落盘转存文件的创建时间旧，则直接返回
	if( nDBFDateTime <= nFileDateTime && 0 != nFileDateTime )
	{
		return 0;
	}

	if( false == PrepareFinancialDumper( XDF_SZ, m_oCSVDumper ) )				///< 深圳财经数据
	{
		return -2048;
	}

	ServerStatus::GetStatusObj().UpdateFinancialDT();
	nRecordCount = GetRecordCount();
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

		char	pszRecord[1024*2] = { 0 };
		int		nSize = ::sprintf( pszRecord, "%s,%s,%d,%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%d,%.3f,%.3f,%.4f,%.4f,%.4f,%s,%d,%.2f,%.3f,%.3f,%s\n"
									, STOCK_CODE, STOCK_NAME, START_DATE, VOCATION, FIELD, ZGB, AG, BG, KZQ, ZZC, LDZC, GDZC, WXDYZC, QTZC, ZFZ, CQFZ, LDFZ, QTFZ, GDQY, ZBGJJ
									, WFPLR, MGJZC, LRZE, JLR, ZYSR, ZYYWLR, ZQMGSY, NDMGSY, SYL, ZCFZB, LDBL, SDBL, GDQYB, FP_DATE, M10G_SG, M10G_PG, PGJ_HIGH, PGJ_LOW, MGHL
									, NEWS, UPDATEDATE, HG, YFPSZ, YFPHL, UPDATETIME );

		m_oCSVDumper.write( pszRecord, nSize );
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "SZL1FinancialDbf::Redirect2File() : %d Records dumped ....... ", nRecordCount );

	return 0;
}





