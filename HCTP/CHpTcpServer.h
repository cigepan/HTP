#pragma once
#include "SocketInterface.h"
#include "TcpServer.h"

typedef int (*OnReceiveCallBack)(const char* info);
class CHpTcpServer :
    public CTcpServerListener, public CTcpServer
{
public:
    CHpTcpServer();
	~CHpTcpServer();
public:
	int SetReceiveCall(OnReceiveCallBack pCallBack);
	CONNID m_CurClientID;
private:
	//virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen);
	//virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient);
	//virtual EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
	//virtual EnHandleResult OnShutdown(ITcpServer* pSender);
	
	OnReceiveCallBack m_OnReceiveCallBack;
};

