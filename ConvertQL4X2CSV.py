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


class CSVSaver:
    """
        CSV文件格式数据转储类
    """
    def __init__( self, sFilePath ):
        """
            sFilePath   目标CSV文件路径
        """
        self.__sFilePath = sFilePath

class SHSZL1DayLineSaver(CSVSaver):
    """
        沪深L1日线数据保存CSV模块
    """
    def __init__( self, nMarketNo,  sFilePath ):
        """
            nMarketNo   市场编号: 0上海 1深圳
        """
        CSVSaver.__init__( self, sFilePath )
        self.__nMarketNo = nMarketNo

    def Save2CSV( self, sCode, nDate, nTime, dOpen, dHigh, dLow, dClose, dAmount, nVolume, nTradeNumber, dVoip ):
        print( nDate, nTime, dOpen, dHigh, dLow, dClose, dAmount, nVolume, nTradeNumber, dVoip )


class QL4XDataPump:
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

    def __init__( self, objFile, bIsIndex, objCallBackInterface, sCode ):
        """
            构造函数
            objFile                     是待加载的文件
            bIsIndex                    是否是指数数据(true是指数)
            objCallBackInterface        数据转存CSV回调接口
            sCode                       商品代码字符串
        """
        self.__sCode = sCode                                # 商品代码
        self.__bIsIndex = bIsIndex                          # 是指数类型
        self.__fileHandle = objFile                         # 文件句柄
        self.__nDataType = 0                                # 用以标识是日线、还是分钟线等 ( 0, 代表未赋值 )
        self.__nDataOffset = 0                              # 文件头偏移量
        self.__nBlockSize = 0                               # 每个文件块结构大小
        self.__objCallBackInterface = objCallBackInterface  # 原始数据转存CSV文件的回调接口

    def Pump( self ):
        """
            根据文件名的规模，对不同数据结构指定对应的解析函数
        """
        ### 解析文件头 ########################
        if False == self.__LoadHeaderOfFile(): 
            return False
        ### 判断数据类型是否合法 ###############
        if self.__nDataType != QL4XDataPump.T_DAY_LINE and self.__nDataType != QL4XDataPump.T_1MIN_LINE:
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
        bytesQLHisFileHead = self.__fileHandle.read( struct.calcsize(QL4XDataPump.FMT_Unpack_STR) )
        if not bytesQLHisFileHead:
            print( "QL4XDataPump::__LoadHeaderOfFile() : cannot read header struct from file" )
            return False

        ulImagic, self.__nDataType, ulFileVer, self.__nBlockSize, __nDataOffset = struct.unpack( QL4XDataPump.FMT_Unpack_STR, bytesQLHisFileHead )
        if 0x46484c51 != ulImagic:
            print( "QL4XDataPump::__LoadHeaderOfFile() : Invalid Image ID : %x " % ulImagic )
            return False

        if 0x640001 != ulFileVer:
            print( "QL4XDataPump::__LoadHeaderOfFile() : Invalid File Version : %x " % ulFileVer )
            return False

        if self.__nDataType != QL4XDataPump.T_DAY_LINE and self.__nDataType != QL4XDataPump.T_1MIN_LINE:
            print( "QL4XDataPump::__LoadHeaderOfFile() : Invalid Data Identifier : %d " % self.__nDataType )
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
            bytesQLTime = self.__fileHandle.read( struct.calcsize(QL4XDataPump.FMT_UnpackTime_STR) )
            if not bytesQLTime:
                return True
            nObjTime, = struct.unpack( QL4XDataPump.FMT_UnpackTime_STR, bytesQLTime )
            nYear = nObjTime >> 20
            nMonth = (nObjTime & 0b00000000000011110000000000000000) >> 16
            nDay = (nObjTime &   0b00000000000000001111100000000000) >> 11
            nHour = (nObjTime &  0b00000000000000000000011111000000) >> 6
            nMinute = nObjTime & 0b00000000000000000000000000111111
            nDate = ((nYear*10000) + (nMonth*100) + nDay);
            nTime = nHour * 10000 + nMinute * 100
            ### 解析数据体行情部分 ###############################
            bytesQLData = self.__fileHandle.read( struct.calcsize(QL4XDataPump.FMT_UnpackData_STR) )
            if not bytesQLData:
                print( "QL4XDataPump::__LoadHeaderOfFile() : cannot read data struct from file" )
                return False

            dOpen, dHigh, dLow, dClose, dAmount, nVolume = struct.unpack( QL4XDataPump.FMT_UnpackData_STR, bytesQLData )
            dVoip = 0.          # 基金模拟净值
            nTradeNumber = 0    # 今日成交笔数
            dOpen /= 1000.      # 开盘价
            dHigh /= 1000.      # 最高价
            dLow /= 1000.       # 最低价
            dClose /= 1000.     # 收盘价
            #print( dOpen, dHigh, dLow, dClose, dAmount, nVolume )
            ### 解析股票扩展数据 (成交笔数、模净、流通股本) #########
            bytesQLExtension = self.__fileHandle.read( struct.calcsize(QL4XDataPump.FMT_UnpackExt_STR) )
            if not bytesQLExtension:
                print( "QL4XDataPump::__LoadHeaderOfFile() : cannot read extension struct from file" )
                return False
            if False == self.__bIsIndex:
                nTradeNumber, dVoip, _ = struct.unpack( QL4XDataPump.FMT_UnpackExt_STR, bytesQLExtension )
                dVoip /= 1000.
            ### 回调数据给数据转存CSV接口 #########################
            self.__objCallBackInterface.Save2CSV( self.__sCode, nDate, nTime, dOpen, dHigh, dLow, dClose, dAmount, nVolume, nTradeNumber, dVoip )


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

    def Format2CSV():
        pass


if __name__ == '__main__':
    try:
        print( r"--------------------- [COMMENCE] ---------------------------" )
        ### 从参数取解析文件路径+参数
        opts, args = getopt.getopt( sys.argv[1:], "s:d:", ["src=","dest="] )
        print( opts, args )

        lstFilePath = [value for op, value in opts if op in ( "-f", "--file" )]
        sFilePath = "" if (len(lstFilePath) == 0) else lstFilePath[0]
        f = open( sFilePath, 'rb' )
        if None == f:
            raise Exception( "Failed 2 open data file : {path}".format( path=sFilePath )  )

        ### 开始解析
        oTargetFileSaver = SHSZL1DayLineSaver( MKID_SHL1, ""  )
        oRawDataReader = QL4XDataPump( f, True, oTargetFileSaver, "" )
        oRawDataReader.Pump()

    except Exception as e:
        print( e )
    finally:
        print( r"--------------------- [DONE] ---------------------------" )






