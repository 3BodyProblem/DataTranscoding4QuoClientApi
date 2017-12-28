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

__inline std::string GenWeightFilePath( enum XDFMarket eMkID, std::string sCode )
{
	std::string				sFilePath;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };

	switch( eMkID )
	{
	case XDF_SH:		///< �Ϻ�Lv1
		::strcpy( pszFilePath, "SSE/WEIGHT/" );
		break;
	case XDF_SZ:		///< ��֤Lv1
		::strcpy( pszFilePath, "SZSE/WEIGHT/" );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GenWeightFilePath() : invalid market id (%s)", eMkID );
		return "";
	}

	if( true == sCode.empty() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "GenWeightFilePath() : mkid(%d), invalid code(%s)", eMkID, sCode.c_str() );
		return "";
	}

	sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );							///< ��Ŀ¼���������½�
	::sprintf( pszFileName, "WEIGHT%s.csv", sCode.c_str() );
	sFilePath += pszFileName;										///< ƴ�ӳ��ļ�ȫ·��

	return sFilePath;
}


///< -----------------------------------------------------------------------------------------------------------


std::string	GrabCodeFromPath( std::string& sPath )
{
	int	nEndPos = sPath.rfind( ".wgt" );
	int	nBeginPos1 = sPath.rfind( "/" );
	int	nBeginPos2 = sPath.rfind( "\\" );
	int	nBeginPos = max( nBeginPos1, nBeginPos2 );

	return sPath.substr( nBeginPos + 1, nEndPos - nBeginPos - 1 );
}

WeightFile::WeightFile()
{
}

void WeightFile::ScanWeightFiles()
{
	int							nErrCode = 0;
	std::vector<std::string>	vctFilesPath4SHL1;																///< ����SHL1Ȩ��Դ�ļ�·��
	std::vector<std::string>	vctFilesPath4SZL1;																///< ����SZL1Ȩ��Դ�ļ�·��
	std::string&				sFolder = Configuration::GetConfig().GetWeightFileFolder();						///< ȨϢ�ļ����ڵĸ�Ŀ¼
	std::string					sSHL1WeightFolder = JoinPath( sFolder, "SSE/" );								///< �Ϻ�L1Weight������Ŀ¼
	std::string					sSZL1WeightFolder = JoinPath( sFolder, "SZSE/" );								///< ����L1Weight������Ŀ¼
	int							nFileCount4SHL1 = EnumAllFiles( sSHL1WeightFolder.c_str(), vctFilesPath4SHL1 );	///< �г�����SHL1Ȩ��Դ�ļ�
	int							nFileCount4SZL1 = EnumAllFiles( sSZL1WeightFolder.c_str(), vctFilesPath4SZL1 );	///< �г�����SZL1Ȩ��Դ�ļ�

	///< �Ϻ�L1�г���ȨϢ��Ϣ�г�
	for( int i = 0; i < nFileCount4SHL1; i++ )
	{
		std::string				sSrcFilePath = vctFilesPath4SHL1[i];
		std::string				sCode = GrabCodeFromPath( sSrcFilePath );
		std::string				sDestFilePath = GenWeightFilePath( XDF_SH, sCode );

		if( (nErrCode = Redirect2File( sSrcFilePath, sDestFilePath )) != 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "WeightFile::ScanWeightFiles() : [SHL1] an error occur while %s ---> %s, errorcode = %d", sSrcFilePath.c_str(), sDestFilePath.c_str(), nErrCode );
		}
	}

	///< ����L1�г���ȨϢ��Ϣ�г�
	for( int j = 0; j < nFileCount4SZL1; j++ )
	{
		std::string				sSrcFilePath = vctFilesPath4SZL1[j];
		std::string				sCode = GrabCodeFromPath( sSrcFilePath );
		std::string				sDestFilePath = GenWeightFilePath( XDF_SZ, sCode );

		if( (nErrCode = Redirect2File( sSrcFilePath, sDestFilePath )) != 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "WeightFile::ScanWeightFiles() : [SZL1] an error occur while %s ---> %s, errorcode = %d", sSrcFilePath.c_str(), sDestFilePath.c_str(), nErrCode );
		}
	}
}

int WeightFile::Redirect2File( std::string sSourceFile, std::string sDestFile )
{
	std::string				sCSVTitle;						///< CSV�ļ�����
	std::ofstream			oCSVDumper;						///< CSV����ļ�
	std::ifstream			oWeightReader;					///< ȨϢ��Ϣ�ļ�
	SYSTEMTIME				oReadFileTime = { 0 };			///< Դͷ�ļ�ʱ��
	SYSTEMTIME				oDumpFileTime = { 0 };			///< �����ļ�ʱ��

	if( sSourceFile.empty() || sDestFile.empty() )
	{
		return -1;		///< �ļ�·��Ϊ����Ч
	}

	if( false == GetWeightFileWriteTime( sSourceFile, oReadFileTime ) )
	{
		return -1024;	///< Դ�ļ�������
	}
	else				///< ȡһ��Ŀ���ļ����ļ�ʱ�䣬���ļ������ڣ���ȡ0ֵ
	{
		GetWeightFileWriteTime( sDestFile, oDumpFileTime );
	}

	unsigned __int64	nSrcDateTime = oReadFileTime.wYear * 10000000000000 + oReadFileTime.wMonth * 100000000000 + (__int64)oReadFileTime.wDay * 1000000000 + oReadFileTime.wHour * 10000000 + oReadFileTime.wMinute * 100000 + oReadFileTime.wSecond * 1000 + oReadFileTime.wMilliseconds;
	unsigned __int64	nDestDateTime = oDumpFileTime.wYear * 10000000000000 + oDumpFileTime.wMonth * 100000000000 + (__int64)oDumpFileTime.wDay * 1000000000 + oDumpFileTime.wHour * 10000000 + oDumpFileTime.wMinute * 100000 + oDumpFileTime.wSecond * 1000 + oDumpFileTime.wMilliseconds;

	///< �ж�Դ�ļ��Ƿ�Ƚ�����ת���ļ��Ĵ���ʱ��ɣ���ֱ�ӷ���
	if( nSrcDateTime <= nDestDateTime && 0 != nDestDateTime )
	{
		return 0;
	}

	///< ��Դ�ļ� & Ŀ���ļ�
	oWeightReader.open( sSourceFile.c_str(), std::ios::in|std::ios::binary );
	if( !oWeightReader.is_open() )
	{
		return -1025;
	}

	oCSVDumper.open( sDestFile.c_str() , std::ios::out|std::ios::binary );
	if( !oCSVDumper.is_open() )
	{
		return -1026;
	}

	///< ��������Դ�ļ���ת�浽Ŀ���ļ�
	oWeightReader.seekg( 0, std::ios::beg );
	sCSVTitle = "date,a,b,c,d,e,base,flowbase\n";					///< formatĿ���ļ�����(title)
	oCSVDumper.write( sCSVTitle.c_str(), sCSVTitle.length() );		///< д��title
	while( true )
	{
		int				nLen = 0;
		char			pszRecords[512] = { 0 };
		T_WEIGHT		tagSrcWeight = { 0 };

		if( oWeightReader.peek() == EOF || true == oWeightReader.eof() )
		{
			break;		///< �ļ�����
		}

		oWeightReader.read( (char*)&tagSrcWeight, sizeof(tagSrcWeight) );///<oWeightReader.gcount();
		nLen = ::sprintf( pszRecords, "%d,%d,%d,%f,%f,%d,%f,%f\n"
				, tagSrcWeight.Date, tagSrcWeight.A, tagSrcWeight.B, tagSrcWeight.C, tagSrcWeight.D, tagSrcWeight.E, tagSrcWeight.BASE, tagSrcWeight.FLOWBASE );

		oCSVDumper.write( pszRecords, nLen );
	}

	return 0;
}





