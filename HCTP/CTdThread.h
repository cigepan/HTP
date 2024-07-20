#pragma once
#include "stdafx.h"
#include "ThostFtdcTraderApi.h"
#include "CMyLib.h"
#include "CCycleQueue.h"
#include "CMyData.h"
#include "CMongoDB.h"
#include "CRiskThread.h"

#define OFFSETFLAG_ISOPEN(OffsetFlag) (OffsetFlag == THOST_FTDC_OF_Open)
#define HTP_TICKSTATUS_UNSETTIMES 4
#define INT_CFFEX_AUCTIME_BEGIN 92530
#define INT_CFFEX_AUCTIME_END 92830
#define INT_AM_AUCTIME_BEGIN 85530
#define INT_AM_AUCTIME_END 85830
#define INT_PM_AUCTIME_BEGIN 205530
#define INT_PM_AUCTIME_END 205830
#define INT_CTPAM_OPEN 80000
#define INT_CTPAM_CLOSE 40000
#define INT_CTPPM_OPEN 200000
#define INT_CTPPM_CLOSE 160000
class CTdThread : public CThostFtdcTraderSpi
{
public:
	CTdThread();
	~CTdThread();
	int Start(shared_ptr<const CUserConfig> Config);
	int Stop();
	bool GetStatus(const eCtpStatus& eFlag) { return m_bStatus[eFlag]; } //
	map<CFastKey, CThostFtdcInstrumentField, CFastKey> GetMapInstrument() const { return m_mapInstrument; }
	map<CFastKey, CThostFtdcInstrumentCommissionRateField, CFastKey> GetMapCommissionRate() const { return m_mapCommission; }
	int ReqOrderInsert(CThostFtdcInputOrderField& stOrder); //下单
	void InitOrderModel(); //下单模板
	map<CFastKey, CCtpLineK, CFastKey>& GetMapLineK() { return m_mapLineK; }
	shared_ptr<CThostFtdcInputOrderField> GetOrderModel(eCtpOrderModel eModel);
	int OnTick(shared_ptr<const CCtpTick> ctpTick);
	int GetLineK(CCtpLineK& ctpLineK);
	int SetLineK(CCtpLineK& ctpLineK);
	//获取持仓可用，对消挂单后的
	bool GetPosValid(const CFastKey& stID, CThostFtdcInvestorPositionField& stLong, CThostFtdcInvestorPositionField& stShort, SHtpGD& stGDX);
	bool GetPosNet(const CFastKey& stID, TThostFtdcVolumeType& volNetPos); //获取净持仓
	shared_ptr<CRiskThread> m_cRisk; //风控
	shared_ptr<CMongoDB> m_MDB; //数据库
private:
	//-----------------------------------------------K线增量计算备份，来自于行情线程---------------------------------------------------------------
	CCycleQueue<shared_ptr<const CCtpTick>> m_cqTick;
	map<CFastKey, shared_ptr<const CCtpTick>, CFastKey> m_mapTick; //时分秒增量计算，键值：合约
	map<CFastKey, atomic_int, CFastKey> m_mapTickCnt; //Tick计数，键值：合约类别
	mutex m_lockTick; //值传递访问锁
	map<CFastKey, CCtpLineK, CFastKey> m_mapLineK; //键值：合约
	mutex m_lockLineK;
	map<CFastKey, TThostFtdcInstrumentStatusType, CFastKey> m_mapStatus; //合约状态，键值：合约
	mutex m_lockStatus;
	//--------------------------------------------------------------------------------------------------------------
	CThostFtdcInputOrderField m_stOrderModel[CTP_ORDM_ENDF]; //订单模板
	//CCycleQueue<CThostFtdcQryInvestorPositionField> m_cqQryPosition; //查询持仓队列
	CTargetCtrl m_ctpTargetCtrl; //策略TARGET缓存
	bool TargetWay(const SCtpTick& stTgtTick, TThostFtdcVolumeType nTgtPos, const string& strTargetID); //合约相关与目标仓位
	mutex m_lockTarget; //TARGET生成函数锁，TargetWay生成需要线程互斥，不然可能会导致订单重复生成
	bool TradDoing(shared_ptr<CThostFtdcInputOrderField> stOrder); //下单函数
	CThostFtdcTradingAccountField m_stAccount; //账户资金相关
	
	CCycleQueue<shared_ptr<CThostFtdcInputOrderFieldEx>> m_cqOrderTD; //交易执行订单队列
	CCycleQueue<shared_ptr<const CThostFtdcInputOrderFieldEx>> m_cqOrderLog; //本地委托日志
	//订单执行管理，键值为：InstrumentID->OrderRef  a时间缀
	CHtpManage m_htpManage; //订单时间缀与状态管理
	CCycleQueue<shared_ptr<const SHtpTimeSheet>> m_cqTimeSheet; //时间缀输出队列
	CCycleQueue<shared_ptr<const CThostFtdcOrderField>> m_cqOrder; //委托回报-->交易单状态维护
	//订单维护二级map，键值为：InstrumentID->OrderRef，用于状态标记与计算持仓
	map<CFastKey, map<CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey>, CFastKey> m_mapOrder;
	mutex m_lockOrder; //订单锁
	CCycleQueue<shared_ptr<CThostFtdcInputOrderActionField>> m_cqActionTD; //撤单执行队列
	CCycleQueue<shared_ptr<const CThostFtdcTradeField>> m_cqTrade; //成交回报-->交易单状态维护
	//交易单状态维护二级map，键值为：InstrumentID->OrderRef
	map<CFastKey, map<CFastKey, shared_ptr<const CThostFtdcTradeField>, CFastKey>, CFastKey> m_mapTrade;
	mutex m_lockTrade; //交易锁
	bool OrderWay(shared_ptr<const CThostFtdcOrderField> stOrder); //订单方法-->交易单状态维护
	bool TradeWay(shared_ptr<const CThostFtdcTradeField> stTrade); //成交方法-->交易单状态维护
	bool CompareORTD(const SCtpTick& stTick);
	map<CFastKey, bool, CFastKey> m_mapCROP; //撤单是否需要重开
	mutex m_lockCROP;
	int CancelWay(shared_ptr<const CThostFtdcOrderField> itOrder, bool bReOpen=true, bool bCheckLimit=true);
	SHtpGD ScanGD(const CFastKey& stID, map<CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey>& mapGD, ULONG64 ullTimeOutMS = 0); //扫描挂单，bDoCancel控制撤单
	CCycleQueue<CThostFtdcInvestorPositionField> m_cqPosition; //持仓
	map<CFastKey, CThostFtdcInvestorPositionField, CFastKey> m_mapPositionTRD; //交易更新持仓
	map<CFastKey, CThostFtdcInvestorPositionField, CFastKey> m_mapPositionORD; //订单更新持仓
	mutex m_lockPosition; //持仓锁
	map<CFastKey, SHtpPNL, CFastKey> m_mapPNL; //（键值：合约 ）PNL计算盈亏
	mutex m_lockPNL; //PNL锁
	map<CFastKey, shared_ptr<const CThostFtdcQryInvestorPositionField>, CFastKey> m_mapSynPosTRD; //持仓同步：合约
	map<CFastKey, shared_ptr<const CThostFtdcQryInvestorPositionField>, CFastKey> m_mapSynPosORD; //持仓同步：交易所
	//CCycleQueue<string> m_cqQryDepthTick;
	mutex m_lockTaskSyn; //同步锁，同步持仓，成交，委托，行情
	//atomic_int m_nDoFlashPosition; //原子int
	mutex m_lockPOS_GD; //挂单与持仓访问组合锁，面对部分成交的场合
	bool PositionUpdate(map<CFastKey, CThostFtdcInvestorPositionField, CFastKey>& mapPosition, SHtpUpdPos& stUpdPos, bool bUpdateMDB);
	bool PositionWay(CThostFtdcInvestorPositionField& stPosition, bool bUpdateOnly); //持仓方法, bUpdateOnly=true表示只更新map而不返回修正后的持仓
	void AuctionWay();
	bool CheckDoing(const SCtpTick& stTgtTick);
	//void AuctionTick(bool bCFFEX);
	//------------------------------------------------线程相关------------------------------------------------//
	mutex m_pthLock; //线程锁
	mutex m_cthLock; //线程锁
	
	atomic_bool m_bThRunning; //运行状态
	atomic<eCtpStatus> m_eStatus;
	atomic_bool m_bStatus[CTPST_ENDF];
	thread* m_pthTrading; //TD交易信息处理线程
	thread* m_pthFlash;
	thread* m_pthTaskTime;
	thread* m_pthAlgo; //Algo交易监控执行线程
	CSEM m_smTrading;
	CSEM m_smFlash; //分钟线程信号量
	CSEM m_smTime;
	CSEM m_smAlgo; //分钟线程信号量
	virtual void ThreadTrading();
	virtual void ThreadFlash();
	virtual void ThreadTime();
	virtual void ThreadAlgo();
	//------------------------------------------------CTP接口相关------------------------------------------------//
	CThostFtdcTraderApi* m_pApi;
	shared_ptr<const CUserConfig> m_UserCfg;
	atomic_int m_nRequestID; //调用计数
	map<CFastKey, CThostFtdcInstrumentField, CFastKey> m_mapInstrument;//可交易合约行情：键值合约ID
	map<CFastKey, map<CFastKey, CThostFtdcInstrumentField, CFastKey>, CFastKey> m_mapProduct;//可交易合约行情：键值：合约类别->合约
	map<CFastKey, CThostFtdcInstrumentCommissionRateField, CFastKey> m_mapCommission;//合约手续费
	//------------------------------------------------交易函数继承------------------------------------------------//
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

	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);


	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////用户口令更新请求响应
	//virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////资金账户口令更新请求响应
	//virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////预埋单录入请求响应
	//virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////预埋撤单录入请求响应
	//virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////查询最大报单数量响应
	//virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField* pQueryMaxOrderVolume, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////删除预埋单响应
	//virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////删除预埋撤单响应
	//virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////执行宣告录入请求响应
	//virtual void OnRspExecOrderInsert(CThostFtdcInputExecOrderField* pInputExecOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////执行宣告操作请求响应
	//virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField* pInputExecOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////询价录入请求响应
	//virtual void OnRspForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////报价录入请求响应
	//virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField* pInputQuote, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////报价操作请求响应
	//virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField* pInputQuoteAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////批量报单操作请求响应
	//virtual void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField* pInputBatchOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////期权自对冲录入请求响应
	//virtual void OnRspOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField* pInputOptionSelfClose, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////期权自对冲操作请求响应
	//virtual void OnRspOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField* pInputOptionSelfCloseAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求核心间资金转移响应
	//virtual void OnRspTransFund(CThostFtdcTransFundField* pTransFund, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询投资者响应
	//virtual void OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询交易编码响应
	//virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询合约保证金率响应
	//virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询合约手续费率响应
	//virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询交易所响应
	//virtual void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询产品响应
	//virtual void OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询行情响应
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询投资者结算结果响应
	//virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询合约状态响应
	//virtual void OnRspQryInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询转帐银行响应
	//virtual void OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询投资者持仓明细响应
	//virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询客户通知响应
	//virtual void OnRspQryNotice(CThostFtdcNoticeField* pNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询结算信息确认响应
	//virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	/////请求查询投资者持仓明细响应
	//virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField* pOrder);

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade);

	///报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo);

	///报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo);

	///合约交易状态通知
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus);

};

