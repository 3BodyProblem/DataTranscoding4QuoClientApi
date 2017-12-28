#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


#pragma pack(4)
/**
 * @class			T_WEIGHT
 * @brief			权息信息
 * @author			barry
 */
typedef struct
{
	int				Date;				///< 商品日期(YYYYMMDD)
	int				A;					///< 每10000股送股数(单位:股)
	int				B;					///< 每10000股配股数(单位:股)
	double			C;					///< 配股价(单位:元)
	double			D;					///< 每10股红利(单位:元)
	int				E;					///< 每10股增股数(单位:股)
	double			BASE;				///< 总股本(单位:万股)
	double			FLOWBASE;			///< 流通股本(单位:万股)
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












