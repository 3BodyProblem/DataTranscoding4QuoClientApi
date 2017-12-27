#ifndef __FILE_SCANNER_H__
#define __FILE_SCANNER_H__


#pragma warning(disable:4786)
#include <set>
#include <stdexcept>
#include "DataDump.h"
#include "DataCenter.h"
#include "DataCollector.h"


/**
 * @class			FileScanner
 * @brief			����Դ�ļ�ɨ�輰ת����
 * @author			barry
 */
class FileScanner : public SimpleTask
{
public:
	FileScanner();
	~FileScanner();

	/**
	 * @brief					��ʼ��ctp����ӿ�
	 * @return					>=0			�ɹ�
								<0			����
	 * @note					������������������У�ֻ������ʱ��ʵ�ĵ���һ��
	 */
	int							Initialize();

	/**
	 * @brief					�ͷ�ctp����ӿ�
	 */
	int							Release();

protected:
	/**
	 * @brief					������(��ѭ��)
	 * @return					==0					�ɹ�
								!=0					ʧ��
	 */
	virtual int					Execute();

protected:
	/**
	 * @brief					����ת��ƾ����ݿ�
	 */
	void						ResaveFinancialFile();

	/**
	 * @brief					����ת��Ȩ����Ϣ��
	 */
	void						ResaveWeightFile();

private:
	CriticalObject				m_oLock;				///< �ٽ�������
};




#endif


