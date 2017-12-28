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

		::GetFileTime( hFile, &ftCreate, &ftAccess, &ftWrite );		///< 获取文件创建/存取/写入时间
		::FileTimeToLocalFileTime( &ftWrite, &ftLocalTime );		///< 转换文件时间到本地文件时间
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

__inline std::string GenWeightFilePath( enum XDFMarket eMkID, std::string sCode )
{
	std::string				sFilePath;
	char					pszFileName[128] = { 0 };
	char					pszFilePath[512] = { 0 };

	switch( eMkID )
	{
	case XDF_SH:		///< 上海Lv1
		::strcpy( pszFilePath, "SSE/WEIGHT/" );
		break;
	case XDF_SZ:		///< 深证Lv1
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
	File::CreateDirectoryTree( sFilePath );							///< 若目录不存在则新建
	::sprintf( pszFileName, "WEIGHT%s.csv", sCode.c_str() );
	sFilePath += pszFileName;										///< 拼接出文件全路径

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
	std::vector<std::string>	vctFilesPath4SHL1;																///< 所有SHL1权重源文件路径
	std::vector<std::string>	vctFilesPath4SZL1;																///< 所有SZL1权重源文件路径
	std::string&				sFolder = Configuration::GetConfig().GetWeightFileFolder();						///< 权息文件所在的根目录
	std::string					sSHL1WeightFolder = JoinPath( sFolder, "SSE/" );								///< 上海L1Weight所在子目录
	std::string					sSZL1WeightFolder = JoinPath( sFolder, "SZSE/" );								///< 深圳L1Weight所在子目录
	int							nFileCount4SHL1 = EnumAllFiles( sSHL1WeightFolder.c_str(), vctFilesPath4SHL1 );	///< 列出所有SHL1权重源文件
	int							nFileCount4SZL1 = EnumAllFiles( sSZL1WeightFolder.c_str(), vctFilesPath4SZL1 );	///< 列出所有SZL1权重源文件

	///< 上海L1市场的权息信息列出
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

	///< 深圳L1市场的权息信息列出
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
	std::string				sCSVTitle;						///< CSV文件首行
	std::ofstream			oCSVDumper;						///< CSV输出文件
	std::ifstream			oWeightReader;					///< 权息信息文件
	SYSTEMTIME				oReadFileTime = { 0 };			///< 源头文件时间
	SYSTEMTIME				oDumpFileTime = { 0 };			///< 落盘文件时间

	if( sSourceFile.empty() || sDestFile.empty() )
	{
		return -1;		///< 文件路径为空无效
	}

	if( false == GetWeightFileWriteTime( sSourceFile, oReadFileTime ) )
	{
		return -1024;	///< 源文件不存在
	}
	else				///< 取一个目标文件的文件时间，若文件不存在，则取0值
	{
		GetWeightFileWriteTime( sDestFile, oDumpFileTime );
	}

	unsigned __int64	nSrcDateTime = oReadFileTime.wYear * 10000000000000 + oReadFileTime.wMonth * 100000000000 + (__int64)oReadFileTime.wDay * 1000000000 + oReadFileTime.wHour * 10000000 + oReadFileTime.wMinute * 100000 + oReadFileTime.wSecond * 1000 + oReadFileTime.wMilliseconds;
	unsigned __int64	nDestDateTime = oDumpFileTime.wYear * 10000000000000 + oDumpFileTime.wMonth * 100000000000 + (__int64)oDumpFileTime.wDay * 1000000000 + oDumpFileTime.wHour * 10000000 + oDumpFileTime.wMinute * 100000 + oDumpFileTime.wSecond * 1000 + oDumpFileTime.wMilliseconds;

	///< 判断源文件是否比较落盘转存文件的创建时间旧，则直接返回
	if( nSrcDateTime <= nDestDateTime && 0 != nDestDateTime )
	{
		return 0;
	}

	///< 打开源文件 & 目标文件
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

	///< 遍历数据源文件并转存到目标文件
	oWeightReader.seekg( 0, std::ios::beg );
	sCSVTitle = "date,a,b,c,d,e,base,flowbase\n";					///< format目标文件首行(title)
	oCSVDumper.write( sCSVTitle.c_str(), sCSVTitle.length() );		///< 写入title
	while( true )
	{
		int				nLen = 0;
		char			pszRecords[512] = { 0 };
		T_WEIGHT		tagSrcWeight = { 0 };

		if( oWeightReader.peek() == EOF || true == oWeightReader.eof() )
		{
			break;		///< 文件读完
		}

		oWeightReader.read( (char*)&tagSrcWeight, sizeof(tagSrcWeight) );///<oWeightReader.gcount();
		nLen = ::sprintf( pszRecords, "%d,%d,%d,%f,%f,%d,%f,%f\n"
				, tagSrcWeight.Date, tagSrcWeight.A, tagSrcWeight.B, tagSrcWeight.C, tagSrcWeight.D, tagSrcWeight.E, tagSrcWeight.BASE, tagSrcWeight.FLOWBASE );

		oCSVDumper.write( pszRecords, nLen );
	}

	return 0;
}





