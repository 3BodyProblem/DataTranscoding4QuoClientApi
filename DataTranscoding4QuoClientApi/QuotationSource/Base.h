#ifndef __DATATRANSCODEING_4_QUOTECLIENT_BASE__H__
#define __DATATRANSCODEING_4_QUOTECLIENT_BASE__H__


#include <stdio.h>
#include <string.h>


/**
 * @brief			格式数值进行输出： 输出格式指定精确小数位（不输出末尾无意义的0)
 * @param[in]		dVal				需要格式化成字符后输出的数值
 * @param[out]		pszOutputBuffer		输出缓存
 * @param[in]		nOutputBufLen		输出缓存长度
 * @param[in]		nPrecision			格式化的精确小数位数
 * @param[in]		bEconomicalModel	true:开启节约模式，如果为0值，则输出空串 --> '', false:不开启节约模式，如果为0值，输出'0'或'0.0'
 * @return			返回格式化以后的字符串指针
 */
char* FormatDouble2Str( const double& dVal, char* pszOutputBuffer, unsigned int nOutputBufLen, unsigned int nPrecision, bool bEconomicalModel = false );








#endif
