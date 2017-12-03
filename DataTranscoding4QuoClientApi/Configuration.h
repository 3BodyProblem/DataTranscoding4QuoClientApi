#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <map>
#include <vector>
#include <string>
#include "Infrastructure/Lock.h"


extern	HMODULE					g_oModule;							///< ��ǰdllģ�����
extern	std::string				GetModulePath( void* hModule );		///< ��ȡ��ǰģ��·��


/**
 * @class						T_CLOSE_CFG
 * @brief						����������Ϣ
 * @author						barry
 */
typedef struct
{
	unsigned int				LastDate;				///< �����������
	unsigned int				CloseTime;				///< ����ʱ��
} CLOSE_CFG;


typedef std::vector<CLOSE_CFG>	TB_MK_CLOSECFG;			///< �г��������ñ�
typedef std::map<short,TB_MK_CLOSECFG>	MAP_MK_CLOSECFG;///< ���г���������


/**
 * @class						Configuration
 * @brief						������Ϣ
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					��ȡ���ö���ĵ������ö���
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					����������
	 * @return					==0					�ɹ�
								!=					����
	 */
    int							Initialize();

public:
	/**
	 * @brief					ȡ�ÿ�������Ŀ¼(���ļ���)
	 */
	const std::string&			GetDumpFolder() const;

	/**
	 * @brief					��ȡ�������·��(���ļ���)
	 */
	const std::string&			GetDataCollectorPluginPath() const;

	/**
	 * @brief					�Ƿ�Ϊ����ģʽ
	 * @return					true				�����Զ�����ģʽ
	 */
	bool						IsBroadcastModel() const;

	/**
	 * @brief					��ȡ�����ļ���·��
	 */
	std::string					GetTradeFilePath() const;

	/**
	 * @brief					��ȡ�����ļ���·��
	 */
	std::string					GetQuotationFilePath() const;

	/**
	 * @brief					��ȡ�����ٶȲ��ŵĿ�ʼʱ��
	 */
	unsigned int				GetBroadcastBeginTime() const;

	/**
	 * @brief					��ȡ�г�������Ϣ����
	 */
	MAP_MK_CLOSECFG&			GetMkCloseCfg();

private:
	std::string					m_sQuoPluginPath;		///< ������·��
	std::string					m_sDumpFileFolder;		///< ��������·��(��Ҫ���ļ���)
	bool						m_bBroadcastModel;		///< �����Զ�����ģʽ
	std::string					m_sBcTradeFile;			///< ���ŵ������ļ�·��
	std::string					m_sBcQuotationFile;		///< ���ŵ�ʵʱ�ļ�·��
	unsigned int				m_nBcBeginTime;			///< �����ٶȵĲ���ʱ��
	MAP_MK_CLOSECFG				m_mapMkCloseCfg;		///< �г�����ʱ������
};


#endif







