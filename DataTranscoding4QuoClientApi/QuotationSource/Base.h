#ifndef __DATATRANSCODEING_4_QUOTECLIENT_BASE__H__
#define __DATATRANSCODEING_4_QUOTECLIENT_BASE__H__


#include <stdio.h>
#include <string.h>


/**
 * @brief			��ʽ��ֵ��������� �����ʽָ����ȷС��λ�������ĩβ�������0)
 * @param[in]		dVal				��Ҫ��ʽ�����ַ����������ֵ
 * @param[out]		pszOutputBuffer		�������
 * @param[in]		nOutputBufLen		������泤��
 * @param[in]		nPrecision			��ʽ���ľ�ȷС��λ��
 * @param[in]		bEconomicalModel	true:������Լģʽ�����Ϊ0ֵ��������մ� --> '', false:��������Լģʽ�����Ϊ0ֵ�����'0'��'0.0'
 * @return			���ظ�ʽ���Ժ���ַ���ָ��
 */
char* FormatDouble2Str( const double& dVal, char* pszOutputBuffer, unsigned int nOutputBufLen, unsigned int nPrecision, bool bEconomicalModel = false );








#endif
