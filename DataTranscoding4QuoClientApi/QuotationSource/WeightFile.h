#ifndef __WEIGHT_FILE_H__
#define __WEIGHT_FILE_H__


#include <stdio.h>
#include <fstream>
#include <windows.h>
#include "../Infrastructure/ReadDbfFile.h"


#pragma pack(4)
/**
 * @class			T_WEIGHT
 * @brief			ȨϢ��Ϣ
 * @author			barry
 */
typedef struct
{
	int				Date;				///< ��Ʒ����(YYYYMMDD)
	int				A;					///< ÿ10000���͹���(��λ:��)
	int				B;					///< ÿ10000�������(��λ:��)
	double			C;					///< ��ɼ�(��λ:Ԫ)
	double			D;					///< ÿ10�ɺ���(��λ:Ԫ)
	int				E;					///< ÿ10��������(��λ:��)
	double			BASE;				///< �ܹɱ�(��λ:���)
	double			FLOWBASE;			///< ��ͨ�ɱ�(��λ:���)
} T_WEIGHT;
#pragma pack()


/**
 * @class					WeightFile
 * @brief					ȨϢ�ļ�
 * @author					barry
 */
class WeightFile
{
public:
	WeightFile();

	/**
	 * @brief				ɨ������ȨϢ�����ļ�
	 */
	void					ScanWeightFiles();

protected:
	/**
	 * @brief				����Ϣ�ļ�ת�浽�ļ�
	 * @param[in]			sSourceFile		����Դ�ļ�
	 * @param[in]			sDestFile		����Ŀ���ļ�
	 * @return				==0				�����ɹ�
							!=				����
	 */
	int						Redirect2File( std::string sSourceFile, std::string sDestFile );
};


#endif












