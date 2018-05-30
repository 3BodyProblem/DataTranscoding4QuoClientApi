# -*- coding: utf-8 -*-
"""
	@summary:
		钱龙4x转码机落盘文件转钱育转码机的CSV落盘文件格式
	@version:
		1.0.1
	@date:
		2018/5/28
    @enviroment:
        需要使用 python3.x 以上的版本, 无需安装其他组件
	@usage:
		python3 ConvertQL4XData2CSV.py -f 000001.d01
"""


import struct, getopt, time, sys, os


MKID_SHL1 = 0           ### 上海L1市场编号
MKID_SZL1 = 1           ### 深圳L1市场编号


################### 基础功能封装 ##################################
def JudgeIndexType( nMarketNo, sCode ):
    """
        根据市场编号 和 商品代码判断是否为指数
        nMarketNo       市场编号
        sCode           商品代码
    """
    bIsIndex = False

    if MKID_SHL1 == nMarketNo:
        if sCode.find( "000" ) == 0:
            bIsIndex = True
    elif  MKID_SZL1 == nMarketNo:
        if sCode.find( "399" ) == 0:
            bIsIndex = True

    return bIsIndex


class CSVSaver:
    """
        CSV文件格式数据转储类
    """
    def __init__( self, sFilePath, sTitle ):
        """
            sFilePath   目标CSV文件路径
        """
        self.__sFilePath = sFilePath
        self.__fileHandle = None
        self.__sTitle = sTitle

    def Open( self ):
        """
            打开要写的目标文件(CSV)
        """
        if None == self.__fileHandle:
            if False == os.path.exists( os.path.split(self.__sFilePath)[0] ):
                os.makedirs( os.path.split(self.__sFilePath)[0] )      # 递归创建目标目录(CSV)
            self.__fileHandle = open( self.__sFilePath, 'w' )          # 文件句柄
            if None == self.__fileHandle:
                raise Exception( "CSVSaver::Open : cannot open destination file : {path}".format( path=self.__sFilePath )  )
            self.WriteString( self.__sTitle )                          # 写入标题行

    def Close( self ):
        """
            关闭目标文件(CSV)
        """
        if None != self.__fileHandle:
            self.__fileHandle.close()

    def WriteString( self, sRecord ):
        """
            将csv记录字符串写入文件
        """
        if None != self.__fileHandle:
            self.__fileHandle.write( sRecord )


################### CSV 文件转存策略类 ##################################
class SHSZL1DayLineSaver(CSVSaver):
    """
        沪深L1日线数据保存CSV模块
    """
    def __init__( self, nMarketNo,  sFilePath ):
        """
            nMarketNo   市场编号: 0上海 1深圳
        """
        CSVSaver.__init__( self, sFilePath, "date,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n" )
        self.__nMarketNo = nMarketNo

    def Save2CSV( self, sCode, nDate, nTime, dOpen, dHigh, dLow, dClose, dAmount, nVolume, nTradeNumber, dVoip ):
        self.Open()
        sRecord = "{date},{openpx:.4f},{highpx:.4f},{lowpx:.4f},{closepx:.4f},0.0000,{amount:.4f},{volume},0,{numtrades},{voip:.4f}\n".format( date = nDate, openpx = dOpen, highpx = dHigh, lowpx = dLow, closepx = dClose, amount = dAmount, volume = nVolume, numtrades = nTradeNumber, voip = dVoip )
        self.WriteString( sRecord )


################### 钱龙转码机原始文件加类 ################################
class QL4XFilePump:
    """
        钱龙4x转码机落盘文件数据解析泵，[***注：仅支持 日线、1分钟线***]
        补充： 日线、1分钟线 的落盘数据格式是统一的!
    """
    T_DAY_LINE = (ord('y') << 8 * 3) | (ord('a') << 8 * 2) | (ord( 'd' ) << 8) | ord( 'k' )         # 日线类型值
    T_1MIN_LINE = (ord('1') << 8 * 3) | (ord('n') << 8 * 2) | (ord( 'm' ) << 8) | ord( 'k' )        # 1分钟线类型值
    FMT_Unpack_STR = r"<IIIHH"                                                                      # 数据头：文件头解析格式串
    FMT_UnpackTime_STR = r"<I"                                                                      # 数据体：时间头解析格式串
    FMT_UnpackData_STR = r"<IIIIdQ"                                                                 # 数据体：价格部分解析格式串
    FMT_UnpackExt_STR = r"<III"                                                                     # 数据体：扩展部分解析格式串

    def __init__( self, nMarketNo, sSrcFile, objCallBackInterface, sCode ):
        """
            构造函数
            nMarketNo                   市场编号
            sSrcFile                    待加载的文件的路径
            objCallBackInterface        数据转存CSV回调接口
            sCode                       商品代码字符串
        """
        self.__sSrcFile = sSrcFile                          # 数据原始文件路径
        self.__nMarketNo = nMarketNo                        # 市场编号
        self.__sCode = sCode                                # 商品代码
        self.__bIsIndex = JudgeIndexType( nMarketNo, sCode )# 是指数类型
        self.__nDataType = 0                                # 用以标识是日线、还是分钟线等 ( 0, 代表未赋值 )
        self.__nDataOffset = 0                              # 文件头偏移量
        self.__nBlockSize = 0                               # 每个文件块结构大小
        self.__objCallBackInterface = objCallBackInterface  # 原始数据转存CSV文件的回调接口
        self.__fileHandle = open( sSrcFile, 'rb' )          # 文件句柄
        if None == self.__fileHandle:
            raise Exception( "QL4XFilePump::__init()__ : cannot open source file : {path}".format( path=sSrcFile )  )

    def Close( self ):
        """
            释放资源
        """
        self.__fileHandle.close()

    def Pump( self ):
        """
            根据文件名的规模，对不同数据结构指定对应的解析函数
        """
        ### 解析文件头 ########################
        if False == self.__LoadHeaderOfFile(): 
            return False
        ### 判断数据类型是否合法 ###############
        if self.__nDataType != QL4XFilePump.T_DAY_LINE and self.__nDataType != QL4XFilePump.T_1MIN_LINE:
            return False
        ### 解析数据体 ########################
        return self.__PumpDataBody()

    def __LoadHeaderOfFile( self ):
        """
            加载文件头结构,C++结构定义如下：
            typedef struct {
                unsigned long	ulImagic;		//文件标识
                unsigned long	ulIdenty;		//文件用途，kmn1, km15, kday, kmon
                unsigned long	ulFileVer;		//文件版本
                unsigned short	usBlockSz;		//每个文件块结构大小
                unsigned short	usOffset;		//文件内容开始偏移
            } tagQLHisFileHead;
        """
        bytesQLHisFileHead = self.__fileHandle.read( struct.calcsize(QL4XFilePump.FMT_Unpack_STR) )
        if not bytesQLHisFileHead:
            print( "QL4XFilePump::__LoadHeaderOfFile() : cannot read header struct from file : ", self.__sSrcFile )
            return False

        ulImagic, self.__nDataType, ulFileVer, self.__nBlockSize, __nDataOffset = struct.unpack( QL4XFilePump.FMT_Unpack_STR, bytesQLHisFileHead )
        if 0x46484c51 != ulImagic:
            print( "QL4XFilePump::__LoadHeaderOfFile() : Invalid Image ID : %x, File : %s " % (ulImagic, self.__sSrcFile) )
            return False

        if 0x640001 != ulFileVer:
            print( "QL4XFilePump::__LoadHeaderOfFile() : Invalid File Version : %x, File : %s " % (ulFileVer, self.__sSrcFile) )
            return False

        if self.__nDataType != QL4XFilePump.T_DAY_LINE and self.__nDataType != QL4XFilePump.T_1MIN_LINE:
            print( "QL4XFilePump::__LoadHeaderOfFile() : Invalid Data Identifier : %d, File : %s" % (self.__nDataType, self.__sSrcFile) )
            return False

        return True

    def __PumpDataBody( self ):
        """
            解析 && 遍历数据体结构 && 回调数据内容！  C++原始结构定义如下：
            typedef struct {                //通用Ql时间格式(4字节)
                unsigned long               Minute  : 6;        //分[0~59]
                unsigned long               Hour    : 5;        //时[0~23]
                unsigned long               Day     : 5;        //日[0~31]
                unsigned long               Month   : 4;        //月[0~12]
                unsigned long               Year    : 12;       //年[0~4095]
            } tagQlDateTime;
            typedef struct {
                tagQlDateTime               stLondate;      // 日期，LONDATE形式
                unsigned long               ulOpen;         // 今日开盘价，放大1000倍
                unsigned long               ulHigh;         // 今日最高价，放大1000倍
                unsigned long               ulLow;          // 今日最低价，放大1000倍
                unsigned long               ulClose;        // 今日收盘价，放大1000倍
                double                      dAmount;        // 今日成交额，元
                unsigned __int64            uiVolume;       // 今日成交量，股
                union {
                    struct {                //指数数据
                        short               sSec5;          // 指标
                        unsigned short      usRaise;        // 大盘上涨家数
                        unsigned short      usDown;         // 大盘下跌家数
                        short               sRDDiff;        // 指标
                    };
                    struct {                //个股数据
                        unsigned long       ulRecord;       // 今日成交笔数
                        unsigned long       ulOtherData;    // 国债利息、基金净值、模净等
                        unsigned long       ulBase;         // 个股流通股本，万股
                    };
                };
            } tagHqFile_HisData;
        """
        while True:
            ### 解析数据体的时间 #################################
            bytesQLTime = self.__fileHandle.read( struct.calcsize(QL4XFilePump.FMT_UnpackTime_STR) )
            if not bytesQLTime:
                return True
            nObjTime, = struct.unpack( QL4XFilePump.FMT_UnpackTime_STR, bytesQLTime )
            nYear = nObjTime >> 20
            nMonth = (nObjTime & 0b00000000000011110000000000000000) >> 16
            nDay = (nObjTime &   0b00000000000000001111100000000000) >> 11
            nHour = (nObjTime &  0b00000000000000000000011111000000) >> 6
            nMinute = nObjTime & 0b00000000000000000000000000111111
            nDate = ((nYear*10000) + (nMonth*100) + nDay);
            nTime = nHour * 10000 + nMinute * 100
            ### 解析数据体行情部分 ###############################
            bytesQLData = self.__fileHandle.read( struct.calcsize(QL4XFilePump.FMT_UnpackData_STR) )
            if not bytesQLData:
                print( "QL4XFilePump::__LoadHeaderOfFile() : cannot read data struct from file : ", self.__sSrcFile )
                return False

            dOpen, dHigh, dLow, dClose, dAmount, nVolume = struct.unpack( QL4XFilePump.FMT_UnpackData_STR, bytesQLData )
            dVoip = 0.          # 基金模拟净值
            nTradeNumber = 0    # 今日成交笔数
            dOpen /= 1000.      # 开盘价
            dHigh /= 1000.      # 最高价
            dLow /= 1000.       # 最低价
            dClose /= 1000.     # 收盘价
            #print( dOpen, dHigh, dLow, dClose, dAmount, nVolume )
            ### 解析股票扩展数据 (成交笔数、模净、流通股本) #########
            bytesQLExtension = self.__fileHandle.read( struct.calcsize(QL4XFilePump.FMT_UnpackExt_STR) )
            if not bytesQLExtension:
                print( "QL4XFilePump::__LoadHeaderOfFile() : cannot read extension struct from file : ", self.__sSrcFile )
                return False
            if False == self.__bIsIndex:
                nTradeNumber, dVoip, _ = struct.unpack( QL4XFilePump.FMT_UnpackExt_STR, bytesQLExtension )
                dVoip /= 1000.
            ### 回调数据给数据转存CSV接口 #########################
            self.__objCallBackInterface.Save2CSV( self.__sCode, nDate, nTime, dOpen, dHigh, dLow, dClose, dAmount, nVolume, nTradeNumber, dVoip )


################### 目录递归读取并转换控制类 #############################
class Conversion:
    """
        数据格式转换器
        钱龙4x转码机数据(指定源数据Input目录) ---> 钱育CSV转码机落盘格式(指定目标数据Output目录)
    """
    def __init__( self, sSourceFolder, sTargetFolder ):
        """
            构造函数
            sSourceFolder               钱龙4x转码机输入文件所在根目录
            sTargetFolder               钱育CSV转码机输出文件所在根目录
        """
        self.__sSourceFolder = sSourceFolder
        self.__sTargetFolder = sTargetFolder

    def Do( self ):
        """
            1. 递归遍历钱龙4x转码机的源目录
            2. 对找到的合法文件进行格式转换
            3. 转储转换后的数据到钱育CSV文件目录
        """
        ### 先递归遍历数据源目录 ###########################
        for lstSrcPath in os.walk( self.__sSourceFolder ):
            sRootFolder = lstSrcPath[0]
            if "shase" in sRootFolder and "kday" in sRootFolder:              # 上海L1市场(日线)
                sCSVFolder = os.path.join(self.__sTargetFolder, "SSE/DAY/" )
                for lstFiles in lstSrcPath[1:]:
                    for sFile in lstFiles:
                        sTmpPath, sTmpFileName = os.path.split( sFile )
                        sCode, sExtName = os.path.splitext( sTmpFileName )
                        sMainFileName = "DAY" + sCode + ".csv"
                        self.__FormatDAYLine2CSV( MKID_SHL1, sCode, os.path.join(sRootFolder, sFile), os.path.join(sCSVFolder, sMainFileName) )
            elif "sznse" in sRootFolder and "kday" in sRootFolder:            # 深圳L1市场(日线)
                sCSVFolder = os.path.join(self.__sTargetFolder, "SZSE/DAY/" )
                for lstFiles in lstSrcPath[1:]:
                    for sFile in lstFiles:
                        sTmpPath, sTmpFileName = os.path.split( sFile )
                        sCode, sExtName = os.path.splitext( sTmpFileName )
                        sMainFileName = "DAY" + sCode + ".csv"
                        self.__FormatDAYLine2CSV( MKID_SZL1, sCode, os.path.join(sRootFolder, sFile), os.path.join(sCSVFolder, sMainFileName) )
            elif "shase" in sRootFolder and "kmn1" in sRootFolder:            # 上海L1市场(1分钟线)
                sCSVFolder = os.path.join(self.__sTargetFolder, "SSE/MIN/" )
                for lstFiles in lstSrcPath[1:]:
                    for sFile in lstFiles:
                        sTmpPath, sTmpFileName = os.path.split( sFile )
                        sCode, sExtName = os.path.splitext( sTmpFileName )
                        sCSVPath = os.path.join(sCSVFolder, sCode)
                        sCSVPath = os.path.join(sCSVPath, "MIN" + sCode + "_")
                        self.__Format1MINLine2CSV( MKID_SHL1, sCode, os.path.join(sRootFolder, sFile), sCSVPath )
            elif "sznse" in sRootFolder and "kmn1" in sRootFolder:            # 深圳L1市场(1分钟线)
                sCSVFolder = os.path.join(self.__sTargetFolder, "SZSE/MIN/" )
                for lstFiles in lstSrcPath[1:]:
                    for sFile in lstFiles:
                        sTmpPath, sTmpFileName = os.path.split( sFile )
                        sCode, sExtName = os.path.splitext( sTmpFileName )
                        sCSVPath = os.path.join(sCSVFolder, sCode)
                        sCSVPath = os.path.join(sCSVPath, "MIN" + sCode + "_")
                        self.__Format1MINLine2CSV( MKID_SZL1, sCode, os.path.join(sRootFolder, sFile), sCSVPath )

    def __FormatDAYLine2CSV( self, nMarketNo, sCode, sSrcFile, sDestFile ):
        """
            将每个日线原始文件重新格式化后输出到新的CSV格式的目标目录中
            nMarketNo               市场编号
            sSrcFile                数据源文件路径
            sDestFile               目标文件路径(CSV)
        """
        ### 开始转换原始文件数据到CSV文件
        oTargetFileSaver = SHSZL1DayLineSaver( MKID_SHL1, sDestFile  )
        oRawFilePump = QL4XFilePump( MKID_SHL1, sSrcFile, oTargetFileSaver, sCode )
        oRawFilePump.Pump()
        oRawFilePump.Close()
        oTargetFileSaver.Close()

    def __Format1MINLine2CSV( self, nMarketNo, sCode, sSrcFile, sDestFile ):
        """
            将每一个一分钟线源文件数据分派出多个Folder/SZSE/MIN/000001/MIN000001_YYYY.csv
            其中 sDestFile 的形式为 Folder/MARKETSTRING/MIN/CODENUMBER/MINnnnnnn_
            需要在根据文件年份重新拼接生成Folder/SZSE/MIN/000001/MIN000001_2018.csv这样路径
            nMarketNo               市场编号
            sSrcFile                数据源文件路径
            sDestFile               目标文件基础路径(CSV)
        """
        #print( sSrcFile, sDestFile )




################# Entrance #############################################################
if __name__ == '__main__':
    print( 'usage:  python3 ConvertQL4X2CSV.py --src=/srcroot/ --dest=/destroot/\n\n\n' )
    print( r"--------------------- [COMMENCE] ------------------------" )
    ### 先获取所有的命令行输入
    opts, _ = getopt.getopt( sys.argv[1:], "s:d:", ["src=","dest="] )
    lstSrcFolder = [value for op, value in opts if op in ( "-s", "--src" )]
    lstDestFolder = [value for op, value in opts if op in ( "-d", "--dest" )]

    ### 再从参数取解析文件路径(I/O)
    sSrcFolder = "./QL4X/" if (len(lstSrcFolder) == 0) else lstSrcFolder[0]     ### 数据源设定
    sDestFolder = "./CSV/" if (len(lstDestFolder) == 0) else lstDestFolder[0]   ### 目标目录设定
    print( r"*** NOTE! Conversion: [{QL4XData}] ---> [{QYCSV}]".format( QL4XData = sSrcFolder, QYCSV = sDestFolder ) )

    ### 最后开始将源数据转换到目标位置
    objConversion = Conversion( sSourceFolder = sSrcFolder, sTargetFolder = sDestFolder )
    objConversion.Do()

    print( r"--------------------- [COMPLETE] ------------------------" )






