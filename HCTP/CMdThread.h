#pragma once
#include "stdafx.h"
#include "CMongoDB.h"
#include "ThostFtdcMdApi.h"
#include "CMyData.h"
#include "CHtpSystem.h"
#include "CRiskThread.h"


using namespace std;
#define INT_MAX_MKTLOOK 0x07FF
#define INT_OPEN_PM_TIME 203001
#define INT_OPEN_AM_TIME 83001
#define INT_CFFEX_CLOSE_TIME 152001
#define INT_CLOSE_TIME 150501
using FuncTDTick = std::function<int(shared_ptr<const CCtpTick>)>; //发送TICK到其他线程
using FuncTDLineK = std::function<int(CCtpLineK&)>;

class CMdThread : public CThostFtdcMdSpi
{
public:
	CMdThread();
	~CMdThread();
	int Start(shared_ptr<const CUserConfig> Config, const map<CFastKey, CThostFtdcInstrumentField, CFastKey>& mapInstrument);
	int Stop();
	//shared_ptr<const CCtpTick> GetFastMarket(const TThostFtdcInstrumentIDType& InstrumentID);
	//void AuctionTick(bool bCFFEX);
	map<CFastKey, CCtpLineK, CFastKey>& GetMapLineK() { return m_mapLineK; }
	FuncTDTick m_FuncTDTick;
	FuncTDLineK m_FuncTDLineKGet;
	FuncTDLineK m_FuncTDLineKSet;

	shared_ptr<CRiskThread> m_cRisk; //风控
	shared_ptr<CMongoDB> m_MDB; //数据库
private:
	//------------------------------------------------线程相关------------------------------------------------//
	mutex m_pthLock; //线程指针使用锁，方便快速访问
	mutex m_cthLock; //值传递访问锁
	//mutex m_lockLineK; //LINEK数据锁
	atomic_bool m_bThRunning; //运行状态
	//CCycleQueue<SCtpTick> m_cqDepthTick; //深度行情队列
	
	CCycleQueue<shared_ptr<const CCtpTick>> m_cqTick;
	map<CFastKey, shared_ptr<CCtpTick>, CFastKey> m_mapTick; //时分秒增量计算
	mutex m_mapLock; //值传递访问锁
	CCycleQueue<shared_ptr<const CCtpTick>> m_cqLineKTick; //用于计算K线的TICK
	CCycleQueue<CCtpLineK> m_cqLineK; //缓存的K线数据
	map<CFastKey, CCtpLineK, CFastKey> m_mapLineK;
	int CalcLineK(const CCtpTick& ctpTick); //TICK驱动
	void LineKWay(); //超时补TICK
	//int ReadLineK(const string& strID); //TICK驱动
	thread* m_pthTaskTick;
	thread* m_pthTaskLineK;

	CSEM m_smTick;
	CSEM m_smLineK;

	virtual void ThreadTick();
	virtual void ThreadLineK();

	//------------------------------------------------CTP接口相关------------------------------------------------//
	atomic<eCtpStatus> m_eStatus;
	atomic_bool m_bStatus[CTPST_ENDF];
	CThostFtdcMdApi *m_pApi;
	shared_ptr<const CUserConfig> m_UserCfg;
	CThostFtdcRspUserLoginField m_stLogin;
	map<CFastKey, CThostFtdcInstrumentField, CFastKey> m_mapInstrument;//可交易合约行情
	atomic_int m_nRequestID; //调用计数
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////取消订阅行情应答
	//virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////订阅询价应答
	//virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////取消订阅询价应答
	//virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData);

	/////分价表通知
	//virtual void OnRtnMBLMarketData(CThostFtdcMBLMarketDataField* pMBLMarketData);

	/////询价通知
	//virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp);
};

