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




//�ص�֪ͨ����
class QuoteClientSpi
{
public:
	/*
	* ״̬�仯
	* @cMarket �г�ID
	* @nStatus �г���״̬�仯, ���ĵ�����
	* 					
	*/
	virtual bool STDCALL		XDF_OnRspStatusChanged(unsigned char cMarket, int nStatus)=0;
	
	/*
	* ���յ�����
	* @pHead ����ͷ
	* pszbuf = ���ݿ�(EzMsg) + ���ݿ�(EzMsg) ......
	* ���ݿ�(EzMsg) = [XDFAPI_MsgHead + �ṹ������] MsgHead�����QuoteCltDef.h
	*/
	virtual void STDCALL		XDF_OnRspRecvData(XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes)=0;
	
	/*
	* ��־���
	* @nLogType		0=��ˮ��־��1=��Ϣ��־��2=������־��3=������־
	* @nLogLevel	(0-7)
	* @pLogBuf		������־������
	*/
	virtual void STDCALL		XDF_OnRspOutLog(unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf)=0;
	
	/*
	* ����֪ͨ����չ�ӿ� 
	* @nNotifyNo	֪ͨ��
	* @wParam		WPARAM����
	* @lParam		LPARAM����
	*/
	virtual int	STDCALL			XDF_OnRspNotify(unsigned int nNotifyNo, void* wParam, void* lParam )=0;
	
};





class QuoteClientApi
{
public:
	/*
	* ��ʼ���ӿڶ���
	* @return >0 is ok
	*/
	virtual int STDCALL			Init() = 0;
	
	/*
	* ɾ���ӿڶ�����
	*/
	virtual void STDCALL		Release() = 0;
	
	/*
	* @ע��ص��ӿ�
	* @pspi  [in]�����Իص��ӿ����ʵ��
	*/
	virtual void STDCALL		RegisterSpi(QuoteClientSpi * pspi) = 0;
	
	/*
	* ��ʼ����
	* @return (>0) mean ok, (<=0) error
	*/
	virtual int STDCALL			BeginWork() = 0;
	
	/*
	* ��ֹ����
	*/
	virtual void STDCALL		EndWork() = 0;
	
	/*
	* ��ȡ�г�������Ϣ(�������࣬�Ŵ�����)
	*/
	virtual int	 STDCALL		GetMarketInfo(unsigned char cMarket, char* pszInBuf, int nInBytes) =0;

	/*
	* ��ȡ���ƴ����
	* (1)NULL= pszInBuf		����nCount�����ܸ���
	* (2)NULL != pszBuf		����min(nCount,nMsg)����,����nInBytes������
	* @cMarket				[in]XDFMarketȡֵ
	* @pszInBuf/nInBytes	[in]
	* @nCount				[in out]
	* @return (>0) mean ok,it is output bytes, (<=0) error
	* ���ص�pszInBuf, ��ʽΪ   ���ݿ�(EzMsg) + ���ݿ�(EzMsg) ......
	*/
	virtual int	STDCALL			GetCodeTable(unsigned char cMarket, char* pszInBuf, int nInBytes, int& nCount) = 0;
	
	/*
	* ��ȡ��������
	* @cMarket				[in]XDFMarketȡֵ
	* @pCode				[in]Code
	* @pszInBuf				[in]�ַ�������, ���صĸ�ʽΪ   ���ݿ�(EzMsg) + ���ݿ�(EzMsg) ......
	* @nInBytes				[in]��������С
	* @return (>0) mean ok,it is output bytes (<=0) error
	*/
	virtual int STDCALL			GetLastMarketDataAll(unsigned char cMarket, char* pszInBuf, int nInBytes) = 0;
	
	/*
	* ����ã����ĳһ�г�������/�г�ʱ��
	* @cMarket				[in]XDFMarketȡֵ
	* @nStatus				[in out]��ǰ�г���Status(���ĵ�)
	* @ulTime				[in out]��ǰ���г�ʱ��
	* @pI64Send				[in out]���͵��ֽ���
	* @pI64Recv				[in out]���ܵ��ֽ���
	*/
	virtual int STDCALL			GetMarketStatus(unsigned char cMarket,int& nStatus, unsigned int& ulTime, __int64 * pI64Send, __int64 * pI64Recv)=0;
	
protected:
	//virtual STDCALL ~QuoteClientApi(){};
	
	
};





/*
* @pszDebugPath  Ĭ��ֵΪNULL������[���Խӿ�]
* @return ����ʵ��
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API QuoteClientApi * STDCALL CreateQuoteApi(const char *pszDebugPath);


/*
* @nMajorVersion  ���汾
* @nMinorVersion  �Ӱ汾
* @return V1.00 B021��ʽ���ַ���
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API const char * STDCALL GetDllVersion(int &nMajorVersion, int &nMinorVersion);


#pragma pack(1)

typedef struct  
{
	unsigned	char			cMarketID;				//�г�ID
	char						cMarketChn[63];			//��������	
	char						cAddress[128];			//���ӵ�ַ�Ͷ˿�
	int							nStatus;				//��ǰ״̬//0=������ //1=δ֪//2=δ����//5=����
}tagQuoteSettingInfo;											 
																  
#pragma pack()													  
																  
/*
* ��ȡ�������õ���Ϣ
* @pArrMarket, ���ΪNULL�� ���ظ����������NULL������ֵ
* @nCount, ����
* @return ʵ���г�����
//CTYPENAMEFUNC QUOTECLIENTAPI_API int 	STDCALL GetSettingInfo(tagQuoteSettingInfo* pArrMarket, int nCount);
*/




class QuotePrimeApi
{
public:
	/*
	* ����ӿ�
	* @FuncNo		���ܺ�
	* @wParam		����1
	* @lParam		����2
	*/
	virtual int		STDCALL		ReqFuncData(int FuncNo, void* wParam, void* lParam) =0;

protected:
	//virtual ~QuotePrimeApi();

};

/*
* ��ȡ������Ϣ
* @
* @return PrimeApiָ��
*/
//CTYPENAMEFUNC QUOTECLIENTAPI_API QuotePrimeApi * STDCALL CreatePrimeApi();


#endif