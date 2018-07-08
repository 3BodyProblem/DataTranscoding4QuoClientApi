"""

用途：按指定的日期，依次读取钱育转码机1分钟线的每条记录，并从对应的TICK文件中获取相应分钟内的TICK数据，以验证该条1分钟线的正确性

使用：
    通过python3调起脚本前，需要在[Configuration]章节中配置输入的通达信分钟线文件所在根目录

    python3 QYCheckMin1ByDay1.py

编写：    Barry


"""


import os, sys


############ Configuration #############################################################

### 必选参数
# 钱育某市场的1分钟线所在目录
sQYMin1CSVFolder = r"D:\ServiceManager\srvunit\DataTranscoding4QuoClientApi\data\SSE\MIN"
# 钱育某市场的TICK线所在目录
sQYTickCSVFolder = r"D:\ServiceManager\srvunit\DataTranscoding4QuoClientApi\data\SSE\TICK"
# 待从CSV文件过滤出来的数据日期
nExtractDate = 20180705


############ Convertion Code ###########################################################

### 私有全局变量
sMarketCode = ""        # 市场编号


def CheckMin1RecordWithTickFile( sMinData, lstAllTickData, nBeginOffset ):
    """
        用对应的TICK线数据来验证一条1分钟线数据

        sMinData            1分钟线数据
        lstAllTickData      指定TICK内的所有的tick线
        nBeginOffset        读取TICK线的开始位置索引

        返回，1分钟线是否正确 + 读到的TICK线的最后位置
    """
    return bool, 0


def CheckMin1CSVFile( sMarketCode, sCode, nExtractDate, sQYMinFilePath, sQYTickCSVFolder ):
    """
        将钱育的分钟线数据文件(sQYMinFilePath)逐每到对应的TICK文件中，计算验证正确性

        sMarketCode         市场编号
        sCode               商品代码
        nExtractDate        需要过滤出来的数据的日期指定
        sQYMinFilePath      钱育分钟线数据文件路径 
        sQYTickCSVFolder    钱育日线数据文件路径

    """
    objMinCSVFile = None
    objTickCSVFile = None
    nTickDataIndex  = 0
    ########### 先过滤掉非指定的年份文件 #########################################
    sYear = str(int(nExtractDate/10000))
    if "MIN" + sCode + "_" + sYear not in sQYMinFilePath:
        return False

    try:
        ###### 打开分钟线和对应的TICK文件 ###############
        objMinCSVFile = open( sQYMinFilePath )
        sTickFile = os.path.join( sQYTickCSVFolder, sMarketCode + "/TICK/" + sCode + '/' + str(nExtractDate) + '/' )
        sTickFile = os.path.join( sTickFile, "TICK" + sCode + "_" + str(nExtractDate) + ".csv" )
        objTickCSVFile = open( sTickFile, 'w' )
        ###### 循环，过滤出需要年份的记录来验证 ##########
        lstAllMin1Data = objMinCSVFile.readlines()[1:]
        lstAllTickData = objTickCSVFile.readlines()[1:]
        for sMinData in lstAllMin1Data:
            if str(nExtractDate)+"," in sMinData:
                bOK, nTickDataIndex = CheckMin1RecordWithTickFile(sMinData, lstAllTickData, nTickDataIndex)

                if False == bOK:
                    raise Exception( "Invalid Min1 Record! " + sQYMinFilePath )
    finally:
        if objMinCSVFile:
            objMinCSVFile.close()
        if objTickCSVFile:
            objTickCSVFile.close()

    return True



################# Entrance #############################################################

if __name__ == '__main__':

    sQYMin1CSVFolder = sQYMin1CSVFolder.replace( "\\", "/" )
    if False == os.path.exists( sQYMin1CSVFolder ) :
        print( r"设定的钱育分钟线目录不存在：", sQYMin1CSVFolder  )
    else:
        ########## 根据提供的数据目录路径确定所属市场 ##############
        if r"SSE/MIN" in sQYMin1CSVFolder:
            sMarketCode = "SSE"
        elif r"SZSE/MIN" in sQYMin1CSVFolder:
            sMarketCode = "SZSE"
        else:
            print( "指定的钱育数据源路径中，不包含市场归属信息!" )
            sys.exit( 1024 )

        ########## 遍历数据目录下的所有文件 & 转存 #################
        nCount = 0
        lstQYFolderPath = os.listdir( sQYMin1CSVFolder )
        ########## 先遍历第1层目录
        for sQYPath in lstQYFolderPath:
            sQYFullMinPath = os.path.join( sQYMin1CSVFolder, sQYPath )

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
                        if True == CheckMin1CSVFile( sMarketCode, sCode, nExtractDate, sQYFullMinFilePath, sQYTickCSVFolder ):
                            print( str(nCount) + " [完成] 提取 ---> ", sQYFullMinFilePath )
                            nCount += 1





