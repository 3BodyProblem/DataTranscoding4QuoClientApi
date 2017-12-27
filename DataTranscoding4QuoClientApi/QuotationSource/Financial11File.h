#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


#include <fstream>
#include "../Infrastructure/ReadDbfFile.h"


class SHL1FinancialDbf : public ReadDbfFile
{
public:
	SHL1FinancialDbf();
	~SHL1FinancialDbf();

	/**
	 * @brief		初始化
	 * @return		==0				成功
					!=0				失败
	 */
	int				Instance();

	/**
	 * @brief		释放
	 */
	void			Release();

	/**
	 * @brief		将信息文件转存到文件
	 * @param[in]	oDumper			文件句柄
	 */
	int				Redirect2File( std::ofstream& oDumper );

private:
	unsigned long		m_ulSaveDate;

};



#endif




