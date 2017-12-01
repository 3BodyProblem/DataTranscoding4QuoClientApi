#ifndef __INFRASTRUCTURE__FILE_H__
#define __INFRASTRUCTURE__FILE_H__


#include <string>
#include "DateTime.h"
#include <sys/stat.h>  // windows 下调用stat函数，anzl 20091211
#ifdef	LINUXCODE
#include <utime.h>
#include <fcntl.h>
#include <dirent.h>
#endif


class File
{
public:
	static int  CreateOneDirectory( std::string strFileName );
	static int  CreateDirectoryTree( std::string strDirTreeName );
	static bool IsExist( std::string strFileName );
};


#endif



