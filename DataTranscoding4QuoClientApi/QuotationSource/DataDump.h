#ifndef __DATA_DUMP_H__
#define __DATA_DUMP_H__


#pragma warning(disable:4786)
#include <string>
#include <fstream>
#include "../Configuration.h"
#include "../Infrastructure/DateTime.h"


/**
 * @brief					连接路径和文件名(处理了是否路径有带分隔符的问题)
 * @param[in]				sPath				保存路径
 * @param[in]				sFileName			文件名
 * @detail					不需要考虑sPath是否以/或\结束
 */
std::string	JoinPath( std::string sPath, std::string sFileName );


/**
 * @brief					生成文件名
 * @param[in]				sFolderPath			保存路径
 * @param[in]				sFileName			文件名
 * @param[in]				nMkDate				日期( 通过日期生成 -> 文件扩展名(星期的索引值) )
 * @note					本函数生成的文件名，可以通过“星期”，保证最多只生成七个文件名，保证不占用过多磁盘空间
 */
std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate );


#pragma pack(1)
struct RecordMetaInfo
{
    unsigned int		nMs_time;		///< 本机毫秒时间
    unsigned int		nLen;			///< 数据块长度
    unsigned __int64	nStart;			///< 写入位置索引
	unsigned short		nMsgID;			///< 消息ID
};
#pragma pack()


/**
 * @class				QuotationRecorder
 * @brief				行情落盘类
 * @author				barry
 */
class QuotationRecorder
{
public:
	QuotationRecorder();
    ~QuotationRecorder();

	/**
	 * @brief			打开文件
	 * @param[in]		pszFilePath			落盘文件路径
	 * @param[in]		bIsOverwrite		是否重写同名的文件
	 * @return			==0					成功
						<0					失败
	 */
    int					OpenFile( const char *pszFilePath, bool bIsOverwrite = false );

	/**
	 * @brief			关闭文件
	 */
    void				CloseFile();

	/**
	 * @brief			行情数据落盘
	 * @param[in]		nMsgID				消息ID
	 * @param[in]		pszData				数据地址
	 * @param[in]		nLen				数据长度
	 * @return			==0					写成功
						<0					失败
	 */
    int					Record( unsigned short nMsgID, const char* pszData, unsigned int nLen );

	/**
	 * @brief			刷新文件
	 */
	void				Flush();

protected:
    std::ofstream		m_oDataFile;		///< 行情数据文件
    std::ofstream		m_oIndexFile;		///< 行情元信息文件
    unsigned __int64	m_nDataPos;			///< 数据写位置
};


/**
 * @class				QuotationRecover
 * @brief				行情加载类
 * @author				barry
 */
class QuotationRecover
{
public:
	QuotationRecover();
    ~QuotationRecover();

	/**
	 * @brief			打开文件
	 * @param[in]		pszFilePath			落盘文件路径
	 * @param[in]		nBeginTime			开始自常播的时间（之前都是快播)
	 * @return			==0					成功
						<0					失败
	 */
    int					OpenFile( const char *pszFilePath, unsigned int nBeginTime = 0xffffffff );

	/**
	 * @brief			关闭文件
	 */
    void				CloseFile();

	/**
	 * @brief			行情数据读出
	 * @param[out]		nMsgID				消息ID
	 * @param[out]		pszData				数据缓存地址
	 * @param[in]		nLen				数据缓存长度
	 * @return			>0					写成功
						==0					读完
						<0					失败
	 */
    int					Read( unsigned short& nMsgID, char* pszData, unsigned int nLen );

	/**
	 * @brief			Enf Of File
	 */
	bool				IsEOF();

protected:
    std::ifstream		m_oDataFile;		///< 行情数据文件
    std::ifstream		m_oIndexFile;		///< 行情元信息文件
protected:
	unsigned int		m_nBeginTime;		///< 开始正常速度播放的时间（之前是快播)
	unsigned int		m_nLastTime;		///< 最后一次读出的市场时间记录
};




#endif









