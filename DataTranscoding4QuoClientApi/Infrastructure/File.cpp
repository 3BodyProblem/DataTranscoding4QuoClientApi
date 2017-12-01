#include <io.h>
#include <stdlib.h>
#include "File.h"
#include <windows.h>


#ifndef LINUXCODE
#else
#include <execinfo.h>
#endif


int  File::CreateOneDirectory( std::string strFileName )
{
	#ifndef LINUXCODE
		if ( ::CreateDirectory(strFileName.c_str(),NULL) == 0 )
		{
			return -1;
		}
		
		return 1;
	#else
		if ( mkdir(strFileName.c_str(),0x1FF) < 0 )
		{
			return(MErrorCode::GetSysErr());
		}
		
		return(1);
	#endif
}

int  File::CreateDirectoryTree( std::string strDirTreeName )
{
	char				szdirtree[256];
	char				tempbuf[256];
	register int		i;
	register int		errorcode;
	register int		ilength;
	
	strncpy(szdirtree,strDirTreeName.c_str(),256);
	ilength = (int)strlen(szdirtree);
	for ( i=0;i<ilength;i++ )
	{
		if ( i > 0 && (szdirtree[i] == '\\' || szdirtree[i] == '/') && szdirtree[i-1] != '.' && szdirtree[i-1] != ':' )
		{
			memcpy(tempbuf,szdirtree,i);
			tempbuf[i] = 0;

			if ( !File::IsExist(tempbuf)&&(errorcode = File::CreateOneDirectory(tempbuf)) < 0 )
			{
 				return(errorcode);
			}
		}
	}
	
	return(File::CreateOneDirectory(szdirtree));
}

bool File::IsExist( std::string strFileName )
{
	#ifndef LINUXCODE

		if ( _access(strFileName.c_str(),0) == -1 )
		{
			return(false);
		}
		
		return(true);

	#else

		if ( access(strFileName.c_str(),0) == -1 )
		{
			return(false);
		}
		
		return(true);

	#endif
}



