#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


/**
 * @class			SHL1FinancialDbf
 * @brief			上海财经数据读取转存类
 * @author			barry
 */
class SHL1FinancialDbf : public ReadDbfFile
{
public:
	SHL1FinancialDbf();
	~SHL1FinancialDbf();

	/**
	 * @brief				将信息文件转存到文件
	 * @return				==0				操作成功
							!=				出错
	 */
	int						Redirect2File();

protected:
	SYSTEMTIME				m_oDumpFileTime;			///< 落盘文件时间
	std::ofstream			m_oCSVDumper;				///< CSV输出文件
};


/**
 * @class			SZL1FinancialDbf
 * @brief			深圳财经数据读取转存类
 * @author			barry
 */
class SZL1FinancialDbf : public ReadDbfFile
{
public:
	SZL1FinancialDbf();
	~SZL1FinancialDbf();

	/**
	 * @brief				将信息文件转存到文件
	 * @return				==0				操作成功
							!=				出错
	 */
	int						Redirect2File();

protected:
	SYSTEMTIME				m_oDumpFileTime;			///< 落盘文件时间
	std::ofstream			m_oCSVDumper;				///< CSV输出文件
};



#endif












