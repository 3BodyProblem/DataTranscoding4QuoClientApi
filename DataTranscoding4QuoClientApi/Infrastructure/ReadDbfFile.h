#ifndef __MEngine_MReadDbfFileH__
#define __MEngine_MReadDbfFileH__


#include "File.h"
#include "Lock.h"
#include <fstream>


typedef struct
{
    char                    Market;         //数据库开始标志
    unsigned char           UpdateYear;     //更新日期(年)
    unsigned char           UpdateMonth;    //更新日期(月)
    unsigned char           UpdateDay;      //更新日期(日)
    unsigned long           RecordCount;    //记录数
    unsigned short          DataAddr;       //数据区开始位置
    unsigned short          RecordSize;     //每条纪录长度
    char                    Reserved[20];   //保留
} tagDbfHeadInfo;

//DBF字段描述
typedef struct
{
    char                    FieldName[11];  //字段名称
    char                    FieldType;      //字段类型 C 字符 N 数字 L D 日期
    unsigned long           FieldOffset;    //对于每一条纪录的偏移量
    unsigned char           FieldSize;      //字段长度
    unsigned char           DecSize;        //小数位数
    char                    Reserved[14];   //保留      
} tagDbfFieldInfo;


//注意：仅仅用于读取DBF文件
class ReadDbfFile
{
protected:
	CriticalObject			m_Section;
	tagDbfHeadInfo			m_DbfHeadInfo;
	tagDbfFieldInfo		*	m_DbfFieldRecord;
	int						m_FieldCount;
	char				*	m_RecordData;
	int						m_RecordCount;
	int						m_CurRecordNo;
protected:
	std::string				m_FileName;
	std::ifstream			m_FilePtr;
protected:
	__inline void inner_delete(void);
	__inline int  inner_loadfromfile(void);
	__inline int  inner_findfieldno(const char * fieldname);
public:
	ReadDbfFile(void);
	virtual ~ReadDbfFile();
public:
	int  Open(std::string filename);
	void Close(void);
public:
	int  ReloadFromFile(void);
public:
	int  ReadString(unsigned short fieldno,char * value,unsigned short insize);
	int  ReadString(const char * fieldname,char * value,unsigned short insize);
	int  ReadInteger(unsigned short fieldno,int * value);
	int  ReadInteger(const char * fieldname,int * value);
	int  ReadFloat(unsigned short fieldno,double * value);
	int  ReadFloat(const char * fieldname,double * value);
public:
	int  First(void);
	int  Last(void);
	int  Prior(void);
	int  Next(void);
	int  Goto(int recno);
public:
	int  GetRecordCount(void);
	int  GetFieldCount(void);
};


#endif





