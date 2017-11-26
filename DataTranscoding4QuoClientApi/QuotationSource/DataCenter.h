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
 * @brief				�������ݴ洢��
 */
class QuotationData
{
public:
	QuotationData();

	/**
	 * @brief			��ʼ��
	 * @return			==0				�ɹ�
						!=				ʧ��
	 */
	int					Initialize();

	/**
	 * @brief			�ͷ���Դ
	 */
	void				Release();

public:

};



#endif









