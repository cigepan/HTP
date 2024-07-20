#pragma once
#include "stdafx.h"
#include "ThostFtdcTraderApi.h"
#include "CMyLib.h"
#include "CCycleQueue.h"
#include "CMyData.h"
#include <shared_mutex>

enum eRiskEnable
{
	REN_GLOBAL = 0, //风控总开关
	REN_SNAPSHOT_TICK, //行情快照
	REN_SNAPSHOT_LINEK, //K线快照
	REN_SNAPSHOT_POSITION, //持仓快照
	REN_SNAPSHOT_ORDER, //委托快照
	REN_MARKET, //行情开关，记录行情 
	REN_TDRADE, //交易，执行交易
	REN_ENDF
};
const char g_strRiskEN[REN_ENDF][32] =
{
	"GLOBAL_EN", //风控总开关
	"SHOT_TICK", //行情快照
	"SHOT_LINEK", //K线快照
	"SHOT_POSITION", //持仓快照
	"SHOT_ORDER", //委托快照
	"MARKET_EN", //行情开关
	"TRADE_EN", //交易开关
};
enum eRiskSysCnt
{
	RSC_INIT = 0, //初始化次数
	RSC_LOGIN, //登录次数
	RSC_ORDFAIL, //总下单出错次数
	RSC_VOLX, //量差异常次数
	RSC_SYN_LINEK, //读取加载K线次数
	RSC_CMDX, //CMD执行失败次数
	RSC_LOGOUT, //注销登录次数
	RSC_ENDF
};
const char g_strSysCnt[RSC_ENDF][32] =
{
	"RSC_INIT", //初始化次数
	"RSC_LOGIN", //登录次数
	"RSC_ORDFAIL", //总下单出错次数
	"RSC_VOLX", //量差异常次数
	"RSC_SYN_LINEK", //读取加载K线次数
	"RSC_CMDX" //CMD执行失败次数
	"RSC_LOGOUT" //注销登录次数
};
const int g_nSysCntLimit[RSC_ENDF]
{
	50, 50, 1000, 10000, 3000, 30, 50
};
class CRiskThread
{
	atomic_bool m_bEnable[REN_ENDF]; 
	atomic_int m_nSysCnt[RSC_ENDF];
	SCtpRiskLimit m_stLimit;
	map<CFastKey, SCtpRiskCtrl, CFastKey> m_mapRiskCtrl; //风控控制器，键值：合约
	map<CFastKey, SCtpRiskCnt, CFastKey> m_mapRiskCnt; //风控计数器, 键值：合约
	shared_mutex m_cthLock;
	shared_ptr<const CUserConfig> m_Config;
public:
	CRiskThread();
	~CRiskThread();
	void Start(shared_ptr<const CUserConfig> Config);
	bool GetEnable(eRiskEnable eType) const;
	//使能开关
	atomic_bool& Enable(eRiskEnable eType);
	void SetEnable(eRiskEnable eType, bool bEnable);
	//控制器
	SCtpRiskCtrl& Ctrl(const CFastKey& cKey);
	//计数器
	SCtpRiskCnt& Cnt(const CFastKey& cKey);
	//风控限制器
	SCtpRiskLimit& Limit();
	//系统计数器
	atomic_int& SysCnt(eRiskSysCnt eType);
	bool Check(shared_ptr<const CThostFtdcInputOrderField> stOrder);
};

