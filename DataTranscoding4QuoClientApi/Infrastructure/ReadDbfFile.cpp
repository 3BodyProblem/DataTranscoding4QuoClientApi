#include "ReadDbfFile.h"


ReadDbfFile::ReadDbfFile(void)
{
	memset((char *)&m_DbfHeadInfo,0x00,sizeof(tagDbfHeadInfo));
	m_DbfFieldRecord = NULL;
	m_FieldCount = 0;
	m_RecordData = NULL;
	m_RecordCount = 0;
	m_CurRecordNo = 0;
	m_FileName = "";
}

ReadDbfFile::~ReadDbfFile()
{
	Close();
}

int  ReadDbfFile::Open(std::string filename)
{
	Close();

	m_FileName = filename;
	return(ReloadFromFile());
}

void ReadDbfFile::Close(void)
{
	CriticalLock				section(m_Section);

	inner_delete();
}

void ReadDbfFile::inner_delete(void)
{
	if ( m_DbfFieldRecord != NULL )
	{
		delete [] m_DbfFieldRecord;
		m_DbfFieldRecord = NULL;
	}
	m_FieldCount = 0;

	if ( m_RecordData != NULL )
	{
		delete [] m_RecordData;
		m_RecordData = NULL;
	}
	m_RecordCount = 0;

	m_CurRecordNo = 0;

	memset((char *)&m_DbfHeadInfo,0x00,sizeof(tagDbfHeadInfo));

	m_FileName = "";
}

int  ReadDbfFile::inner_loadfromfile(void)
{
	register int			errorcode;
	register int			i;

	if ( !m_FilePtr.is_open() )
	{
		m_FilePtr.open( m_FileName.c_str(), std::ios::in|std::ios::binary );

		if( !m_FilePtr.is_open() )
		{
			return -100;			//打开文件错误
		}
	}

	m_FilePtr.seekg( 0, std::ios::beg );
	m_FilePtr.read( (char *)&m_DbfHeadInfo,sizeof(tagDbfHeadInfo) );

	if ( m_FilePtr.gcount() != sizeof(tagDbfHeadInfo) )
	{
		m_FilePtr.close();			//读取文件长度错误

		return -1;
	}

	errorcode = (m_DbfHeadInfo.DataAddr - sizeof(tagDbfHeadInfo)) / sizeof(tagDbfFieldInfo);
	if ( errorcode <= 0 )
	{
		m_FilePtr.close();			//字段数量<=0

		return -2;
	}

	if ( m_DbfFieldRecord != NULL && errorcode != m_FieldCount )
	{
		m_FilePtr.close();			//刷新数据库过程中发现字段数量不匹配

		return -3;
	}

	if ( m_DbfFieldRecord == NULL )
	{
		m_FieldCount = errorcode;
		m_DbfFieldRecord = new tagDbfFieldInfo[m_FieldCount];
		if ( m_DbfFieldRecord == NULL )
		{
			assert(0);
			m_FilePtr.close();		//分配字段数量内存错误

			return -4;
		}

		m_FilePtr.read((char *)m_DbfFieldRecord,sizeof(tagDbfFieldInfo) * m_FieldCount);
		if( m_FilePtr.gcount() != (int)(sizeof(tagDbfFieldInfo) * m_FieldCount) )
		{
			m_FilePtr.close();		//读取自段信息发生错误

			return -5;
		}

		m_DbfFieldRecord[0].FieldOffset = 1;
		for ( i=1;i<=m_FieldCount-1;i++ )
		{
			m_DbfFieldRecord[i].FieldOffset = m_DbfFieldRecord[i-1].FieldOffset + m_DbfFieldRecord[i-1].FieldSize;
		}
	}

	if ( m_RecordData != NULL && (int)m_DbfHeadInfo.RecordCount != m_RecordCount )
	{
		/*modified by luozn 2007.3.7
		//纪录数量增加了
		m_FilePtr.Close();

		return(-6);
		*/
		if( m_RecordCount>0 )
			m_DbfHeadInfo.RecordCount = m_RecordCount = min( (long)m_DbfHeadInfo.RecordCount, m_RecordCount );
		else
		{
			m_FilePtr.close();
			return -6;
		}
	}

	if ( m_RecordData == NULL )
	{
		m_RecordData = new char[m_DbfHeadInfo.RecordCount * m_DbfHeadInfo.RecordSize];
		if ( m_RecordData == NULL )
		{
			//申请内存错误
			assert(0);
			m_FilePtr.close();

			return -7;
		}
		m_RecordCount = m_DbfHeadInfo.RecordCount;
	}

	m_FilePtr.seekg(m_DbfHeadInfo.DataAddr,0);
	m_FilePtr.read(m_RecordData,m_RecordCount * m_DbfHeadInfo.RecordSize);

	if ( m_FilePtr.gcount() != m_RecordCount * m_DbfHeadInfo.RecordSize )
	{
		//读取数据错误
		m_FilePtr.close();
		return -8;
	}

	m_CurRecordNo = 0;

	return 1;
}

int  ReadDbfFile::inner_findfieldno(const char * fieldname)
{
	register int				i;

	if ( m_DbfFieldRecord == NULL || fieldname == NULL )
	{
		return(-1);
	}

	for ( i=0;i<m_FieldCount;i++ )
	{
		if ( strncmp(fieldname,m_DbfFieldRecord[i].FieldName,11) == 0 )
		{
			return(i);
		}
	}

	return(-2);
}

int  ReadDbfFile::ReloadFromFile(void)
{
	CriticalLock				section(m_Section);
	register int				errorcode;

	errorcode = inner_loadfromfile();

	return(errorcode);
}

int  ReadDbfFile::ReadString(unsigned short fieldno,char * value,unsigned short insize)
{
	CriticalLock				section(m_Section);
	register int				errorcode;

	if ( m_DbfFieldRecord == NULL || m_RecordData == NULL || fieldno >= m_FieldCount || insize <= m_DbfFieldRecord[fieldno].FieldSize )
	{
		return(-1);
	}

	memcpy(value,m_RecordData + m_DbfHeadInfo.RecordSize * m_CurRecordNo + m_DbfFieldRecord[fieldno].FieldOffset,m_DbfFieldRecord[fieldno].FieldSize);
	errorcode = m_DbfFieldRecord[fieldno].FieldSize;

	value[errorcode] = 0;

	return(errorcode);
}

int  ReadDbfFile::ReadString(const char * fieldname,char * value,unsigned short insize)
{
	register int				errorcode;
	register int				fieldno;
	CriticalLock				section(m_Section);

	if ( (fieldno = inner_findfieldno(fieldname)) < 0 )
	{
		return(fieldno);
	}

	if ( m_DbfFieldRecord == NULL || m_RecordData == NULL || fieldno >= m_FieldCount || insize <= m_DbfFieldRecord[fieldno].FieldSize )
	{
		return(-1);
	}
	
	memcpy(value,m_RecordData + m_DbfHeadInfo.RecordSize * m_CurRecordNo + m_DbfFieldRecord[fieldno].FieldOffset,m_DbfFieldRecord[fieldno].FieldSize);
	errorcode = m_DbfFieldRecord[fieldno].FieldSize;
	value[errorcode] = 0;

	return(errorcode);
}

int  ReadDbfFile::ReadInteger(unsigned short fieldno,int * value)
{
	char						tempbuf[256];
	register int				errorcode;

	if ( (errorcode = ReadString(fieldno,tempbuf,256)) < 0 )
	{
		return(errorcode);
	}

	* value = strtol(tempbuf,NULL,10);

	return(1);
}

int  ReadDbfFile::ReadInteger(const char * fieldname,int * value)
{
	char						tempbuf[256];
	register int				errorcode;
	
	if ( (errorcode = ReadString(fieldname,tempbuf,256)) < 0 )
	{
		return(errorcode);
	}
	
	* value = strtol(tempbuf,NULL,10);
	
	return(1);
}

int  ReadDbfFile::ReadFloat(unsigned short fieldno,double * value)
{
	char						tempbuf[256];
	register int				errorcode;
	
	if ( (errorcode = ReadString(fieldno,tempbuf,256)) < 0 )
	{
		return(errorcode);
	}
	
	* value = strtod(tempbuf,NULL);
	
	return(1);
}

int  ReadDbfFile::ReadFloat(const char * fieldname,double * value)
{
	char						tempbuf[256];
	register int				errorcode;
	
	if ( (errorcode = ReadString(fieldname,tempbuf,256)) < 0 )
	{
		return(errorcode);
	}
	
	* value = strtod(tempbuf,NULL);
	
	return(1);
}

int  ReadDbfFile::First(void)
{
	CriticalLock				section(m_Section);

	m_CurRecordNo = 0;

	return(1);
}

int  ReadDbfFile::Last(void)
{
	CriticalLock				section(m_Section);

	m_CurRecordNo = m_DbfHeadInfo.RecordCount - 1;
	
	return(1);
}

int  ReadDbfFile::Prior(void)
{
	CriticalLock				section(m_Section);
	
	if ( m_CurRecordNo == 0 )
	{
		return(-1);
	}

	m_CurRecordNo --;
	
	return(1);
}

int  ReadDbfFile::Next(void)
{
	CriticalLock				section(m_Section);
	
	if ( m_CurRecordNo >= (int)(m_DbfHeadInfo.RecordCount - 1) )
	{
		return(-1);
	}
	
	m_CurRecordNo ++;
	
	return(1);
}

int  ReadDbfFile::Goto(int recno)
{
	CriticalLock				section(m_Section);
	
	if ( recno < 0 || recno >= (int)m_DbfHeadInfo.RecordCount )
	{
		return(-1);
	}

	m_CurRecordNo = recno;
	
	return(1);
}

int  ReadDbfFile::GetRecordCount(void)
{
	CriticalLock				section(m_Section);
	register int				errorcode;

	errorcode = m_RecordCount;

	return(errorcode);
}

int  ReadDbfFile::GetFieldCount(void)
{
	CriticalLock				section(m_Section);
	register int				errorcode;

	errorcode = m_FieldCount;
	
	return(errorcode);
}








