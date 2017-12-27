/*
	仅仅从L1库中取得码表
*/
#ifndef __L1_DBFFILE_IO_H__
#define __L1_DBFFILE_IO_H__

#include "../DataCenter/MemStructure.h"

bool is_found(const char (&code)[6]);

class SHShow2003Dbf : public MReadDbfFile
{
private:
	MThread					m_thread;
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
protected:

};






#endif//__L1_DBFFILE_IO_H__