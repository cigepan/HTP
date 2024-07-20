#include "CRiskThread.h"
#include "CLogThread.h"



CRiskThread::CRiskThread()
{
	m_mapRiskCtrl.clear();
	m_mapRiskCnt.clear();
	m_Config = NULL;
	for (int i = 0; i < REN_ENDF; i++)
	{
		m_bEnable[i] = false; //默认关闭所有功能
	}
	for (int i = 0; i < RSC_ENDF; i++)
	{
		m_nSysCnt[i] = 0;
	}
	m_stLimit.nOrderDaySum = 1000; //累计挂单数量
	m_stLimit.nCancelDaySum = 100; //累计撤单数量
	m_stLimit.nOrderMinSum = 100; //累计分钟委托数量
	m_stLimit.nOrderVolMax = 1000; //单笔报单最大数量
	m_stLimit.nLongPosMax = 1000; //多头最大持仓
	m_stLimit.nShortPosMax = 1000; //空头最大持仓
}
CRiskThread::~CRiskThread() {}
void CRiskThread::Start(shared_ptr<const CUserConfig> Config) { m_Config = Config; }
bool CRiskThread::GetEnable(eRiskEnable eType) const { return (m_bEnable[eType] && m_bEnable[REN_GLOBAL]); }
atomic_int& CRiskThread::SysCnt(eRiskSysCnt eType) { return m_nSysCnt[eType]; }
atomic_bool& CRiskThread::Enable(eRiskEnable eType) { return m_bEnable[eType]; }
void CRiskThread::SetEnable(eRiskEnable eType, bool bEnable) { m_bEnable[eType] = bEnable; }
//控制器
SCtpRiskCtrl& CRiskThread::Ctrl(const CFastKey& cKey)
{
	m_cthLock.lock_shared();
	auto it = m_mapRiskCtrl.find(cKey);
	m_cthLock.unlock_shared();
	if (it == m_mapRiskCtrl.end())
		unique_lock<shared_mutex> lock(m_cthLock); //写锁
	else
		shared_lock<shared_mutex> lock(m_cthLock); //读锁
	return m_mapRiskCtrl[cKey];
}
//限制器
SCtpRiskLimit& CRiskThread::Limit()
{
	shared_lock<shared_mutex> lock(m_cthLock); //读锁
	return m_stLimit;
}
//计数器
SCtpRiskCnt& CRiskThread::Cnt(const CFastKey& cKey) //提供可修改计数，底层为计数统一为原子操作
{
	m_cthLock.lock_shared();
	auto it = m_mapRiskCnt.find(cKey);
	m_cthLock.unlock_shared();
	if (it == m_mapRiskCnt.end())
		unique_lock<shared_mutex> lock(m_cthLock); //写锁
	else
		shared_lock<shared_mutex> lock(m_cthLock); //读锁
	return m_mapRiskCnt[cKey];
}
//风控检查
bool CRiskThread::Check(shared_ptr<const CThostFtdcInputOrderField> stOrder)
{
	char szMsg[256] = { 0 };
	sprintf_s(szMsg, "->CRiskThread::Check::stOrder.LimitPrice[%lf] ", stOrder->LimitPrice);
	if (CTP_PRICE_ISOUT(stOrder->LimitPrice)) //报单价格异常，不进行下单
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("ERROR"), HTP_CID_FAIL | 9999);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	m_cthLock.lock_shared();
	auto it = m_mapRiskCtrl.find(stOrder->InstrumentID);
	m_cthLock.unlock_shared();
	if (it == m_mapRiskCtrl.end())
		return true; //没有控制约束
	m_cthLock.lock_shared();
	SCtpRiskCtrl& stRiskCtrl = m_mapRiskCtrl[stOrder->InstrumentID]; //结构体单变量访问是原子的
	SCtpRiskCnt& stRiskCnt = m_mapRiskCnt[stOrder->InstrumentID]; //结构体单变量访问是原子的
	m_cthLock.unlock_shared();
	//sprintf_s(szMsg, "->CRiskThread::Check::XXXXXXXXXX stRiskCnt[%d] m_stLimit[%d] ", stRiskCnt.XXXXXX, m_stLimit.XXXX);
	if (stRiskCtrl.bStopTrading)
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append("->CRiskThread::Check bStopTrading FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append("->CRiskThread::Check bStopTrading OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	if ((stRiskCtrl.bStopOpen)
		&& stOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open)
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append("->CRiskThread::Check bStopOpen FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append("->CRiskThread::Check bStopOpen OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	sprintf_s(szMsg, "->CRiskThread::Check::nOrderMinSum stRiskCnt[%d] m_stLimit[%d] "
		, stRiskCnt.nOrderMinSum.load(), m_stLimit.nOrderMinSum.load());
	if (stRiskCnt.nOrderMinSum > m_stLimit.nOrderMinSum)//风控限制
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	sprintf_s(szMsg, "->CRiskThread::Check::nCancelDaySum stRiskCnt[%d] m_stLimit[%d] "
		, stRiskCnt.nCancelDaySum.load(), m_stLimit.nCancelDaySum.load());
	if (stRiskCnt.nCancelDaySum > m_stLimit.nCancelDaySum)//风控限制
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	sprintf_s(szMsg, "->CRiskThread::Check::nOrderDaySum stRiskCnt[%d] m_stLimit[%d] "
		, stRiskCnt.nOrderDaySum.load(), m_stLimit.nOrderDaySum.load());
	if (stRiskCnt.nOrderDaySum > m_stLimit.nOrderDaySum)//风控限制
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	sprintf_s(szMsg, "->CRiskThread::Check::nOrderVolMax stOrder[%d] m_stLimit[%d] "
		, stOrder->VolumeTotalOriginal, m_stLimit.nOrderVolMax.load());
	if (stOrder->VolumeTotalOriginal > m_stLimit.nOrderVolMax)//风控限制
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	//if (stRiskCnt.nLongPosMax > m_stLimit.nLongPosMax)//风控限制
	//{
	//	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
	//		.append("->CRiskThread::Check nLongPosMax"), HTP_CID_FAIL | 7878);
	//	return false;
	//}
	//if (stRiskCnt.nShortPosMax > m_stLimit.nShortPosMax)//风控限制
	//{
	//	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
	//		.append("->CRiskThread::Check nShortPosMax"), HTP_CID_FAIL | 7878);
	//	return false;
	//}
	return true;
}