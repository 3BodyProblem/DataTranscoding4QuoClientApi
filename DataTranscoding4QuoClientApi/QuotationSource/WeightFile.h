#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


#pragma pack(1)
typedef struct								
{
	unsigned long			Minute  : 6;		///< ��[0~59]
	unsigned long			Hour    : 5;		///< ʱ[0~23]
	unsigned long			Day     : 5;		///< ��[0~31]
	unsigned long			Month   : 4;		///< ��[0~12]
	unsigned long			Year    : 12;		///< ��[0~4095]
} tagQlDateTime;
/**
 * @class					T_WEIGHT
 * @brief					ȨϢ��Ϣ
 * @author					barry
 */
typedef struct						
{
	tagQlDateTime			Date;				///< ��������
	unsigned long			A;					///< ÿ10000���͹���
	unsigned long			B;					///< ÿ10000�������
	unsigned long			C;					///< ��ɼ�[Ԫ * �Ŵ���]
	unsigned long			D;					///< ÿ10000�ɺ���[Ԫ * �Ŵ���]
	unsigned long			E;					///< ÿ10000��������
	unsigned long			BASE;				///< �ܹɱ�[���]
	unsigned long			FLOWBASE;			///< ��ͨ�ɱ�[���]
	char					Reserved[4];		///< ����
} T_WEIGHT;
#pragma pack()


/**
 * @class					WeightFile
 * @brief					ȨϢ�ļ�
 * @author					barry
 */
class WeightFile
{
public:
	WeightFile();

	/**
	 * @brief				ɨ������ȨϢ�����ļ�
	 */
	void					ScanWeightFiles();

protected:
	/**
	 * @brief				����Ϣ�ļ�ת�浽�ļ�
	 * @param[in]			sSourceFile		����Դ�ļ�
	 * @param[in]			sDestFile		����Ŀ���ļ�
	 * @return				==0				�����ɹ�
							!=				����
	 */
	int						Redirect2File( std::string sSourceFile, std::string sDestFile );
};


#endif












