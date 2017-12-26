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
 * @brief			数据源文件扫描及转存类
 * @author			barry
 */
class FileScanner
{
public:
	FileScanner();
	~FileScanner();

	/**
	 * @brief					初始化ctp行情接口
	 * @return					>=0			成功
								<0			错误
	 * @note					整个对象的生命过程中，只会启动时真实的调用一次
	 */
	int							Initialize();

	/**
	 * @brief					释放ctp行情接口
	 */
	int							Release();

private:
	CriticalObject				m_oLock;				///< 临界区对象
};




#endif



