#include "Base.h"


char* FormatDouble2Str( const double& dVal, char* pszOutputBuffer, unsigned int nOutputBufLen, unsigned int nPrecision, bool bEconomicalModel )
{
	int				nFormatStrLen = 0;

	if( NULL == pszOutputBuffer || nOutputBufLen < 1 ) {
		return NULL;
	}

	//////////////////< ��ջ���
	::strncpy( pszOutputBuffer, "0.0", 3 );

	//////////////////< ������ֵ�����
	if( dVal == 0. ) {
		if( false == bEconomicalModel ) {
			return pszOutputBuffer;			///< �ǽ�Լģʽ�µ�0.0ֵ����
		} else {
			pszOutputBuffer[0] = '\0';
			return pszOutputBuffer;			///< ��Լģʽ�µĿ�''ֵ����
		}
	}

	/////////////////// ��������ֵ����Ҫ����ȷλ�� & ȥĩβ���������ֵ
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

	///< �Ծ�ȷλ������1λ��С������ȥĩβ��������0
	while( nFormatStrLen > 3 && nPrecision > 1 ) {
		nFormatStrLen--;

		if( pszOutputBuffer[nFormatStrLen] == '0' ) {
			pszOutputBuffer[nFormatStrLen] = '\0';
		}
	}

	return pszOutputBuffer;
};

