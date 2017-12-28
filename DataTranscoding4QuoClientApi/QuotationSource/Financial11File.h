#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


#include <fstream>
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
	 * @brief				初始化
	 * @return				==0				成功
							!=0				失败
	 */
	int						Instance();

	/**
	 * @brief				释放
	 */
	void					Release();

	/**
	 * @brief				将信息文件转存到文件
	 */
	int						Redirect2File();

protected:
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
	 * @brief				初始化
	 * @return				==0				成功
							!=0				失败
	 */
	int						Instance();

	/**
	 * @brief				释放
	 */
	void					Release();

	/**
	 * @brief				将信息文件转存到文件
	 */
	int						Redirect2File();

protected:
	std::ofstream			m_oCSVDumper;				///< CSV输出文件
};



#endif












