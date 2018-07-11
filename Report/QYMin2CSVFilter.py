"""

用途：将钱育转码机落盘文件(.CSV)将指定日期的数据部分过滤出来，方便同通达信的对应日期的数据的Diff比较

使用：
    通过python3调起脚本前，需要在[Configuration]章节中配置输入的通达信分钟线文件所在根目录

    python3 QYMin2CSVFilter.py

编写：    Barry


"""


import os, sys


############ Configuration #############################################################

### 必选参数
# 钱育某市场的分钟线所在目录
sQYMinCSVFolder = r"D:\ServiceManager\srvunit\DataTranscoding4QuoClientApi\data\SSE\MIN"
# 生成的CSV新格式数据文件的根目录
sCSVFileFolder = r"./"
# 待从CSV文件过滤出来的数据日期
nExtractDate = 20180711


############ Convertion Code ###########################################################

### 私有全局变量
sMarketCode = ""        # 市场编号
sMinTitle = "date,time,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n"

def Dispatch2CSVFiles( nDate, sMarketCode, sCode, sQYMinFilePath, sDispathFolder, nExtractDate ):
    """
        将钱育的分钟线数据文件(sQYMinFilePath)，根据指定的日期，提取转存到对应的根目录（sDispathFolder）

        sCode               商品代码
        sQYMinFilePath      钱育分钟线数据文件路径 
        sDispathFolder      数据分发转存文件根目录
        nExtractDate        需要过滤出来的数据的日期指定
    """
    objSrcCSVFile = None
    objDestCSVFile = None
    ########### 先过滤掉非指定的年份文件 #########################################
    sYear = str(int(nExtractDate/10000))
    if "MIN" + sCode + "_" + sYear not in sQYMinFilePath:
        return False
    ########### 循环遍历数据记录，按年份把分钟线数据转存到对应年份的文件中 ##########
    try:
        objSrcCSVFile = open( sQYMinFilePath )
        sOutputFolder = os.path.join( sDispathFolder, sMarketCode + "/MIN/" + sCode )
        sOutputFile = os.path.join( sOutputFolder, "MIN" + sCode + "_" + sYear + ".csv" )
        if False == os.path.exists( sOutputFolder ):
            os.makedirs( sOutputFolder )       # 递归创建目标目录(CSV)

        objDestCSVFile = open( sOutputFile, 'w' )
        objDestCSVFile.write( sMinTitle )
        ###### 过滤出需要年份的记录到文件 ##########
        lstAllData = objSrcCSVFile.readlines()[1:]
        for sMinData in lstAllData:
            if str(nExtractDate)+"," in sMinData:
                objDestCSVFile.write( sMinData )
    finally:
        if objSrcCSVFile:
            objSrcCSVFile.close()
        if objDestCSVFile:
            objDestCSVFile.close()

    return True



################# Entrance #############################################################

if __name__ == '__main__':

    sQYMinCSVFolder = sQYMinCSVFolder.replace( "\\", "/" )
    if False == os.path.exists( sQYMinCSVFolder ) :
        print( r"设定的钱育分钟线目录不存在：", sQYMinCSVFolder  )
    else:
        ########## 根据提供的数据目录路径确定所属市场 ##############
        if r"SSE/MIN" in sQYMinCSVFolder:
            sMarketCode = "SSE"
        elif r"SZSE/MIN" in sQYMinCSVFolder:
            sMarketCode = "SZSE"
        else:
            print( "指定的钱育数据源路径中，不包含市场归属信息!" )
            sys.exit( 1024 )

        ########## 遍历数据目录下的所有文件 & 转存 #################
        nCount = 0
        lstQYFolderPath = os.listdir( sQYMinCSVFolder )
        ########## 先遍历第1层目录
        for sQYPath in lstQYFolderPath:
            sQYFullMinPath = os.path.join( sQYMinCSVFolder, sQYPath )

            if True == os.path.isdir( sQYFullMinPath ):
                sCode = sQYPath
                nCode = int(sCode)
                if nCode <= 0:
                    print( "从钱育文件的路径中提取到无效的商品代码", sCode, sQYPath )
                    continue

                bDo = False
                if "SSE" == sMarketCode:
                    if (nCode > 0 and nCode < 999) or (nCode>=600000 and nCode<=609999) or (nCode>=510000 and nCode<=519999):
                        bDo = True
                elif "SZSE" == sMarketCode:
                    if (nCode >= 399000 and nCode < 399999) or (nCode>=1 and nCode<=9999) or (nCode>=159000 and nCode<=159999) or (nCode>=300000 and nCode<=300999):
                        bDo = True

                if False == bDo:
                    continue

                ########## 再遍历第2层目录
                lstQYFilePath = os.listdir( sQYFullMinPath )
                for sQYFilePath in lstQYFilePath:
                    sQYFullMinFilePath = os.path.join( sQYFullMinPath, sQYFilePath )
                    if not os.path.isdir( sQYFullMinFilePath ):
                        ### 将每个通达信源文件中的数据按年份归类分别转存 #####
                        if True == Dispatch2CSVFiles( nExtractDate, sMarketCode, sCode, sQYFullMinFilePath, sCSVFileFolder, nExtractDate ):
                            print( str(nCount) + " [完成] 提取 ---> ", sQYFullMinFilePath )
                            nCount += 1





