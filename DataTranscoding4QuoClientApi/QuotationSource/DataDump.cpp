#include "DataDump.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>


std::string	JoinPath( std::string sPath, std::string sFileName )
{
	unsigned int		nSepPos = sPath.length() - 1;

	if( sPath[nSepPos] == '/' || sPath[nSepPos] == '\\' ) {
		return sPath + sFileName;
	} else {
		return sPath + "/" + sFileName;
	}
}

std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate )
{
	char				pszTmp[32] = { 0 };
	DateTime			oDate( nMkDate/10000, nMkDate%10000/100, nMkDate%100 );
	std::string&		sNewPath = JoinPath( sFolderPath, sFileName );

	::itoa( oDate.GetDayOfWeek(), pszTmp, 10 );
	sNewPath += ".";sNewPath += pszTmp;
	return sNewPath;
}


QuotationRecorder::QuotationRecorder()
 : m_nDataPos( 0 )
{
}

QuotationRecorder::~QuotationRecorder()
{
	CloseFile();
}

void QuotationRecorder::CloseFile()
{
	if( m_oDataFile.is_open() )
	{
		m_oDataFile.flush();
		m_oDataFile.close();
		m_oDataFile.clear();
	}

	if( m_oIndexFile.is_open() )
	{
		m_oIndexFile.flush();
		m_oIndexFile.close();
		m_oIndexFile.clear();
	}
}

int QuotationRecorder::OpenFile( const char *pszFilePath, bool bIsOverwrite )
{
	char							pszIndexFilePath[1024] = { 0 };
	std::ios_base::open_mode		nOpenMode = (true==bIsOverwrite) ? (std::ios::out|std::ios::binary) : (std::ios::out|std::ios::binary | std::ios::app);

	if( NULL == pszFilePath )
	{
		return -1;
	}

	CloseFile();
	::sprintf( pszIndexFilePath, "%s.index", pszFilePath );

	m_oDataFile.open( pszFilePath , nOpenMode );
	if( !m_oDataFile.is_open() )
	{
		CloseFile();
		return -2;
	}

	m_oIndexFile.open( pszIndexFilePath , nOpenMode );
	if( !m_oIndexFile.is_open() )
	{
		CloseFile();
		return -3;
	}

	m_oDataFile.seekp( 0, std::ios::end );

    if( true == bIsOverwrite )
	{
        m_nDataPos = 0;
    } else {
		m_nDataPos = m_oDataFile.tellp();
    }

	return 0;
}

unsigned int get_ms_time()
{
    _timeb tb;
    _ftime(&tb);
    tm *t = localtime(&tb.time);
    return (t->tm_hour * 10000 + t->tm_min * 100 + t->tm_sec) * 1000 + tb.millitm;
}

int QuotationRecorder::Record( unsigned short nMsgID, const char* pszData, unsigned int nLen )
{
	if( NULL == pszData || 0 == nLen )
	{
		return -1;
	}

	if( !m_oDataFile.is_open() )
	{
		return -2;
	}

	if( !m_oIndexFile.is_open() )
	{
		return -3;
	}

	m_oDataFile.write( pszData, nLen );

	RecordMetaInfo		oIndex = { 0 };
	oIndex.nLen = nLen;
	oIndex.nStart = m_nDataPos;
	oIndex.nMs_time = get_ms_time();
	oIndex.nMsgID = nMsgID;
	m_oIndexFile.write( (const char*)&oIndex, sizeof(RecordMetaInfo) );

	m_nDataPos += nLen;

	return 0;
}

void QuotationRecorder::Flush()
{
	if( m_oDataFile.is_open() )
	{
		m_oDataFile.flush();
	}

	if( m_oIndexFile.is_open() )
	{
		m_oIndexFile.flush();
	}
}


QuotationRecover::QuotationRecover()
 : m_nLastTime( 0 )
{
}

QuotationRecover::~QuotationRecover()
{
	CloseFile();
}

int QuotationRecover::OpenFile( const char *pszFilePath, unsigned int nBeginTime )
{
	char							pszIndexFilePath[1024] = { 0 };

	if( NULL == pszFilePath )
	{
		return -1;
	}

	CloseFile();
	if( nBeginTime < 0xffffffff )
	{
		m_nBeginTime = nBeginTime * 1000;			///< 快播时间
	}
	else
	{
		m_nBeginTime = nBeginTime;					///< 全程快播
	}
	::sprintf( pszIndexFilePath, "%s.index", pszFilePath );

	m_oDataFile.open( pszFilePath , std::ios::in|std::ios::binary );
	if( !m_oDataFile.is_open() )
	{
		CloseFile();
		return -2;
	}

	m_oIndexFile.open( pszIndexFilePath , std::ios::in|std::ios::binary );
	if( !m_oIndexFile.is_open() )
	{
		CloseFile();
		return -3;
	}

	m_oDataFile.seekg( 0, std::ios::beg );

	return 0;
}

void QuotationRecover::CloseFile()
{
	m_nLastTime = 0;

	if( m_oDataFile.is_open() )
	{
		m_oDataFile.close();
	}

	if( m_oIndexFile.is_open() )
	{
		m_oIndexFile.close();
	}
}

bool QuotationRecover::IsEOF()
{
	if( m_oIndexFile.peek() == EOF || true == m_oIndexFile.eof() )
	{
		return true;
	}

	if( m_oDataFile.peek() == EOF || true == m_oDataFile.eof() )
	{
		return true;
	}

	return false;
}

int QuotationRecover::Read( unsigned short& nMsgID, char* pszData, unsigned int nLen )
{
	if( NULL == pszData || 0 == nLen )
	{
		return -1;
	}

	if( !m_oDataFile.is_open() )
	{
		return -2;
	}

	if( !m_oIndexFile.is_open() )
	{
		return -3;
	}

	if( m_oIndexFile.peek() == EOF || true == m_oIndexFile.eof() )
	{
		return 0;				///< 文件读完
	}

	RecordMetaInfo		oIndex = { 0 };

	m_oIndexFile.read( (char*)&oIndex, sizeof(RecordMetaInfo) );

	if( oIndex.nLen > nLen )
	{
		CloseFile();
		::printf( "QuotationRecover::Read() : close file ... \n" );
		return -4;
	}

	nMsgID = oIndex.nMsgID;
	if( 0 == m_nLastTime )
	{
		m_nLastTime = oIndex.nMs_time;
	}

	int		nDiff = oIndex.nMs_time - m_nLastTime;		///< 计算需要sleep的时间

	if( nDiff > 0 && oIndex.nMs_time > m_nBeginTime )
	{
		::Sleep( nDiff );
	}

	m_nLastTime = oIndex.nMs_time;
	m_oDataFile.seekg( oIndex.nStart, std::ios::beg );
	m_oDataFile.read( pszData, oIndex.nLen );

	return m_oDataFile.gcount();
}







