#pragma once
#include "stdafx.h"
#include <shellapi.h>
#include ".\LIBWin\NTP\VxNtpHelper.h"
#include "CLogThread.h"
#include "CTimeCalc.h"
#include "CMyData.h"
#ifdef _WIN32
#include <WinSock2.h>
#endif // _WIN32
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
/**
* @class vxWSASocketInit
* @brief 自动 加载/卸载 WinSock 库的操作类。
*/
class vxWSASocketInit
{
	// constructor/destructor
public:
	vxWSASocketInit(x_int32_t xit_main_ver = 2, x_int32_t xit_sub_ver = 0)
	{
		WSAStartup(MAKEWORD(xit_main_ver, xit_sub_ver), &m_wsaData);
	}

	~vxWSASocketInit(x_void_t)
	{
		WSACleanup();
	}

	// class data
protected:
	WSAData      m_wsaData;
};
#endif // _WIN32
struct SHtpCmd
{
	SHtpCmd() 
	{ 
		strFile.clear(); 
		strParam.clear();
		strCmdLine.clear();
	}
	string strFile;
	string strParam;
	string strCmdLine;
};
class CHtpSystem
{
#ifdef _WIN32
	vxWSASocketInit m_Init;
#endif // _WIN32
	mutex m_cthLock[CTP_TIME_ENDF]; //线程锁
	thread* m_pthTask; //执行线程
	atomic_bool m_bThRunning; //运行状态
	CSEM m_smTask; //线程信号量
	CTimeCalc m_cTime[CTP_TIME_ENDF];
	atomic_ullong m_ullPingDlay; //当前PING延迟
	atomic_ullong m_ullYdOffset[CTP_TIME_ENDF]; //昨日时间平均补偿
	atomic_ullong m_ullFixOffset[CTP_TIME_ENDF]; //固定时间补偿
	atomic_ullong m_ullTimeTick[CTP_TIME_ENDF]; //本机TICK计数,用于计算最新时间
	vector<SHtpCmd> m_vecCmd;
	shared_ptr<const CUserConfig> m_Config;
	void Run();
	
	//atomic_ullong GetOffsetMS(const eCtpTime& eTime);
public:
	CHtpSystem();
	~CHtpSystem();
	void Stop();
	bool GetCmd(string& strText);
	bool AddCmd(vector<string>& vecCmd)
	{
		for (const auto& it : vecCmd)
		{
			AddCmd(string(it));
		}
		return (vecCmd.size() > 0);
	}
	bool AddCmd(string& strCmd);
	void Start(shared_ptr<const CUserConfig> Config);
	CTimeCalc GetTime(const eCtpTime& eTime);
	void SetTime(const eCtpTime& eTime, const CTimeCalc& ctpTime);
	bool SetTime(shared_ptr<const CCtpTick> ctpTick);
	bool TimeStand(shared_ptr<CCtpTick> ctpTick); //时间规整
	bool SynNetTime(); //同步时间
	DWORD GetPingMS(); //阻塞式用于获取PING延迟
};

extern CHtpSystem g_htpSys;
#define HTP_SYS g_htpSys