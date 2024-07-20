#include "CRiskThread.h"
#include "CLogThread.h"



CRiskThread::CRiskThread()
{
	m_mapRiskCtrl.clear();
	m_mapRiskCnt.clear();
	m_Config = NULL;
	for (int i = 0; i < REN_ENDF; i++)
	{
		m_bEnable[i] = false; //Ĭ�Ϲر����й���
	}
	for (int i = 0; i < RSC_ENDF; i++)
	{
		m_nSysCnt[i] = 0;
	}
	m_stLimit.nOrderDaySum = 1000; //�ۼƹҵ�����
	m_stLimit.nCancelDaySum = 100; //�ۼƳ�������
	m_stLimit.nOrderMinSum = 100; //�ۼƷ���ί������
	m_stLimit.nOrderVolMax = 1000; //���ʱ����������
	m_stLimit.nLongPosMax = 1000; //��ͷ���ֲ�
	m_stLimit.nShortPosMax = 1000; //��ͷ���ֲ�
}
CRiskThread::~CRiskThread() {}
void CRiskThread::Start(shared_ptr<const CUserConfig> Config) { m_Config = Config; }
bool CRiskThread::GetEnable(eRiskEnable eType) const { return (m_bEnable[eType] && m_bEnable[REN_GLOBAL]); }
atomic_int& CRiskThread::SysCnt(eRiskSysCnt eType) { return m_nSysCnt[eType]; }
atomic_bool& CRiskThread::Enable(eRiskEnable eType) { return m_bEnable[eType]; }
void CRiskThread::SetEnable(eRiskEnable eType, bool bEnable) { m_bEnable[eType] = bEnable; }
//������
SCtpRiskCtrl& CRiskThread::Ctrl(const CFastKey& cKey)
{
	m_cthLock.lock_shared();
	auto it = m_mapRiskCtrl.find(cKey);
	m_cthLock.unlock_shared();
	if (it == m_mapRiskCtrl.end())
		unique_lock<shared_mutex> lock(m_cthLock); //д��
	else
		shared_lock<shared_mutex> lock(m_cthLock); //����
	return m_mapRiskCtrl[cKey];
}
//������
SCtpRiskLimit& CRiskThread::Limit()
{
	shared_lock<shared_mutex> lock(m_cthLock); //����
	return m_stLimit;
}
//������
SCtpRiskCnt& CRiskThread::Cnt(const CFastKey& cKey) //�ṩ���޸ļ������ײ�Ϊ����ͳһΪԭ�Ӳ���
{
	m_cthLock.lock_shared();
	auto it = m_mapRiskCnt.find(cKey);
	m_cthLock.unlock_shared();
	if (it == m_mapRiskCnt.end())
		unique_lock<shared_mutex> lock(m_cthLock); //д��
	else
		shared_lock<shared_mutex> lock(m_cthLock); //����
	return m_mapRiskCnt[cKey];
}
//��ؼ��
bool CRiskThread::Check(shared_ptr<const CThostFtdcInputOrderField> stOrder)
{
	char szMsg[256] = { 0 };
	sprintf_s(szMsg, "->CRiskThread::Check::stOrder.LimitPrice[%lf] ", stOrder->LimitPrice);
	if (CTP_PRICE_ISOUT(stOrder->LimitPrice)) //�����۸��쳣���������µ�
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
		return true; //û�п���Լ��
	m_cthLock.lock_shared();
	SCtpRiskCtrl& stRiskCtrl = m_mapRiskCtrl[stOrder->InstrumentID]; //�ṹ�嵥����������ԭ�ӵ�
	SCtpRiskCnt& stRiskCnt = m_mapRiskCnt[stOrder->InstrumentID]; //�ṹ�嵥����������ԭ�ӵ�
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
	if (stRiskCnt.nOrderMinSum > m_stLimit.nOrderMinSum)//�������
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
	if (stRiskCnt.nCancelDaySum > m_stLimit.nCancelDaySum)//�������
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
	if (stRiskCnt.nOrderDaySum > m_stLimit.nOrderDaySum)//�������
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
	if (stOrder->VolumeTotalOriginal > m_stLimit.nOrderVolMax)//�������
	{
		HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
			.append(szMsg).append("FAIL"), HTP_CID_FAIL | 7878);
		return false;
	}
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
		.append(szMsg).append("OK"), HTP_CID_OK);
#endif // _SHOW_HTS
	//if (stRiskCnt.nLongPosMax > m_stLimit.nLongPosMax)//�������
	//{
	//	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
	//		.append("->CRiskThread::Check nLongPosMax"), HTP_CID_FAIL | 7878);
	//	return false;
	//}
	//if (stRiskCnt.nShortPosMax > m_stLimit.nShortPosMax)//�������
	//{
	//	HTP_LOG << HTP_MSG(string(stOrder->InstrumentID).append(" ").append(stOrder->OrderRef)
	//		.append("->CRiskThread::Check nShortPosMax"), HTP_CID_FAIL | 7878);
	//	return false;
	//}
	return true;
}