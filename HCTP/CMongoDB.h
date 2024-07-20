#pragma once
#include "stdafx.h"
#include "CMongoTaskDef.h"
#include "CRiskThread.h"
#include "CTargetCtrl.h"

using FuncGetPosNet = std::function<bool(const CFastKey&, TThostFtdcVolumeType&)>;

class CMongoDB : public CMdbClient
{
public:
	CMongoDB();
	~CMongoDB();
	CHtpMsg Start(shared_ptr<const CUserConfig> stConfig);
	int OnLineK(CCycleQueue<CCtpLineK>& cqLineK); //行情MIN调用
	int LoadLineK(CCtpLineK& refCtpLineK); //加载特定合约过程数据
	int LoadLineKMap(map<CFastKey, CCtpLineK, CFastKey>& mapLineK); //加载所有K线过程数据
	int OnTick(CCycleQueue<shared_ptr<const CCtpTick>>& cqTick); //行情Tick处理BULK
	int OnTdAccount(const CThostFtdcTradingAccountField& stField, const map<CFastKey, SHtpPNL, CFastKey>& mapPNL); //交易
	int OnTdOrder(shared_ptr < const CThostFtdcOrderField> stField); //交易
	int OnTdOrderLog(shared_ptr<const CThostFtdcInputOrderFieldEx> stField); //交易
	int OnTdTrade(shared_ptr<const CThostFtdcTradeField> stField); //交易
	int OnTdPosition(const CThostFtdcInvestorPositionField& stField, const SHtpGD& stGD); //交易
	int OnTdTimeSheet(shared_ptr<const SHtpTimeSheet> stTimeSheet); //时间缀
	int LoadRiskCnt();
	int LoadRiskCtrl();
	int UpdateTask(const map<CFastKey, SCtpTaskRatio, CFastKey>& mapRatio);
	int UpdateContract(const map<CFastKey, CThostFtdcInstrumentField, CFastKey>& mapInstrument,
		const map<CFastKey, CThostFtdcInstrumentCommissionRateField, CFastKey>& m_mapCommission);
	int LoadTarget(CTargetCtrl& cTargetCtrl);
	int LoadTimeTD();
	int PushLogToDB(const CHtpMsg& cMsg);
	int SendEmail(const CHtpMsg& cMsg);
	shared_ptr<CRiskThread> m_cRisk; //风控
	FuncGetPosNet m_FuncGetPosNet;
private:
	shared_ptr<const CUserConfig> m_UserCfg; //配置
	mutex m_cthLock;
	//CSEM m_smTask; //分钟线程信号量
	thread* m_pthTask; //mdb操作线程
	atomic_bool m_bThRunning; //运行状态
	atomic_bool m_bMdbConnected; //数据库链接状态
	map<CFastKey, CTarget, CFastKey> m_mapTargetCtrl; //TARGET管理，键值：合约ID
	map<CFastKey, vector<CTimeTD>, CFastKey> m_mapTimeTD; //交易时间，键值：合约ID
	mutex m_lockTarget; //TARGET解析任务锁
	//void Run();
};

