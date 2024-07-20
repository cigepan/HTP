#include "CTdThread.h"
#include "CLogThread.h"
#include "CHtpSystem.h"

using namespace std;

CTdThread::CTdThread() 
	: m_cqPosition(CQ_SIZE_TDL) , m_cqTrade(CQ_SIZE_TDL) , m_cqOrder(CQ_SIZE_TDL)
	, m_cqOrderLog(CQ_SIZE_TDL), m_cqTick(CQ_SIZE_TDTICK), m_cqTimeSheet(CQ_SIZE_TDL)
	, m_cqOrderTD(CQ_SIZE_TDH), m_cqActionTD(CQ_SIZE_TDL)
	, m_smTrading(5), m_smFlash(5), m_smTime(3), m_smAlgo(5)
{
	memset(&m_stAccount, 0, sizeof(CThostFtdcTradingAccountField));
	m_mapPositionORD.clear();
	m_mapPositionTRD.clear();
	m_mapSynPosTRD.clear();
	m_mapSynPosORD.clear();
	m_mapPNL.clear();
	m_mapInstrument.clear();
	m_mapCROP.clear();
	m_mapProduct.clear();
	m_mapCommission.clear();
	m_mapOrder.clear();
	m_mapTrade.clear();
	m_mapStatus.clear();
	m_mapTick.clear();
	m_mapTickCnt.clear();
	m_pApi = CThostFtdcTraderApi::CreateFtdcTraderApi(); //
	m_pApi->RegisterSpi(this);
	m_pApi->SubscribePublicTopic(THOST_TERT_QUICK);
	m_pApi->SubscribePrivateTopic(THOST_TERT_QUICK); //设置私有流订阅模式
	m_bThRunning = true;
	for (int i = 0; i < CTPST_ENDF; i++)
	{
		m_bStatus[CTPST_ENDF] = false;
	}
	m_eStatus = eCtpStatus::CTPST_NULL;
	m_nRequestID = 0;
	m_pthTrading = NULL;
	m_pthFlash = NULL;
	m_pthTaskTime = NULL;
	m_MDB = NULL;
	m_UserCfg = NULL;
	m_cRisk = NULL;
	m_pthAlgo = NULL;
}
CTdThread::~CTdThread()
{
	m_bThRunning = false; //设置停止运行标志
	m_smTrading.Release();
	m_smFlash.Release();
	m_smTime.Release();
	m_smAlgo.Release();
	if (m_pthTrading)
	{
		m_pthTrading->join(); //等待线程退出
		delete m_pthTrading; //释放线程空间
	}
	if (m_pthFlash)
	{
		m_pthFlash->join(); //等待线程退出
		delete m_pthFlash; //释放线程空间
	}
	if (m_pthTaskTime)
	{
		m_pthTaskTime->join(); //等待线程退出
		delete m_pthTaskTime; //释放线程空间
	}
	if (m_pthAlgo)
	{
		m_pthAlgo->join(); //等待线程退出
		delete m_pthAlgo; //释放线程空间
	}
	if (m_pApi)
	{
		m_pApi->Release();
		//m_pApi->Join(); //等待行情线程退出
	}
}
//流程范式结合状态机，可以优化封装成事务队列的形式（非阻塞式地执行与超时以及返回），时间有限
int CTdThread::Start(shared_ptr<const CUserConfig> Config)
{
	if (!Config) { return -1; }
	try
	{
		HTP_LOG << "CTdThread::Start" << HTP_ENDL;
		if (NULL == m_pApi) { return -1; }  //API注册失败或异常
		m_nRequestID = HTP_SYS.GetTime(CTP_TIME_NET).GetHMSL(); //1S1000下单计数分配
		m_UserCfg = Config; //初始化配置：重要
		m_ctpTargetCtrl.SetUserID(m_UserCfg->GetLogin().UserID);
		InitOrderModel(); //初始化交易模板
		eCtpExid eOrderQry = eCtpExid(0);
		//eCtpExid eTradeQry = eCtpExid(0);
		eCtpStatus eStatus = CTPST_NULL;
		m_eStatus = CTPST_INIT;
		int nTimeOut = 0;
		int iResult = 0;
		int nTryTimes = 0;
INIT_LOOP:
		Sleep(100);
		if (CTPST_BUSY == m_eStatus)
		{ 
			if (--nTimeOut > 0)
				goto INIT_LOOP;
			else
				m_eStatus = CTPST_FAILED; //超时,继续执行上一步工作
		}
		if (m_eStatus == CTPST_FAILED)
		{
			if (++nTryTimes > 5) { return -1; } //失败大于5次则账号状态异常，退出登录或使用备用账号
			m_eStatus = eStatus; //执行上一次
		}
		else
		{
			nTryTimes = 0;
			eStatus = m_eStatus; //备份上一次
			if (CTPST_ONQRYORDER == m_eStatus)
			{
				eOrderQry = (eCtpExid)(eOrderQry + 1);
			}
		}
		m_eStatus = CTPST_BUSY; //标记事务忙
		m_cRisk->SysCnt(RSC_INIT)++;
		switch (eStatus)
		{
		case CTPST_INIT:
			HTP_LOG << HTP_MSG("开始初始化交易线程", HTP_CID_ISUTF8);
			m_pApi->RegisterFront((char*)m_UserCfg->GetFrontTD().c_str());
			m_pApi->Init(); 
			nTimeOut = 100; //10S循环一次
			goto INIT_LOOP;
		case CTPST_FRONTON:
			HTP_LOG << HTP_MSG("客户端认证请求！", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqAuthenticate(&m_UserCfg->GetAuthenticate(), ++m_nRequestID);
			nTimeOut = (iResult == 0) ? 50 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONAUTHEN:
			HTP_LOG << HTP_MSG("用户登录请求", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqUserLogin(&m_UserCfg->GetLogin(), ++m_nRequestID);
			nTimeOut = (iResult == 0) ? 50 : 10; //超时
			goto INIT_LOOP;
		case CTPST_LOGINON:
			HTP_LOG << HTP_MSG("投资者结算结果确认！", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqSettlementInfoConfirm(&m_UserCfg->GetConfirm(), ++m_nRequestID); //结算确认
			nTimeOut = (iResult == 0) ? 50 : 10; //超时
			goto INIT_LOOP;
		case CTPST_CONFIRM:
			if (!m_pthFlash) //可以启动数据刷新线程
				m_pthFlash = new thread(&CTdThread::ThreadFlash, this); //延迟开启，等待设置回调后
			HTP_LOG << HTP_MSG("请求查询合约", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqQryInstrument(&CThostFtdcQryInstrumentField({ 0 }), ++m_nRequestID); //查询可交易合约
			nTimeOut = (iResult == 0) ? 600 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONGETID:
			HTP_LOG << HTP_MSG("请求查询合约手续费率", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqQryInstrumentCommissionRate(&m_UserCfg->GetCommission(), ++m_nRequestID); //查询可交易合约
			nTimeOut = (iResult == 0) ? 300 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONCMSRATE:
			HTP_LOG << HTP_MSG("请求查询资金账户", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqQryTradingAccount(&m_UserCfg->GetAccount(), ++m_nRequestID);
			nTimeOut = (iResult == 0) ? 50 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONACCOUNT: 
		case CTPST_ONQRYORDER:
			if (eOrderQry < CTP_EXID_ENDF) //按交易所查询报单
			{
				Sleep(1000);
				HTP_LOG << HTP_MSG(string("请求查询报单").append(g_strCtpExid[eOrderQry]), HTP_CID_ISUTF8);
				CThostFtdcQryOrderField stQry = m_UserCfg->GetOrder();
				strcpy_s(stQry.ExchangeID, g_strCtpExid[eOrderQry]);
				iResult = m_pApi->ReqQryOrder(&stQry, ++m_nRequestID);
				nTimeOut = (iResult == 0) ? 3000 : 10; //超时
				goto INIT_LOOP;
			}
			//委托查询完毕后查成交
			HTP_LOG << HTP_MSG(string("请求查询成交"), HTP_CID_ISUTF8);
			CThostFtdcQryTradeField stQry = m_UserCfg->GetTrade();
			iResult = m_pApi->ReqQryTrade(&stQry, ++m_nRequestID);
			nTimeOut = (iResult == 0) ? 3000 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONQRYTRADE:
			//成交查询完毕后查持仓
			HTP_LOG << HTP_MSG("请求查询投资者持仓！", HTP_CID_ISUTF8);
			iResult = m_pApi->ReqQryInvestorPosition(&m_UserCfg->GetPosition(), ++m_nRequestID);
			nTimeOut = (iResult == 0) ? 600 : 10; //超时
			goto INIT_LOOP;
		case CTPST_ONQRYPOSITION:
			HTP_LOG << HTP_MSG("完成交易线程初始化！", HTP_CID_ISUTF8);
			m_eStatus = CTPST_RUNING; //标记正式启动
			m_bStatus[CTPST_ONQRYTICK] = true;
			m_bStatus[CTPST_RUNING] = true;
			if (!m_pthTrading)
				m_pthTrading = new thread(&CTdThread::ThreadTrading, this); //延迟开启，等待设置回调后
			if (!m_pthTaskTime)
				m_pthTaskTime = new thread(&CTdThread::ThreadTime, this);
			//if (!m_pthFlash)
			//	m_pthFlash = new thread(&CTdThread::ThreadFlash, this); //延迟开启，等待设置回调后
			if (!m_pthAlgo)
				m_pthAlgo = new thread(&CTdThread::ThreadAlgo, this);
			HTP_LOG << "--------------------------------------------HTP TD RUNNING--------------------------------------------" << HTP_ENDL;
			break;
		//case CTPST_ONDOING: if (++nOnDoing > 30) { m_eStatus = eStatus; }goto INIT_LOOP; //30S重新
		default: goto INIT_LOOP;
		}
	}
	catch (...)
	{
		HTP_LOG << HTP_MSG("-----------------------------------TD START ERROR!!!----------------------------------", 1031);
	}
	return 0;
}
//停止CTP，退出时调用，或者由于网络或CTP前置不稳定导致订单回报丢失严重超时后重启CTP调用
int CTdThread::Stop()
{
	HTP_LOG << "CTdThread::Stop" << HTP_ENDL;
	if (NULL == m_pApi) { return -1; }  //API注册失败或异常
	if (!m_bStatus[CTPST_LOGINON]) { return -1; }
	CThostFtdcUserLogoutField req = m_UserCfg->GetLogout();
	int iResult = m_pApi->ReqUserLogout(&req, ++m_nRequestID);
	m_cRisk->SysCnt(RSC_LOGOUT)++;
	if (iResult != 0) 
	{
		return iResult; //失败
	}
	return 0;
}
//用于与行情K线数据进行内存比较
int CTdThread::GetLineK(CCtpLineK& ctpLineK)
{
	unique_lock<mutex> lock(m_lockLineK);
	ctpLineK = m_mapLineK[ctpLineK.m_InstrumentID];
	return 0;
}
//用于与行情K线数据进行内存比较，同级线程采用回调方式
int CTdThread::SetLineK(CCtpLineK& ctpLineK)
{
	unique_lock<mutex> lock(m_lockLineK);
	m_mapLineK[ctpLineK.m_InstrumentID] = ctpLineK;
	return 0;
}
//TICK驱动合约状态变更与产生交易订单，由于此函数被回调在行情线程的深度行情通知中，所以不推荐此处有产生订单的行为
int CTdThread::OnTick(shared_ptr<const CCtpTick> ctpTick)
{
	if (!ctpTick) { return -1; }
	m_lockTick.lock();
	auto& mapTick = m_mapTick[ctpTick->m_stTick.InstrumentID];
	mapTick = ctpTick;
	auto& nTickCnt = m_mapTickCnt[ctpTick->m_stTick.InstrumentID];
	m_lockTick.unlock();
	if (++nTickCnt < HTP_TICKSTATUS_UNSETTIMES) //TICK计数小于HTP_TICKSTATUS_UNSETTIMES认定不可信
	{ 
		return 0; 
	}
	m_lockStatus.lock();
	auto& mapStatus = m_mapStatus[ctpTick->m_stTick.InstrumentID]; //合约状态
	m_lockStatus.unlock();
	if (nTickCnt == HTP_TICKSTATUS_UNSETTIMES) //计数=HTP_TICKSTATUS_UNSETTIMES则进行同步合约状态
	{
		m_lockStatus.lock();
		mapStatus = ctpTick->m_stTick.InstrumentStatus; //更新到已知确定状态
		m_lockStatus.unlock();
		HTP_LOG << string("#").append(ctpTick->m_stTick.InstrumentID).c_str();
	}
	if (mapStatus != ctpTick->m_stTick.InstrumentStatus) //合约状态已知但突然地与TICK不同步
	{
		HTP_LOG << HTP_MSG(string("?").append(ctpTick->m_stTick.InstrumentID), HTP_CID_WARN | 80808);
		nTickCnt = 0; //重新计数，用于确认合约状态
		return -1;
	}
	m_cqTick.Push(ctpTick);  //采用队列方式线程处理，造成微妙级延迟可接受
	m_smAlgo.Release();
	return 0;
}

int CTdThread::ReqOrderInsert(CThostFtdcInputOrderField& stOrder)
{
	if (stOrder.OrderPriceType == 0) { return -1001; }
	if (stOrder.Direction == 0) { return -1002; }
	if (stOrder.CombOffsetFlag[0] == 0) { return -1003; }
	if (stOrder.VolumeTotalOriginal < 1) { return -1004; }
	if (stOrder.TimeCondition == 0) { return -1005; }
	if (stOrder.ContingentCondition == 0) { return -1006; }
	if (stOrder.VolumeCondition == 0) { return -1007; }
	const CThostFtdcReqUserLoginField& stLogin = m_UserCfg->GetLogin();
	strcpy_s(stOrder.BrokerID, stLogin.BrokerID);
	strcpy_s(stOrder.InvestorID, stLogin.UserID);
	sprintf_s(stOrder.OrderRef, "%d", ++m_nRequestID);
	stOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation; //投机
	stOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose; //非强平
	stOrder.UserForceClose = 0; //用户强平：否
	stOrder.IsAutoSuspend = 0; //自动挂起：否
	stOrder.MinVolume = 1; //最小成交量
	int iResult = m_pApi->ReqOrderInsert(&stOrder, ++m_nRequestID);
	//HTP_LOG << "CTdThread::ReqOrderInsert" << stOrder.InstrumentID << HTP_ENDL;
	//----------HTP_LOG << stOrder;
	return iResult;
}
//获取下单模板
shared_ptr<CThostFtdcInputOrderField> CTdThread::GetOrderModel(eCtpOrderModel eModel)
{
	return make_shared<CThostFtdcInputOrderField>(m_stOrderModel[eModel]); //引用+1
}
//初始化下单模板
void CTdThread::InitOrderModel()
{
	const CThostFtdcReqUserLoginField& stLogin = m_UserCfg->GetLogin();
	//固定字段
	CThostFtdcInputOrderField& stOrder = m_stOrderModel[CTP_ORDM_DEFAULT];
	//strcpy_s(stOrder.InstrumentID, stTgtTick.InstrumentID);
	//stOrder.LimitPrice = stTgtTick.LastPrice;
	stOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	stOrder.TimeCondition = THOST_FTDC_TC_GFD;
	stOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
	stOrder.VolumeCondition = THOST_FTDC_VC_AV;
	strcpy_s(stOrder.BrokerID, stLogin.BrokerID);
	strcpy_s(stOrder.InvestorID, stLogin.UserID);
	//sprintf_s(stOrder.OrderRef, "%d", ++m_nRequestID);
	stOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation; //投机
	stOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose; //非强平
	stOrder.UserForceClose = 0; //用户强平：否
	stOrder.IsAutoSuspend = 0; //自动挂起：否
	stOrder.MinVolume = 1; //最小成交量
}
//生成目标仓位所需要的订单, PUSH到订单执行队列
bool CTdThread::TargetWay(const SCtpTick& stTgtTick, TThostFtdcVolumeType nTgtPos, const string& strTargetID) //合约ID与目标仓位
{
	if (!m_cRisk->GetEnable(REN_TDRADE)) { return false; }
	if (!CompareORTD(stTgtTick)) //有订单超时或持仓对比失败，同步持仓（分订单持仓与成交持仓）
	{
		CThostFtdcQryInvestorPositionField stQryPosition = m_UserCfg->GetPosition();
		memcpy(stQryPosition.ExchangeID, stTgtTick.ExchangeID, sizeof(TThostFtdcExchangeIDType));
		memcpy(stQryPosition.InstrumentID, stTgtTick.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
		auto stQry = make_shared<CThostFtdcQryInvestorPositionField>(stQryPosition);
		m_lockTaskSyn.lock();
		m_mapSynPosTRD[stQryPosition.InstrumentID] = stQry; //同步成交持仓
		m_mapSynPosORD[stQryPosition.ExchangeID] = stQry; //同步订单持仓
		m_lockTaskSyn.unlock();
		//m_cqQryPosition.Push(stQryPosition); //添加查询持仓同步
		HTP_LOG << HTP_MSG(string("CTdThread::TargetWay.CHECK FALSE->").append(stQryPosition.InstrumentID), 3777);
		return false;
	}
	TThostFtdcDirectionType	Direction = 0;
	TThostFtdcVolumeType nCmpMin = 0; //比较小值
	bool bCanOpen = true;
	auto stOrderModel = GetOrderModel(CTP_ORDM_DEFAULT); //获取下单模板
	strcpy_s(stOrderModel->InstrumentID, stTgtTick.InstrumentID);
	//--------------------------------------------------------如果当前这笔有交易则先撤再买-------------------------------------------------------
	SHtpGD stGD;
	CThostFtdcInvestorPositionField stPositionLong = { 0 };
	CThostFtdcInvestorPositionField stPositionShort = { 0 };
	if (false == GetPosValid(stTgtTick.InstrumentID, stPositionLong, stPositionShort, stGD)) //获取净持仓，对消后的挂单
	{
		HTP_LOG << HTP_MSG(string("CTdThread::TargetWay.GetPosValid FALSE->").append(stTgtTick.InstrumentID), 3777);
		return false;
	}
	//--------------------------------------------------------查询持仓-------------------------------------------------------
	if (m_cRisk->Limit().nLongPosMax < stPositionLong.Position)
	{
		//HTP_LOG << HTP_MSG(string("CTdThread::TargetWay Check nLongPosMax < Position ").append(stPositionLong.InstrumentID), 3778);
		bCanOpen = false; //禁止开仓
	}
	if (m_cRisk->Limit().nShortPosMax < stPositionShort.Position) //检测持仓是否超出线程，来自风控逻辑
	{
		//HTP_LOG << HTP_MSG(string("CTdThread::TargetWay Check nShortPosMax < Position ").append(stPositionShort.InstrumentID), 3778);
		bCanOpen = false;  //禁止开仓
	}
	TThostFtdcVolumeType nDoAll = nTgtPos
		- stPositionLong.YdPosition - stPositionLong.TodayPosition
		+ stPositionShort.YdPosition + stPositionShort.TodayPosition
		- stGD.VolOBTD + stGD.VolOSTD; //计算本次交易量
	if (nDoAll == 0) { return true; }
	TThostFtdcVolumeType volBuyGD = stGD.VolOBTD + stGD.VolCBTD + stGD.VolCBYD + stGD.VolCBUD;
	TThostFtdcVolumeType volSellGD = stGD.VolOSTD + stGD.VolCSTD + stGD.VolCSYD + stGD.VolCSUD;
	if ((nDoAll > 0 && volSellGD > 0) || (nDoAll < 0 && volBuyGD > 0)) //需要买但是有卖挂单或者反过来
	{
		map< CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey> mapGD;
		ScanGD(stTgtTick.InstrumentID, mapGD, HTP_CANCEL_LIMITTIME); //超时撤单重开
		for (auto& itGD : mapGD)
		{
			if (!itGD.second) { continue; }
			if (0 < CancelWay(itGD.second, false, false)) //超时撤单,V型反转不进行重开和涨跌停检查
			{
#ifdef _SHOW_HTS
				char szMsg[128];
				//合约，目标仓位，今多，昨多，今空，昨空，挂买开仓，挂卖开仓
				sprintf_s(szMsg, "ID[%s] CANCEL REF[%s] TIMEOUT[%d] "
					, itGD.second->InstrumentID, itGD.second->OrderRef, HTP_CANCEL_LIMITTIME);
				HTP_LOG << HTP_MSG(szMsg, HTP_CID_OK | 1888);
#endif // DEBUG
			}
		}
		return false; //持仓量V型反转不丢此次TARGET
	}
	char szMsg[256];
#ifdef _SHOW_HTS
	//合约，目标仓位，今多，昨多，今空，昨空，挂买开仓，挂卖开仓
	sprintf_s(szMsg, "ID[%s] TARGET[%d] LTD[%d] LYD[%d] STD[%d] SYD[%d] OB[%d] OS[%d] "
		, stTgtTick.InstrumentID, nTgtPos
		, stPositionLong.TodayPosition, stPositionLong.YdPosition
		, stPositionShort.TodayPosition, stPositionShort.YdPosition
		, stGD.VolOBTD, stGD.VolOSTD);
	HTP_LOG << HTP_MSG(string(szMsg), HTP_CID_OK | 1888);
#endif
	Direction = (nDoAll > 0) ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell; //确定方向
	switch (stTgtTick.InstrumentStatus)
	{
	case THOST_FTDC_IS_Continous: //对价
		stOrderModel->LimitPrice = (Direction == THOST_FTDC_D_Buy) ? stTgtTick.AskPrice1 : stTgtTick.BidPrice1;
		if(CTP_PRICE_ISOUT(stOrderModel->LimitPrice))
			stOrderModel->LimitPrice = (Direction == THOST_FTDC_D_Buy) ? stTgtTick.UpperLimitPrice : stTgtTick.LowerLimitPrice;
		//stOrder->TimeCondition = THOST_FTDC_TC_GFD; 
		break;
	case THOST_FTDC_IS_AuctionOrdering: //挂涨跌停价
		stOrderModel->LimitPrice = (Direction == THOST_FTDC_D_Buy) ? stTgtTick.UpperLimitPrice : stTgtTick.LowerLimitPrice;
		//stOrder->TimeCondition = THOST_FTDC_TC_GFD;
		break;
	default: 
		stOrderModel->LimitPrice = stTgtTick.LastPrice;
		//stOrder->TimeCondition = THOST_FTDC_TC_IOC;
		break;
	}
	stOrderModel->Direction = Direction;
	//------------------------------------------------------按规则出单/订单分割--------------------------------------------------
	TThostFtdcVolumeType nMaxOnce = 0; //单笔数量限制
	if (CTP_EXID_ISCFFEX(stTgtTick.ExchangeID)) //中金所有开平限制
	{
		if (stTgtTick.InstrumentID[0] == 'I')
			nMaxOnce = 20;
		else if (stTgtTick.InstrumentID[0] == 'T')
			nMaxOnce = 50;
		else
			nMaxOnce = 50;
	}
	else
	{
		nMaxOnce = 200;
	}
	TThostFtdcVolumeType nDoVolAll = abs(nDoAll); //获取数量
#ifdef _SHOW_HTS
	sprintf_s(szMsg, "CTdThread::TargetWay Check bCanOpen LPMAX[%d] LP[%d] SPMAX[%d] SP[%d] "
		, m_cRisk->Limit().nLongPosMax.load(), stPositionLong.Position
		, m_cRisk->Limit().nShortPosMax.load(), stPositionShort.Position);
	HTP_LOG << HTP_MSG(string(szMsg).append(bCanOpen ? "OK->" : "FALSE->").append(stPositionLong.InstrumentID), 3778);
#endif // _SHOW_HTS
//--------------------------------------------------------生成订单-------------------------------------------------------
	string strHead(szMsg);
	int nLoopCnt = 0;
TARGET_LOOP:
	if (++nLoopCnt > 50) //循环次数大于50则认定为下单逻辑异常
	{
		HTP_LOG << HTP_MSG(string("CTdThread::TargetWay.nLoopCnt > 50 ").append(stTgtTick.InstrumentID), HTP_CID_FAIL | 3777);
		return false;
	}
	auto stOrder = make_shared<CThostFtdcInputOrderField>(*stOrderModel);
	TThostFtdcVolumeType nDoOnce = min(nDoVolAll, nMaxOnce); //确定单次数量
	TThostFtdcVolumeType nDoThisTimes = nDoOnce;
	TThostFtdcVolumeType* pCalcTPosition = NULL;
	//nDoVolAll -= nDoOnce;
	string strMSG = strHead;
	strMSG.append(((Direction == THOST_FTDC_D_Buy) ? "BUY " : "SELL "));
	if ((Direction == THOST_FTDC_D_Buy)&& (stPositionShort.YdPosition > 0)) //买多 //有空先平
	{
		stOrder->CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
		pCalcTPosition = &stPositionShort.YdPosition;
		strMSG.append("CloseYesterday ");
	}
	else if ((Direction == THOST_FTDC_D_Buy) && (stPositionShort.TodayPosition > 0)) //卖空 //有多先平
	{
		stOrder->CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		pCalcTPosition = &stPositionShort.TodayPosition;
		strMSG.append("CloseToday ");
	}
	else if ((Direction == THOST_FTDC_D_Sell) && (stPositionLong.YdPosition > 0)) //卖空 //有多先平
	{
		stOrder->CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
		pCalcTPosition = &stPositionLong.YdPosition;
		strMSG.append("CloseYesterday ");
	}
	else if ((Direction == THOST_FTDC_D_Sell) && (stPositionLong.TodayPosition > 0)) //卖空 //有多先平
	{
		stOrder->CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		pCalcTPosition = &stPositionLong.TodayPosition;
		strMSG.append("CloseToday ");
	}
	else{}
	if (pCalcTPosition) //有平仓
	{
		nCmpMin = min(nDoOnce, *pCalcTPosition);
		stOrder->VolumeTotalOriginal = nCmpMin;
		if (stOrder->VolumeTotalOriginal > 0)
		{
			sprintf_s(stOrder->OrderRef, "%d", ++m_nRequestID);
			m_cqOrderTD.Push(make_shared<CThostFtdcInputOrderFieldEx>(stOrder, strTargetID, ""));
			m_htpManage.SetStatus(HTS_ORD_NEW, stOrder->InstrumentID, stOrder->OrderRef);
			sprintf_s(szMsg, "VOL[%d] REF[%s] ", stOrder->VolumeTotalOriginal, stOrder->OrderRef);
			strMSG.append(szMsg);
			HTP_LOG << HTP_MSG(strMSG, HTP_CID_OK | 1888);
		}
		nDoOnce -= nCmpMin;
		*pCalcTPosition -= nCmpMin;
		goto TARGET_END;
	}
	else//先平对仓，然后开仓
	{
		if (false == bCanOpen) { nDoOnce = 0; goto TARGET_END; } //禁止开仓
		if (nDoOnce <= 0) { goto TARGET_END; }
		stOrder->CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		stOrder->VolumeTotalOriginal = nDoOnce;
		if (stOrder->VolumeTotalOriginal > 0)
		{
			sprintf_s(stOrder->OrderRef, "%d", ++m_nRequestID);
			m_cqOrderTD.Push(make_shared<CThostFtdcInputOrderFieldEx>(stOrder, strTargetID, ""));
			m_htpManage.SetStatus(HTS_ORD_NEW, stOrder->InstrumentID, stOrder->OrderRef);
			sprintf_s(szMsg, "Open VOL[%d] REF[%s] ", stOrder->VolumeTotalOriginal, stOrder->OrderRef);
			strMSG.append(szMsg);
			HTP_LOG << HTP_MSG(strMSG, HTP_CID_OK | 1888);
		}
		nDoOnce = 0;
	}

TARGET_END:
	nDoVolAll -= nDoThisTimes - nDoOnce;
	if (nDoVolAll > 0) //还有剩余量
	{
		goto TARGET_LOOP;
	}
	map< CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey> mapGD;
	ScanGD(stTgtTick.InstrumentID, mapGD, HTP_CANCEL_LIMITTIME); //超时撤单重开
	for (auto& itGD : mapGD)
	{
		if (!itGD.second) { continue; }
		if (0 < CancelWay(itGD.second)) //超时撤单,V型反转不进行重开和涨跌停检查
		{
#ifdef _SHOW_HTS
			char szMsg[128];
			//合约，目标仓位，今多，昨多，今空，昨空，挂买开仓，挂卖开仓
			sprintf_s(szMsg, "ID[%s] CANCEL REF[%s] TIMEOUT[%d] "
				, itGD.second->InstrumentID, itGD.second->OrderRef, HTP_CANCEL_LIMITTIME);
			HTP_LOG << HTP_MSG(szMsg, HTP_CID_OK | 1888);
#endif // DEBUG
		}
	}
	return true;
}
int CTdThread::CancelWay(shared_ptr<const CThostFtdcOrderField> itOrder, bool bReOpen, bool bCheckLimit)
{
	if (HTS_CCL_DOING <= m_htpManage.GetStatus(itOrder->InstrumentID, itOrder->OrderRef))
	{
#ifdef _SHOW_HTS
		HTP_LOG << HTP_MSG("CTdThread::CancelWay HTS_CCL_DOING > 0", HTP_CID_FAIL | 1633);
#endif // _SHOW_HTS
		return -1; //属于撤单执行中，不重复撤单
	}
	//撤单重开
	if (HTP_CANCEL_LIMITTIME > m_htpManage.GetTickChange(HTS_ORDED, itOrder->InstrumentID, itOrder->OrderRef))
	{
		return 0; //未过限定时间的挂单不撤
	}
	m_lockTick.lock();
	auto itFind = m_mapTick.find(itOrder->InstrumentID);
	m_lockTick.unlock();
	if ((bCheckLimit)&&(itFind->second))
	{
		if (((THOST_FTDC_D_Buy == itOrder->Direction) 
				&& (itOrder->LimitPrice == itFind->second->m_stTick.UpperLimitPrice))
			|| ((THOST_FTDC_D_Sell == itOrder->Direction) 
				&& (itOrder->LimitPrice == itFind->second->m_stTick.LowerLimitPrice)))
		{
#ifdef _SHOW_HTS
			HTP_LOG << HTP_MSG("CTdThread::CancelWay PRICE LIMIT GD", HTP_CID_WARN | 1633);
#endif // _SHOW_HTS
			return 0; //涨跌停板追单不撤，包括在集合竞价阶段
		}
	}
	m_htpManage.SetStatus(HTS_CCL_DOING, itOrder->InstrumentID, itOrder->OrderRef); //标记撤单中
	auto stAction = make_shared<CThostFtdcInputOrderActionField>();
	memset(stAction.get(), 0, sizeof(CThostFtdcInputOrderActionField));
	strcpy_s(stAction->BrokerID, itOrder->BrokerID);
	strcpy_s(stAction->InvestorID, itOrder->InvestorID);
	strcpy_s(stAction->InstrumentID, itOrder->InstrumentID);
	stAction->ActionFlag = THOST_FTDC_AF_Delete;
	stAction->FrontID = itOrder->FrontID;
	stAction->SessionID = itOrder->SessionID;
	strcpy_s(stAction->OrderRef, itOrder->OrderRef);
	m_lockCROP.lock();
	m_mapCROP[itOrder->OrderRef] = bReOpen;
	m_lockCROP.unlock();
	m_cqActionTD.Push(stAction); //PUSH到撤单队列
	return 1;
}
//检测挂单并对超时ullTimeOutMS未成交的订单进行撤单重开
//撤单重开虽然简化了逻辑，但也存在当TARGET持仓量出现顶尖回撤的时刻可能会造成的自成交导致的下单失败
//目前自成交相关统计并到撤单风控里，超出次数变不在进行下单操作
//后续可通过订单管理优化成如果TARGET持仓量出现反向变化，可通过修改挂单需要成交的数量或撤单不重开，而不是直接撤单重开
//又或者对自成交订单进行本地拦截，时间有限，后续优化
SHtpGD CTdThread::ScanGD(const CFastKey& stID, map<CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey>& mapGD, ULONG64 ullTimeOutMS)
{
	vector<CFastKey> vecGD;
	mapGD.clear();
	SHtpGD stGD = { 0 };
	if (1 > m_htpManage.GetOrderGD(stID, vecGD, ullTimeOutMS)) { return stGD; } //获取超时挂单
	for (const auto& itGD : vecGD) //撤销相关委托单
	{
		m_lockOrder.lock();
		const auto itOrder = m_mapOrder[stID][itGD]; //查询委托表
		m_lockOrder.unlock();
		if (!itOrder) { continue; }
		TThostFtdcOrderRefType OrderRef = {0};
		strcpy_s(OrderRef, itGD.Key());
		if (itOrder->OrderSysID[0] == 0) { continue; } //过滤被取消或异常的订单没有引用ID，未知单
		if ((itOrder->OrderStatus > THOST_FTDC_OST_AllTraded)
			&& (itOrder->OrderStatus < THOST_FTDC_OST_Canceled))
		{
			mapGD[itGD] = itOrder;
			//挂单获取计数持仓
			switch (itOrder->CombOffsetFlag[0])
			{
			///开仓
			case THOST_FTDC_OF_Open: 
				if (THOST_FTDC_D_Buy == itOrder->Direction)
					stGD.VolOBTD += itOrder->VolumeTotal;
				else
					stGD.VolOSTD += itOrder->VolumeTotal;
				break;
			///平今
			case THOST_FTDC_OF_CloseToday:
				if (THOST_FTDC_D_Buy == itOrder->Direction)
					stGD.VolCBTD += itOrder->VolumeTotal;
				else
					stGD.VolCSTD += itOrder->VolumeTotal;
				break;
			///平昨
			case THOST_FTDC_OF_CloseYesterday: 
				if (THOST_FTDC_D_Buy == itOrder->Direction)
					stGD.VolCBYD += itOrder->VolumeTotal;
				else
					stGD.VolCSYD += itOrder->VolumeTotal;
				break;
			//其他情况：累计到未定义的平仓
			default: 
				if (THOST_FTDC_D_Buy == itOrder->Direction)
					stGD.VolCBUD += itOrder->VolumeTotal;
				else
					stGD.VolCSUD += itOrder->VolumeTotal;
				break;
			}
		}
	}
	return stGD; //返回还在交易中的数量
}
//对比委托持仓与成交持仓，对比不一致则认定持仓更新异常，需要进行持仓同步
bool CTdThread::CompareORTD(const SCtpTick& stTick)
{
	char szKey[64] = { 0 };
	unique_lock<mutex> lock(m_lockPosition);
	//-------------------------------------------------比较多头----------------------------------------------
	sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Long, THOST_FTDC_PSD_Today);
	CThostFtdcInvestorPositionField& stPositionTdayORDL = m_mapPositionORD[szKey];
	sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Long, THOST_FTDC_PSD_Today);
	CThostFtdcInvestorPositionField& stPositionTdayTRDL = m_mapPositionTRD[szKey];
	if (stPositionTdayORDL.Position != stPositionTdayTRDL.Position) { return false; }
	if (CTP_EXID_ISSHFE(stTick.ExchangeID) || CTP_EXID_ISINE(stTick.ExchangeID)) //上期所/能源中心区分昨天与今天
	{ 
		sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Long, THOST_FTDC_PSD_History);
		CThostFtdcInvestorPositionField& stPositionYdayORDL = m_mapPositionORD[szKey];
		sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Long, THOST_FTDC_PSD_History);
		CThostFtdcInvestorPositionField& stPositionYdayTRDL = m_mapPositionTRD[szKey];
		if (stPositionYdayORDL.Position != stPositionYdayTRDL.Position) { return false; }
	}
	//-------------------------------------------------比较空头----------------------------------------------
	sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Short, THOST_FTDC_PSD_Today);
	CThostFtdcInvestorPositionField& stPositionTdayORDS = m_mapPositionORD[szKey];
	sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Short, THOST_FTDC_PSD_Today);
	CThostFtdcInvestorPositionField& stPositionTdayTRDS = m_mapPositionTRD[szKey];
	if (stPositionTdayORDS.Position != stPositionTdayTRDS.Position) { return false; }
	if (CTP_EXID_ISSHFE(stTick.ExchangeID) || CTP_EXID_ISINE(stTick.ExchangeID)) //上期所/能源中心区分昨天与今天
	{
		sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Short, THOST_FTDC_PSD_History);
		CThostFtdcInvestorPositionField& stPositionYdayORDS = m_mapPositionORD[szKey];
		sprintf_s(szKey, "%s|%c%c", stTick.InstrumentID, THOST_FTDC_PD_Short, THOST_FTDC_PSD_History);
		CThostFtdcInvestorPositionField& stPositionYdayTRDS = m_mapPositionTRD[szKey];
		if (stPositionYdayORDS.Position != stPositionYdayTRDS.Position) { return false; }
	}
	return true;
}
//获取当前净持仓
//挂平仓对净持仓计减，挂开仓对净持仓计加
bool CTdThread::GetPosNet(const CFastKey& stID, TThostFtdcVolumeType& volNetPos)
{
	SHtpGD stGD;
	CThostFtdcInvestorPositionField stPositionLong = { 0 };
	CThostFtdcInvestorPositionField stPositionShort = { 0 };
	if (false == GetPosValid(stID, stPositionLong, stPositionShort, stGD)) //获取净持仓，对消后的挂单
	{
		HTP_LOG << HTP_MSG(string("CTdThread::TargetWay.GetPosValid FALSE->").append(stID.Key()), 3777);
		return false;
	}
	//计加开仓对冲后
	volNetPos = (stPositionLong.Position + stGD.VolOBTD) - (stPositionShort.Position + stGD.VolOSTD);
	return true;
}
//获取持仓可用，注意资源共享问题
//挂平仓对可用计减，挂开仓对可用不计加
bool CTdThread::GetPosValid(const CFastKey& stID, CThostFtdcInvestorPositionField& stLong, CThostFtdcInvestorPositionField& stShort, SHtpGD& stGDX) //获取净持仓
{
	map< CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey> mapGD;
	unique_lock<mutex> lock(m_lockPOS_GD); //关系组合资源锁，暂时不改成只读共享锁
	SHtpGD stGD = ScanGD(stID, mapGD); //查询所有挂单
	stGDX = stGD; //返回原始挂单数
	//--------------------------------------------------------查询持仓-------------------------------------------------------
	TThostFtdcDirectionType	Direction = 0;
	TThostFtdcVolumeType nCmpMin = 0; //比较小值
	strcpy_s(stLong.InstrumentID, stID.Key());
	strcpy_s(stLong.ExchangeID, m_mapInstrument[stID].ExchangeID);
	stLong.PosiDirection = THOST_FTDC_PD_Long; //查询多头
	PositionWay(stLong, false);
	strcpy_s(stShort.InstrumentID, stID.Key());
	strcpy_s(stShort.ExchangeID, m_mapInstrument[stID].ExchangeID);
	stShort.PosiDirection = THOST_FTDC_PD_Short; //查询空头
	PositionWay(stShort, false);
	//剔除冻结计算可用
	//计算多头
	stLong.Position -= (stGD.VolCSYD + stGD.VolCSTD + stGD.VolCSUD);
	stLong.YdPosition -= stGD.VolCSYD;
	stLong.TodayPosition -= stGD.VolCSTD;
	nCmpMin = min(stLong.YdPosition, stGD.VolCSUD); //未定义的先对消昨仓
	stLong.YdPosition -= nCmpMin;
	stGD.VolCSUD -= nCmpMin;
	nCmpMin = min(stLong.TodayPosition, stGD.VolCSUD); //然后对消今仓
	stLong.TodayPosition -= nCmpMin;
	stGD.VolCSUD -= nCmpMin;
	//计算空头
	stShort.Position -= (stGD.VolCBYD + stGD.VolCBTD + stGD.VolCBUD);
	stShort.YdPosition -= stGD.VolCBYD;
	stShort.TodayPosition -= stGD.VolCBTD;
	nCmpMin = min(stShort.YdPosition, stGD.VolCBUD); //未定义的先对消昨仓
	stShort.YdPosition -= nCmpMin;
	stGD.VolCBUD -= nCmpMin;
	nCmpMin = min(stShort.TodayPosition, stGD.VolCBUD); //然后对消今仓
	stShort.TodayPosition -= nCmpMin;
	stGD.VolCBUD -= nCmpMin;
	if (stLong.Position<0 || stShort.Position<0 || stGD.VolCSUD > 0 || stGD.VolCBUD > 0) //平仓未被对冲掉的
	{
		HTP_LOG << HTP_MSG(string("CTdThread::GetPosValid.Position/VolCSUD/VolCBUD CALC ERROR ").append(stID.Key()), HTP_CID_FAIL | 3777);
		return false;
	}
#ifdef _SHOW_HTS
	char szMsg[256];
	//合约，目标仓位，今多，昨多，今空，昨空，挂买开仓，挂卖开仓
	sprintf_s(szMsg, "ID[%s] TARGET[X] LNET[%d] LTD[%d] LYD[%d] SNET[%d] STD[%d] SYD[%d] OB[%d] OS[%d] "
		, stID.Key()
		, stLong.Position, stLong.TodayPosition, stLong.YdPosition
		, stShort.Position, stShort.TodayPosition, stShort.YdPosition
		, stGD.VolOBTD, stGD.VolOSTD);
	HTP_LOG << HTP_MSG(string(szMsg), HTP_CID_OK | 1888);
#endif
	return true;
}
//订单方法-->交易单状态维护
//主要是处理委托回报，并对全部成交的委托更新维护到委托持仓
bool CTdThread::OrderWay(shared_ptr<const CThostFtdcOrderField> stOrder) 
{
	if (!stOrder) { return false; }
	if (stOrder->InstrumentID[0] == 0 || stOrder->OrderRef[0] == 0) { return false; } //没有引用ID
	m_lockOrder.lock();
	auto& mapOrder = m_mapOrder[stOrder->InstrumentID][stOrder->OrderRef];
	m_lockOrder.unlock();
	if ((mapOrder)&&((mapOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
		|| (mapOrder->OrderStatus == THOST_FTDC_OST_Canceled))) //该笔订单已经记录成交或取消
	{
		return false; //不再次维护
	}
	m_lockOrder.lock();
	mapOrder = stOrder; //状态维护
	m_lockOrder.unlock();
	if ((mapOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)
		|| (mapOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing))
	{
		m_htpManage.SetStatus(HTS_TRADED_PART, stOrder->InstrumentID, stOrder->OrderRef); //订单管理维护
	}
	else
	{
		m_htpManage.SetStatus(HTS_ORDED, stOrder->InstrumentID, stOrder->OrderRef); //订单管理维护
	}
	//20210924 有回报则刷新持仓到MDB
	CThostFtdcInvestorPositionField stField = {0};
	memcpy(stField.InstrumentID, stOrder->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	stField.PositionDate = THOST_FTDC_PSD_Today;
	stField.PosiDirection = THOST_FTDC_PD_Long;
	m_cqPosition.Push(stField);
	stField.PosiDirection = THOST_FTDC_PD_Short;
	m_cqPosition.Push(stField);
	if (stOrder->OrderStatus == THOST_FTDC_OST_AllTraded) //未全部成交不刷新持仓
	{
		unique_lock<mutex> lock(m_lockPOS_GD); //关系组合资源锁
		m_htpManage.SetStatus(HTS_TRADED, stOrder->InstrumentID, stOrder->OrderRef); //标记全部成交
		m_cqTimeSheet.Push(m_htpManage.GetDelay(stOrder->InstrumentID, stOrder->OrderRef));
		m_htpManage.Delete(stOrder->InstrumentID, stOrder->OrderRef); //及时从管理器中清除
		SHtpUpdPos stUpdPos = { 0 };
		stUpdPos.Direction = stOrder->Direction; ///买卖方向
		stUpdPos.OffsetFlag = stOrder->CombOffsetFlag[0]; ///开平标志
		stUpdPos.Volume = stOrder->VolumeTotalOriginal; //成交的数量
		stUpdPos.Price = stOrder->LimitPrice; //成交价格
		strcpy_s(stUpdPos.ExchangeID, stOrder->ExchangeID); ///交易所代码
		strcpy_s(stUpdPos.InstrumentID, stOrder->InstrumentID); ///合约代码
		return PositionUpdate(m_mapPositionORD, stUpdPos, true);
	}
	if (stOrder->OrderStatus != THOST_FTDC_OST_Canceled) //撤单重开
	{
		return true;
	}

	unique_lock<mutex> lock(m_lockPOS_GD); //关系组合资源锁
	m_htpManage.SetStatus(HTS_CANCEL, stOrder->InstrumentID, stOrder->OrderRef);
	m_cqTimeSheet.Push(m_htpManage.GetDelay(stOrder->InstrumentID, stOrder->OrderRef));
	m_htpManage.Delete(stOrder->InstrumentID, stOrder->OrderRef); //及时从管理器中清除
	//部分撤单需要更新已成交的部分
	if ((stOrder->VolumeTotal > 0) && (stOrder->VolumeTotalOriginal != stOrder->VolumeTotal))
	{
#ifdef _SHOW_HTS
		char szMsg[256];
		//合约，目标仓位，今多可用，昨多可用，今空可用，昨空可用，挂买开仓数，挂卖开仓数，操作方式，所有数量， 剩余未成交数量，订单引用
		sprintf_s(szMsg, "ID[%s] TARGET[X] %s %s VALL[%d] VRM[%d]  REF[%s] "
			, stOrder->InstrumentID
			, (THOST_FTDC_D_Buy == stOrder->Direction) ? "REBUY " : "RESELL "
			, (THOST_FTDC_OF_Open == stOrder->CombOffsetFlag[0])
			? "Open" : (THOST_FTDC_OF_CloseYesterday == stOrder->CombOffsetFlag[0])
			? "CloseYesterday" : "CloseToday", stOrder->VolumeTotalOriginal, stOrder->VolumeTotal, stOrder->OrderRef);
		HTP_LOG << HTP_MSG(szMsg, HTP_CID_OK | 1888);
#endif // _SHOW_HTS
		SHtpUpdPos stUpdPos = { 0 };
		stUpdPos.Direction = stOrder->Direction; ///买卖方向
		stUpdPos.OffsetFlag = stOrder->CombOffsetFlag[0]; ///开平标志
		stUpdPos.Volume = stOrder->VolumeTotalOriginal - stOrder->VolumeTotal; //已成交的数量
		stUpdPos.Price = stOrder->LimitPrice; //成交价格
		strcpy_s(stUpdPos.ExchangeID, stOrder->ExchangeID); ///交易所代码
		strcpy_s(stUpdPos.InstrumentID, stOrder->InstrumentID); ///合约代码
		if (!PositionUpdate(m_mapPositionORD, stUpdPos, true)) { return false; }
	}
	if (false == m_bStatus[CTPST_RUNING]) { return false; } //未初始化完成不进行持仓更新
	m_cRisk->Cnt(stOrder->InstrumentID).nCancelDaySum++; //撤单计加
	//检查是否需要重开
	m_lockCROP.lock();
	auto itFindCROP = m_mapCROP.find(stOrder->OrderRef);
	if ((itFindCROP != m_mapCROP.end()) && (itFindCROP->second != true)) //有标记并且标记为不重开
	{
		m_lockCROP.unlock();
		return true; //退出不重开此单
	}
	m_lockCROP.unlock();
	//没有最新行情源获取方法则不进行重新下单 //没有或找不到剩余不进行交易
	//撤单成功，撤单后再开单
	shared_ptr<CThostFtdcInputOrderField> stInputOrder = GetOrderModel(CTP_ORDM_DEFAULT); //获取下单模板
	strcpy_s(stInputOrder->InstrumentID, stOrder->InstrumentID);
	stInputOrder->Direction = stOrder->Direction;
	stInputOrder->CombOffsetFlag[0] = stOrder->CombOffsetFlag[0];
	stInputOrder->VolumeTotalOriginal = stOrder->VolumeTotal; //继续交易剩余
	m_lockTick.lock();
	shared_ptr<const CCtpTick> ctpTick = m_mapTick[stOrder->InstrumentID]; //查询最新行情
	m_lockTick.unlock();
	if (NULL == ctpTick)  //没有行情哪来的撤单重开？
	{
		HTP_LOG << HTP_MSG("CTdThread::OrderWay.ctpTick = NULL, InputOrder ERROR", HTP_CID_FAIL | 9999);
		return false;
	}
	if (ctpTick->m_stTick.InstrumentID[0] == stOrder->InstrumentID[0])
	{
		if ((stInputOrder->LimitPrice < ctpTick->m_stTick.UpperLimitPrice)
			&& (stInputOrder->LimitPrice > ctpTick->m_stTick.LowerLimitPrice)) //涨跌停区间,对价
		{
			stInputOrder->LimitPrice = (stInputOrder->Direction == THOST_FTDC_D_Buy) ? ctpTick->m_stTick.AskPrice1 : ctpTick->m_stTick.BidPrice1;
		}
		else if ((stInputOrder->LimitPrice == ctpTick->m_stTick.UpperLimitPrice)
			|| (stInputOrder->LimitPrice == ctpTick->m_stTick.LowerLimitPrice)) //涨跌停价被撤，则挂减一变动价
		{
			stInputOrder->LimitPrice += (stInputOrder->Direction == THOST_FTDC_D_Buy)
				? ctpTick->m_stTick.UpperLimitPrice - m_mapInstrument[stInputOrder->InstrumentID].PriceTick
				: ctpTick->m_stTick.LowerLimitPrice + m_mapInstrument[stInputOrder->InstrumentID].PriceTick;
		}
		else //超出涨跌停价
		{
			stInputOrder->LimitPrice += (stInputOrder->Direction == THOST_FTDC_D_Buy)
				? ctpTick->m_stTick.UpperLimitPrice
				: ctpTick->m_stTick.LowerLimitPrice;
		}
		if (CTP_PRICE_ISOUT(stOrder->LimitPrice)) //价格无效，追买
			stInputOrder->LimitPrice = (stInputOrder->Direction == THOST_FTDC_D_Buy) ? ctpTick->m_stTick.UpperLimitPrice : ctpTick->m_stTick.LowerLimitPrice;
		sprintf_s(stInputOrder->OrderRef, "%d", ++m_nRequestID);
		m_cqOrderTD.Push(make_shared<CThostFtdcInputOrderFieldEx>(stInputOrder, "", stOrder->OrderRef));
		m_htpManage.SetStatus(HTS_ORD_NEW, stInputOrder->InstrumentID, stInputOrder->OrderRef);
		m_smTrading.Release();
#ifdef _SHOW_HTS
		char szMsg[128];
		//合约，目标仓位，今多可用，昨多可用，今空可用，昨空可用，挂买开仓数，挂卖开仓数，操作方式，数量，订单引用
		sprintf_s(szMsg, "ID[%s] %s %s VOL[%d] REF[%s] FROM[%s]"
			, stInputOrder->InstrumentID
			, (THOST_FTDC_D_Buy == stInputOrder->Direction) ? "BUY " : "SELL "
			, (THOST_FTDC_OF_Open == stInputOrder->CombOffsetFlag[0])
			? "Open" : (THOST_FTDC_OF_CloseYesterday == stInputOrder->CombOffsetFlag[0])
			? "CloseYesterday" : "CloseToday", stInputOrder->VolumeTotalOriginal
			, stInputOrder->OrderRef, stOrder->OrderRef);
		HTP_LOG << HTP_MSG(szMsg, HTP_CID_OK | 1888);
#endif // _SHOW_HTS
	}
	return true;
}
//成交方法-->交易单状态维护
//主要是处理成交回报，并对全部成交的成交回报更新维护到成交持仓
bool CTdThread::TradeWay(shared_ptr<const CThostFtdcTradeField> stTrade) 
{
	if (!stTrade) { return false; }
	if (stTrade->InstrumentID[0] == 0 || stTrade->OrderRef[0] == 0) { return false; } //没有引用ID
	m_lockTrade.lock();
	auto& mapTrade = m_mapTrade[stTrade->InstrumentID][string(stTrade->OrderRef).append("|").append(stTrade->TradeID)];
	m_lockTrade.unlock();
	if (mapTrade) //该笔订单已经记录成交或取消
	{
		return false; //不再次维护
	}
	m_lockTrade.lock();
	mapTrade = stTrade; //状态维护
	m_lockTrade.unlock();
	//-----------------------------------------------计算盈亏----------------------------------------------
	m_lockPNL.lock();
	SHtpPNL& stPNL = m_mapPNL[stTrade->InstrumentID];
	TThostFtdcMoneyType dbVSumK;
	if (OFFSETFLAG_ISOPEN(stTrade->OffsetFlag))
	{
		dbVSumK = stTrade->Volume
			* ((THOST_FTDC_D_Buy == stTrade->Direction) ? 1.0 : -1.0)
			* m_mapInstrument[stTrade->InstrumentID].VolumeMultiple;
		stPNL.m_dbPVSumK += dbVSumK; //开仓量和
		stPNL.m_dbOpenSumK += stTrade->Price * dbVSumK; //开仓价量和
	}
	else
	{
		dbVSumK = stTrade->Volume
			* ((THOST_FTDC_D_Buy == stTrade->Direction) ? 1.0 : -1.0)
			* m_mapInstrument[stTrade->InstrumentID].VolumeMultiple;
		stPNL.m_dbCVSumK += dbVSumK;
		stPNL.m_dbCloseSumK += stTrade->Price * dbVSumK;
	}
	m_lockPNL.unlock();
	SHtpUpdPos stUpdPos = { 0 };
	stUpdPos.Direction = stTrade->Direction; ///买卖方向
	stUpdPos.OffsetFlag = stTrade->OffsetFlag; ///开平标志
	stUpdPos.Volume = stTrade->Volume; //成交的数量
	stUpdPos.Price = stTrade->Price; //成交价格
	strcpy_s(stUpdPos.ExchangeID, stTrade->ExchangeID); ///交易所代码
	strcpy_s(stUpdPos.InstrumentID, stTrade->InstrumentID); ///合约代码
	return PositionUpdate(m_mapPositionTRD, stUpdPos, false);
}
bool CTdThread::PositionUpdate(map<CFastKey, CThostFtdcInvestorPositionField, CFastKey>& mapPosition, SHtpUpdPos& stUpdPos, bool bUpdateMDB)
{
	//-----------------------------------------------更新持仓----------------------------------------------
	unique_lock<mutex> lock(m_lockPosition);
	TThostFtdcVolumeType minVolume = 0; //用于存储较小值，中间计算
	char szKey[64] = { 0 };
	TThostFtdcPosiDirectionType PosiDirection = 0;
	if (OFFSETFLAG_ISOPEN(stUpdPos.OffsetFlag)) //开仓对今 
		PosiDirection = (stUpdPos.Direction == THOST_FTDC_D_Buy) ? THOST_FTDC_PD_Long : THOST_FTDC_PD_Short;
	else //平仓处理
		PosiDirection = (stUpdPos.Direction == THOST_FTDC_D_Sell) ? THOST_FTDC_PD_Long : THOST_FTDC_PD_Short;

	sprintf_s(szKey, "%s|%c%c", stUpdPos.InstrumentID, PosiDirection, THOST_FTDC_PSD_Today);
	CThostFtdcInvestorPositionField& stPositionTday = mapPosition[szKey];
	if ('0' > stPositionTday.InstrumentID[0])
	{
		stPositionTday.PosiDirection = PosiDirection;
		stPositionTday.PositionDate = THOST_FTDC_PSD_Today;
		strcpy_s(stPositionTday.InstrumentID, stUpdPos.InstrumentID);
		strcpy_s(stPositionTday.ExchangeID, stUpdPos.ExchangeID);
	}
	if (OFFSETFLAG_ISOPEN(stUpdPos.OffsetFlag)) //开仓对今 
		stPositionTday.OpenVolume += stUpdPos.Volume; //开仓量累计到今仓
	else //平仓处理
		stPositionTday.CloseVolume += stUpdPos.Volume; //平仓量累计到今仓
	if (false == m_bStatus[CTPST_RUNING]) { return false; } //未初始化完成不进行持仓更新
	if (OFFSETFLAG_ISOPEN(stUpdPos.OffsetFlag)) //开仓对今 
	{
		stPositionTday.TodayPosition += stUpdPos.Volume;
		stPositionTday.Position += stUpdPos.Volume; //昨仓不随平仓而减小，总持仓与今仓随开平变化
	}
	else //平仓处理
	{
		//条件处理昨仓
		if (stUpdPos.OffsetFlag == THOST_FTDC_OF_CloseYesterday
			|| stUpdPos.OffsetFlag != THOST_FTDC_OF_CloseToday) //确定平昨对昨仓，未知仓则先对消昨仓，然后对消今仓
		{
			if (CTP_EXID_ISSHFE(stUpdPos.ExchangeID) || CTP_EXID_ISINE(stUpdPos.ExchangeID)) //上期所/能源中心持仓区分昨天与今天
			{
				sprintf_s(szKey, "%s|%c%c", stUpdPos.InstrumentID, PosiDirection, THOST_FTDC_PSD_History);
				CThostFtdcInvestorPositionField& stPositionYday = mapPosition[szKey];
				if ('0' > stPositionYday.InstrumentID[0])
				{
					stPositionYday.PosiDirection = PosiDirection;
					stPositionYday.PositionDate = THOST_FTDC_PSD_History;
					strcpy_s(stPositionYday.InstrumentID, stUpdPos.InstrumentID);
					strcpy_s(stPositionYday.ExchangeID, stUpdPos.ExchangeID);
				}
				minVolume = min(stPositionYday.Position, stUpdPos.Volume);
				stPositionYday.Position -= minVolume; //昨仓不随平仓而减小，总持仓与今仓随开平变化
				stUpdPos.Volume -= minVolume;
			}
			else //其他交易所平昨
			{
				minVolume = min((stPositionTday.Position - stPositionTday.TodayPosition), stUpdPos.Volume);
				stPositionTday.Position -= minVolume;
				stUpdPos.Volume -= minVolume;
			}
		}
		if (stUpdPos.OffsetFlag == THOST_FTDC_OF_CloseToday
			|| stUpdPos.OffsetFlag != THOST_FTDC_OF_CloseYesterday) //确定平今对今仓，未知仓则先对消昨仓，然后对消今仓
		{
			minVolume = min(stPositionTday.Position, stUpdPos.Volume);
			stPositionTday.Position -= minVolume; //昨仓不随平仓而减小，总持仓与今仓随开平变化
			stPositionTday.TodayPosition -= minVolume; //昨仓不随平仓而减小，总持仓与今仓随开平变化
			stUpdPos.Volume -= minVolume;
		}
		if (stUpdPos.Volume != 0)
		{
			HTP_LOG << HTP_MSG(string("CTdThread::TradeWay Close UpdatePos Fail->").append(stUpdPos.InstrumentID), HTP_CID_FAIL | 1477);
			return false;
		}
	}
	if (bUpdateMDB)
	{
		m_cqPosition.Push(stPositionTday);
	}
	return true;
}
//持仓方法，主要用于初始化持仓或者用于持仓查询，默认查询使用成交持仓
bool CTdThread::PositionWay(CThostFtdcInvestorPositionField& stPosition, bool bUpdateOnly)
{
	if ('0' > stPosition.InstrumentID[0]) { return false; }
	if ('0' > stPosition.ExchangeID[0]) 
	{
		strcpy_s(stPosition.ExchangeID, m_mapInstrument[stPosition.InstrumentID].ExchangeID);
	}
	unique_lock<mutex> lock(m_lockPosition);
	char szKey[64] = { 0 };
	if (bUpdateOnly) //更新持仓，确保初始化时刻更新一次，其他时刻通过回报维护持仓信息
	{
		sprintf_s(szKey, "%s|%c%c", stPosition.InstrumentID, stPosition.PosiDirection, stPosition.PositionDate);
		m_mapPositionTRD[szKey] = stPosition; //存入内存持仓表，用于计算定制持仓
		m_mapPositionORD[szKey] = stPosition; //存入内存持仓表，用于计算定制持仓	
		if (stPosition.YdPosition > 0) //统计昨仓
		{
			//-----------------------------------------------计算昨日净持仓量和系数---------------------------------------------
			m_lockPNL.lock();
			SHtpPNL& stPNL = m_mapPNL[stPosition.InstrumentID];
			if (THOST_FTDC_PD_Long == stPosition.PosiDirection)
			{
				stPNL.m_dbLYDVolK = stPosition.YdPosition
					* m_mapInstrument[stPosition.InstrumentID].VolumeMultiple;
			}
			else if (THOST_FTDC_PD_Short == stPosition.PosiDirection)
			{
				stPNL.m_dbSYDVolK = stPosition.YdPosition
					* m_mapInstrument[stPosition.InstrumentID].VolumeMultiple;
			}
			m_lockPNL.unlock();
		}
		return true;
	}
	if (CTP_EXID_ISSHFE(stPosition.ExchangeID) || CTP_EXID_ISINE(stPosition.ExchangeID)) //上期所/能源中心
	{
		memset(szKey, 0, sizeof(szKey));
		sprintf_s(szKey, "%s|%c1", stPosition.InstrumentID, stPosition.PosiDirection); //今仓
		CThostFtdcInvestorPositionField& mapPositionTD = m_mapPositionORD[szKey];
		stPosition.TodayPosition = mapPositionTD.TodayPosition;
		stPosition.CloseVolume = mapPositionTD.CloseVolume;
		stPosition.OpenVolume = mapPositionTD.OpenVolume;
		memset(szKey, 0, sizeof(szKey));
		sprintf_s(szKey, "%s|%c2", stPosition.InstrumentID, stPosition.PosiDirection); //昨仓
		CThostFtdcInvestorPositionField& mapPositionYD = m_mapPositionORD[szKey];
		stPosition.YdPosition = mapPositionYD.Position;
		stPosition.Position = stPosition.YdPosition + stPosition.TodayPosition; //总仓
		//if (THOST_FTDC_PD_Long == stPosition.PosiDirection) //冻结单独自定义维护
		//	stPosition.LongFrozen = mapPositionTD.LongFrozen+ mapPositionYD.LongFrozen;
		//else if (THOST_FTDC_PD_Short == stPosition.PosiDirection)
		//	stPosition.ShortFrozen = mapPositionTD.ShortFrozen + mapPositionYD.ShortFrozen;
		//else {}
	}
	else
	{
		memset(szKey, 0, sizeof(szKey));
		sprintf_s(szKey, "%s|%c1", stPosition.InstrumentID, stPosition.PosiDirection); //今仓
		CThostFtdcInvestorPositionField& mapPosition = m_mapPositionORD[szKey];
		stPosition.TodayPosition = mapPosition.TodayPosition;
		stPosition.CloseVolume = mapPosition.CloseVolume;
		stPosition.OpenVolume = mapPosition.OpenVolume;
		stPosition.Position = mapPosition.Position;
		stPosition.YdPosition = stPosition.Position - stPosition.TodayPosition; //非上期/能源
		//if(THOST_FTDC_PD_Long == stPosition.PosiDirection) //冻结单独自定义维护
		//	stPosition.LongFrozen = mapPosition.LongFrozen;
		//else if (THOST_FTDC_PD_Short == stPosition.PosiDirection)
		//	stPosition.ShortFrozen = mapPosition.ShortFrozen;
		//else{}
	}
	return true;
}
//集中竞价方法，规定的时间推送集合竞价订单
void CTdThread::AuctionWay()
{
	if (!m_cRisk->GetEnable(REN_TDRADE)) { return; }
	CTimeCalc ctpTime = HTP_SYS.GetTime(CTP_TIME_NET);
	CTPTIME nTime = ctpTime.GetHMS(); //商品所时间
	bool bCFFEX = false;
	if ((INT_CFFEX_AUCTIME_BEGIN < nTime) && (nTime < INT_CFFEX_AUCTIME_END))  //20-40区间，推中金所集合竞价
	{
		bCFFEX = true;
	}
	else if (((INT_AM_AUCTIME_BEGIN < nTime) && (nTime < INT_AM_AUCTIME_END))
		|| ((INT_PM_AUCTIME_BEGIN < nTime) && (nTime < INT_PM_AUCTIME_END))) //20-40区间，推非中金所集合竞价
	{
		bCFFEX = false;
	}
	else { return; } //其他时间有行情TICK驱动交易而非时间驱动
	//---------------------------------------------------------集合竞价-------------------------------------------------
	map<CFastKey, SCtpTarget, CFastKey> mapTarget;
	string strTargetID;
	if (1 > m_ctpTargetCtrl.GetTarget(mapTarget, ctpTime, strTargetID)) { return; }
	for (const auto& itTarget : mapTarget) //规则检查
	{
		//检查交易所
		const auto& itInfo = m_mapInstrument[itTarget.first];
		if (((bCFFEX) && !CTP_EXID_ISCFFEX(itInfo.ExchangeID))
			|| ((!bCFFEX) && CTP_EXID_ISCFFEX(itInfo.ExchangeID)))
		{
			continue; //交易所过滤
		}
		//检查状态
		//m_lockStatus.lock();
		//auto itStatus = m_mapStatus.find(itTarget.first);
		//m_lockStatus.unlock();
		//if ((itStatus == m_mapStatus.end())
		//	|| (itStatus->second != THOST_FTDC_IS_AuctionOrdering))
		//{
		//	continue; //不处于集合竞价阶段/合约状态不对
		//}
		//检查行情
		m_lockTick.lock();
		shared_ptr<const CCtpTick> ctpTick = m_mapTick[itTarget.first]; //查询最新行情
		m_lockTick.unlock();
		if (NULL == ctpTick)  //找不到行情，则需要询价获取价格
		{
			m_bStatus[CTPST_ONQRYTICK] = false; continue;
		}
		if (240 < abs(ctpTick->m_ctNature.GetTickHMS() - ctpTime.GetTickHMS())) //TICK过期4分钟
		{
			m_bStatus[CTPST_ONQRYTICK] = false; continue;
		}
		m_lockTarget.lock();
		//生成订单
		if (TargetWay(ctpTick->m_stTick, mapTarget[itTarget.first].Position, strTargetID))
		{
			m_ctpTargetCtrl.PopTarget(ctpTick->m_stTick.InstrumentID); //清除target
			m_smTrading.Release(); //触发交易
		}
		m_lockTarget.unlock();
	}
}
//定时任务
void CTdThread::ThreadTime()
{
	HTP_LOG << "CTdThread::ThreadTime" << HTP_ENDL;
	ULONGLONG nTimeTick = GetTickCount64();
	ULONGLONG nTickCnt[10] = { 0 };
	while (m_bThRunning)
	{
		nTimeTick = GetTickCount64() - nTimeTick;
		if (10000 > nTimeTick)
			m_smTime.Wait(DWORD(10000 - nTimeTick));
		nTimeTick = GetTickCount64();
		//if (!m_bIsFrontConnected) //需要连接前置
		//{
		//	HTP_LOG << HTP_MSG("正在尝试重新连接前置", HTP_CID_ISUTF8);
		//	m_pApi->Init();
		//	Sleep(1000); //查询事务延迟
		//}
		//---------------------------------------------------掉线登录------------------------------------------------
		if ((++nTickCnt[1] > 0) && (!m_bStatus[CTPST_LOGINON]) && (m_bStatus[CTPST_FRONTON])) //需要重新登录
		{
			nTickCnt[1] = 0;
			HTP_LOG << HTP_MSG("正在尝试重新登录", HTP_CID_ISUTF8);
			m_cRisk->SysCnt(RSC_LOGIN)++;
			//m_pApi->ReqUserLogout(&m_UserCfg->GetLogout(), ++m_nRequestID);
			//Sleep(2000); //查询事务延迟
			m_pApi->ReqAuthenticate(&m_UserCfg->GetAuthenticate(), ++m_nRequestID);
			Sleep(2000); //查询事务延迟
			m_pApi->ReqUserLogin(&m_UserCfg->GetLogin(), ++m_nRequestID);
			Sleep(2000); //查询事务延迟
		}
		//---------------------------------------------------资金同步------------------------------------------------
		if ((++nTickCnt[2] > 2) && m_bStatus[CTPST_LOGINON]) //10S刷新一次资金
		{
			nTickCnt[2] = 0;
			int iResult = m_pApi->ReqQryTradingAccount(&m_UserCfg->GetAccount(), ++m_nRequestID);
			if (iResult != 0)
				HTP_LOG << "CTdThread::ThreadTime.ReqQryTradingAccount FAILED" << HTP_ENDL;
			Sleep(1000); //查询事务延迟
		}
		if (!m_bStatus[CTPST_ONQRYTICK])
		{
			CThostFtdcQryDepthMarketDataField stQry = { 0 };
			//strcpy_s(stQry.InstrumentID, strQryTick.c_str());
			int iResult = m_pApi->ReqQryDepthMarketData(&stQry, ++m_nRequestID);
			if (iResult < 0)
			{
				HTP_LOG << HTP_MSG("CTdThread::ThreadTime.ReqQryDepthMarketData Failed", HTP_CID_FAIL | iResult);
			}
			HTP_LOG << HTP_MSG(string("CTdThread::ThreadTime.ReqQryDepthMarketData OK"), HTP_CID_OK | iResult);
			Sleep(10000);
		}
		//---------------------------------------------------TARGET获取------------------------------------------------
		if ((++nTickCnt[3] > 0) && (0 < m_MDB->LoadTarget(m_ctpTargetCtrl)))
		{
			nTickCnt[3] = 0;
			HTP_LOG << string("*********************************************Get A New Target :")
				.append(m_ctpTargetCtrl.GetUserID())
				.append("*********************************************") << HTP_ENDL;
		}
		//---------------------------------------------------RiskCtrl获取------------------------------------------------
		if ((++nTickCnt[4] > 0) && (0 < m_MDB->LoadRiskCtrl()))
		{
			nTickCnt[4] = 0;
			HTP_LOG << string("*********************************************NEW LoadRiskCtrl :")
				.append(HTP_SYS.GetTime(CTP_TIME_NET).GetStrYMDHMSL())
				.append("*********************************************") << HTP_ENDL;
		}
		//---------------------------------------------------TARGET进度刷新------------------------------------------------
		if (++nTickCnt[5] > 2)
		{
			nTickCnt[5] = 0;
			map<CFastKey, SCtpTaskRatio, CFastKey> mapRatio;
			if (0 < m_ctpTargetCtrl.GetTaskRatio(mapRatio)) //没有target不刷进度
			{
				m_MDB->UpdateTask(mapRatio);
			}
		}
		//---------------------------------------------------持仓同步------------------------------------------------
		m_lockTaskSyn.lock();
		auto& itORD = m_mapSynPosORD.begin();
		m_lockTaskSyn.unlock();
		if ((m_bStatus[CTPST_LOGINON]) && (itORD != m_mapSynPosORD.end())) //刷新相关持仓
		{
			CThostFtdcQryOrderField stQryOrder = m_UserCfg->GetOrder();
			memcpy(stQryOrder.ExchangeID, itORD->second->ExchangeID, sizeof(TThostFtdcExchangeIDType));
			//memcpy(stQryOrder.InstrumentID, stQryPosition.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
			int iResult = m_pApi->ReqQryOrder(&stQryOrder, ++m_nRequestID);
			if (iResult < 0)
			{
				HTP_LOG << HTP_MSG("CTdThread::ThreadTime.ReqQryOrder Failed", HTP_CID_FAIL | iResult);
			}
			HTP_LOG << HTP_MSG(string("CTdThread::ThreadTime.ReqQryOrder OK->").append(stQryOrder.ExchangeID), HTP_CID_OK | iResult);
			Sleep(5000);
			m_lockTaskSyn.lock();
			m_mapSynPosORD.erase(itORD->first);
			m_lockTaskSyn.unlock();
		}
		m_lockTaskSyn.lock();
		auto& itTRD = m_mapSynPosTRD.begin();
		m_lockTaskSyn.unlock();
		if ((m_bStatus[CTPST_LOGINON]) && (itTRD != m_mapSynPosTRD.end())) //刷新相关持仓
		{
			CThostFtdcQryTradeField stQryTrade = m_UserCfg->GetTrade();
			memcpy(stQryTrade.ExchangeID, itTRD->second->ExchangeID, sizeof(TThostFtdcExchangeIDType));
			memcpy(stQryTrade.InstrumentID, itTRD->second->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
			int iResult = m_pApi->ReqQryTrade(&stQryTrade, ++m_nRequestID);
			if (iResult < 0)
			{
				HTP_LOG << HTP_MSG("CTdThread::ThreadTime.ReqQryTrade Failed", HTP_CID_FAIL | iResult);
			}
			HTP_LOG << HTP_MSG(string("CTdThread::ThreadTime.ReqQryTrade OK->").append(stQryTrade.InstrumentID), HTP_CID_OK | iResult);
			Sleep(2000);
			m_lockTaskSyn.lock();
			m_mapSynPosTRD.erase(itTRD->first);
			m_lockTaskSyn.unlock();
		}
	}
	HTP_LOG << "CTdThread::ThreadTime END" << HTP_ENDL;
}
//MDB数据存储交易数据，包括成交回报，委托日志与回报，资金持仓等
void CTdThread::ThreadFlash()
{
	HTP_LOG << "CTdThread::ThreadFlash" << HTP_ENDL;
	shared_ptr < const CThostFtdcOrderField> stOrder = NULL;
	shared_ptr<const CThostFtdcInputOrderFieldEx> stOrderLog = NULL; //本地产生的订单
	shared_ptr<const CThostFtdcTradeField> stTrade = NULL;
	shared_ptr<const SHtpTimeSheet> stTimeSheet = NULL;
	CThostFtdcInvestorPositionField stPosition = { 0 };
	map< CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey> mapGD;
	char szMsg[256] = { 0 };
	ULONGLONG nTimeTick = GetTickCount64();
	while (m_bThRunning)
	{
		nTimeTick = GetTickCount64() - nTimeTick;
		if (1000 > nTimeTick)
			m_smFlash.Wait(DWORD(1000 - nTimeTick));
		nTimeTick = GetTickCount64();
		AuctionWay(); //集中竞价检测报单
		while (m_cqPosition.Front(stPosition)) //刷新持仓
		{
			m_cqPosition.Pop();
			m_lockPOS_GD.lock(); //关系组合资源锁，暂时不改成只读共享锁
			PositionWay(stPosition, false); //转换为MDB输出要求的持仓
			SHtpGD stGD = ScanGD(stPosition.InstrumentID, mapGD);
			m_lockPOS_GD.unlock();
			m_MDB->OnTdPosition(stPosition, stGD);
			if ((THOST_FTDC_PD_Long == stPosition.PosiDirection)
				&& (m_cRisk->Cnt(stPosition.InstrumentID).nLongPosMax < stPosition.Position))
				m_cRisk->Cnt(stPosition.InstrumentID).nLongPosMax = stPosition.Position;
			else if ((THOST_FTDC_PD_Short == stPosition.PosiDirection)
				&& (m_cRisk->Cnt(stPosition.InstrumentID).nShortPosMax < stPosition.Position))
				m_cRisk->Cnt(stPosition.InstrumentID).nShortPosMax = stPosition.Position;
		}
		while (m_cqTrade.Front(stTrade)) //刷新交易数据
		{
			m_cqTrade.Pop();
			if (!stTrade) { continue; }
			m_MDB->OnTdTrade(stTrade);
			if (true == m_bStatus[CTPST_RUNING]) //初始化完成后
			{
				sprintf_s(szMsg, "%s[%d]", stTrade->InstrumentID, (int)(100.0 * m_ctpTargetCtrl.GetTaskRatio(stTrade->InstrumentID).fVlmRatioDone));
				HTP_LOG << szMsg;
			}
		}
		while (m_cqOrder.Front(stOrder)) //刷新合约订单数据
		{
			m_cqOrder.Pop();
			if (!stOrder) { continue; }
			m_MDB->OnTdOrder(stOrder);
		}
		while (m_cqOrderLog.Front(stOrderLog)) //刷新合约订单数据
		{
			m_cqOrderLog.Pop();
			if (!stOrderLog) { continue; }
			m_MDB->OnTdOrderLog(stOrderLog);
		}
		while (m_cqTimeSheet.Front(stTimeSheet)) //刷新延迟
		{
			m_cqTimeSheet.Pop();
			if (!stTimeSheet) { continue; }
			m_MDB->OnTdTimeSheet(stTimeSheet);
		}
		
	}
	HTP_LOG << "CTdThread::ThreadFlash EXIT" << HTP_ENDL;
}
//执行交易，交易执行前会进行相关的风控检查
bool CTdThread::TradDoing(shared_ptr<CThostFtdcInputOrderField> stOrder)
{
	if (!stOrder) { return false; }
	if (false == m_cRisk->Check(stOrder)) { return false; } //风控检查失败
	if (false == m_bStatus[CTPST_LOGINON]) { return false; } //掉线不进行下单
	m_htpManage.SetStatus(HTS_ORD_DOING, stOrder->InstrumentID, stOrder->OrderRef);
	int iResult = m_pApi->ReqOrderInsert(stOrder.get(), ++m_nRequestID);
	m_htpManage.SetStatus(HTS_ORD_DONE, stOrder->InstrumentID, stOrder->OrderRef);
	char szMsg[16] = { 0 };
	if (iResult == 0)
	{
		m_cRisk->Cnt(stOrder->InstrumentID).nOrderDaySum++; //挂单计加
		m_cRisk->Cnt(stOrder->InstrumentID).nOrderMinSum++; //挂单计加
		if (m_cRisk->Cnt(stOrder->InstrumentID).nOrderVolMax < stOrder->VolumeTotalOriginal)
			m_cRisk->Cnt(stOrder->InstrumentID).nOrderVolMax = stOrder->VolumeTotalOriginal;
	}
	else
	{
		m_cRisk->SysCnt(RSC_ORDFAIL)++;
		sprintf_s(szMsg, "%d", stOrder->VolumeTotalOriginal);
		HTP_LOG << HTP_MSG(string("CTdThread::ThreadTrading.ReqOrderInsert FAIL-")
			.append(stOrder->InstrumentID).append("-").append(szMsg), 1200);
		return false;
	}
	sprintf_s(szMsg, " %d ", stOrder->VolumeTotalOriginal);
	string strMsg(stOrder->InstrumentID);
	strMsg.append((stOrder->Direction == '0') ? " BUY " : " SELL ").append(szMsg);
	switch (stOrder->CombOffsetFlag[0])
	{
	case THOST_FTDC_OF_Open: strMsg.append("Open"); break;
		///平仓
	case THOST_FTDC_OF_Close: strMsg.append("Close"); break;
		/////强平
		//case THOST_FTDC_OF_ForceClose: strMsg.append("ForceClose"); break;
		///平今
	case THOST_FTDC_OF_CloseToday: strMsg.append("CloseToday"); break;
		///平昨
	case THOST_FTDC_OF_CloseYesterday: strMsg.append("CloseYesterday"); break;
		/////强减
		//case THOST_FTDC_OF_ForceOff: strMsg.append("ForceOff"); break;
		/////本地强平
		//case THOST_FTDC_OF_LocalForceClose: strMsg.append("LocalForceClose"); break;
	default: break;
	}
	HTP_LOG << HTP_MSG(strMsg, 1500);
	return (iResult == 0);
}
bool CTdThread::CheckDoing(const SCtpTick& stTgtTick)
{
	int HaveDoing = m_htpManage.HaveDoing(stTgtTick.InstrumentID);
	if (HaveDoing > 0)
	{
		HTP_LOG << "CTdThread::CheckDoing HaveDoing"; return false;
	} //检测是否可以生成订单,有未完成的订单
	if (HaveDoing < 0) //有订单超时或持仓对比失败，同步持仓（分订单持仓与成交持仓）
	{
		if (HaveDoing == -2)
		{
			HTP_LOG << "CTdThread::CheckDoing HaveDoing TIMEOUT";
			Stop(); //严重超时，可能接口掉线，需要重新登录
			//Sleep(10000);
		}
		CThostFtdcQryInvestorPositionField stQryPosition = m_UserCfg->GetPosition();
		memcpy(stQryPosition.ExchangeID, stTgtTick.ExchangeID, sizeof(TThostFtdcExchangeIDType));
		memcpy(stQryPosition.InstrumentID, stTgtTick.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
		auto stQry = make_shared<CThostFtdcQryInvestorPositionField>(stQryPosition);
		m_lockTaskSyn.lock();
		m_mapSynPosTRD[stQryPosition.InstrumentID] = stQry; //同步成交持仓
		m_mapSynPosORD[stQryPosition.ExchangeID] = stQry; //同步订单持仓
		m_lockTaskSyn.unlock();
		//m_cqQryPosition.Push(stQryPosition); //添加查询持仓同步
		HTP_LOG << HTP_MSG(string("CTdThread::CheckDoing.CHECK FALSE->").append(stQryPosition.InstrumentID), 3777);
		return false;
	}
	return true;
}
//TARGET触发线程，处理来自行情的TICK触发
void CTdThread::ThreadAlgo()
{
	HTP_LOG << "CTdThread::ThreadAlgo" << HTP_ENDL;
	shared_ptr<const CCtpTick> ctpTick = NULL;
	SCtpTarget stTarget = { 0 };
	string strTargetID;
	while (m_bThRunning)
	{
		m_smAlgo.Wait(1000);
		while (m_cqTick.Front(ctpTick)) //有订单需要交易 引用+1
		{
			m_cqTick.Pop(); //引用被替换-1
			if (!ctpTick) { continue; }
			if ((CheckDoing(ctpTick->m_stTick)) && (m_ctpTargetCtrl.GetTarget(ctpTick, stTarget, strTargetID))) //获取target&不清除
			{
				m_lockTarget.lock();
				if (TargetWay(ctpTick->m_stTick, stTarget.Position, strTargetID))//生成订单
				{
					m_ctpTargetCtrl.PopTarget(ctpTick->m_stTick.InstrumentID); //清除target
					m_smTrading.Release(); //触发交易
				}
				m_lockTarget.unlock();
			}
			else //没有TARGET则检查超时挂单
			{
				map< CFastKey, shared_ptr<const CThostFtdcOrderField>, CFastKey> mapGD;
				ScanGD(ctpTick->m_stTick.InstrumentID, mapGD, HTP_TRADE_TIMEOUTMS); //超时撤单重开
				for (auto& itGD : mapGD)
				{
					if (!itGD.second) { continue; }
					if (0 < CancelWay(itGD.second)) //超时撤单
					{
#ifdef _SHOW_HTS
						char szMsg[128];
						//合约，目标仓位，今多，昨多，今空，昨空，挂买开仓，挂卖开仓
						sprintf_s(szMsg, "ID[%s] CANCEL REF[%s] TIMEOUT[%d] "
							, itGD.second->InstrumentID, itGD.second->OrderRef, HTP_TRADE_TIMEOUTMS);
						HTP_LOG << HTP_MSG(szMsg, HTP_CID_OK | 1888);
#endif // DEBUG
					}
				}
			}
		}
	}
	HTP_LOG << "CTdThread::ThreadAlgo EXIT" << HTP_ENDL;
}
//订单交易执行线程：执行下单或撤单等相关
//下单不成功则需要维护订单管理队列
void CTdThread::ThreadTrading()
{
	HTP_LOG << "CTdThread::ThreadTrading" << HTP_ENDL;
	shared_ptr<CThostFtdcInputOrderFieldEx> stOrderTD;
	shared_ptr<CThostFtdcInputOrderActionField> stAction = { 0 };
	char szMsg[256] = { 0 };
	int iResult = 0;
	while (m_bThRunning)
	{
		m_smTrading.Wait(1000);
		while (m_cqOrderTD.Front(stOrderTD)) //有订单需要交易 引用+1
		{
			m_cqOrderLog.Push(stOrderTD);
			m_cqOrderTD.Pop(); //引用被替换-1
			if (!stOrderTD) { continue; }
			if (false == TradDoing(stOrderTD->data())) //行情变动引发交易
			{
				m_cqTimeSheet.Push(m_htpManage.GetDelay(stOrderTD->data()->InstrumentID, stOrderTD->data()->OrderRef));
				m_htpManage.Delete(stOrderTD->data()->InstrumentID, stOrderTD->data()->OrderRef); //下单失败则清除
			}
		}
		while (m_cqActionTD.Front(stAction))//有撤单
		{
			m_cqActionTD.Pop();
			if (!stAction) { continue; }
			if (false == m_bStatus[CTPST_LOGINON]) { return; } //掉线不进行下单
			m_htpManage.SetStatus(HTS_CCL_DOING, stAction->InstrumentID, stAction->OrderRef); //标记撤单中
			iResult = m_pApi->ReqOrderAction(stAction.get(), ++m_nRequestID);
			if (iResult == 0)
			{
				m_htpManage.SetStatus(HTS_CCL_DONE, stAction->InstrumentID, stAction->OrderRef); //标记撤单中
			}
			else
			{
				HTP_LOG << HTP_MSG(string("CTdThread::ThreadTrading.ReqOrderAction FAIL")
					.append("-").append(stAction->InstrumentID)
					.append("-").append(stAction->OrderSysID)
					.append("-").append(stAction->OrderRef), 1200);
			}
			sprintf_s(szMsg, "%s-%s-%d DELETE", stAction->InstrumentID, stAction->OrderRef, stAction->SessionID);
			HTP_LOG << HTP_MSG(szMsg, 1500);
		}
	}
	HTP_LOG << "CTdThread::ThreadTrading EXIT" << HTP_ENDL;
}

///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void CTdThread::OnFrontConnected() 
{
	HTP_LOG << "CTdThread::OnFrontConnected" << HTP_ENDL;
	m_bStatus[CTPST_FRONTON] = true;
	m_eStatus = eCtpStatus::CTPST_FRONTON;
};

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
void CTdThread::OnFrontDisconnected(int nReason) 
{
	HTP_LOG << "CTdThread::OnFrontDisconnected" << HTP_ENDL;
	m_bStatus[CTPST_FRONTON] = false;
	m_bStatus[CTPST_LOGINON] = false;
	m_eStatus = eCtpStatus::CTPST_FRONTOFF;
}

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
void CTdThread::OnHeartBeatWarning(int nTimeLapse) 
{
	HTP_LOG << "CTdThread::OnHeartBeatWarning" << HTP_ENDL;
	m_bStatus[CTPST_FRONTON] = false;
	m_bStatus[CTPST_LOGINON] = false;
	m_eStatus = CTPST_FAILED;
}

///客户端认证响应
void CTdThread::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	HTP_LOG << "CTdThread::OnRspAuthenticate" << HTP_ENDL;
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED;
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (!pRspAuthenticateField) { return; }
	m_eStatus = eCtpStatus::CTPST_ONAUTHEN;
}


	///登录请求响应
void CTdThread::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	HTP_LOG << "CTdThread::OnRspUserLogin" << HTP_ENDL;
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED;
		m_bStatus[CTPST_LOGINON] = (pRspInfo->ErrorID == 60);
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	m_bStatus[CTPST_LOGINON] = true;
	if (!pRspUserLogin) { return; }
	//初始化相关时间等信息
	HTP_SYS.SetTime(CTP_TIME_TRADING, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->SHFETime, 0));
	HTP_SYS.SetTime(CTP_TIME_INE, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->INETime, 0));
	HTP_SYS.SetTime(CTP_TIME_CFFEX, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->FFEXTime, 0));
	HTP_SYS.SetTime(CTP_TIME_CZCE, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->CZCETime, 0));
	HTP_SYS.SetTime(CTP_TIME_DCE, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->DCETime, 0));
	HTP_SYS.SetTime(CTP_TIME_SHFE, CTimeCalc(pRspUserLogin->TradingDay, pRspUserLogin->SHFETime, 0));
	m_eStatus = eCtpStatus::CTPST_LOGINON;
}

///登出请求响应
void CTdThread::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	HTP_LOG << "CTdThread::OnRspUserLogout" << HTP_ENDL;
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED;
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (!pUserLogout) { return; }
	m_bStatus[CTPST_LOGINON] = false;
	m_eStatus = CTPST_LOGINOFF;
}

//	///用户口令更新请求响应
//void CTdThread::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///资金账户口令更新请求响应
//void CTdThread::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
///报单录入请求响应
void CTdThread::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		if ((pInputOrder) && (pInputOrder->OrderRef[0] != '\0')) //订单管理：标记已下单&移除
		{
			m_cqTimeSheet.Push(m_htpManage.GetDelay(pInputOrder->InstrumentID, pInputOrder->OrderRef));
			m_htpManage.Delete(pInputOrder->InstrumentID, pInputOrder->OrderRef);
		}
		m_eStatus = CTPST_FAILED;
		HTP_LOG << HTP_MSG(string("CTdThread::OnRspOrderInsert: ").append(pRspInfo->ErrorMsg)
			.append("->").append((!pInputOrder) ? "*" : pInputOrder->InstrumentID)
			.append("-").append((!pInputOrder) ? "*" : pInputOrder->OrderRef), HTP_CID_FAIL | pRspInfo->ErrorID);
		m_cRisk->Cnt((!pInputOrder) ? "*" : pInputOrder->InstrumentID).nCancelDaySum++; //撤单计加
		m_cRisk->SysCnt(RSC_ORDFAIL)++; //订单失败计加
		return;
	}
	//----------HTP_LOG << *pInputOrder;
}

//	///预埋单录入请求响应
//void CTdThread::OnRspParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///预埋撤单录入请求响应
//void CTdThread::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
///报单操作请求响应
void CTdThread::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(string("CTdThread::OnRspOrderAction: ").append(pRspInfo->ErrorMsg)
			.append("->").append(pInputOrderAction->InstrumentID)
			.append("-").append(pInputOrderAction->OrderRef), pRspInfo->ErrorID); 
		m_cRisk->Cnt(pInputOrderAction->InstrumentID).nCancelDaySum++; //撤单计加
		m_cRisk->SysCnt(RSC_ORDFAIL)++; //订单失败计加
		return;
	}
	//----------HTP_LOG << *pInputOrderAction;
}

//	///查询最大报单数量响应
//void CTdThread::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField* pQueryMaxOrderVolume, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
///投资者结算结果确认响应
void CTdThread::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	HTP_LOG << "CTdThread::OnRspSettlementInfoConfirm" << HTP_ENDL;
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	m_eStatus = eCtpStatus::CTPST_CONFIRM;
	//----------HTP_LOG << *pSettlementInfoConfirm;
}

//	///删除预埋单响应
//void CTdThread::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///删除预埋撤单响应
//void CTdThread::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///执行宣告录入请求响应
//void CTdThread::OnRspExecOrderInsert(CThostFtdcInputExecOrderField* pInputExecOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///执行宣告操作请求响应
//void CTdThread::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField* pInputExecOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///询价录入请求响应
//void CTdThread::OnRspForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///报价录入请求响应
//void CTdThread::OnRspQuoteInsert(CThostFtdcInputQuoteField* pInputQuote, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///报价操作请求响应
//void CTdThread::OnRspQuoteAction(CThostFtdcInputQuoteActionField* pInputQuoteAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///批量报单操作请求响应
//void CTdThread::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField* pInputBatchOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///期权自对冲录入请求响应
//void CTdThread::OnRspOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField* pInputOptionSelfClose, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///期权自对冲操作请求响应
//void CTdThread::OnRspOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField* pInputOptionSelfCloseAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求核心间资金转移响应
//void CTdThread::OnRspTransFund(CThostFtdcTransFundField* pTransFund, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
	///请求查询报单响应
void CTdThread::OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (pOrder)
	{
		auto stOrder = make_shared<CThostFtdcOrderField>(*pOrder);
		memcpy(stOrder->ExchangeID, m_mapInstrument[stOrder->InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
		OrderWay(stOrder);
		m_cqOrder.Push(stOrder);
	}
	if ((bIsLast) || (!pOrder))
	{
		m_smFlash.Release();
		HTP_LOG << "CTdThread::OnRspQryOrder" << HTP_ENDL;
		m_eStatus = eCtpStatus::CTPST_ONQRYORDER;
	}
}

	///请求查询成交响应
void CTdThread::OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (pTrade)
	{
		auto stTrade = make_shared<CThostFtdcTradeField>(*pTrade);
		memcpy(stTrade->ExchangeID, m_mapInstrument[stTrade->InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
		TradeWay(stTrade);
		m_cqTrade.Push(stTrade);
	}
	if ((bIsLast) || (!pTrade))
	{
		m_smFlash.Release();
		HTP_LOG << "CTdThread::OnRspQryTrade" << HTP_ENDL;
		m_eStatus = eCtpStatus::CTPST_ONQRYTRADE;
	}
}

///请求查询投资者持仓响应
void CTdThread::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		string strInfo(pRspInfo->ErrorMsg);
		strInfo.append("->").append(pInvestorPosition->InstrumentID)
			.append("-").push_back(pInvestorPosition->PosiDirection);
		HTP_LOG << HTP_MSG(strInfo, pRspInfo->ErrorID); return;
	}
	if (pInvestorPosition)
	{
		CThostFtdcInvestorPositionField stField = *pInvestorPosition;
		memcpy(stField.ExchangeID, m_mapInstrument[stField.InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
		PositionWay(stField, true);
		m_cqPosition.Push(stField);
		m_smFlash.Release();
	}
	if ((bIsLast) || (!pInvestorPosition))
	{
		m_eStatus = CTPST_ONQRYPOSITION;
	}
}

///请求查询资金账户响应
void CTdThread::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (!pTradingAccount) { return; }
	m_bStatus[CTPST_ONACCOUNT] = true;
	CThostFtdcTradingAccountField stAccount = *pTradingAccount;
	m_cthLock.lock();
	m_stAccount = stAccount;
	m_cthLock.unlock();
	//-----------------------------------------------计算PNL---------------------------------------------------
	m_lockPNL.lock();
	auto mapPNL = m_mapPNL; //获取PNL快照
	m_lockPNL.unlock();
	auto& itAll = mapPNL["ALL"];
	memset(&itAll, 0, sizeof(SHtpPNL)); //清除客户级汇总
	for (auto& itPNL : mapPNL) //计算汇总PNL
	{
		m_lockTick.lock();
		shared_ptr<const CCtpTick> ctpTick = m_mapTick[itPNL.first]; //获取合约的行情
		m_lockTick.unlock();
		if (!ctpTick) { continue; } //有行情则进行数据刷新
		const SCtpTick& stTick = ctpTick->m_stTick;
		itPNL.second.m_dbProfitOpenTD = stTick.LastPrice * itPNL.second.m_dbPVSumK
			- itPNL.second.m_dbOpenSumK;
		itAll.m_dbProfitOpenTD += itPNL.second.m_dbProfitOpenTD;
		itPNL.second.m_dbProfitCloseTD = stTick.LastPrice * itPNL.second.m_dbCVSumK
			- itPNL.second.m_dbCloseSumK;
		itAll.m_dbProfitCloseTD += itPNL.second.m_dbProfitCloseTD;
		itPNL.second.m_dbProfitPositionYD = (stTick.LastPrice - stTick.PreSettlementPrice)
			* (itPNL.second.m_dbLYDVolK - itPNL.second.m_dbSYDVolK); //对冲后盈亏
		itAll.m_dbProfitPositionYD += itPNL.second.m_dbProfitPositionYD;
	}
	m_MDB->OnTdAccount(stAccount, mapPNL);
	m_eStatus = CTPST_ONACCOUNT;
}

//	///请求查询投资者响应
//void CTdThread::OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询交易编码响应
//void CTdThread::OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询合约保证金率响应
//void CTdThread::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
///请求查询合约手续费率响应
void CTdThread::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (pInstrumentCommissionRate)
	{
		//m_cthLock.lock();
		m_mapCommission[pInstrumentCommissionRate->InstrumentID] = (*pInstrumentCommissionRate);
		//m_cthLock.unlock();
	}

	if ((bIsLast) || (!pInstrumentCommissionRate))//最后一个或者没有
	{
		m_eStatus = eCtpStatus::CTPST_ONCMSRATE;
	}
}
//	///请求查询交易所响应
//void CTdThread::OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询产品响应
//void CTdThread::OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
///请求查询合约响应
void CTdThread::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if ((pInstrument)&&(pInstrument->ProductClass == THOST_FTDC_PC_Futures)) //过滤期货
	{
		//m_cthLock.lock();
		m_mapInstrument[pInstrument->InstrumentID] = (*pInstrument);
		m_mapProduct[pInstrument->ProductID][pInstrument->InstrumentID] = (*pInstrument);
		//m_cthLock.unlock();
	}

	if ((bIsLast) || (!pInstrument))//最后一个或者没有
	{
		m_eStatus = eCtpStatus::CTPST_ONGETID;
	}
}

	///请求查询行情响应
void CTdThread::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
	if (pDepthMarketData)
	{
		if (m_mapInstrument.find(pDepthMarketData->InstrumentID) == m_mapInstrument.end()) { return; } //非标准合约
		auto& ctpTick = make_shared<CCtpTick>(*pDepthMarketData);
		//memcpy(ctpTick->m_stTick.ExchangeID, m_mapInstrument[ctpTick->m_stTick.InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
		m_lockTick.lock();
		m_mapTick[ctpTick->m_stTick.InstrumentID] = ctpTick;
		m_lockTick.unlock();
	}
	m_bStatus[CTPST_ONQRYTICK] = true;
	if ((bIsLast) || (!pDepthMarketData))//最后一个或者没有
	{
		m_eStatus = eCtpStatus::CTPST_ONQRYTICK;
		HTP_LOG << "CTdThread::OnRspQryDepthMarketData" << HTP_ENDL;
	}
}
//	///请求查询投资者结算结果响应
//void CTdThread::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询合约状态响应
//void CTdThread::OnRspQryInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询转帐银行响应
//void CTdThread::OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询投资者持仓明细响应
//void CTdThread::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询客户通知响应
//void CTdThread::OnRspQryNotice(CThostFtdcNoticeField* pNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询结算信息确认响应
//void CTdThread::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
//
//	///请求查询投资者持仓明细响应
//void CTdThread::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}

///错误应答
void CTdThread::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
	}
}

///报单通知
void CTdThread::OnRtnOrder(CThostFtdcOrderField* pOrder) 
{
	if (!pOrder) { return; }
	auto stField = make_shared<CThostFtdcOrderField>(*pOrder);
	memcpy(stField->ExchangeID, m_mapInstrument[stField->InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
	OrderWay(stField);
#ifdef _SHOW_HTS
	GetPosValid(pOrder->InstrumentID, CThostFtdcInvestorPositionField(), CThostFtdcInvestorPositionField(), SHtpGD());
#endif
	m_cqOrder.Push(stField);
	m_smFlash.Release();
}

///成交通知
void CTdThread::OnRtnTrade(CThostFtdcTradeField* pTrade) 
{
	if (!pTrade) { return; }
	auto stField = make_shared<CThostFtdcTradeField>(*pTrade);
	memcpy(stField->ExchangeID, m_mapInstrument[stField->InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
	TradeWay(stField);
#ifdef _SHOW_HTS
	GetPosValid(pTrade->InstrumentID, CThostFtdcInvestorPositionField(), CThostFtdcInvestorPositionField(), SHtpGD());
#endif
	m_cqTrade.Push(stField);
	m_smFlash.Release();
}
///报单录入错误回报
void CTdThread::OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo)
{
	if ((pInputOrder) && (pInputOrder->OrderRef[0] != '\0')) //订单管理：标记已下单&移除
	{
		m_cqTimeSheet.Push(m_htpManage.GetDelay(pInputOrder->InstrumentID, pInputOrder->OrderRef));
		m_htpManage.Delete(pInputOrder->InstrumentID, pInputOrder->OrderRef);
	}
	if (HTP_ISFAIL(pRspInfo)) //报错
	{
		m_eStatus = CTPST_FAILED; //故障
		HTP_LOG << HTP_MSG(string("CTdThread::OnErrRtnOrderInsert: ").append(pRspInfo->ErrorMsg)
			.append("->").append((!pInputOrder) ? "*" : pInputOrder->InstrumentID)
			.append("-").append((!pInputOrder) ? "*" : pInputOrder->OrderRef), HTP_CID_FAIL | pRspInfo->ErrorID);
		m_cRisk->Cnt((!pInputOrder) ? "*" : pInputOrder->InstrumentID).nCancelDaySum++; //撤单计加
		m_cRisk->SysCnt(RSC_ORDFAIL)++; //订单失败计加
		return;
	}
	//----------HTP_LOG << *pOrderAction;
 }

///报单操作错误回报
 void CTdThread::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
{
	 if (HTP_ISFAIL(pRspInfo)) //报错
	 {
		 m_eStatus = CTPST_FAILED; //故障
		 HTP_LOG << HTP_MSG(string("CTdThread::OnErrRtnOrderAction: ").append(pRspInfo->ErrorMsg)
			 .append("->").append(pOrderAction->InstrumentID)
			 .append("-").append(pOrderAction->OrderRef), pRspInfo->ErrorID); 
		 m_cRisk->Cnt(pOrderAction->InstrumentID).nCancelDaySum++; //撤单计加
		 m_cRisk->SysCnt(RSC_ORDFAIL)++; //订单失败计加
		 return;
	 }
	 //----------HTP_LOG << *pOrderAction;
}

///合约交易状态通知
 void CTdThread::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus)
 {
	 if (!pInstrumentStatus) { return; }
	 auto& mapProduct = m_mapProduct[GetKind(pInstrumentStatus->InstrumentID)]; //获取产品列表
	 for (const auto& it : mapProduct)
	 {
		 m_lockStatus.lock();
		 m_mapStatus[it.second.InstrumentID] = pInstrumentStatus->InstrumentStatus;
		 m_lockStatus.unlock();
		 m_lockTick.lock();
		 m_mapTickCnt[it.second.InstrumentID] = 0; //每一次的状态切换都将TICK计数清零
		 m_mapTick.erase(it.second.InstrumentID); //清除行情
		 m_lockTick.unlock();
	 }

	 string strStatus;
	 switch (pInstrumentStatus->InstrumentStatus)
	 {
		 ///开盘前
		case THOST_FTDC_IS_BeforeTrading: strStatus = "BeforeTrading"; break;
		///非交易
		case THOST_FTDC_IS_NoTrading: strStatus = "NoTrading"; break;
		///连续交易
		case THOST_FTDC_IS_Continous: strStatus = "Continous"; break;
		///集合竞价报单
		case THOST_FTDC_IS_AuctionOrdering: strStatus = "AuctionOrdering"; break;
		///集合竞价价格平衡
		case THOST_FTDC_IS_AuctionBalance: strStatus = "AuctionBalance"; break;
		///集合竞价撮合
		case THOST_FTDC_IS_AuctionMatch: strStatus = "AuctionMatch"; break;
		///收盘
		case THOST_FTDC_IS_Closed: strStatus = "Closed"; break;
		default: strStatus = "Undefine";  break;
	 }
	 HTP_LOG << HTP_MSG(string("CTdThread::OnRtnInstrumentStatus-->").append(pInstrumentStatus->InstrumentID).append("-").append(strStatus), HTP_CID_WARN | 101010);
	 //----------HTP_LOG << *pOrderAction;
 }
