#pragma once
#include "stdafx.h"
#include "ThostFtdcTraderApi.h"
#include "CMyLib.h"
#include "CCycleQueue.h"
#include "CMyData.h"
#include <shared_mutex>

enum eRiskEnable
{
	REN_GLOBAL = 0, //����ܿ���
	REN_SNAPSHOT_TICK, //�������
	REN_SNAPSHOT_LINEK, //K�߿���
	REN_SNAPSHOT_POSITION, //�ֲֿ���
	REN_SNAPSHOT_ORDER, //ί�п���
	REN_MARKET, //���鿪�أ���¼���� 
	REN_TDRADE, //���ף�ִ�н���
	REN_ENDF
};
const char g_strRiskEN[REN_ENDF][32] =
{
	"GLOBAL_EN", //����ܿ���
	"SHOT_TICK", //�������
	"SHOT_LINEK", //K�߿���
	"SHOT_POSITION", //�ֲֿ���
	"SHOT_ORDER", //ί�п���
	"MARKET_EN", //���鿪��
	"TRADE_EN", //���׿���
};
enum eRiskSysCnt
{
	RSC_INIT = 0, //��ʼ������
	RSC_LOGIN, //��¼����
	RSC_ORDFAIL, //���µ��������
	RSC_VOLX, //�����쳣����
	RSC_SYN_LINEK, //��ȡ����K�ߴ���
	RSC_CMDX, //CMDִ��ʧ�ܴ���
	RSC_LOGOUT, //ע����¼����
	RSC_ENDF
};
const char g_strSysCnt[RSC_ENDF][32] =
{
	"RSC_INIT", //��ʼ������
	"RSC_LOGIN", //��¼����
	"RSC_ORDFAIL", //���µ��������
	"RSC_VOLX", //�����쳣����
	"RSC_SYN_LINEK", //��ȡ����K�ߴ���
	"RSC_CMDX" //CMDִ��ʧ�ܴ���
	"RSC_LOGOUT" //ע����¼����
};
const int g_nSysCntLimit[RSC_ENDF]
{
	50, 50, 1000, 10000, 3000, 30, 50
};
class CRiskThread
{
	atomic_bool m_bEnable[REN_ENDF]; 
	atomic_int m_nSysCnt[RSC_ENDF];
	SCtpRiskLimit m_stLimit;
	map<CFastKey, SCtpRiskCtrl, CFastKey> m_mapRiskCtrl; //��ؿ���������ֵ����Լ
	map<CFastKey, SCtpRiskCnt, CFastKey> m_mapRiskCnt; //��ؼ�����, ��ֵ����Լ
	shared_mutex m_cthLock;
	shared_ptr<const CUserConfig> m_Config;
public:
	CRiskThread();
	~CRiskThread();
	void Start(shared_ptr<const CUserConfig> Config);
	bool GetEnable(eRiskEnable eType) const;
	//ʹ�ܿ���
	atomic_bool& Enable(eRiskEnable eType);
	void SetEnable(eRiskEnable eType, bool bEnable);
	//������
	SCtpRiskCtrl& Ctrl(const CFastKey& cKey);
	//������
	SCtpRiskCnt& Cnt(const CFastKey& cKey);
	//���������
	SCtpRiskLimit& Limit();
	//ϵͳ������
	atomic_int& SysCnt(eRiskSysCnt eType);
	bool Check(shared_ptr<const CThostFtdcInputOrderField> stOrder);
};

