#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/DateTime.h"


typedef	std::map<enum XDFMarket,int>		TMAP_MKID2STATUS;


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
	/**
	 * @brief			����ģ�鹤��״̬
	 * @param[in]		cMarket			�г�ID
	 * @param[in]		nStatus			״ֵ̬
	 */
	void				UpdateModuleStatus( enum XDFMarket nMarket, int nStatus );

protected:
	TMAP_MKID2STATUS	m_mapModuleStatus;		///< ģ��״̬��

};



#endif









