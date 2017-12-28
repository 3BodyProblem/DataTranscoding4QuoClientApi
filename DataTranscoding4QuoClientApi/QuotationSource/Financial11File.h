#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


#include <fstream>
#include "../Infrastructure/ReadDbfFile.h"


/**
 * @class			SHL1FinancialDbf
 * @brief			�Ϻ��ƾ����ݶ�ȡת����
 * @author			barry
 */
class SHL1FinancialDbf : public ReadDbfFile
{
public:
	SHL1FinancialDbf();
	~SHL1FinancialDbf();

	/**
	 * @brief				��ʼ��
	 * @return				==0				�ɹ�
							!=0				ʧ��
	 */
	int						Instance();

	/**
	 * @brief				�ͷ�
	 */
	void					Release();

	/**
	 * @brief				����Ϣ�ļ�ת�浽�ļ�
	 */
	int						Redirect2File();

protected:
	std::ofstream			m_oCSVDumper;				///< CSV����ļ�
};


/**
 * @class			SZL1FinancialDbf
 * @brief			���ڲƾ����ݶ�ȡת����
 * @author			barry
 */
class SZL1FinancialDbf : public ReadDbfFile
{
public:
	SZL1FinancialDbf();
	~SZL1FinancialDbf();

	/**
	 * @brief				��ʼ��
	 * @return				==0				�ɹ�
							!=0				ʧ��
	 */
	int						Instance();

	/**
	 * @brief				�ͷ�
	 */
	void					Release();

	/**
	 * @brief				����Ϣ�ļ�ת�浽�ļ�
	 */
	int						Redirect2File();

protected:
	std::ofstream			m_oCSVDumper;				///< CSV����ļ�
};



#endif












