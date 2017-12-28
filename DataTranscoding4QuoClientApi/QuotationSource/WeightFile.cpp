#include "math.h"
#include <vector>
#include "./WeightFile.h"
#include "../DataTranscoding4QuoClientApi.h"


int EnumAllFiles( const char* folder, std::vector<std::string>& files )
{
	WIN32_FIND_DATAA	wfdata = { 0 };
	int					ret = 0;
	char				findpath[MAX_PATH] = { 0 };

	files.clear();
	sprintf(findpath, "%s\\*", folder);

	HANDLE				hFind = FindFirstFileA(findpath, &wfdata);

	if( hFind != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if(wfdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{

				if (0 != strcmp(wfdata.cFileName, ".") && 0 != strcmp(wfdata.cFileName, "..")) 
				{

					std::vector<std::string> subfiles;
					char findsubpath[MAX_PATH];

					_makepath(findsubpath, NULL, folder, wfdata.cFileName, NULL);

					if(EnumAllFiles(findsubpath, subfiles) != 0) 
					{
						int count = 0;

						int i = 0;
						for (i = 0; i < count; i++)
						{
							files.push_back(subfiles[i]);
						}

						subfiles.clear();
					}

				}

			} 
			else 
			{
				char filepath[MAX_PATH];

				_makepath(filepath, NULL, folder, wfdata.cFileName, NULL);

				files.push_back(filepath);
			}

		} 
		while (FindNextFileA(hFind, &wfdata));

		FindClose(hFind);

	}

	return (ret = (int)files.size());
}

__inline bool GetWeightFileWriteTime( std::string sFilePath, SYSTEMTIME& refFileTime )
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

		::GetFileTime( hFile, &ftCreate, &ftAccess, &ftWrite );		///< ��ȡ�ļ�����/��ȡ/д��ʱ��
		::FileTimeToLocalFileTime( &ftWrite, &ftLocalTime );		///< ת���ļ�ʱ�䵽�����ļ�ʱ��
		::FileTimeToSystemTime( &ftLocalTime, &refFileTime );		///< ת���ļ�����ʱ�䵽ϵͳʱ��
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

__inline bool PrepareWeightlDumper( enum XDFMarket eMkID, std::ofstream& oDumper, SYSTEMTIME& refFileTime )
{
	std::string				sFilePath;
	std::string				sFirstLineOfCSV;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };

	switch( eMkID )
	{
	case XDF_SH:		///< �Ϻ�Lv1
		{
			::sprintf( pszFilePath, "SSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
			sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		}
		break;
	case XDF_SZ:		///< ��֤Lv1
		{
			::sprintf( pszFilePath, "SZSE/FINANCIAL/%d/", DateTime::Now().DateToLong()/10000 );
			sFirstLineOfCSV = "STOCK_CODE,STOCK_NAME,START_DATE,VOCATION,FIELD,ZGB,AG,BG,KZQ,ZZC,LDZC,GDZC,WXDYZC,QTZC,ZFZ,CQFZ,LDFZ,QTFZ,GDQY,ZBGJJ,WFPLR,MGJZC,LRZE,JLR,ZYSR,ZYYWLR,ZQMGSY,NDMGSY,SYL,ZCFZB,LDBL,SDBL,GDQYB,FP_DATE,M10G_SG,M10G_PG,PGJ_HIGH,PGJ_LOW,MGHL,NEWS,UPDATEDATE,HG,YFPSZ,YFPHL,UPDATETIME\n";
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : invalid market id (%s)", eMkID );
		return false;
	}

	sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );							///< ��Ŀ¼���������½�
	::sprintf( pszFileName, "Financial%u.csv", DateTime::Now().DateToLong() );
	sFilePath += pszFileName;										///< ƴ�ӳ��ļ�ȫ·��
	GetWeightFileWriteTime( sFilePath.c_str(), refFileTime );		///< ��ȡ�ļ��Ĵ���ʱ��
	if( oDumper.is_open() )		oDumper.close();
	oDumper.open( sFilePath.c_str() , std::ios::out );				///< �����ļ�

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "PrepareFinancialFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );								///< �ļ�ָ�������ļ�β
	if( 0 == oDumper.tellp() )										///< �ж��ļ��Ƿ�Ϊȫ��
	{
		oDumper << sFirstLineOfCSV;									///< ��д���һ��(Title��)
	}

	return true;
}


///< -----------------------------------------------------------------------------------------------------------


WeightFile::WeightFile()
{
}

void WeightFile::ScanWeightFiles()
{
	std::vector<std::string>	vctFilesPath4SHL1;																///< ����SHL1Ȩ��Դ�ļ�·��
	std::vector<std::string>	vctFilesPath4SZL1;																///< ����SZL1Ȩ��Դ�ļ�·��
	std::string&				sFolder = Configuration::GetConfig().GetFinancialDataFolder();					///< ȨϢ�ļ����ڵĸ�Ŀ¼
	std::string					sSHL1WeightFolder = JoinPath( sFolder, "SSE/" );								///< �Ϻ�L1Weight������Ŀ¼
	std::string					sSZL1WeightFolder = JoinPath( sFolder, "SZSE/" );								///< ����L1Weight������Ŀ¼
	int							nFileCount4SHL1 = EnumAllFiles( sSHL1WeightFolder.c_str(), vctFilesPath4SHL1 );	///< �г�����SHL1Ȩ��Դ�ļ�
	int							nFileCount4SZL1 = EnumAllFiles( sSZL1WeightFolder.c_str(), vctFilesPath4SZL1 );	///< �г�����SZL1Ȩ��Դ�ļ�



}

int WeightFile::Redirect2File( std::string sSourceFile, std::string sDestFile )
{
	std::ofstream			oCSVDumper;						///< CSV����ļ�
	std::ifstream			oWeightReader;					///< ȨϢ��Ϣ�ļ�
	SYSTEMTIME				oReadFileTime = { 0 };			///< Դͷ�ļ�ʱ��
	SYSTEMTIME				oDumpFileTime = { 0 };			///< �����ļ�ʱ��

	if( false == GetWeightFileWriteTime( sSourceFile, oReadFileTime ) )
	{
		return -1024;	///< Դ�ļ�������
	}

	if( false == GetWeightFileWriteTime( sDestFile, oDumpFileTime ) )
	{
		return -1025;	///< Ŀ���ļ�������
	}

	unsigned __int64	nSrcDateTime = oReadFileTime.wYear * 10000000000000 + oReadFileTime.wMonth * 100000000000 + (__int64)oReadFileTime.wDay * 1000000000 + oReadFileTime.wHour * 10000000 + oReadFileTime.wMinute * 100000 + oReadFileTime.wSecond * 1000 + oReadFileTime.wMilliseconds;
	unsigned __int64	nDestDateTime = oDumpFileTime.wYear * 10000000000000 + oDumpFileTime.wMonth * 100000000000 + (__int64)oDumpFileTime.wDay * 1000000000 + oDumpFileTime.wHour * 10000000 + oDumpFileTime.wMinute * 100000 + oDumpFileTime.wSecond * 1000 + oDumpFileTime.wMilliseconds;

	///< �ж�Դ�ļ��Ƿ�Ƚ�����ת���ļ��Ĵ���ʱ��ɣ���ֱ�ӷ���
	if( nSrcDateTime <= nDestDateTime/* && 0 != nDestDateTime*/ )
	{
		return 0;
	}

	return 0;
}





