#pragma once
#include "SocketInterface.h"
#include "TcpClient.h"

typedef int (*OnReceiveCallBack)(const char* info);
class CHpTcpClient :
    public CTcpClientListener, public CTcpClient
{
public:
	CHpTcpClient();
	~CHpTcpClient();
public:
	int SetReceiveCall(OnReceiveCallBack pCallBack);
	
private:
	//virtual EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
	//virtual EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID);

	OnReceiveCallBack m_OnReceiveCallBack;
};

