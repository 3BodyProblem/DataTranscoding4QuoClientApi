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
class FileScanner
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

private:
	CriticalObject				m_oLock;				///< �ٽ�������
};




#endif



