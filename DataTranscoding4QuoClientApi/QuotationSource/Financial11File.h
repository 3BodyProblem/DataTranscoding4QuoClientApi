#ifndef __FINANCIAL_11_FILE_H__
#define __FINANCIAL_11_FILE_H__


#include "../Infrastructure/ReadDbfFile.h"


class SHL1FinancialDbf : public MReadDbfFile
{
private:
	unsigned long			m_ulSaveDate;
public:
	SHShow2003Dbf();
	~SHShow2003Dbf();
	int Instance( bool = true );
	void Release();
	int PreProcessFile();
private:
	int GetShNameTableData( unsigned long RecordPos, tagSHL2Mem_NameTable * Out, unsigned long * ulClose, unsigned long * ulOpen );
	int GetDbfDate( unsigned long * );
protected:
	static void * __stdcall ThreadFunc( void * Param );
public:
	static bool IsIndex( const char (&)[6] );
	static bool IsAGu( const char (&)[6] );
	static bool IsBGu( const char (&)[6] );
	static bool IsZhaiQuan( const char (&)[6] );
	static bool IsZhuanZhai( const char (&)[6] );
	static bool IsHuig( const char (&)[6] );
	static bool IsETF( const char (&)[6] );
	static bool IsJiJinTong( const char (&)[6] );
	static bool IsGzlx( const char (&)[6] );
	static bool IsQzxx( const char (&)[6] );
	static bool IsJijin( const char (&)[6] );
};



#endif




