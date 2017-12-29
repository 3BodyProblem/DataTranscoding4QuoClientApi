#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


#pragma pack(1)
typedef struct								
{
	unsigned long			Minute  : 6;		///< 分[0~59]
	unsigned long			Hour    : 5;		///< 时[0~23]
	unsigned long			Day     : 5;		///< 日[0~31]
	unsigned long			Month   : 4;		///< 月[0~12]
	unsigned long			Year    : 12;		///< 年[0~4095]
} tagQlDateTime;
/**
 * @class					T_WEIGHT
 * @brief					权息信息
 * @author					barry
 */
typedef struct						
{
	tagQlDateTime			Date;				///< 资料日期
	unsigned long			A;					///< 每10000股送股数
	unsigned long			B;					///< 每10000股配股数
	unsigned long			C;					///< 配股价[元 * 放大倍数]
	unsigned long			D;					///< 每10000股红利[元 * 放大倍数]
	unsigned long			E;					///< 每10000股增股数
	unsigned long			BASE;				///< 总股本[万股]
	unsigned long			FLOWBASE;			///< 流通股本[万股]
	char					Reserved[4];		///< 保留
} T_WEIGHT;
#pragma pack()


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












