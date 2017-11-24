#ifndef __DATA_DUMP_H__
#define __DATA_DUMP_H__


#pragma warning(disable:4786)
#include <string>
#include <fstream>
#include "../Configuration.h"
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


#pragma pack(1)
struct RecordMetaInfo
{
    unsigned int		nMs_time;		///< ��������ʱ��
    unsigned int		nLen;			///< ���ݿ鳤��
    unsigned __int64	nStart;			///< д��λ������
	unsigned short		nMsgID;			///< ��ϢID
};
#pragma pack()


/**
 * @class				QuotationRecorder
 * @brief				����������
 * @author				barry
 */
class QuotationRecorder
{
public:
	QuotationRecorder();
    ~QuotationRecorder();

	/**
	 * @brief			���ļ�
	 * @param[in]		pszFilePath			�����ļ�·��
	 * @param[in]		bIsOverwrite		�Ƿ���дͬ�����ļ�
	 * @return			==0					�ɹ�
						<0					ʧ��
	 */
    int					OpenFile( const char *pszFilePath, bool bIsOverwrite = false );

	/**
	 * @brief			�ر��ļ�
	 */
    void				CloseFile();

	/**
	 * @brief			������������
	 * @param[in]		nMsgID				��ϢID
	 * @param[in]		pszData				���ݵ�ַ
	 * @param[in]		nLen				���ݳ���
	 * @return			==0					д�ɹ�
						<0					ʧ��
	 */
    int					Record( unsigned short nMsgID, const char* pszData, unsigned int nLen );

	/**
	 * @brief			ˢ���ļ�
	 */
	void				Flush();

protected:
    std::ofstream		m_oDataFile;		///< ���������ļ�
    std::ofstream		m_oIndexFile;		///< ����Ԫ��Ϣ�ļ�
    unsigned __int64	m_nDataPos;			///< ����дλ��
};


/**
 * @class				QuotationRecover
 * @brief				���������
 * @author				barry
 */
class QuotationRecover
{
public:
	QuotationRecover();
    ~QuotationRecover();

	/**
	 * @brief			���ļ�
	 * @param[in]		pszFilePath			�����ļ�·��
	 * @param[in]		nBeginTime			��ʼ�Գ�����ʱ�䣨֮ǰ���ǿ첥)
	 * @return			==0					�ɹ�
						<0					ʧ��
	 */
    int					OpenFile( const char *pszFilePath, unsigned int nBeginTime = 0xffffffff );

	/**
	 * @brief			�ر��ļ�
	 */
    void				CloseFile();

	/**
	 * @brief			�������ݶ���
	 * @param[out]		nMsgID				��ϢID
	 * @param[out]		pszData				���ݻ����ַ
	 * @param[in]		nLen				���ݻ��泤��
	 * @return			>0					д�ɹ�
						==0					����
						<0					ʧ��
	 */
    int					Read( unsigned short& nMsgID, char* pszData, unsigned int nLen );

	/**
	 * @brief			Enf Of File
	 */
	bool				IsEOF();

protected:
    std::ifstream		m_oDataFile;		///< ���������ļ�
    std::ifstream		m_oIndexFile;		///< ����Ԫ��Ϣ�ļ�
protected:
	unsigned int		m_nBeginTime;		///< ��ʼ�����ٶȲ��ŵ�ʱ�䣨֮ǰ�ǿ첥)
	unsigned int		m_nLastTime;		///< ���һ�ζ������г�ʱ���¼
};




#endif









