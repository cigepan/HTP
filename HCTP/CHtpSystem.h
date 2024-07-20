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
* @brief �Զ� ����/ж�� WinSock ��Ĳ����ࡣ
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
	mutex m_cthLock[CTP_TIME_ENDF]; //�߳���
	thread* m_pthTask; //ִ���߳�
	atomic_bool m_bThRunning; //����״̬
	CSEM m_smTask; //�߳��ź���
	CTimeCalc m_cTime[CTP_TIME_ENDF];
	atomic_ullong m_ullPingDlay; //��ǰPING�ӳ�
	atomic_ullong m_ullYdOffset[CTP_TIME_ENDF]; //����ʱ��ƽ������
	atomic_ullong m_ullFixOffset[CTP_TIME_ENDF]; //�̶�ʱ�䲹��
	atomic_ullong m_ullTimeTick[CTP_TIME_ENDF]; //����TICK����,���ڼ�������ʱ��
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
	bool TimeStand(shared_ptr<CCtpTick> ctpTick); //ʱ�����
	bool SynNetTime(); //ͬ��ʱ��
	DWORD GetPingMS(); //����ʽ���ڻ�ȡPING�ӳ�
};

extern CHtpSystem g_htpSys;
#define HTP_SYS g_htpSys