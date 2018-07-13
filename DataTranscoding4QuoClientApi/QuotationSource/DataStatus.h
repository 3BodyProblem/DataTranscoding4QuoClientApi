#ifndef __DATA_STATUS_H__
#define __DATA_STATUS_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../QuoteClientApi.h"
#include "../Infrastructure/Lock.h"


///< �������Ʊ��ͣ�Ʊ�Ǳ� //////////////////////////////////////////////////////////////////////////////////
/**
 * @class						SecurityStatus
 * @brief						��Ʒ״̬��(ͣ�Ʊ��)
 * @detail						ͣ�Ʊ��������
									1) �Ϻ�ͣ�Ʊ�ʶ���壺
									 ' '=����
									 *=ͣ��

									2) ����ͣ�Ʊ�ʶ���壺
									 '.'=����
									'Y'=����ͣ�� 
									'H'=��ͣ����   
									'N'=�������� 
									'M'=�ָ���������
									 'J'=��˾������ 
									'X'=��Ȩ��Ϣ 
									'V'=����ͶƱ 
									'A'=��������Ҳ����ȯ 
									'C'=�������� 
									'S'=������ȯ 
									'W'=����ҪԼ�չ��� 
									'I'=��ǰ֤ȯ���ڷ����ڼ�
 */
class SecurityStatus
{
public:
	typedef std::map<std::string,unsigned char>		T_CODE_SFlag_Table;	///< ͣ�Ʊ�Ǳ�
public:
	SecurityStatus();

	/**
	 * @brief					��ȡ������Ʒ��ͣ�Ʊ�Ƿ�
	 * @param[in]				eMkID				�г����
	 * @param[in]				pQuoteQueryApi		��ѯ�ӿ�
	 * @param[in]				nWareCount			��Ʒ����
	 * @return					���ػ�ȡ��������-1��ʾʧ��
	 */
	int							FetchAllSFlag( enum XDFMarket eMkID, QuotePrimeApi* pQuoteQueryApi, unsigned int nWareCount );

	/**
	 * @brief					�Ƿ�Ϊȫ��ͣ��
	 * @param[in]				sCode				��Ʒ����
	 * @return					1					��ȫ��ͣ��
								0					����ȫ��ͣ��
								-1					��Ʒ���벻����
								<-1					��������
	 */
	int							IsStop( std::string sCode );

protected:
	enum XDFMarket				m_eMarketID;			///< �г����
	T_CODE_SFlag_Table			m_mapSFlagTable;		///< ��Ʊͣ�Ʊ�Ǳ�
	CriticalObject				m_oLockData;			///< ״̬��������
};




#endif










