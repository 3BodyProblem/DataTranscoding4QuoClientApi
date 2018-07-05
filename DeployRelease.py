# -*- coding: utf-8 -*-
"""
        @brief  将程序布置到对应的运行或调试环境
	@barry	barry
	@date	2018/7/3
"""


import os, sys, time, shutil


def copyfile( sSrcFile, sDestFile ):
        if not os.path.isfile( sSrcFile ):
                print( "%s is not exists!" % sSrcFile )
        else:
                sDestFilePath, sDestFileName = os.path.split( sDestFile )       #分离文件名和路径
                if not os.path.exists( sDestFilePath ):
                        os.makedirs( sDestFilePath )                            #创建路径
                shutil.copyfile( sSrcFile, sDestFile )                          #复制文件
                print( "Copy: %s->%s" % ( sSrcFile, sDestFile) )

        print( "[DONE] ##################################################################" )
        time.sleep( 3 )






if __name__ == '__main__':
	try:
                copyfile( "DataTranscoding4QuoClientApi\Release\DataTranscoding4QuoClientApi.dll", "D:\ServiceManager\srvunit\DataTranscoding4QuoClientApi\DataTranscoding4QuoClientApi.dll" )
	except Exception as e:
		print( r'[EXCEPTION] ' + str(e) )
	finally:
                print( "#######################################################################" )



