#include "Base.h"


char* FormatDouble2Str( const double& dVal, char* pszOutputBuffer, unsigned int nOutputBufLen, unsigned int nPrecision, bool bEconomicalModel )
{
	int				nFormatStrLen = 0;

	if( NULL == pszOutputBuffer || nOutputBufLen < 1 ) {
		return NULL;
	}

	//////////////////< 清空缓存
	::strncpy( pszOutputBuffer, "0.0", 3 );

	//////////////////< 处理，零值的情况
	if( dVal == 0. ) {
		if( false == bEconomicalModel ) {
			return pszOutputBuffer;			///< 非节约模式下的0.0值返回
		} else {
			pszOutputBuffer[0] = '\0';
			return pszOutputBuffer;			///< 节约模式下的空''值返回
		}
	}

	/////////////////// 处理，非零值，需要做精确位数 & 去末尾无意义的零值
	switch( nPrecision )
	{
	case 1:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.1f", dVal );
	case 2:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.2f", dVal );
	case 3:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.3f", dVal );
	case 4:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.4f", dVal );
	case 5:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.5f", dVal );
	case 6:
		nFormatStrLen = ::sprintf( pszOutputBuffer, "%.6f", dVal );
		break;
	default:
		return NULL;
	}

	///< 对精确位数大于1位的小数，剔去末尾的无意义0
	while( nFormatStrLen > 3 && nPrecision > 1 ) {
		nFormatStrLen--;

		if( pszOutputBuffer[nFormatStrLen] == '0' ) {
			pszOutputBuffer[nFormatStrLen] = '\0';
		}
	}

	return pszOutputBuffer;
};

