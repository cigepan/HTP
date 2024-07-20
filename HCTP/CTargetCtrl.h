#pragma once
#include"CMyData.h"
#include "CMongoTaskDef.h"
//--------------------------------------------------------TARGET生成--------------------------------------------------------------
//ALGO-TARGET 模式
enum eAlgoMode
{
	AGMD_TWAPAD = 0,
	AGMD_TWAPK,
	AGMD_OPENK,
	AGMD_CLOSEK,
	AGMD_PREOPEN,
	//AGMD_T1, //日间模式
	//AGMD_HOUR1, //1小时交易计划
	//AGMD_MINK, //5分钟交易计划
	//AGMD_MIN1, //1分钟交易计划
	//AGMD_T0, //日内模式
	AGMD_ENDF
};
extern const char g_strAlgoMD[AGMD_ENDF][32];
//TARGET分配方式
enum eAlignType
{
	ALIGN_LEFT, //左对齐
	ALIGN_RIGHT, //右对齐
	ALIGN_MID, //中间，其次左
	ALIGN_ENDF
};

//交易时间
struct CTimeTD
{
	CTimeCalc ctmStart;
	CTimeCalc ctmEnd;
};
//没有TARGET的情况下PUSH重复的TARGET时间
#define CTC_PUSH_NOTGT_TIME 10000

//target基础类，按合约级进行存放，用于解析或获取可执行的TARGET
class CTarget
{
	//-------------------------------TARGET生成结果-------------------------------------------
	list<SCtpTarget> m_listTarget;
	size_t m_llnSizeAll; //总target执行数
	TThostFtdcVolumeType m_nVolumeAll; //总量差
	TThostFtdcVolumeType m_nVolumeDone; //已完成
	TThostFtdcVolumeType m_nPositionLast;//上一次的目标数量
	mutex m_lock; 
public:
	//-------------------------------TARGET生成参数（来自MDB解析）-------------------------------------------
	//对以下数据的读写确保无需加锁的线程安全
	bool m_bEnable[AGMD_ENDF]; //target模式，接受多种模式融合
	int m_nParamK[AGMD_ENDF]; //target模式附带参数，接受多种模式融合
	eAlignType m_eAlign; //对齐方式
	int m_nFreqMS; //节点频率
	double m_dbTimeAdj; //时间偏移适应参数
	double m_dbVolK; //交易量适应参数K
	double m_dbVolPowK; //交易量适应算数根参数K，默认不开根
	TThostFtdcVolumeType	m_volLastQty; //最新持仓
	TThostFtdcVolumeType	m_volTarget; //目标持仓
	TThostFtdcVolumeType m_volTradeQty; //交易量
	CTimeCalc m_ctmCurrent; //当前时间
	CTimeCalc m_ctmTD; //交易日
	CTimeCalc m_ctmTdStart; //策略开始时间
	CTimeCalc m_ctmTdEnd; //策略结束时间
	vector<CTimeTD> m_vecTimeTD; //行情交易时间
	string m_strID; //合约ID
	string m_strExchange; //合约交易所
	string m_strClass; //产品类型(格式化大写)
	string m_strProduct; //产品名称
	CTarget();
	~CTarget();
	void Clear();
	void Push(const SCtpTarget& stTatget);
	bool Pop();
	bool Front(SCtpTarget& stTarget);
	SCtpTarget Current();
	void GetTaskRatio(SCtpTaskRatio& stRatio);
	bool GetTarget(const CTimeCalc& ctpTimeNT, SCtpTarget& stTarget);
	CHtpMsg GetTimeList(list<CTimeCalc>& listTimeTD); //获取时间节点, 返回时间跨度量与交易时间节点
	CHtpMsg Create(document& docPlan); ////生成TARGET，根据不同模式执行不同生成操作
	//CTargetCtrl& operator=(const CTargetCtrl& ctpTarget); //旧版采用从数据库读取TARGET，新版则执行与解析
};
bool TargetCompare(SCtpTarget& A, SCtpTarget& B);

//---------------------------------------------------TARGET控制器(带锁)--------------------------------------------------------------------
//由于TARGET涉及到的数据和结构较多，所以使用类方便进行管理
//最大TARGET缓存BUF，不采用共享指针的方式主要原因是因为需要有极快的更新切换速度，以免影响到交易
#define CTGT_MAX 0x03
class CTargetCtrl
{
	//采用缓冲而不采用共享指针的原因是追求状态更新速度，也就是状态需要非常快的更新开关不然会影响交易
	map<CFastKey, unique_ptr<CTarget>, CFastKey> m_MapTarget[CTGT_MAX + 1];
	TThostFtdcUserIDType m_stUserID;
	atomic_llong m_llTimeStamp; //原子操作
	string m_strTargetID;
	char m_chID; // 当前ID
	mutex m_lock;
	//MAP的二次封装接口，CTarget也带有锁互斥所以无需担心资源竞争
	CTarget* GetMapTarget(const CFastKey& strKey) 
	{
		unique_lock<mutex> lock(m_lock);
		map<CFastKey, unique_ptr<CTarget>, CFastKey>& refMap = m_MapTarget[(m_chID & CTGT_MAX)];
		if (refMap.find(strKey) == refMap.end())
		{
			return NULL;
		}
		return refMap[strKey].get();
	}
public:
	CTargetCtrl()
	{
		memset(m_stUserID, 0, sizeof(TThostFtdcUserIDType));
		m_llTimeStamp = 0;
		m_chID = 0;
		for (int i = 0; i <= CTGT_MAX; i++)
		{
			m_MapTarget[i].clear();
		}
	}
	~CTargetCtrl()
	{
	}
	void SetUserID(const TThostFtdcUserIDType& stUserID)
	{
		unique_lock<mutex> lock(m_lock);
		memcpy(m_stUserID, stUserID, sizeof(TThostFtdcUserIDType));
	}
	const TThostFtdcUserIDType& GetUserID()
	{
		unique_lock<mutex> lock(m_lock);
		return m_stUserID;
	}
	void SetTargetID(const string& strTargetID)
	{
		unique_lock<mutex> lock(m_lock);
		m_strTargetID = strTargetID;
	}
	const string GetTargetID()
	{
		unique_lock<mutex> lock(m_lock);
		return m_strTargetID;
	}
	void SetTimeStamp(const LONG64& llTime) //设置时间缀，用于判断是否为需要更新当前target
	{
		m_llTimeStamp = llTime;
	}
	LONG64 GetTimeStamp()
	{
		return m_llTimeStamp;
	}

	SCtpTarget operator[](const CFastKey& strKey)
	{
		CTarget* pCtpTarget = GetMapTarget(strKey);
		return (NULL == pCtpTarget) ? SCtpTarget({ 0 }) : pCtpTarget->Current();
	}
	//获取满足该时间点所有的TARGET
	int GetTarget(map<CFastKey, SCtpTarget, CFastKey>& mapTarget, const CTimeCalc& ctpTimeNT, string& strTargetID)
	{
		strTargetID = GetTargetID();
		mapTarget.clear();
		map<CFastKey, unique_ptr<CTarget>, CFastKey>& refMap = m_MapTarget[(m_chID & CTGT_MAX)];
		SCtpTarget stTarget = { 0 };
		for (auto& it : refMap)
		{
			if (!it.second) { continue; }
			if (it.second->GetTarget(ctpTimeNT, stTarget))
			{
				mapTarget[it.first] = stTarget;
			}
		}
		return (int)mapTarget.size();
	}
	//获取满足该TICK合约时间驱动的TARGET,两种TARGET驱动方式，盘中采用TICK驱动，集中交易采用时间驱动
	bool GetTarget(shared_ptr<const CCtpTick> stTick, SCtpTarget& stTarget, string& strTargetID)
	{
		strTargetID = GetTargetID();
		CTarget* pCtpTarget = GetMapTarget(stTick->m_stTick.InstrumentID);
		return (pCtpTarget) ? pCtpTarget->GetTarget(stTick->m_ctNature, stTarget) : false;
	}
	SCtpTaskRatio GetTaskRatio(const CFastKey& strKey)//const，没办法，底层有锁
	{
		SCtpTaskRatio stRatio = { 0 };
		//map数组写完后只读，索引可无需加锁，读写小心
		auto itTarget = m_MapTarget[(m_chID & CTGT_MAX)].find(strKey);
		if (itTarget == m_MapTarget[(m_chID & CTGT_MAX)].end()) { return stRatio; }
		if (!itTarget->second) { return stRatio; }
		itTarget->second->GetTaskRatio(stRatio); //获取该合约的TARGET执行情况
		return stRatio;
	}
	//批量获取任务执行情况
	size_t GetTaskRatio(map<CFastKey, SCtpTaskRatio, CFastKey>& mapRatio)//const，没办法，底层有锁
	{
		mapRatio.clear();
		auto& mapTarget = m_MapTarget[(m_chID & CTGT_MAX)]; //无需加锁，读写小心
		for (auto& it : mapTarget)
		{
			if (!it.second) { continue; }
			auto& itRatio = mapRatio[it.first];
			it.second->GetTaskRatio(itRatio);
		}
		return mapTarget.size();
	}
	//新建target，会通过MDB外部进行写入map，考虑时效问题采用多缓冲
	map<CFastKey, unique_ptr<CTarget>, CFastKey>& NewTarget()
	{
		unique_lock<mutex> lock(m_lock);
		return m_MapTarget[((m_chID + 1) & CTGT_MAX)];
	}
	void CheckOut() //切换到最新target
	{
		unique_lock<mutex> lock(m_lock);
		m_chID++; m_chID &= CTGT_MAX;
	}
	bool PopTarget(const CFastKey& strKey) //删除合约级的TARGET,前提是该TARGET已经被触发
	{
		CTarget* pTarget = GetMapTarget(strKey);
		if (pTarget)
		{
			return pTarget->Pop();
		}
		return false;
	}
};

bool ToValue(SCtpTarget& stTarget, const string& strTime);


