#include "UnitTest.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <time.h>


///< --------------------- ��Ԫ�����ඨ�� --------------------------------





///< ---------------------- ��Ԫ���Գ�ʼ���ඨ�� -------------------------




///< ---------------- ��Ԫ���Ե����������� -------------------------------


void UnitTestEnv::SetUp()
{
}

void UnitTestEnv::TearDown()
{
}


void ExecuteTestCase()
{
	static	bool	s_bInit = false;

	if( false == s_bInit )	{
		int			nArgc = 1;
		char*		pszArgv[32] = { "DataTranscoding4QuoClientApi.dll", };

		s_bInit = true;
		testing::AddGlobalTestEnvironment( new UnitTestEnv() );
		testing::InitGoogleTest( &nArgc, pszArgv );
		RUN_ALL_TESTS();
	}
}











