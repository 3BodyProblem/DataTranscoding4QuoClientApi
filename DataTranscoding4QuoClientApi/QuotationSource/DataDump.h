#ifndef __DATA_DUMP_H__
#define __DATA_DUMP_H__


#pragma warning(disable:4786)
#include <string>
#include <fstream>
#include "../Infrastructure/DateTime.h"


/**
 * @brief					����·�����ļ���(�������Ƿ�·���д��ָ���������)
 * @param[in]				sPath				����·��
 * @param[in]				sFileName			�ļ���
 * @detail					����Ҫ����sPath�Ƿ���/��\����
 */
std::string	JoinPath( std::string sPath, std::string sFileName );


/**
 * @brief					�����ļ���
 * @param[in]				sFolderPath			����·��
 * @param[in]				sFileName			�ļ���
 * @param[in]				nMkDate				����( ͨ���������� -> �ļ���չ��(���ڵ�����ֵ) )
 * @note					���������ɵ��ļ���������ͨ�������ڡ�����֤���ֻ�����߸��ļ�������֤��ռ�ù�����̿ռ�
 */
std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate );


/**
 * @class					MemoDumper
 * @brief					���ڴ����ݽ������л�/�����л�
 * @note					�ļ�ͷ��һ������ֵ(��������,unsigned int)
							&
							д�ļ�ʱ����д��һ����ʱ�ļ�����Ȼ����rename���Ա�֤�ļ����ݵ�"������"
 * @author					barry
 */
template<class TYPE>
class MemoDumper
{
public:
	/**
	 * @brief				����
	 * @detail				�򿪻򴴽�һ���ļ�
	 * @param[in]			pszFileFolder		�ļ�Ŀ¼
	 * @param[in]			bIsRead				true:Ϊ����һ���ļ�; false:Ϊдһ���ļ�
	 * @param[in]			nTradingDay			���齻������
	 * @param[in]			bAppendModel		׷��ģʽ(Ĭ��Ϊ����)
	 */
	MemoDumper( bool bIsRead, const char* pszFileFolder, unsigned int nTradingDay, bool bAppendModel = false );
	MemoDumper();
	~MemoDumper();

	/**
	 * @brief				�򿪻򴴽�һ���ļ�
	 * @param[in]			pszFileFolder		�ļ�Ŀ¼
	 * @param[in]			bIsRead				true:Ϊ����һ���ļ�; true:Ϊдһ���ļ�
	 * @param[in]			nTradingDay			���齻������
	 * @return				true				�ɹ�
	 */
	bool					Open( bool bIsRead, const char* pszFileFolder, unsigned int nTradingDay );

	/**
	 * @brief				�ر��ļ�
	 */
	void					Close();

	/**
	 * @brief				�Ƿ�򿪳ɹ�
	 */
	bool					IsOpen();

	/**
	 * @brief				ȡ���ļ����ݶ�Ӧ�����齻������
	 * @return				>0			�Ϸ�����
							=0			�Ƿ�����
	 */
	unsigned int			GetTradingDay();

	/**
	 * @brief				��/д����
	 * @return				>0			�ɹ������к�����������
							=0			�Ѿ�����������
							<0			����
	 */
	int						Read( TYPE& refData );
	int						Read( char* pData, unsigned int nDataLen );
	int						Write( const TYPE& refData );
	int						Write( const char* pData, unsigned int nDataLen );

protected:
	bool					m_bAppendModel;			///< �ļ�д׷��ģʽ
	unsigned int			m_nTradingDay;			///< �������ݵĽ�������
	std::ifstream			m_fInput;				///< д�ļ�
	std::ofstream			m_fOutput;				///< ���ļ�
	bool					m_bIsRead;				///< �Ƿ�Ϊ���ļ�
	char					m_pszTargetFile[128];	///< Ŀ��д�ļ�
	char					m_pszTmpFilePath[128];	///< ��ʱд�ļ�
};


template<class TYPE>
MemoDumper<TYPE>::~MemoDumper()
{
	Close();
}

template<class TYPE>
MemoDumper<TYPE>::MemoDumper()
 : m_bIsRead( true ), m_nTradingDay( 0 ), m_bAppendModel( false )
{
	::memset( m_pszTargetFile, 0, sizeof(m_pszTargetFile) );
	::memset( m_pszTmpFilePath, 0, sizeof(m_pszTmpFilePath) );
}

template<class TYPE>
MemoDumper<TYPE>::MemoDumper( bool bIsRead, const char* pszFileFolder, unsigned int nTradingDay, bool bAppendModel )
 : m_bIsRead( bIsRead ), m_nTradingDay( 0 ), m_bAppendModel( bAppendModel )
{
	if( NULL != pszFileFolder )
	{
		Open( bIsRead, pszFileFolder, nTradingDay );
	}
}

template<class TYPE>
void MemoDumper<TYPE>::Close()
{
	if( true == IsOpen() )
	{
		if( true == m_bIsRead )
		{
			m_fInput.close();
		}
		else
		{
			m_fOutput.close();
			::MoveFileEx( m_pszTmpFilePath, m_pszTargetFile, MOVEFILE_REPLACE_EXISTING );
		}
	}
}

template<class TYPE>
bool MemoDumper<TYPE>::Open( bool bIsRead, const char* pszFileFolder, unsigned int nTradingDay )
{
	m_bIsRead = bIsRead;
	::memset( m_pszTargetFile, 0, sizeof(m_pszTargetFile) );
	::printf( "MemoDumper::Open() : File Path = %s, read flag = %d, trading day = %u\n", pszFileFolder, bIsRead, nTradingDay );

	if( true == m_bIsRead )
	{
		m_fInput.open( pszFileFolder, std::ios::in|std::ios::binary );

		if( m_fInput.is_open() )
		{
			m_fInput.seekg( 0, std::ios::beg );
			m_fInput.read( (char*)&m_nTradingDay, sizeof(m_nTradingDay) );
		}
	}
	else
	{
		m_nTradingDay = nTradingDay;
		::strcpy( m_pszTmpFilePath, pszFileFolder );
		::strcpy( m_pszTargetFile, pszFileFolder );
		::strcat( m_pszTmpFilePath, ".tmp" );

		if( true == m_bAppendModel )
		{
			m_fOutput.open( m_pszTmpFilePath , std::ios::out|std::ios::binary|std::ios::app );
		}
		else
		{
			m_fOutput.open( m_pszTmpFilePath , std::ios::out|std::ios::binary );
		}

		if( !m_fOutput.is_open() ) {
			m_nTradingDay = 0;
		}

		m_fOutput.write( (char*)&m_nTradingDay, sizeof(m_nTradingDay) );
	}

	return IsOpen();
}

template<class TYPE>
unsigned int MemoDumper<TYPE>::GetTradingDay()
{
	return m_nTradingDay;
}

template<class TYPE>
bool MemoDumper<TYPE>::IsOpen()
{
	if( true == m_bIsRead )
	{
		return m_fInput.is_open();
	}
	else
	{
		return m_fOutput.is_open();
	}
}

template<class TYPE>
int MemoDumper<TYPE>::Read( TYPE& refData )
{
	return Read( (char*)&refData, sizeof(TYPE) );
}

template<class TYPE>
int MemoDumper<TYPE>::Read( char* pData, unsigned int nDataLen )
{
	if( false == IsOpen() )
	{
		return -1;
	}

//	if( true == m_fInput.eof() )
	if( m_fInput.peek() == EOF || true == m_fInput.eof() )
	{
		return 0;		///< �ļ�����
	}

	m_fInput.read( pData, nDataLen );

	return m_fInput.gcount();
}

template<class TYPE>
int MemoDumper<TYPE>::Write( const TYPE& refData )
{
	return Write( (const char*)&refData, sizeof(TYPE) );
}

template<class TYPE>
int MemoDumper<TYPE>::Write( const char* pData, unsigned int nDataLen )
{
	if( false == IsOpen() )
	{
		return -1;
	}

	m_fOutput.write( pData, nDataLen );

	return 1;
}








#endif









