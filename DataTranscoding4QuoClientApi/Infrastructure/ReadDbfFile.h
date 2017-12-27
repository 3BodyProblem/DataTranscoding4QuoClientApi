#ifndef __MEngine_MReadDbfFileH__
#define __MEngine_MReadDbfFileH__


#include "File.h"
#include "Lock.h"
#include <fstream>


typedef struct
{
    char                    Market;         //���ݿ⿪ʼ��־
    unsigned char           UpdateYear;     //��������(��)
    unsigned char           UpdateMonth;    //��������(��)
    unsigned char           UpdateDay;      //��������(��)
    unsigned long           RecordCount;    //��¼��
    unsigned short          DataAddr;       //��������ʼλ��
    unsigned short          RecordSize;     //ÿ����¼����
    char                    Reserved[20];   //����
} tagDbfHeadInfo;

//DBF�ֶ�����
typedef struct
{
    char                    FieldName[11];  //�ֶ�����
    char                    FieldType;      //�ֶ����� C �ַ� N ���� L D ����
    unsigned long           FieldOffset;    //����ÿһ����¼��ƫ����
    unsigned char           FieldSize;      //�ֶγ���
    unsigned char           DecSize;        //С��λ��
    char                    Reserved[14];   //����      
} tagDbfFieldInfo;


//ע�⣺�������ڶ�ȡDBF�ļ�
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





