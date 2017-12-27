#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


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

public:
	int				GetShNameTableData( unsigned long RecordPos, tagSHL2Mem_NameTable * Out, unsigned long * ulClose, unsigned long * ulOpen );
	int				GetDbfDate( unsigned long * );

protected:
	static void*	__stdcall ThreadFunc( void* pParam );

private:
	unsigned long		m_ulSaveDate;

};



#endif




