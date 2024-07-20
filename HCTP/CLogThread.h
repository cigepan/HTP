#pragma once
#include "stdafx.h"
#include "CMyData.h"
#include "CMongoDB.h"

class CLogThread
{
	ofstream m_ioTxt;
	mutex m_thLock; //��
	SHtpMsg m_stMsg;
	atomic_ullong m_llnOneIDX;
	atomic_ullong m_llnOneIDY;
	char m_szOneMsg[HTP_ONEMSG_SIZEX +4][HTP_MSG_SIZEY+4]; //4MB
	atomic_ullong m_llnApdIDX;
	atomic_ullong m_llnApdIDY;
	char m_szApdMsg[HTP_APDMSG_SIZEX + 4][HTP_MSG_SIZEY + 4]; //4MB
	CCycleQueue<SHtpMsg> m_cqLOG;

	CSEM m_smTask; //�߳��ź���
	thread* m_pthTask; //�����߳�
	atomic_bool m_bThRunning; //����״̬
	shared_ptr<const CUserConfig> m_Config;
public:
	shared_ptr<CMongoDB> m_MDB;
	CLogThread();
	~CLogThread();
	void Start(shared_ptr<const CUserConfig> Config);
	void Run();
	void Stop();
	CLogThread& operator << (CHtpMsg& stMsg);
	CLogThread& operator << (SHtpMsg& stMsg);
	CLogThread& operator <<(const string& strMsg);
	CLogThread& operator << (const char* szMsg);
	CLogThread& operator << (const HTP_CID& CID);
};

extern CLogThread g_Log;
#define HTP_LOG (g_Log)
