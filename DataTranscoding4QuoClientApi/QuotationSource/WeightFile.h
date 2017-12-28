#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


/**
 * @class					WeightFile
 * @brief					权息文件
 * @author					barry
 */
class WeightFile
{
public:
	WeightFile();

	/**
	 * @brief				扫描所有权息数据文件
	 */
	void					ScanWeightFiles();

protected:
	/**
	 * @brief				将信息文件转存到文件
	 * @param[in]			sSourceFile		数据源文件
	 * @param[in]			sDestFile		数据目标文件
	 * @return				==0				操作成功
							!=				出错
	 */
	int						Redirect2File( std::string sSourceFile, std::string sDestFile );
};


#endif












