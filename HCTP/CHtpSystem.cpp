#include "CHtpSystem.h"
#include "CLogThread.h"

#define NTP_HOST_NUM 5
//NTPʱ������ڵ�
const char* g_strNtpHost[NTP_HOST_NUM] = {
"1.cn.pool.ntp.org",
"2.cn.pool.ntp.org",
"3.cn.pool.ntp.org",
"0.cn.pool.ntp.org",
"cn.pool.ntp.org"
//"tw.pool.ntp.org",
//"0.tw.pool.ntp.org",
//"1.tw.pool.ntp.org",
//"2.tw.pool.ntp.org",
//"3.tw.pool.ntp.org",
//"pool.ntp.org",
//"time.windows.com",
//"time.nist.gov",
//"time-nw.nist.gov",
//"asia.pool.ntp.org",
//"europe.pool.ntp.org",
//"oceania.pool.ntp.org",
//"north-america.pool.ntp.org",
//"south-america.pool.ntp.org",
//"africa.pool.ntp.org",
//"ca.pool.ntp.org",
//"uk.pool.ntp.org",
//"us.pool.ntp.org",
//"au.pool.ntp.org"
};
CHtpSystem g_htpSys;

CHtpSystem::CHtpSystem() : m_smTask(5), m_bThRunning(true)
{
    m_ullPingDlay = 0;
    memset(m_ullYdOffset, 0, sizeof(m_ullYdOffset));
    memset(m_ullFixOffset, 0, sizeof(m_ullFixOffset));
    memset(m_ullTimeTick, 0, sizeof(m_ullTimeTick));
	m_vecCmd.clear();
}

CHtpSystem::~CHtpSystem()
{
    Stop();
}
void CHtpSystem::Stop()
{
	m_bThRunning = false;
	m_smTask.Release();
	if (m_pthTask) {
		m_pthTask->join();
		delete m_pthTask;
        m_pthTask = NULL;
	}
}
void CHtpSystem::Start(shared_ptr<const CUserConfig> Config)
{
    m_Config = Config; 
    AddCmd(Config->GetCmd());
    if (!m_pthTask)
        m_pthTask = new thread(&CHtpSystem::Run, this);
    m_smTask.Release();
}
//���ر���CMD�б�����֧�ֶ�ʱִ��CMD�������ping��˿��ӳ٣��ɽ�ϵ��������߲�ѯ����ʾ�����Ϣ
bool CHtpSystem::AddCmd(string& strCmd)
{
    SHtpCmd stCmd;
    size_t nFind = strCmd.find(".exe ");
    if (0 < nFind)
    {
        stCmd.strFile = string(m_Config->GetExeFolder()).append("\\").append(strCmd.substr(0, nFind + 5));
        stCmd.strParam = strCmd.substr(nFind + 5);
    }
    stCmd.strCmdLine.append("/C ").append(m_Config->GetExeFolder()).append("\\").append(strCmd)
        .append((m_vecCmd.size() > 0) ? ">" : ">").append(m_Config->GetExeFolder()).append("\\Cmd.txt");
	m_cthLock[CTP_TIME_DEFAULT].lock();
	m_vecCmd.push_back(stCmd);
    m_cthLock[CTP_TIME_DEFAULT].unlock();
	return true;
}
//��ȡCMD����֮���������Ϣ��������ʾ
bool CHtpSystem::GetCmd(string& strText)
{
	ifstream ifCmd(m_Config->GetExeFolder().append("\\Cmd.txt").c_str());
	string strLine;
	if (!ifCmd.is_open())
	{
		return false;
	}
	while (getline(ifCmd, strLine))
	{
		strText.append(strLine);
	}
	ifCmd.close();
	return true;
}
void CHtpSystem::Run()
{
	HTP_LOG << "CHtpSystem::Run" << HTP_ENDL;
	string strText;
	static int nCmdIdx = 0;
    unsigned int nTickCnt[5] = { 0 };
    ULONGLONG ullTickTime = GetTickCount64();
	while (m_bThRunning) //��ѭ��
	{
        ullTickTime = GetTickCount64() - ullTickTime+1;
        if (1000 > ullTickTime)
		    m_smTask.Wait(DWORD(1000 - ullTickTime));
        ullTickTime = GetTickCount64();
        if (++nTickCnt[0] > 0) //1Sˢ��һ��ʱ��
        {
            nTickCnt[0] = 0;
            SYSTEMTIME sysTime;
            GetLocalTime(&sysTime);
            SetTime(CTP_TIME_SYS, CTimeCalc(sysTime));
            string strTime("");
            strTime.append("TD-").append(GetTime(CTP_TIME_TRADING).GetStrYMD()).append(" ");
            strTime.append("ND-").append(GetTime(CTP_TIME_NATURE).GetStrYMD()).append(" ");
            for (int i = 0; i < CTP_TIME_ENDF; i++)
            {
                //strTime.append(g_strCtpTime[i]).append("-").append(GetTime(eCtpTime(i)).GetStrHMSL()).append(" ");
                strTime.append(g_strCtpTime[i]).append("-").append(GetTime(eCtpTime(i)).GetStrHMSL()).append(" ");
            }
            HTP_LOG << HTP_MSG(strTime, (HTP_CID_OK | HTP_CID_USE_LOGCOUT | 121314));
        }
        if (++nTickCnt[1] > 60) //60S
        {
            nTickCnt[1] = 0;
            SynNetTime(); //��ʱͬ��NTPʱ��
        }
        if (++nTickCnt[2] > 20)
        {
            nTickCnt[2] = 0;
            strText.clear();
            if (GetCmd(strText)) //��ȡ�ӳ�
            {
                string strPing = string(strText.substr(0, strText.find("ms") + 2));
                HTP_LOG << (HTP_MSG(strPing, HTP_CID_ISASCII)); //��ӡCMD������Ϣ
                if (strPing.find("=") > 0)
                {
                    string strDalay = string(strPing.substr(strPing.find("=") + 1, strPing.find("ms"))).c_str();
                    m_ullPingDlay = atoi(strDalay.c_str());
                }
            }
            //windows������÷���
            ShellExecute(NULL, "open", "cmd", m_vecCmd[nCmdIdx].strCmdLine.c_str(), NULL, SW_HIDE);
            if (++nCmdIdx >= m_vecCmd.size())
            {
                nCmdIdx = 0;
            }
        }
	}
	HTP_LOG << "CHtpSystem::EXIT" << HTP_ENDL;
}
DWORD CHtpSystem::GetPingMS()
{
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    for (auto& itCmd : m_vecCmd)
    {
        if (itCmd.strFile.empty() || itCmd.strParam.empty())
        {
            continue; //������CMD�����򲻽�����ͨ�Լ�飬���ܵ��·ǽ����յ�¼����
        }
        ShExecInfo.lpParameters = itCmd.strParam.c_str();
        ShExecInfo.lpFile = itCmd.strFile.c_str();//m_vecCmd[nCmdIdx].c_str();
        ShellExecuteEx(&ShExecInfo);
        WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        DWORD dwRtnCode = 0;
        GetExitCodeProcess(ShExecInfo.hProcess, &dwRtnCode);
        char szCode[32] = { 0 };
        sprintf_s(szCode, "%ld", dwRtnCode);
        HTP_LOG << HTP_MSG(string("CHtpSystem::GetPingMS.dwRtnCode=").append(szCode)
            , ((dwRtnCode==0)? HTP_CID_OK :HTP_CID_FAIL) | dwRtnCode);
        if (dwRtnCode != 0) { return dwRtnCode; }
    }
    return 0;
}
//��ȡ���µ�ʱ��=�ɼ�ʱ��+����ƫ��
//bool CHtpSystem::GetLatest(const eCtpTime& eTime, CTimeCalc& ctpTime)
//{
//    CTPTIME nMinute = 0;
//    CTPTIME nSecond = 0;
//    CTPTIME nMilSec = 0;
//    CTPTIME nTimeChange = 0;
//    if (eTime == CTP_TIME_NET)
//    {
//        m_cthLock[CTP_TIME_NET].lock();
//        ctpTime = m_cTime[CTP_TIME_NET];
//        nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[CTP_TIME_NET]);
//        m_cthLock[CTP_TIME_NET].unlock();
//    }
//    else
//    {
//#ifdef _DEBUG_TIME
//        ctpTime = m_cTime[eTime];
//#else
//        m_cthLock[eTime].lock(); m_cthLock[CTP_TIME_NET].lock();
//        CTPTIME nHMSX = abs(m_cTime[eTime].GetTickHMS() - m_cTime[CTP_TIME_NET].GetTickHMS()); //��ȡ�����
//        if ((nHMSX < 300) || ((nHMSX > 86100) && (nHMSX < 86401)))//���300S/5�������ڣ��϶�TICKʱ����Ч
//        {
//            ctpTime = m_cTime[eTime];
//            //ϵͳTICK��+PING�ӳ�+�̶�ƫ��
//            nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[eTime] + m_ullPingDlay + m_ullFixOffset[eTime]);
//        }
//        else //����5����,��ʹ��1����ͬ��һ�ε�����ʱ��
//        {
//            ctpTime = m_cTime[CTP_TIME_NET];
//            //ϵͳTICK��+PING�ӳ�+�̶�ƫ�� + ��������ʱ����ý�������ƽ��ʱ���
//            nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[CTP_TIME_NET] + m_ullPingDlay + m_ullFixOffset[eTime] + m_ullYdOffset[eTime]);
//        }
//        m_cthLock[eTime].unlock(); m_cthLock[CTP_TIME_NET].unlock();
//#endif // DEBUG
//    }
//    if (nTimeChange > 59999) { nMinute = nTimeChange / 60000; nTimeChange %= 60000; }
//    if (nTimeChange > 999) { nSecond = nTimeChange / 1000; nTimeChange %= 1000; }
//    nMilSec = nTimeChange;
//    ctpTime.m_nMilSec += nMilSec;
//    if (ctpTime.m_nMilSec > 999) { ctpTime.m_nMilSec -= 1000; ctpTime.m_nSecond++; }
//    ctpTime.m_nSecond += nSecond;
//    if (ctpTime.m_nSecond > 59) { ctpTime.m_nSecond -= 60; ctpTime.m_nMinute++; }
//    ctpTime.m_nMinute += nMinute;
//    if (ctpTime.m_nMinute > 59) { ctpTime.m_nMinute -= 60; ctpTime.GetNextHour(); }
//    return true;
//}
//��ȡ���µ�ʱ��=�ɼ�ʱ��+����ƫ��
CTimeCalc CHtpSystem::GetTime(const eCtpTime& eTime)
{
    CTPTIME nMinute = 0;
    CTPTIME nSecond = 0;
    CTPTIME nMilSec = 0;
    CTPTIME nTimeChange = 0;
    CTimeCalc ctpTime;
    if (eTime == CTP_TIME_NET)
    {
        m_cthLock[CTP_TIME_NET].lock();
        ctpTime = m_cTime[CTP_TIME_NET];
        //ϵͳTICK��+PING�ӳ�+�̶�ƫ�� + ��������ʱ����ý�������ƽ��ʱ���
        nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[CTP_TIME_NET]);
        m_cthLock[CTP_TIME_NET].unlock();
    }
    else if (eTime == CTP_TIME_TRADING)
    {
        m_cthLock[CTP_TIME_TRADING].lock();
        ctpTime = m_cTime[CTP_TIME_TRADING];
        m_cthLock[CTP_TIME_TRADING].unlock();
        return ctpTime;
    }
    else
    {
#ifdef _DEBUG_TIME
        ctpTime = m_cTime[eTime];
#else
        m_cthLock[eTime].lock(); m_cthLock[CTP_TIME_NET].lock();
        CTPTIME nHMSX = abs(m_cTime[eTime].GetTickHMS() - m_cTime[CTP_TIME_NET].GetTickHMS());
        if ((nHMSX < 600) || ((nHMSX > 86100) && (nHMSX < 86401)))//���600S/10�������ڣ��϶�TICKʱ����Ч
        {
            ctpTime = m_cTime[eTime];
            //ϵͳTICK��+PING�ӳ�+�̶�ƫ��
            nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[eTime] + m_ullPingDlay + m_ullFixOffset[eTime]);
        }
        else //����5����,��ʹ��1����ͬ��һ�ε�����ʱ��
        {
            ctpTime = m_cTime[CTP_TIME_NET];
            //ϵͳTICK��+PING�ӳ�+�̶�ƫ�� + ��������ʱ����ý�������ƽ��ʱ���
            nTimeChange = CTPTIME(GetTickCount64() - m_ullTimeTick[CTP_TIME_NET] + m_ullPingDlay + m_ullFixOffset[eTime] + m_ullYdOffset[eTime]);
        }
        m_cthLock[eTime].unlock(); m_cthLock[CTP_TIME_NET].unlock();
#endif // DEBUG
    }
    if (nTimeChange > 59999) { nMinute = nTimeChange / 60000; nTimeChange %= 60000; }
    if (nTimeChange > 999) { nSecond = nTimeChange / 1000; nTimeChange %= 1000; }
    nMilSec = nTimeChange;
    ctpTime.m_nMilSec += nMilSec;
    if (ctpTime.m_nMilSec > 999) { ctpTime.m_nMilSec -= 1000; ctpTime.m_nSecond++; }
    ctpTime.m_nSecond += nSecond;
    if (ctpTime.m_nSecond > 59) { ctpTime.m_nSecond -= 60; ctpTime.m_nMinute++; }
    ctpTime.m_nMinute += nMinute;
    if (ctpTime.m_nMinute > 59) { ctpTime.m_nMinute -= 60; ctpTime.GetNextHour(); }
    return ctpTime;
}
//CTPTIME CHtpSystem::GetLatestHMSL(const eCtpTime& eTime)
//{
//    return GetLatest(eTime).GetHMSL();
//}
//string CHtpSystem::GetStrLatestHMSL(const eCtpTime& eTime)
//{
//    return GetLatest(eTime).GetStrHMSL();
//}
//����TICKʱ�䣬���ڲ�ͬ��������TICK�Ľ�����������Ȼ���ڵ�����
bool CHtpSystem::SetTime(shared_ptr<const CCtpTick> ctpTick)
{
    if (ctpTick->m_stTick.OpenInterest < 10000) { return false; } //�ֲ���̫�ٲ�����
    m_cthLock[CTP_TIME_NATURE].lock();
    CTimeCalc ctNT = m_cTime[CTP_TIME_NATURE]; //��Ȼ�ղο�����ʱ��
    m_cthLock[CTP_TIME_NATURE].unlock();
    if (ctNT.m_nYear < 2000) { return false; }
    CTPTIME nHourX = abs(ctpTick->m_ctNature.m_nHour - ctNT.m_nHour);
    //������,TICK������ʱ�Ӳ���������Ҳ���Ƕ���Ϊһ��Сʱ������
    if ((nHourX != 0) && (nHourX != 1) && (nHourX != 23)) //����Ȼ�յ�ʱ��������Ϊ����TICK������������ʱ������
    {
        return false;
    }
    eCtpTime eTime = GetExID(ctpTick->m_stTick.ExchangeID);
    if (eTime == CTP_TIME_ENDF) { return false; }
    CTimeCalc cTime(ctpTick->m_stTick.TradingDay, ctpTick->m_stTick.UpdateTime, ctpTick->m_stTick.UpdateMillisec);
    if (cTime.m_nYear < 2000) { return false; }
    SetTime(eTime, cTime);
    //�������������Ϊ��д����������ȷ��ʱ����
    if ((eTime== CTP_TIME_DCE || eTime == CTP_TIME_SHFE)
        &&(cTime.m_nDay != m_cTime[CTP_TIME_TRADING].m_nDay)
        && (cTime.m_nDay >= ctNT.m_nDay))  //������������ڵ�����Ȼ��
    {
        SetTime(CTP_TIME_TRADING, cTime); //���ô���������������������
    }
    return true;
}
//ʱ��������������ͬ����������Ȼ�����뽻�������ֶ�
bool CHtpSystem::TimeStand(shared_ptr<CCtpTick> ctpTick) //ʱ�����
{
    m_cthLock[CTP_TIME_NATURE].lock();
    CTimeCalc ctNT = m_cTime[CTP_TIME_NATURE]; //��Ȼ�ղο�����ʱ��
    m_cthLock[CTP_TIME_NATURE].unlock();
    if (ctNT.m_nYear < 2000) { return false; }
    CTPTIME nHourX = ctpTick->m_ctNature.m_nHour - ctNT.m_nHour;
    //������,TICK������ʱ�Ӳ���������Ҳ���Ƕ���Ϊһ��Сʱ������
    if (nHourX == 23) //TICK�ͺ�����ʱ�ӳ�ǰ
    {
        ctNT.GetPrevDay();// ������Ȼ��=����-1��
    }
    else if (nHourX == -23) //TICK��ǰ������ʱ���ͺ�
    {
        ctNT.GetNextDay();// ������Ȼ��=����+1��
    }
    else if (nHourX < -1 || nHourX > 1) //����1��Сʱ����Ϊ���ڵ�TICK
    {
        return false;
    }
    //ctNT.GetCurDay(ctpTick->m_stTick.ActionDay);
    ctpTick->m_ctNature.m_nYear = ctNT.m_nYear;
    ctpTick->m_ctNature.m_nMonth = ctNT.m_nMonth;
    ctpTick->m_ctNature.m_nDay = ctNT.m_nDay;
    m_cthLock[CTP_TIME_TRADING].lock();
    CTimeCalc ctTD = m_cTime[CTP_TIME_TRADING]; //��Ȼ�ղο�����ʱ��
    m_cthLock[CTP_TIME_TRADING].unlock();
    if (ctTD.m_nYear < 2000) { return false; }
    ctpTick->m_ctTrading.m_nYear = ctTD.m_nYear;
    ctpTick->m_ctTrading.m_nMonth = ctTD.m_nMonth;
    ctpTick->m_ctTrading.m_nDay = ctTD.m_nDay; // ��ȡ������=������
    //ctTD.GetCurDay(ctpTick->m_stTick.TradingDay);
    return true;
}
//ͬ��NTP����ʱ�䣬����ʱ��ͬ�����߶�ʱ�������н���
bool CHtpSystem::SynNetTime() //ͬ��ʱ��
{
	x_int32_t xit_err = -1;
	x_ntp_time_context_t xnpt_timec;
	x_uint64_t xut_timev = 0ULL;
	for (int i = 0; i < NTP_HOST_NUM; i++)
	{
		xut_timev = 0ULL;
		xit_err = ntp_get_time(g_strNtpHost[i], NTP_PORT, 2000, &xut_timev);
		if (0 == xit_err)
		{
			ntp_tmctxt_bv(xut_timev, &xnpt_timec);   // ת���� ��-��-��_ʱ-��-��.���� ��ʱ����Ϣ
            SetTime(CTP_TIME_NET, CTimeCalc(xnpt_timec));
            SetTime(CTP_TIME_NATURE, CTimeCalc(xnpt_timec));
			HTP_LOG << HTP_MSG(string(g_strNtpHost[i]) + CTimeCalc(xnpt_timec).GetStrYMDHMSL(), HTP_CID_OK);  // ���ʱ����Ϣ
            return true;
		}
		else
		{
			// ����ʧ�ܣ���������ΪӦ��ʱ......
			HTP_LOG << HTP_CMSG(string("ͬ��ʱ��ʧ�ܣ�").append(g_strNtpHost[i]), HTP_CID_FAIL);
		}
	}
    return false;
}
void CHtpSystem::SetTime(const eCtpTime& eTime, const CTimeCalc& ctpTime)
{
    m_cthLock[eTime].lock();
    m_cTime[eTime] = ctpTime;
    m_ullTimeTick[eTime] = GetTickCount64();
    m_cthLock[eTime].unlock();
}
//CTimeCalc CHtpSystem::GetTime(const eCtpTime& eTime)
//{
//    unique_lock<mutex> lock(m_cthLock[eTime]);
//    return m_cTime[eTime];
//}
