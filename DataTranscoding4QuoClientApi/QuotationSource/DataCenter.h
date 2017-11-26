#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/DateTime.h"


/**
 * @class				QuotationData
 * @brief				行情数据存储类
 */
class QuotationData
{
public:
	QuotationData();

	/**
	 * @brief			初始化
	 * @return			==0				成功
						!=				失败
	 */
	int					Initialize();

	/**
	 * @brief			释放资源
	 */
	void				Release();

public:

};



#endif









