//TARGET交易计划生成
//支持多种交易模式

#include "CTargetCtrl.h"

const char g_strAlgoMD[AGMD_ENDF][32] = {
	"twapAD", "twapK", "openK", "closeK", "preOpen"
};
////TARGET触发时间比对
//bool TargetCompare(SCtpTarget& A, SCtpTarget& B)
//{
//	return (A.nYMD == B.nYMD) ? (A.nHMSL < B.nHMSL) : (A.nYMD < B.nYMD);
//}
bool TimeCompare(CTimeCalc& A, CTimeCalc& B)
{
	if (A.m_nYear != B.m_nYear) { return (A.m_nYear < B.m_nYear); }
	if (A.m_nMonth != B.m_nMonth) { return (A.m_nMonth < B.m_nMonth); }
	if (A.m_nDay != B.m_nDay) { return (A.m_nDay < B.m_nDay); }
	if (A.m_nHour != B.m_nHour) { return (A.m_nHour < B.m_nHour); }
	if (A.m_nMinute != B.m_nMinute) { return (A.m_nMinute < B.m_nMinute); }
	if (A.m_nSecond != B.m_nSecond) { return (A.m_nSecond < B.m_nSecond); }
	if (A.m_nMilSec != B.m_nMilSec) { return (A.m_nMilSec < B.m_nMilSec); }
	return false;
}
//解析TARGET用到的，将字符串解析为数值，目的是提高排序与后续的比对效率
bool ToValue(SCtpTarget& stTarget, const string& strTime)
{
	if (strTime.size() < 19) { return false; }
	stTarget.nYMD = strTime.at(0) - '0';
	//stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(0) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(1) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(2) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(3) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(5) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(6) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(8) - '0';
	stTarget.nYMD *= 10; stTarget.nYMD += strTime.at(9) - '0';
	stTarget.nHMSL = strTime.at(11) - '0';
	stTarget.nHMSL *= 10; stTarget.nHMSL += strTime.at(12) - '0';
	stTarget.nHMSL *= 10; stTarget.nHMSL += strTime.at(14) - '0';
	stTarget.nHMSL *= 10; stTarget.nHMSL += strTime.at(15) - '0';
	stTarget.nHMSL *= 10; stTarget.nHMSL += strTime.at(17) - '0';
	stTarget.nHMSL *= 10; stTarget.nHMSL += strTime.at(18) - '0';
	stTarget.nHMSL *= 1000; //毫秒
	return true;
}

CTarget::CTarget()
{
	Clear();
}
void CTarget::Clear()
{
	for (int i = 0; i < AGMD_ENDF; i++)
	{
		m_bEnable[i] = false;
		m_nParamK[i] = 0;
	}
	m_eAlign = ALIGN_MID;
	m_nFreqMS = 0; //风控频率
	m_dbTimeAdj = 0.0; //时间偏移适应参数
	m_dbVolK = 0.5; //交易量适应参数K
	m_dbVolPowK = 1.0; //交易量适应算数根参数K
	m_volLastQty = 0; //最新持仓
	m_volTarget = 0; //目标持仓
	m_volTradeQty = 0; //交易量
	m_ctmCurrent.Clear(); //当前时间
	m_ctmTD.Clear(); //交易日
	m_ctmTdStart.Clear(); //策略开始时间
	m_ctmTdEnd.Clear(); //策略结束时间
	m_vecTimeTD.clear();
	m_strID.clear(); //合约ID
	m_strExchange.clear(); //合约交易所
	m_strClass.clear(); //产品类型(格式化大写)
	m_strProduct.clear();
	m_listTarget.clear();
	m_llnSizeAll = 0;
	m_nVolumeAll = 0;
	m_nVolumeDone = 0;
	m_nPositionLast = 0;
}
CTarget::~CTarget()
{

}
CHtpMsg CTarget::GetTimeList(list<CTimeCalc>& listTimeTD) //获取时间节点
{
	listTimeTD.clear();
	if (m_nFreqMS < 100) 
	{ return CHtpMsg("CTarget::GetTimeList m_nFreqMS < 100", HTP_CID_FAIL | 5355); }
	vector<CTimeCalc> vecTimeDay;
	vecTimeDay.clear();
	CTimeCalc ctmTD = m_ctmTD; //交易日
	CTimeCalc ctmTdStart = (m_ctmTdStart.m_nYear > 2020) ? m_ctmTdStart : m_ctmCurrent; //策略开始时间
	CTimeCalc ctmCurrent = m_ctmCurrent;
	CTimeCalc ctmPrevDay = ctmCurrent; //当前日的前一天
	ctmPrevDay.GetPrevDay(); //定位到昨天
	CTimeCalc ctmNextDay = ctmCurrent; //当前日的前一天
	ctmNextDay.GetNextDay(); //定位到明天
	if (ctmCurrent.GetYMD() > ctmTD.GetYMD()) //策略开始时间异常
	{
		return CHtpMsg("CTarget::GetTimeList CURRENT DAY > TRADE DAY", HTP_CID_FAIL | 5355);
	}
	if (ctmTdStart.GetYMD() > ctmTD.GetYMD()) //策略开始时间异常
	{
		return CHtpMsg("CTarget::GetTimeList START DAY > TRADE DAY", HTP_CID_FAIL | 5355);
	}
	//集合竞价 = 开盘前第三分钟
	CTimeCalc ctmClock = m_vecTimeTD[0].ctmStart;
	ctmClock.GetPrevMinute();
	ctmClock.GetPrevMinute();
	ctmClock.GetPrevMinute();
	//没有交易日期的单将不会被执行但会参与时间节点计算
	if (ctmCurrent.GetYMD() < ctmTD.GetYMD())//晚盘策略需要执行则给定日期
	{
		ctmClock.SetYMD(ctmCurrent.GetYMD());
	}
	else 
	{
		ctmClock.SetYMD(ctmPrevDay.GetYMD()); //设置为过去的时间
	}
	//if (ctmTdStart.GetYMD() < ctmClock.GetYMD()) //策略开始时间异常
	//{
	//	return CHtpMsg("CTarget::GetTimeList TARGET START TIME IS OUT", HTP_CID_FAIL | 5355);
	//}
	vecTimeDay.push_back(ctmClock); //集合竞价 = 开盘前第三分钟 57/27
	//盘中交易节点
	CTimeCalc ctmFreq; //时间跨度计算
	int nFreqMS = m_nFreqMS; //节点频率
	//计算时间跨度
	ctmFreq.m_nHour = nFreqMS / 3600000; nFreqMS %= 3600000;
	ctmFreq.m_nMinute = nFreqMS / 60000; nFreqMS %= 60000;
	ctmFreq.m_nSecond = nFreqMS / 1000; nFreqMS %= 1000;
	ctmFreq.m_nMilSec = nFreqMS;
	//计算时间适应，向后
	CTimeCalc ctmAdj;
	int nTimeAdj = int(double(m_nFreqMS) * double((m_dbTimeAdj > 0.0f) ? m_dbTimeAdj : 1.0 + m_dbTimeAdj));
	ctmAdj.m_nHour = nTimeAdj / 3600000; nTimeAdj %= 3600000;
	ctmAdj.m_nMinute = nTimeAdj / 60000; nTimeAdj %= 60000;
	ctmAdj.m_nSecond = nTimeAdj / 1000; nTimeAdj %= 1000;
	ctmAdj.m_nMilSec = nTimeAdj;
	for (const auto itTime : m_vecTimeTD)
	{
		ctmClock = itTime.ctmStart;
		if (ctmClock.m_nHour > 20) //晚盘时间
		{
			//没有交易日期的单将不会被执行但会参与时间节点计算
			if (ctmCurrent.GetYMD() < ctmTD.GetYMD())
			{
				if (ctmCurrent.m_nHour < 6)
					ctmClock.SetYMD(ctmPrevDay.GetYMD()); //计算日期
				else
					ctmClock.SetYMD(ctmCurrent.GetYMD()); //计算日期
			}
			else
			{
				//假设没有则设置为过去的时间，过期不影响TARGET生成与计划执行，只是时间上过去了也就不会被执行的
				ctmClock.SetYMD(ctmPrevDay.GetYMD()); 
			}
		}
		else if (ctmClock.m_nHour < 6) //凌晨时间
		{
			//没有交易日期的单将不会被执行但会参与时间节点计算
			if (ctmCurrent.GetYMD() < ctmTD.GetYMD())
			{
				if (ctmCurrent.m_nHour < 6)
					ctmClock.SetYMD(ctmCurrent.GetYMD()); //计算日期
				else
					ctmClock.SetYMD(ctmNextDay.GetYMD()); //计算日期
			}
			else 
			{
				//假设没有则设置为过去的时间，过期不影响TARGET生成与计划执行，只是时间上过去了也就不会被执行的
				ctmClock.SetYMD(ctmCurrent.GetYMD());
			}
		}
		else //白天
		{
			ctmClock.SetYMD(ctmTD.GetYMD()); //日盘时间=交易日
		}
		//时间适应
		ctmClock.m_nMilSec += ctmAdj.m_nMilSec;
		if (ctmClock.m_nMilSec > 999)
		{
			ctmClock.m_nMilSec -= 1000; ctmClock.m_nSecond++;
		}
		ctmClock.m_nSecond += ctmAdj.m_nSecond;
		if (ctmClock.m_nSecond > 59)
		{
			ctmClock.m_nSecond -= 60; ctmClock.m_nMinute++;
		}
		ctmClock.m_nMinute += ctmAdj.m_nMinute;
		if (ctmClock.m_nMinute > 59)
		{
			ctmClock.m_nMinute -= 60; ctmClock.GetNextHour();
		}
		//循环获取交易时间节点
		while ((ctmClock.GetHMS() >= itTime.ctmStart.GetHMS())
			&& (ctmClock.GetHMS() < itTime.ctmEnd.GetHMS())) //交易时间内
		{
			vecTimeDay.push_back(ctmClock);
			//跨度时间
			ctmClock.m_nMilSec += ctmFreq.m_nMilSec;
			if (ctmClock.m_nMilSec > 999) 
			{ ctmClock.m_nMilSec -= 1000; ctmClock.m_nSecond++; }
			ctmClock.m_nSecond += ctmFreq.m_nSecond;
			if (ctmClock.m_nSecond > 59) 
			{ ctmClock.m_nSecond -= 60; ctmClock.m_nMinute++; }
			ctmClock.m_nMinute += ctmFreq.m_nMinute;
			if (ctmClock.m_nMinute > 59) 
			{ ctmClock.m_nMinute -= 60; ctmClock.GetNextHour(); }
		}
	}
	
	//时间过滤
	int nIndex = 0;
	//添加集合竞价时间节点
	if (m_bEnable[AGMD_PREOPEN])   
	{
		listTimeTD.push_back(vecTimeDay[nIndex]);
	} 
	nIndex++;
	//开盘时间：多少个freq时间
	int nFreqCnt = min(int(vecTimeDay.size()), m_nParamK[AGMD_OPENK]);
	if (m_bEnable[AGMD_OPENK])  
	{
		for (; nIndex < nFreqCnt; nIndex++)
		{
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	//从当前时间点开始到收盘
	if (m_bEnable[AGMD_TWAPAD]) 
	{
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			if ((ctmCurrent.m_nYear < 2000)
				|| (ctmCurrent.GetYMDHMS() > vecTimeDay[nIndex].GetYMDHMS())) {
				continue; //策略外的时间则丢掉
			} 
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	//从当前时间点开始后的N个Freq
	if (m_bEnable[AGMD_TWAPK]) 
	{
		nFreqCnt = m_nParamK[AGMD_TWAPK];
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			if ((ctmCurrent.m_nYear < 2000)
				|| (ctmCurrent.GetYMDHMS() > vecTimeDay[nIndex].GetYMDHMS())) {
				continue; //策略外的时间则丢掉
			} 
			listTimeTD.push_back(vecTimeDay[nIndex]);
			if (--nFreqCnt < 1) { break; }
		}
	}
	//收盘前的N个
	if (m_bEnable[AGMD_CLOSEK]) 
	{
		nFreqCnt = m_nParamK[AGMD_CLOSEK];
		nIndex = max(int(vecTimeDay.size() - nFreqCnt), nIndex); //从已经过去了的算起
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	listTimeTD.sort(TimeCompare); //排序时间
	return CHtpMsg(HTP_CID_OK);
}

CHtpMsg CTarget::Create(document& docPlan) //执行TARGET解析
{
	char szKey[32];
	unique_lock<mutex> lock(m_lock);
	list<CTimeCalc> listTimeTD; //获取交易时间节点，确保是正序时间的
	CHtpMsg cMsg = GetTimeList(listTimeTD);
	if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK)
	{
		return cMsg;
	}
	if (1 > listTimeTD.size()) 
	{
		return CHtpMsg("CTarget::Create listTimeTD NOT FIND", HTP_CID_FAIL | 5355);
	}
	SCtpTarget stTarget = { 0 };
	double dbPosDiv = 0.0; //绝对增量单位
	if (m_volTradeQty != 0)
	{
		dbPosDiv = double(abs(m_volTradeQty)) / double(listTimeTD.size());
	}
	double dbPosAdd = 0.0; //保证第一次有交易
	double dbVolumeAdj = 0.0;  //量适应参数
	double dbVolume = 0.0; //交易量，向下取整
	stTarget.Position = m_volLastQty;
	m_listTarget.clear(); //全天TWAP则直接
	size_t nIdx = 0;
	docPlan.clear();
	int nSkipTime = CTC_PUSH_NOTGT_TIME / m_nFreqMS; //秒级的为了避免生成过多没必要的，在连续持仓一致的情况下10S一个重复
	int nTimeCnt = 0;
	for (auto& itTD : listTimeTD)
	{
		stTarget.nYMD = itTD.GetYMD();
		stTarget.nHMSL = itTD.GetHMSL();
		dbPosAdd += dbPosDiv;
		dbVolumeAdj = m_dbVolK * pow((float)(++nIdx / listTimeTD.size()), m_dbVolPowK);
		dbVolume = dbPosAdd + dbVolumeAdj + 0.000001; //补左对齐，补整
		if ((dbVolume > 1.0) || (nIdx == 1)) //确保第一个行情有交易
		{
			nTimeCnt = nSkipTime + 1; //有交易的情况下确保有TARGET
			TThostFtdcVolumeType nDoVolume = 0;
			if (dbVolume < 1.0) //保证第一个有交易
			{
				nDoVolume = 1;
				dbPosAdd = 0.0;
			}
			else
			{
				nDoVolume = (TThostFtdcVolumeType)(dbVolume);
				dbPosAdd -= double(nDoVolume);
			}

			if (m_volTradeQty > 0) //增仓
			{
				stTarget.Position += nDoVolume;
				if (stTarget.Position > m_volTarget) 
				{ stTarget.Position = m_volTarget; } //确保不越界
			}
			else if (m_volTradeQty < 0)//减仓
			{
				stTarget.Position -= nDoVolume;
				if (stTarget.Position < m_volTarget) 
				{ stTarget.Position = m_volTarget; } //确保不越界
			}
		}
		if (++nTimeCnt > nSkipTime) //没有交易的情况下10S一个重复的target用于盘中实时同步持仓
		{
			nTimeCnt = 0;
			m_listTarget.push_back(stTarget);
			sprintf_s(szKey, "%08d%09d", stTarget.nYMD, stTarget.nHMSL);
			docPlan << BOOST_STR(szKey) << stTarget.Position;
		}
	}
	m_nVolumeAll = m_volTradeQty;
	if (m_listTarget.size() < 1)
	{
		return CHtpMsg("CTarget::Create m_listTarget RESULT IS NULL!!!", HTP_CID_OK | 5355);
	}
	//确保最后一个交易到
	if (m_listTarget.back().Position != m_volTarget)
	{
		m_listTarget.back().Position = m_volTarget;
	}
	//m_listTarget.sort(TargetCompare); //先排序，然后计算量差和
	return CHtpMsg(HTP_CID_OK);
}

void CTarget::Push(const SCtpTarget& stTatget)
{
	unique_lock<mutex> lock(m_lock);
	m_listTarget.push_back(stTatget);
	m_llnSizeAll = m_listTarget.size(); //TARGET总数
	return;
}
bool CTarget::Pop() //删除已经执行了的TARGET，减少没必要的比对过程
{
	unique_lock<mutex> lock(m_lock);
	if (m_listTarget.size() < 1) { return false; }
	m_listTarget.pop_front();
	return true;
}
bool CTarget::Front(SCtpTarget& stTarget) //获取最当前需要执行的TARGET
{
	unique_lock<mutex> lock(m_lock);
	if (m_listTarget.size() < 1) { return false; }
	stTarget = m_listTarget.front();
	return true;
}
SCtpTarget CTarget::Current()
{
	unique_lock<mutex> lock(m_lock);
	if (m_listTarget.size() < 1) { return SCtpTarget({ 0 }); }
	return m_listTarget.front();
}
void CTarget::GetTaskRatio(SCtpTaskRatio& stRatio) //获取合约的执行比例，会跟部分风控计数一起写到MDB用于统计参考
{
	m_lock.lock();
	stRatio.nTgtAll = m_llnSizeAll;
	stRatio.nTgtNotDo = m_listTarget.size();
	stRatio.nVlmAll = m_nVolumeAll;
	stRatio.nVlmDone = m_nVolumeDone;
	m_lock.unlock();
	//taregt
	stRatio.nTgtDone = stRatio.nTgtAll - stRatio.nTgtNotDo;
	stRatio.fTgtRatioDone = (stRatio.nTgtDone < 1) ? 0.0f : float(stRatio.nTgtDone) / float(stRatio.nTgtAll);
	stRatio.fTgtRatioNotDo = (stRatio.nTgtNotDo < 1) ? 0.0f : float(stRatio.nTgtNotDo) / float(stRatio.nTgtAll);
	//volume
	stRatio.nVlmNotDo = stRatio.nVlmAll - stRatio.nVlmDone;
	stRatio.fVlmRatioDone = (stRatio.nVlmDone < 1) ? 0.0f : float(stRatio.nVlmDone) / float(stRatio.nVlmAll);
	stRatio.fVlmRatioNotDo = (stRatio.nVlmNotDo < 1) ? 0.0f : float(stRatio.nVlmNotDo) / float(stRatio.nVlmAll);
}
bool CTarget::GetTarget(const CTimeCalc& ctpTimeNT, SCtpTarget& stTarget) //获取最近日期的target
{
	unique_lock<mutex> lock(m_lock);
	int nYMD = ctpTimeNT.GetYMD();
	int nHMSL = ctpTimeNT.GetHMSL();
	bool bOK = false;
	while (m_listTarget.size() > 0)
	{
		SCtpTarget& refTarget = m_listTarget.front();
		if (refTarget.nYMD == nYMD)
		{
			if (refTarget.nHMSL <= nHMSL) //可能的未触发的比较早的
			{
				stTarget = refTarget;
			}
			else //尚未到达触发点的
			{
				goto GET_END;
			}
		}
		else if (refTarget.nYMD > nYMD) //最新尚未到达触发点的
		{
			goto GET_END;
		}
		else //昨天的数据
		{
			stTarget = refTarget;
		}
		m_listTarget.pop_front(); //找到最新
		m_nVolumeDone += abs(stTarget.Position - m_nPositionLast);
		m_nPositionLast = stTarget.Position;
		bOK = true; //有target需要执行
	}
GET_END:
	if (bOK)
	{
		m_listTarget.push_front(stTarget); //保留最近
	}
	return bOK;
}
//CTarget& CTarget::operator=(const CTarget& ctpTarget)
//{
//	unique_lock<mutex> lock(m_lock);
//	//memcpy(InstrumentID, ctpTarget.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	m_listTarget.clear();
//	m_listTarget = ctpTarget.m_listTarget;
//	m_llnSizeAll = ctpTarget.m_llnSizeAll;
//	m_nVolumeAll = ctpTarget.m_nVolumeAll;
//	m_nVolumeDone = ctpTarget.m_nVolumeDone;
//	m_nPositionLast = ctpTarget.m_nPositionLast;
//	return *this;
//}