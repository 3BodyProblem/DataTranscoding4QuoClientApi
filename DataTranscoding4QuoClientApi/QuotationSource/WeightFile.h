#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


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












