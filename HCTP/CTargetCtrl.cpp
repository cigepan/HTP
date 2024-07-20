//TARGET���׼ƻ�����
//֧�ֶ��ֽ���ģʽ

#include "CTargetCtrl.h"

const char g_strAlgoMD[AGMD_ENDF][32] = {
	"twapAD", "twapK", "openK", "closeK", "preOpen"
};
////TARGET����ʱ��ȶ�
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
//����TARGET�õ��ģ����ַ�������Ϊ��ֵ��Ŀ�����������������ıȶ�Ч��
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
	stTarget.nHMSL *= 1000; //����
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
	m_nFreqMS = 0; //���Ƶ��
	m_dbTimeAdj = 0.0; //ʱ��ƫ����Ӧ����
	m_dbVolK = 0.5; //��������Ӧ����K
	m_dbVolPowK = 1.0; //��������Ӧ����������K
	m_volLastQty = 0; //���³ֲ�
	m_volTarget = 0; //Ŀ��ֲ�
	m_volTradeQty = 0; //������
	m_ctmCurrent.Clear(); //��ǰʱ��
	m_ctmTD.Clear(); //������
	m_ctmTdStart.Clear(); //���Կ�ʼʱ��
	m_ctmTdEnd.Clear(); //���Խ���ʱ��
	m_vecTimeTD.clear();
	m_strID.clear(); //��ԼID
	m_strExchange.clear(); //��Լ������
	m_strClass.clear(); //��Ʒ����(��ʽ����д)
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
CHtpMsg CTarget::GetTimeList(list<CTimeCalc>& listTimeTD) //��ȡʱ��ڵ�
{
	listTimeTD.clear();
	if (m_nFreqMS < 100) 
	{ return CHtpMsg("CTarget::GetTimeList m_nFreqMS < 100", HTP_CID_FAIL | 5355); }
	vector<CTimeCalc> vecTimeDay;
	vecTimeDay.clear();
	CTimeCalc ctmTD = m_ctmTD; //������
	CTimeCalc ctmTdStart = (m_ctmTdStart.m_nYear > 2020) ? m_ctmTdStart : m_ctmCurrent; //���Կ�ʼʱ��
	CTimeCalc ctmCurrent = m_ctmCurrent;
	CTimeCalc ctmPrevDay = ctmCurrent; //��ǰ�յ�ǰһ��
	ctmPrevDay.GetPrevDay(); //��λ������
	CTimeCalc ctmNextDay = ctmCurrent; //��ǰ�յ�ǰһ��
	ctmNextDay.GetNextDay(); //��λ������
	if (ctmCurrent.GetYMD() > ctmTD.GetYMD()) //���Կ�ʼʱ���쳣
	{
		return CHtpMsg("CTarget::GetTimeList CURRENT DAY > TRADE DAY", HTP_CID_FAIL | 5355);
	}
	if (ctmTdStart.GetYMD() > ctmTD.GetYMD()) //���Կ�ʼʱ���쳣
	{
		return CHtpMsg("CTarget::GetTimeList START DAY > TRADE DAY", HTP_CID_FAIL | 5355);
	}
	//���Ͼ��� = ����ǰ��������
	CTimeCalc ctmClock = m_vecTimeTD[0].ctmStart;
	ctmClock.GetPrevMinute();
	ctmClock.GetPrevMinute();
	ctmClock.GetPrevMinute();
	//û�н������ڵĵ������ᱻִ�е������ʱ��ڵ����
	if (ctmCurrent.GetYMD() < ctmTD.GetYMD())//���̲�����Ҫִ�����������
	{
		ctmClock.SetYMD(ctmCurrent.GetYMD());
	}
	else 
	{
		ctmClock.SetYMD(ctmPrevDay.GetYMD()); //����Ϊ��ȥ��ʱ��
	}
	//if (ctmTdStart.GetYMD() < ctmClock.GetYMD()) //���Կ�ʼʱ���쳣
	//{
	//	return CHtpMsg("CTarget::GetTimeList TARGET START TIME IS OUT", HTP_CID_FAIL | 5355);
	//}
	vecTimeDay.push_back(ctmClock); //���Ͼ��� = ����ǰ�������� 57/27
	//���н��׽ڵ�
	CTimeCalc ctmFreq; //ʱ���ȼ���
	int nFreqMS = m_nFreqMS; //�ڵ�Ƶ��
	//����ʱ����
	ctmFreq.m_nHour = nFreqMS / 3600000; nFreqMS %= 3600000;
	ctmFreq.m_nMinute = nFreqMS / 60000; nFreqMS %= 60000;
	ctmFreq.m_nSecond = nFreqMS / 1000; nFreqMS %= 1000;
	ctmFreq.m_nMilSec = nFreqMS;
	//����ʱ����Ӧ�����
	CTimeCalc ctmAdj;
	int nTimeAdj = int(double(m_nFreqMS) * double((m_dbTimeAdj > 0.0f) ? m_dbTimeAdj : 1.0 + m_dbTimeAdj));
	ctmAdj.m_nHour = nTimeAdj / 3600000; nTimeAdj %= 3600000;
	ctmAdj.m_nMinute = nTimeAdj / 60000; nTimeAdj %= 60000;
	ctmAdj.m_nSecond = nTimeAdj / 1000; nTimeAdj %= 1000;
	ctmAdj.m_nMilSec = nTimeAdj;
	for (const auto itTime : m_vecTimeTD)
	{
		ctmClock = itTime.ctmStart;
		if (ctmClock.m_nHour > 20) //����ʱ��
		{
			//û�н������ڵĵ������ᱻִ�е������ʱ��ڵ����
			if (ctmCurrent.GetYMD() < ctmTD.GetYMD())
			{
				if (ctmCurrent.m_nHour < 6)
					ctmClock.SetYMD(ctmPrevDay.GetYMD()); //��������
				else
					ctmClock.SetYMD(ctmCurrent.GetYMD()); //��������
			}
			else
			{
				//����û��������Ϊ��ȥ��ʱ�䣬���ڲ�Ӱ��TARGET������ƻ�ִ�У�ֻ��ʱ���Ϲ�ȥ��Ҳ�Ͳ��ᱻִ�е�
				ctmClock.SetYMD(ctmPrevDay.GetYMD()); 
			}
		}
		else if (ctmClock.m_nHour < 6) //�賿ʱ��
		{
			//û�н������ڵĵ������ᱻִ�е������ʱ��ڵ����
			if (ctmCurrent.GetYMD() < ctmTD.GetYMD())
			{
				if (ctmCurrent.m_nHour < 6)
					ctmClock.SetYMD(ctmCurrent.GetYMD()); //��������
				else
					ctmClock.SetYMD(ctmNextDay.GetYMD()); //��������
			}
			else 
			{
				//����û��������Ϊ��ȥ��ʱ�䣬���ڲ�Ӱ��TARGET������ƻ�ִ�У�ֻ��ʱ���Ϲ�ȥ��Ҳ�Ͳ��ᱻִ�е�
				ctmClock.SetYMD(ctmCurrent.GetYMD());
			}
		}
		else //����
		{
			ctmClock.SetYMD(ctmTD.GetYMD()); //����ʱ��=������
		}
		//ʱ����Ӧ
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
		//ѭ����ȡ����ʱ��ڵ�
		while ((ctmClock.GetHMS() >= itTime.ctmStart.GetHMS())
			&& (ctmClock.GetHMS() < itTime.ctmEnd.GetHMS())) //����ʱ����
		{
			vecTimeDay.push_back(ctmClock);
			//���ʱ��
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
	
	//ʱ�����
	int nIndex = 0;
	//��Ӽ��Ͼ���ʱ��ڵ�
	if (m_bEnable[AGMD_PREOPEN])   
	{
		listTimeTD.push_back(vecTimeDay[nIndex]);
	} 
	nIndex++;
	//����ʱ�䣺���ٸ�freqʱ��
	int nFreqCnt = min(int(vecTimeDay.size()), m_nParamK[AGMD_OPENK]);
	if (m_bEnable[AGMD_OPENK])  
	{
		for (; nIndex < nFreqCnt; nIndex++)
		{
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	//�ӵ�ǰʱ��㿪ʼ������
	if (m_bEnable[AGMD_TWAPAD]) 
	{
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			if ((ctmCurrent.m_nYear < 2000)
				|| (ctmCurrent.GetYMDHMS() > vecTimeDay[nIndex].GetYMDHMS())) {
				continue; //�������ʱ���򶪵�
			} 
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	//�ӵ�ǰʱ��㿪ʼ���N��Freq
	if (m_bEnable[AGMD_TWAPK]) 
	{
		nFreqCnt = m_nParamK[AGMD_TWAPK];
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			if ((ctmCurrent.m_nYear < 2000)
				|| (ctmCurrent.GetYMDHMS() > vecTimeDay[nIndex].GetYMDHMS())) {
				continue; //�������ʱ���򶪵�
			} 
			listTimeTD.push_back(vecTimeDay[nIndex]);
			if (--nFreqCnt < 1) { break; }
		}
	}
	//����ǰ��N��
	if (m_bEnable[AGMD_CLOSEK]) 
	{
		nFreqCnt = m_nParamK[AGMD_CLOSEK];
		nIndex = max(int(vecTimeDay.size() - nFreqCnt), nIndex); //���Ѿ���ȥ�˵�����
		for (; nIndex < vecTimeDay.size(); nIndex++)
		{
			listTimeTD.push_back(vecTimeDay[nIndex]);
		}
	}
	listTimeTD.sort(TimeCompare); //����ʱ��
	return CHtpMsg(HTP_CID_OK);
}

CHtpMsg CTarget::Create(document& docPlan) //ִ��TARGET����
{
	char szKey[32];
	unique_lock<mutex> lock(m_lock);
	list<CTimeCalc> listTimeTD; //��ȡ����ʱ��ڵ㣬ȷ��������ʱ���
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
	double dbPosDiv = 0.0; //����������λ
	if (m_volTradeQty != 0)
	{
		dbPosDiv = double(abs(m_volTradeQty)) / double(listTimeTD.size());
	}
	double dbPosAdd = 0.0; //��֤��һ���н���
	double dbVolumeAdj = 0.0;  //����Ӧ����
	double dbVolume = 0.0; //������������ȡ��
	stTarget.Position = m_volLastQty;
	m_listTarget.clear(); //ȫ��TWAP��ֱ��
	size_t nIdx = 0;
	docPlan.clear();
	int nSkipTime = CTC_PUSH_NOTGT_TIME / m_nFreqMS; //�뼶��Ϊ�˱������ɹ���û��Ҫ�ģ��������ֲ�һ�µ������10Sһ���ظ�
	int nTimeCnt = 0;
	for (auto& itTD : listTimeTD)
	{
		stTarget.nYMD = itTD.GetYMD();
		stTarget.nHMSL = itTD.GetHMSL();
		dbPosAdd += dbPosDiv;
		dbVolumeAdj = m_dbVolK * pow((float)(++nIdx / listTimeTD.size()), m_dbVolPowK);
		dbVolume = dbPosAdd + dbVolumeAdj + 0.000001; //������룬����
		if ((dbVolume > 1.0) || (nIdx == 1)) //ȷ����һ�������н���
		{
			nTimeCnt = nSkipTime + 1; //�н��׵������ȷ����TARGET
			TThostFtdcVolumeType nDoVolume = 0;
			if (dbVolume < 1.0) //��֤��һ���н���
			{
				nDoVolume = 1;
				dbPosAdd = 0.0;
			}
			else
			{
				nDoVolume = (TThostFtdcVolumeType)(dbVolume);
				dbPosAdd -= double(nDoVolume);
			}

			if (m_volTradeQty > 0) //����
			{
				stTarget.Position += nDoVolume;
				if (stTarget.Position > m_volTarget) 
				{ stTarget.Position = m_volTarget; } //ȷ����Խ��
			}
			else if (m_volTradeQty < 0)//����
			{
				stTarget.Position -= nDoVolume;
				if (stTarget.Position < m_volTarget) 
				{ stTarget.Position = m_volTarget; } //ȷ����Խ��
			}
		}
		if (++nTimeCnt > nSkipTime) //û�н��׵������10Sһ���ظ���target��������ʵʱͬ���ֲ�
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
	//ȷ�����һ�����׵�
	if (m_listTarget.back().Position != m_volTarget)
	{
		m_listTarget.back().Position = m_volTarget;
	}
	//m_listTarget.sort(TargetCompare); //������Ȼ����������
	return CHtpMsg(HTP_CID_OK);
}

void CTarget::Push(const SCtpTarget& stTatget)
{
	unique_lock<mutex> lock(m_lock);
	m_listTarget.push_back(stTatget);
	m_llnSizeAll = m_listTarget.size(); //TARGET����
	return;
}
bool CTarget::Pop() //ɾ���Ѿ�ִ���˵�TARGET������û��Ҫ�ıȶԹ���
{
	unique_lock<mutex> lock(m_lock);
	if (m_listTarget.size() < 1) { return false; }
	m_listTarget.pop_front();
	return true;
}
bool CTarget::Front(SCtpTarget& stTarget) //��ȡ�ǰ��Ҫִ�е�TARGET
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
void CTarget::GetTaskRatio(SCtpTaskRatio& stRatio) //��ȡ��Լ��ִ�б�����������ַ�ؼ���һ��д��MDB����ͳ�Ʋο�
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
bool CTarget::GetTarget(const CTimeCalc& ctpTimeNT, SCtpTarget& stTarget) //��ȡ������ڵ�target
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
			if (refTarget.nHMSL <= nHMSL) //���ܵ�δ�����ıȽ����
			{
				stTarget = refTarget;
			}
			else //��δ���ﴥ�����
			{
				goto GET_END;
			}
		}
		else if (refTarget.nYMD > nYMD) //������δ���ﴥ�����
		{
			goto GET_END;
		}
		else //���������
		{
			stTarget = refTarget;
		}
		m_listTarget.pop_front(); //�ҵ�����
		m_nVolumeDone += abs(stTarget.Position - m_nPositionLast);
		m_nPositionLast = stTarget.Position;
		bOK = true; //��target��Ҫִ��
	}
GET_END:
	if (bOK)
	{
		m_listTarget.push_front(stTarget); //�������
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