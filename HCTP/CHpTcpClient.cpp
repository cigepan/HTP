#include "stdafx.h"
#include "CHpTcpClient.h"


CHpTcpClient::CHpTcpClient() : CTcpClient(this)
{
	m_OnReceiveCallBack = NULL;
}
CHpTcpClient::~CHpTcpClient()
{
	if (IsConnected())
	{
		Stop();
	}
}
//设置处理回调，可结合PYTHON接口，接收回调定向到PYTHON接收函数上，详情请见项目工程：PY2C_HPTCP
int CHpTcpClient::SetReceiveCall(OnReceiveCallBack pCallBack)
{
	if (NULL != m_OnReceiveCallBack)
	{
		return -1;
	}
	m_OnReceiveCallBack = pCallBack;
	return 0;
}
//接收回调
EnHandleResult CHpTcpClient::OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	char* pszBuf = (char*)malloc(iLength + 1LL); //先缓存
	if (NULL == pszBuf)
	{
		return HR_ERROR;
	}
	memcpy(pszBuf, pData, iLength);
	*(pszBuf + iLength) = '\0'; //添加结束符
#ifdef _RUNLOG
	cout << "C++CHpTcpClient::OnReceive" << GetTickCount64() << pszBuf << endl;
#endif
	if(m_OnReceiveCallBack)
	{
		(*m_OnReceiveCallBack)(pszBuf);
	}
	free(pszBuf);
	return HR_IGNORE;
}
EnHandleResult CHpTcpClient::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) 
{ 
	return HR_IGNORE; 
}
