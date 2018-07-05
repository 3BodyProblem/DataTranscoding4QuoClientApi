"""

通达信5分钟线*.lc5文件和*.lc1文件数据结构：
    文件名即股票代码
    每32个字节为一个5分钟数据，每字段内低字节在前
    00 ~ 01 字节：日期，整型，设其值为num，则日期计算方法为：
                  year=floor(num/2048)+2004;
                  month=floor(mod(num,2048)/100);
                  day=mod(mod(num,2048),100);
    02 ~ 03 字节： 从0点开始至目前的分钟数，整型
    04 ~ 07 字节：开盘价，float型
    08 ~ 11 字节：最高价，float型
    12 ~ 15 字节：最低价，float型
    16 ~ 19 字节：收盘价，float型
    20 ~ 23 字节：成交额，float型
    24 ~ 27 字节：成交量（股），整型
    28 ~ 31 字节：（保留）


本脚本运行依赖包的安装：
   python3 -m pip install --upgrade numpy
   python3 -m pip install --upgrade pandas

使用：
    通过python3调起脚本前，需要在[Configuration]章节中配置输入的通达信分钟线文件所在根目录

"""


import os, sys
import numpy as np
import pandas as pd
from struct import *


############ Configuration #############################################################

### 必选参数
# 通达信分钟线所在目录
sTdxMinDataFolder = r"C:\zd_zszq\vipdoc\sh\minline"


### 可选参数
# 是否开启，预阅模式
bPreviewMode = False


############ Convertion Code ###########################################################

### 私有全局变量
sMarketCode = ""        # 市场编号


def ConvertTDXFile( sTdxFilePath ):
    """
        本函数将通达信一分钟线文件中的数据转换成.csv格式

        sTdxFilePath    通达信一分钟线文件所在路径
    """
    nMinDataEnd = 32                 # 当前1分钟线数据开始位置
    nMinDataBegin = 0                # 当前1分钟线数据结束位置
    lstMinData = []                  # 1分钟线数据合并列表

    ####### 打开通达信的分钟线文件 & 读出全部数据 ######
    objTDXFile = open(sTdxFilePath,'rb')
    objBuffer = objTDXFile.read()
    objTDXFile.close()
    ####### 遍历数据中的每个1分钟线数据块 ##############
    nDataCount = len(objBuffer) / 32 # 计算1分钟线的数量
    for i in range(int(nDataCount)):
        objMinData = unpack('hhfffffii',objBuffer[nMinDataBegin:nMinDataEnd])   # 1分钟线原始数据块
        lstMinData.append( [str(int(objMinData[0]/2048)+2004)+str(int(objMinData[0]%2048/100)).zfill(2)+str(objMinData[0]%2048%100).zfill(2),str(int(objMinData[1]/60)).zfill(2)+str(objMinData[1]%60).zfill(2)+'00', objMinData[2], objMinData[3], objMinData[4], objMinData[5], objMinData[6], objMinData[7]] )

        ### 为下次取1分钟线数据做好偏移
        nMinDataBegin = nMinDataBegin + 32
        nMinDataEnd = nMinDataEnd + 32
    ####### 预阅模式下，打印各文件中头和尾部分的数据 ###
    if True == bPreviewMode:
        df = pd.DataFrame(lstMinData, columns=['date','time','open','high','low','close','amount','volume'])
        print( df )

    return lstMinData                # 返回1分钟线列表


################# Entrance #############################################################
if __name__ == '__main__':
    sTdxMinDataFolder = sTdxMinDataFolder.replace( "\\", "/" )
    if False == os.path.exists( sTdxMinDataFolder ) :
        print( r"设定的通达信分钟线目录不存在：", sTdxMinDataFolder  )
    else:
        ########## 根据提供的数据目录路径确定所属市场 ##############
        if r"sh/minline" in sTdxMinDataFolder:
            sMarketCode = "SSE"
        elif r"sz/minline" in sTdxMinDataFolder:
            sMarketCode = "SZSE"
        else:
            print( "指定的通达信数据源路径中，不包含市场归属信息!" )
            sys.exit( 1024 )

        ########## 遍历数据目录下的所有文件 & 转存 #################
        lstTdxPath = os.listdir( sTdxMinDataFolder )
        for sTdxPath in lstTdxPath:
            if True == os.path.isdir( sTdxPath ):
                continue

            sTdxFullPath = os.path.join( sTdxMinDataFolder, sTdxPath )
            lstTdxMinRecords = ConvertTDXFile( sTdxFullPath )   # 读出每个文件中的1分钟线数据列表





