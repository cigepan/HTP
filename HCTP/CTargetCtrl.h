#pragma once
#include"CMyData.h"
#include "CMongoTaskDef.h"
//--------------------------------------------------------TARGET����--------------------------------------------------------------
//ALGO-TARGET ģʽ
enum eAlgoMode
{
	AGMD_TWAPAD = 0,
	AGMD_TWAPK,
	AGMD_OPENK,
	AGMD_CLOSEK,
	AGMD_PREOPEN,
	//AGMD_T1, //�ռ�ģʽ
	//AGMD_HOUR1, //1Сʱ���׼ƻ�
	//AGMD_MINK, //5���ӽ��׼ƻ�
	//AGMD_MIN1, //1���ӽ��׼ƻ�
	//AGMD_T0, //����ģʽ
	AGMD_ENDF
};
extern const char g_strAlgoMD[AGMD_ENDF][32];
//TARGET���䷽ʽ
enum eAlignType
{
	ALIGN_LEFT, //�����
	ALIGN_RIGHT, //�Ҷ���
	ALIGN_MID, //�м䣬�����
	ALIGN_ENDF
};

//����ʱ��
struct CTimeTD
{
	CTimeCalc ctmStart;
	CTimeCalc ctmEnd;
};
//û��TARGET�������PUSH�ظ���TARGETʱ��
#define CTC_PUSH_NOTGT_TIME 10000

//target�����࣬����Լ�����д�ţ����ڽ������ȡ��ִ�е�TARGET
class CTarget
{
	//-------------------------------TARGET���ɽ��-------------------------------------------
	list<SCtpTarget> m_listTarget;
	size_t m_llnSizeAll; //��targetִ����
	TThostFtdcVolumeType m_nVolumeAll; //������
	TThostFtdcVolumeType m_nVolumeDone; //�����
	TThostFtdcVolumeType m_nPositionLast;//��һ�ε�Ŀ������
	mutex m_lock; 
public:
	//-------------------------------TARGET���ɲ���������MDB������-------------------------------------------
	//���������ݵĶ�дȷ������������̰߳�ȫ
	bool m_bEnable[AGMD_ENDF]; //targetģʽ�����ܶ���ģʽ�ں�
	int m_nParamK[AGMD_ENDF]; //targetģʽ�������������ܶ���ģʽ�ں�
	eAlignType m_eAlign; //���뷽ʽ
	int m_nFreqMS; //�ڵ�Ƶ��
	double m_dbTimeAdj; //ʱ��ƫ����Ӧ����
	double m_dbVolK; //��������Ӧ����K
	double m_dbVolPowK; //��������Ӧ����������K��Ĭ�ϲ�����
	TThostFtdcVolumeType	m_volLastQty; //���³ֲ�
	TThostFtdcVolumeType	m_volTarget; //Ŀ��ֲ�
	TThostFtdcVolumeType m_volTradeQty; //������
	CTimeCalc m_ctmCurrent; //��ǰʱ��
	CTimeCalc m_ctmTD; //������
	CTimeCalc m_ctmTdStart; //���Կ�ʼʱ��
	CTimeCalc m_ctmTdEnd; //���Խ���ʱ��
	vector<CTimeTD> m_vecTimeTD; //���齻��ʱ��
	string m_strID; //��ԼID
	string m_strExchange; //��Լ������
	string m_strClass; //��Ʒ����(��ʽ����д)
	string m_strProduct; //��Ʒ����
	CTarget();
	~CTarget();
	void Clear();
	void Push(const SCtpTarget& stTatget);
	bool Pop();
	bool Front(SCtpTarget& stTarget);
	SCtpTarget Current();
	void GetTaskRatio(SCtpTaskRatio& stRatio);
	bool GetTarget(const CTimeCalc& ctpTimeNT, SCtpTarget& stTarget);
	CHtpMsg GetTimeList(list<CTimeCalc>& listTimeTD); //��ȡʱ��ڵ�, ����ʱ�������뽻��ʱ��ڵ�
	CHtpMsg Create(document& docPlan); ////����TARGET�����ݲ�ͬģʽִ�в�ͬ���ɲ���
	//CTargetCtrl& operator=(const CTargetCtrl& ctpTarget); //�ɰ���ô����ݿ��ȡTARGET���°���ִ�������
};
bool TargetCompare(SCtpTarget& A, SCtpTarget& B);

//---------------------------------------------------TARGET������(����)--------------------------------------------------------------------
//����TARGET�漰�������ݺͽṹ�϶࣬����ʹ���෽����й���
//���TARGET����BUF�������ù���ָ��ķ�ʽ��Ҫԭ������Ϊ��Ҫ�м���ĸ����л��ٶȣ�����Ӱ�쵽����
#define CTGT_MAX 0x03
class CTargetCtrl
{
	//���û���������ù���ָ���ԭ����׷��״̬�����ٶȣ�Ҳ����״̬��Ҫ�ǳ���ĸ��¿��ز�Ȼ��Ӱ�콻��
	map<CFastKey, unique_ptr<CTarget>, CFastKey> m_MapTarget[CTGT_MAX + 1];
	TThostFtdcUserIDType m_stUserID;
	atomic_llong m_llTimeStamp; //ԭ�Ӳ���
	string m_strTargetID;
	char m_chID; // ��ǰID
	mutex m_lock;
	//MAP�Ķ��η�װ�ӿڣ�CTargetҲ�����������������赣����Դ����
	CTarget* GetMapTarget(const CFastKey& strKey) 
	{
		unique_lock<mutex> lock(m_lock);
		map<CFastKey, unique_ptr<CTarget>, CFastKey>& refMap = m_MapTarget[(m_chID & CTGT_MAX)];
		if (refMap.find(strKey) == refMap.end())
		{
			return NULL;
		}
		return refMap[strKey].get();
	}
public:
	CTargetCtrl()
	{
		memset(m_stUserID, 0, sizeof(TThostFtdcUserIDType));
		m_llTimeStamp = 0;
		m_chID = 0;
		for (int i = 0; i <= CTGT_MAX; i++)
		{
			m_MapTarget[i].clear();
		}
	}
	~CTargetCtrl()
	{
	}
	void SetUserID(const TThostFtdcUserIDType& stUserID)
	{
		unique_lock<mutex> lock(m_lock);
		memcpy(m_stUserID, stUserID, sizeof(TThostFtdcUserIDType));
	}
	const TThostFtdcUserIDType& GetUserID()
	{
		unique_lock<mutex> lock(m_lock);
		return m_stUserID;
	}
	void SetTargetID(const string& strTargetID)
	{
		unique_lock<mutex> lock(m_lock);
		m_strTargetID = strTargetID;
	}
	const string GetTargetID()
	{
		unique_lock<mutex> lock(m_lock);
		return m_strTargetID;
	}
	void SetTimeStamp(const LONG64& llTime) //����ʱ��׺�������ж��Ƿ�Ϊ��Ҫ���µ�ǰtarget
	{
		m_llTimeStamp = llTime;
	}
	LONG64 GetTimeStamp()
	{
		return m_llTimeStamp;
	}

	SCtpTarget operator[](const CFastKey& strKey)
	{
		CTarget* pCtpTarget = GetMapTarget(strKey);
		return (NULL == pCtpTarget) ? SCtpTarget({ 0 }) : pCtpTarget->Current();
	}
	//��ȡ�����ʱ������е�TARGET
	int GetTarget(map<CFastKey, SCtpTarget, CFastKey>& mapTarget, const CTimeCalc& ctpTimeNT, string& strTargetID)
	{
		strTargetID = GetTargetID();
		mapTarget.clear();
		map<CFastKey, unique_ptr<CTarget>, CFastKey>& refMap = m_MapTarget[(m_chID & CTGT_MAX)];
		SCtpTarget stTarget = { 0 };
		for (auto& it : refMap)
		{
			if (!it.second) { continue; }
			if (it.second->GetTarget(ctpTimeNT, stTarget))
			{
				mapTarget[it.first] = stTarget;
			}
		}
		return (int)mapTarget.size();
	}
	//��ȡ�����TICK��Լʱ��������TARGET,����TARGET������ʽ�����в���TICK���������н��ײ���ʱ������
	bool GetTarget(shared_ptr<const CCtpTick> stTick, SCtpTarget& stTarget, string& strTargetID)
	{
		strTargetID = GetTargetID();
		CTarget* pCtpTarget = GetMapTarget(stTick->m_stTick.InstrumentID);
		return (pCtpTarget) ? pCtpTarget->GetTarget(stTick->m_ctNature, stTarget) : false;
	}
	SCtpTaskRatio GetTaskRatio(const CFastKey& strKey)//const��û�취���ײ�����
	{
		SCtpTaskRatio stRatio = { 0 };
		//map����д���ֻ���������������������дС��
		auto itTarget = m_MapTarget[(m_chID & CTGT_MAX)].find(strKey);
		if (itTarget == m_MapTarget[(m_chID & CTGT_MAX)].end()) { return stRatio; }
		if (!itTarget->second) { return stRatio; }
		itTarget->second->GetTaskRatio(stRatio); //��ȡ�ú�Լ��TARGETִ�����
		return stRatio;
	}
	//������ȡ����ִ�����
	size_t GetTaskRatio(map<CFastKey, SCtpTaskRatio, CFastKey>& mapRatio)//const��û�취���ײ�����
	{
		mapRatio.clear();
		auto& mapTarget = m_MapTarget[(m_chID & CTGT_MAX)]; //�����������дС��
		for (auto& it : mapTarget)
		{
			if (!it.second) { continue; }
			auto& itRatio = mapRatio[it.first];
			it.second->GetTaskRatio(itRatio);
		}
		return mapTarget.size();
	}
	//�½�target����ͨ��MDB�ⲿ����д��map������ʱЧ������ö໺��
	map<CFastKey, unique_ptr<CTarget>, CFastKey>& NewTarget()
	{
		unique_lock<mutex> lock(m_lock);
		return m_MapTarget[((m_chID + 1) & CTGT_MAX)];
	}
	void CheckOut() //�л�������target
	{
		unique_lock<mutex> lock(m_lock);
		m_chID++; m_chID &= CTGT_MAX;
	}
	bool PopTarget(const CFastKey& strKey) //ɾ����Լ����TARGET,ǰ���Ǹ�TARGET�Ѿ�������
	{
		CTarget* pTarget = GetMapTarget(strKey);
		if (pTarget)
		{
			return pTarget->Pop();
		}
		return false;
	}
};

bool ToValue(SCtpTarget& stTarget, const string& strTime);


