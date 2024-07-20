#include "stdafx.h"
#include "CHpTcpServer.h"

CHpTcpServer::CHpTcpServer() : CTcpServer(this)
{
	m_CurClientID = 0LL;
	m_OnReceiveCallBack = NULL;
}
CHpTcpServer::~CHpTcpServer()
{
	if (IsConnected(m_CurClientID))
	{
		Stop();
	}
}
int CHpTcpServer::SetReceiveCall(OnReceiveCallBack pCallBack)
{
	if (NULL != m_OnReceiveCallBack)
	{
		return -1;
	}
	m_OnReceiveCallBack = pCallBack;
	return 0;
}

//EnHandleResult CHpTcpServer::OnPrepareListen(ITcpServer* pSender, SOCKET soListen) { return HR_IGNORE; }
//EnHandleResult CHpTcpServer::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient){ return HR_IGNORE; }
//EnHandleResult CHpTcpServer::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength){ return HR_IGNORE; }
//设置处理回调，可结合PYTHON接口，接收回调定向到PYTHON接收函数上，详情请见项目工程：PY2C_HPTCP
EnHandleResult CHpTcpServer::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	if (m_CurClientID != dwConnID)
	{
		m_CurClientID = dwConnID;
	}
	char* pszBuf = (char*)malloc(iLength + 1LL); //先缓存
	if (NULL == pszBuf)
	{
		return HR_ERROR;
	}
	memcpy(pszBuf, pData, iLength);
	*(pszBuf + iLength) = '\0'; //添加结束符
#ifdef _RUNLOG
	cout << "C++CHpTcpServer::OnReceive" << GetTickCount64() << pszBuf << endl;
#endif
	if (m_OnReceiveCallBack)
	{
		(*m_OnReceiveCallBack)(pszBuf);
	}
	free(pszBuf);
	return HR_IGNORE;
}
EnHandleResult CHpTcpServer::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{ 
	return HR_IGNORE; 
}
//EnHandleResult CHpTcpServer::OnShutdown(ITcpServer* pSender) { return HR_IGNORE; }