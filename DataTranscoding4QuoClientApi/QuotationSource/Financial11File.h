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
	 * @brief		��ʼ��
	 * @return		==0				�ɹ�
					!=0				ʧ��
	 */
	int				Instance();

	/**
	 * @brief		�ͷ�
	 */
	void			Release();

	/**
	 * @brief		����Ϣ�ļ�ת�浽�ļ�
	 * @param[in]	oDumper			�ļ����
	 */
	int				Redirect2File( std::ofstream& oDumper );

private:
	unsigned long		m_ulSaveDate;

};



#endif




