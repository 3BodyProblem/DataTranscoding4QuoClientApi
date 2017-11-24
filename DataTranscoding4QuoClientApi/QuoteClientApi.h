#ifndef  _QUOTECLIENT_APIDEFINE_H
#define  _QUOTECLIENT_APIDEFINE_H

#include "QuoteCltDef.h"

#ifdef QUOTECLIENTAPI_EXPORTS
#define QUOTECLIENTAPI_API __declspec(dllexport)
#else
#define QUOTECLIENTAPI_API __declspec(dllimport)
#endif

#define STDCALL	__stdcall
#define CTYPENAMEFUNC	extern "C"




//回调通知函数
class QuoteClientSpi
{
public:
	/*
	* 状态变化
	* @cMarket 市场ID
	* @nStatus 市场的状态变化, 见文档定义
	* 					
	*/
	virtual bool STDCALL		XDF_OnRspStatusChanged(unsigned char cMarket, int nStatus)=0;
	
	/*
	* 接收到数据
	* @pHead 数据头
	* pszbuf = 数据块(EzMsg) + 数据块(EzMsg) ......
	* 数据块(EzMsg) = [XDFAPI_MsgHead + 结构体内容] MsgHead定义见QuoteCltDef.h
	*/
	virtual void STDCALL		XDF_OnRspRecvData(XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes)=0;
	
	/*
	* 日志输出
	* @nLogType		0=流水日志、1=信息日志、2=警告日志、3=错误日志
	* @nLogLevel	(0-7)
	* @pLogBuf		输入日志的内容
	*/
	virtual void STDCALL		XDF_OnRspOutLog(unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf)=0;
	
	/*
	* 其他通知，扩展接口 
	* @nNotifyNo	通知号
	* @wParam		WPARAM参数
	* @lParam		LPARAM参数
	*/
	virtual int	STDCALL			XDF_OnRspNotify(unsigned int nNotifyNo, void* wParam, void* lParam )=0;
	
};





class QuoteClientApi
{
public:
	/*
	* 初始化接口对象
	* @return >0 is ok
	*/
	virtual int STDCALL			Init() = 0;
	
	/*
	* 删除接口对象本身
	*/
	virtual void STDCALL		Release() = 0;
	
	/*
	* @注册回调接口
	* @pspi  [in]派生自回调接口类的实例
	*/
	virtual void STDCALL		RegisterSpi(QuoteClientSpi * pspi) = 0;
	
	/*
	* 开始工作
	* @return (>0) mean ok, (<=0) error
	*/
	virtual int STDCALL			BeginWork() = 0;
	
	/*
	* 终止工作
	*/
	virtual void STDCALL		EndWork() = 0;
	
	/*
	* 获取市场分类信息(包括分类，放大倍数等)
	*/
	virtual int	 STDCALL		GetMarketInfo(unsigned char cMarket, char* pszInBuf, int nInBytes) =0;

	/*
	* 获取名称代码表
	* (1)NULL= pszInBuf		返回nCount代表总个数
	* (2)NULL != pszBuf		拷贝min(nCount,nMsg)个数,除非nInBytes不满足
	* @cMarket				[in]XDFMarket取值
	* @pszInBuf/nInBytes	[in]
	* @nCount				[in out]
	* @return (>0) mean ok,it is output bytes, (<=0) error
	* 返回的pszInBuf, 格式为   数据块(EzMsg) + 数据块(EzMsg) ......
	*/
	virtual int	STDCALL			GetCodeTable(unsigned char cMarket, char* pszInBuf, int nInBytes, int& nCount) = 0;
	
	/*
	* 获取最新行情
	* @cMarket				[in]XDFMarket取值
	* @pCode				[in]Code
	* @pszInBuf				[in]字符缓冲区, 返回的格式为   数据块(EzMsg) + 数据块(EzMsg) ......
	* @nInBytes				[in]缓冲区大小
	* @return (>0) mean ok,it is output bytes (<=0) error
	*/
	virtual int STDCALL			GetLastMarketDataAll(unsigned char cMarket, char* pszInBuf, int nInBytes) = 0;
	
	/*
	* 监控用，监控某一市场的流量/市场时间
	* @cMarket				[in]XDFMarket取值
	* @nStatus				[in out]当前市场的Status(见文档)
	* @ulTime				[in out]当前的市场时间
	* @pI64Send				[in out]发送的字节数
	* @pI64Recv				[in out]接受的字节数
	*/
	virtual int STDCALL			GetMarketStatus(unsigned char cMarket,int& nStatus, unsigned int& ulTime, __int64 * pI64Send, __int64 * pI64Recv)=0;
	
protected:
	//virtual STDCALL ~QuoteClientApi(){};
	
	
};





/*
* @pszDebugPath  默认值为NULL，保留[调试接口]
* @return 对象实例
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API QuoteClientApi * STDCALL CreateQuoteApi(const char *pszDebugPath);


/*
* @nMajorVersion  主版本
* @nMinorVersion  子版本
* @return V1.00 B021格式的字符串
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API const char * STDCALL GetDllVersion(int &nMajorVersion, int &nMinorVersion);


#pragma pack(1)

typedef struct  
{
	unsigned	char			cMarketID;				//市场ID
	char						cMarketChn[63];			//描述文字	
	char						cAddress[128];			//连接地址和端口
	int							nStatus;				//当前状态//0=不可用 //1=未知//2=未就绪//5=可用
}tagQuoteSettingInfo;											 
																  
#pragma pack()													  
																  
/*
* 获取行情配置的信息
* @pArrMarket, 如果为NULL， 返回个数；如果非NULL，填入值
* @nCount, 数量
* @return 实际市场个数
//CTYPENAMEFUNC QUOTECLIENTAPI_API int 	STDCALL GetSettingInfo(tagQuoteSettingInfo* pArrMarket, int nCount);
*/




class QuotePrimeApi
{
public:
	/*
	* 扩充接口
	* @FuncNo		功能号
	* @wParam		参数1
	* @lParam		参数2
	*/
	virtual int		STDCALL		ReqFuncData(int FuncNo, void* wParam, void* lParam) =0;

protected:
	//virtual ~QuotePrimeApi();

};

/*
* 获取补充信息
* @
* @return PrimeApi指针
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API QuotePrimeApi * STDCALL CreatePrimeApi();


#endif