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

    python3 TdxMin2CSV.py

编写：    Barry
日期：    2018/7/5


"""


import os, sys
import numpy as np
import pandas as pd
from struct import *


############ Configuration #############################################################

### 必选参数
# 通达信分钟线所在目录
sTdxMinDataFolder = r"C:\zd_zszq\vipdoc\sh\minline"
# 生成的CSV新格式数据文件的根目录
sCSVFileFolder = r"./"

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


def Dispatch2CSVFiles( sCode, lstTdxMinRecords, sDispathFolder ):
    """
        将通达信的分钟线数据，根据市场、数据类型、代码、年份转存到对应的子目录和分文件中,如：
            路径格式：  ./XXX/SSE(SZSE)/MIN/000001/MIN000001_2918.csv

        sCode               商品代码
        lstTdxMinRecords    通达信分钟线数据列表
        sDispathFolder      数据分发转存文件根目录
    """
    sLastYear = ""                  # 记录上一次数据的年份
    objCSVFile = None               # CSV文件句柄

    ########### 循环遍历数据记录，按年份把分钟线数据转存到对应年份的文件中 ##########
    for lstMinData in lstTdxMinRecords:
        sDate = lstMinData[0]
        sYear = sDate[:4]             # 当前数据记录的年份

        ####### 根据年份判断是否需要新写一个文件（一个年份一个文件）#################
        if "" == sLastYear or sLastYear != sYear:
            sLastYear = sYear
            sCSVFullFolder = os.path.join( sDispathFolder, sMarketCode + "/MIN/" + sCode )
            sCSVFullFilePath = os.path.join( sCSVFullFolder, "MIN" + sCode + "_" + sYear + ".csv" )
            if False == os.path.exists( sCSVFullFolder ):
                os.makedirs( sCSVFullFolder )       # 递归创建目标目录(CSV)

            if None != objCSVFile:                  # 关闭已经写过的文件
                objCSVFile.close()

            objCSVFile = open( sCSVFullFilePath, 'w' )
            if None == sCSVFullFilePath:
                raise Exception( "Dispatch2CSVFiles : 打开CSV目标文件打败 : {path}".format( path=sCSVFullFilePath )  )
            ### 新建的CSV文件，先写入Min标题 ####
            objCSVFile.write( "date,time,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n" )

        ####### 将数据格式化后写入CSV文件中 #####################################3###
        objCSVFile.write( "{date},{time},{openpx:.4f},{highpx:.4f},{lowpx:.4f},{closepx:.4f},0.0000,{amount:.4f},{volume},0,0,0.0000\n".format( date=lstMinData[0], time=lstMinData[1], openpx=lstMinData[2], highpx=lstMinData[3], lowpx=lstMinData[4], closepx=lstMinData[5], amount=lstMinData[6], volume=lstMinData[7] ) )

    ########### 退出前关闭文件 ######################
    if None != objCSVFile:
        objCSVFile.close()






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
        nCount = 0
        lstTdxPath = os.listdir( sTdxMinDataFolder )
        for sTdxPath in lstTdxPath:
            if True == os.path.isdir( sTdxPath ):
                continue

            ### 读出每个文件中的1分钟线数据列表
            sTdxFullPath = os.path.join( sTdxMinDataFolder, sTdxPath )
            lstTdxMinRecords = ConvertTDXFile( sTdxFullPath )

            sCode = sTdxPath[2:8] 
            if int(sCode) <= 0:
                print( "从tdx路径中提取到无效的商品代码", sCode, sTdxPath )
                continue

            ### 将每个通达信源文件中的数据按年份归类分别转存 #####
            Dispatch2CSVFiles( sCode, lstTdxMinRecords, sCSVFileFolder )
            print( str(nCount) + " [完成] 转储 ---> ", sTdxFullPath )
            nCount += 1




