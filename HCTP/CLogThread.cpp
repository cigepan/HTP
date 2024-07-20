#include "CLogThread.h"
#include <io.h>
#include <direct.h>

CLogThread g_Log;
CLogThread::CLogThread() : m_smTask(5), m_cqLOG(0x03FF)
{
	m_bThRunning = true;
	m_llnOneIDX = m_llnOneIDY = m_llnApdIDX = m_llnApdIDY = 0ULL;
	memset(m_szOneMsg, 0, (HTP_ONEMSG_SIZEX + 4) * (HTP_MSG_SIZEY + 4));
	memset(m_szApdMsg, 0, (HTP_APDMSG_SIZEX + 4) * (HTP_MSG_SIZEY + 4));
}
CLogThread::~CLogThread() 
{
	Stop();
}
void CLogThread::Start(shared_ptr<const CUserConfig> Config)
{
	m_Config = Config;
	if(!m_pthTask)
		m_pthTask = new thread(&CLogThread::Run, this);
}
void CLogThread::Stop()
{
	m_bThRunning = false; //原子操作不加锁，设置停止运行标志，线程不会马上停止，需要等待队列任务执行完后
	m_smTask.Release();
	if (m_pthTask)
	{
		m_pthTask->join(); //等待线程退出
		delete m_pthTask; //释放线程空间
		m_pthTask = NULL;
	}
}
CLogThread& CLogThread::operator << (CHtpMsg& stMsg)
{
	*this << HTP_CMSG(stMsg.m_strMsg, stMsg.m_CID); return *this;
}
CLogThread& CLogThread::operator << (SHtpMsg& stMsg)
{
	try 
	{
		size_t IDY = 0;
		while (stMsg.SIZE > 0)
		{
			size_t IDX = (++m_llnOneIDX) & HTP_ONEMSG_SIZEX;
			size_t SIZE1S = min(HTP_MSG_SIZEY, stMsg.SIZE);
			memcpy_s(m_szOneMsg[IDX], HTP_MSG_SIZEY, stMsg.MSG + IDY, SIZE1S);
			m_szOneMsg[IDX][SIZE1S] = '\0';
			m_cqLOG.Push(HTP_PMSG(m_szOneMsg[IDX], SIZE1S, stMsg.CID));
			stMsg.SIZE -= SIZE1S;
			IDY += (0 < IDY)? SIZE1S : SIZE1S-1;
		}
	}
	catch (...)
	{
		char szError[] = "CLogThread.memcpy_s SHtpMsg ERROR";
		m_cqLOG.Push(HTP_PMSG(szError, strlen(szError) , HTP_CID_FAIL));
	}
	return *this;
}
CLogThread& CLogThread::operator << (const string& strMsg)
{
	*this << HTP_CMSG(strMsg, HTP_CID_OK); return *this;
}
//日志缓存到可追加的缓存中，通过ID结束某一段时间区间产生的日志，适用于单次字符量小但触发频繁的场合
CLogThread& CLogThread::operator << (const HTP_CID& CID)
{
	size_t IDX = (m_llnApdIDX)&HTP_APDMSG_SIZEX;
	size_t IDY = (m_llnApdIDY)&HTP_MSG_SIZEY;
	m_llnApdIDX++; m_llnApdIDY = 0;
	m_szApdMsg[IDX][IDY] = '\0';
	m_cqLOG.Push(HTP_PMSG(m_szApdMsg[IDX], IDY, CID));
	return *this;
}
//日志缓存到可追加的缓存中，通过ID结束某一段时间区间产生的日志，适用于单次字符量小但触发频繁的场合
CLogThread& CLogThread::operator << (const char* pMsg)
{
	size_t IDX =0, IDY = 0;
	while (*pMsg != '\0')
	{
		IDX = (m_llnApdIDX)&HTP_APDMSG_SIZEX;
		IDY = (m_llnApdIDY)&HTP_MSG_SIZEY; m_llnApdIDY++;
		m_szApdMsg[IDX][IDY] = *pMsg; pMsg++;
		if ((m_llnApdIDY & HTP_MSG_SIZEY) == 0X0000)
		{
			m_llnApdIDX++;
			m_szApdMsg[IDX][HTP_MSG_SIZEY+1] = '\0'; 
			m_cqLOG.Push(HTP_PMSG(m_szApdMsg[IDX], HTP_MSG_SIZEY +1, HTP_CID_WARN));
		}
	}
	return *this;
}

void CLogThread::Run() //数据库执行主线程循环
{
	HTP_LOG << "CLogThread::Run" << HTP_ENDL;
	SHtpMsg stMsg;
	SYSTEMTIME sysTime;
	char szBUFF[512];
	char szMsg[HTP_MSGUTF8_SIZEY+16];
	string strWorkFolder = m_Config->GetExeFolder().append("\\LOG\\").append(m_Config->GetLogin().UserID);
	
	if (0 != _access(strWorkFolder.c_str(), 0))
	{
		int ret = _mkdir(m_Config->GetExeFolder().append("\\LOG\\").c_str());
		ret = _mkdir(strWorkFolder.c_str());
	}
	while (m_bThRunning) //主循环
	{
#ifdef _RUNLOG
		//cout << "CMongoDB Threat COUNT" << nCount++ << endl;
#endif // _RUN_LOG
		m_smTask.Wait(200);
		GetLocalTime(&sysTime);
		if (m_Config)
		{
			sprintf_s(szBUFF, "%s\\%04d%02d%02d.txt", strWorkFolder.c_str(), sysTime.wYear, sysTime.wMonth, sysTime.wDay);
			m_ioTxt.open(szBUFF, fstream::out | fstream::app);
		}
		sprintf_s(szBUFF, "\r\n%02d%02d%02d: ", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		while (m_cqLOG.Front(stMsg))
		{
			m_cqLOG.Pop();
			if (stMsg.SIZE < 1) { continue; }
			if (0 == HTP_CID_LOG(stMsg.CID)) { stMsg.CID |= HTP_CID_USE_LOGCOUT| HTP_CID_USE_LOGTXT| HTP_CID_USE_LOGMDB; } //假如没有加入LOG控制则默认为输出到所有日志
			memcpy(szMsg, stMsg.MSG, stMsg.SIZE);
			szMsg[((stMsg.SIZE - 1) & HTP_MSGUTF8_SIZEY)+1] = '\0'; //字节防溢出

			if (HTP_CID_DECODE(stMsg.CID) == HTP_CID_ISUTF8) //由于项目统一采用UTF8格式存储，而控制台却是ASCII
			{
				string strASCII = UTF_82ASCII(string(szMsg)); //UTF8转码
				cout << strASCII.c_str() << endl; //显示
				if ((stMsg.CID & HTP_CID_USE_LOGTXT)&&(m_ioTxt.is_open())) //写文件
				{
					m_ioTxt.write(szBUFF, strlen(szBUFF));
					m_ioTxt.write(strASCII.c_str(), strASCII.size());
				}
			}
			else if (HTP_CID_DECODE(stMsg.CID) == HTP_CID_ISASCII)//UT8存储
			{
				if ((stMsg.CID & HTP_CID_USE_LOGTXT) && (m_ioTxt.is_open())) //写文件
				{
					m_ioTxt.write(szBUFF, strlen(szBUFF));
					m_ioTxt.write(szMsg, strlen(szMsg));
				}
				cout << szMsg << endl; //显示
				string strMsg = ASCII2UTF_8(string(szMsg)); //ASCII转成UTF8应用于写入MONGO，因为MONGO默认的编码为UTF8
				memcpy(szMsg, strMsg.c_str(), strMsg.size());
				szMsg[strMsg.size()] = '\0';
			}
			else {
				cout << szMsg << endl; //显示
			}
			if (stMsg.CID & HTP_CID_USE_LOGMDB)
			{
				m_MDB->PushLogToDB(CHtpMsg(szMsg, stMsg.CID)); //日志网络输出到EMAIL
			}
			if (stMsg.CID & HTP_CID_USE_LOGNET) 
			{
				m_MDB->SendEmail(CHtpMsg(szMsg, stMsg.CID)); //日志网络输出到EMAIL
			}
		}
		if (m_ioTxt.is_open())
		{
			m_ioTxt.close();
		}
	}
	cout << "CLogThread::EXIT" << endl; //因为日志已经被析构了
}

