#include "stdafx.h"
#include "CMongoDB.h"
#include "CLogThread.h"
#include "CTimeCalc.h"
#include "CMyData.h"
#include "CHtpSystem.h"

CMongoDB::CMongoDB()
{
	m_bThRunning = true; //线程循环标志
	m_bMdbConnected = false;
	m_pthTask = NULL; // new thread(&CMongoDB::Run, this);
	m_FuncGetPosNet = NULL;
}
CMongoDB::~CMongoDB()
{
	m_bThRunning = false; //原子操作不加锁，设置停止运行标志，线程不会马上停止，需要等待队列任务执行完后
	if (m_pthTask)
	{
		m_pthTask->join(); //等待线程退出
		delete m_pthTask; //释放线程空间
	}
	//while (!m_qTask.empty()) //删除剩余未执行完的任务
	//{
	//	delete m_qTask.front();
	//	m_qTask.pop();
	//}
}

CHtpMsg CMongoDB::Start(shared_ptr<const CUserConfig> stConfig)
{
	m_UserCfg = stConfig;
	//CHtpMsg cMsg;
	//HTP_LOG << "CMongoDB::Start"<< HTP_ENDL;
	vector<string> vecDB;
	for (char emt = MDB_DEFAULT; emt < MDB_ENDEF; emt++)
	{
		vecDB.push_back(g_szMdbTbl[emt]);
	}
	CHtpMsg cMsg = AddClient(stConfig->GetMDB(), vecDB);
	if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK) { return cMsg; }
	//cMsg = remove(g_szMdbTbl[MDB_RUNTIME], g_szMdbTbl[MDB_MKT_TICK], document{});//清空RUNTIME
	//if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK) { return cMsg; }
	cMsg = remove(g_szMdbTbl[MDB_RUNTIME], g_szMdbTbl[MDB_MKT_LINEK], document{});//清空RUNTIME
	if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK) { return cMsg; }
	cMsg = remove(g_szMdbTbl[MDB_RUNTIME], g_szMdbTbl[MDB_TRD_POSITION], document{});//清空RUNTIME
	if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK) { return cMsg; }
	return cMsg;
}
//MDB存储交易线程的资金信息
int CMongoDB::OnTdAccount(const CThostFtdcTradingAccountField& stField, const map<CFastKey, SHtpPNL, CFastKey>& mapPNL) //交易
{
	document doc = document{};
	doc
		<< "accountid" << stField.AccountID
		<< "balance" << stField.Balance
		<< "FrozenMargin" << stField.FrozenMargin
		<< "FrozenCash" << stField.FrozenCash
		<< "FrozenCommission" << stField.FrozenCommission
		<< "PreDeposit" << stField.PreDeposit
		<< "PreBalance" << stField.PreBalance
		<< "PreMargin" << stField.PreMargin
		<< "CurrMargin" << stField.CurrMargin
		<< "Deposit" << stField.Deposit
		<< "Balance" << stField.Balance
		<< "Available" << stField.Available
		<< "WithdrawQuota" << stField.WithdrawQuota
		<< "ExchangeMargin" << stField.ExchangeMargin
		<< "DeliveryMargin" << stField.DeliveryMargin
		<< "ExchangeDeliveryMargin" << stField.ExchangeDeliveryMargin
		<< "MortgageableFund" << stField.MortgageableFund
		<< "Commission" << stField.Commission
		<< "CloseProfit" << stField.CloseProfit
		<< "PositionProfit" << stField.PositionProfit
		<< "Reserve" << stField.Reserve
		<< "available" << stField.Available;
	doc << "PNL" << bsoncxx::builder::stream::open_document; //按K线分页
	for (auto& itPNL: mapPNL)
	{
			doc << BOOST_STR(itPNL.first.Key()) << bsoncxx::builder::stream::open_document; //按K线分页
			doc << "PNL1" << itPNL.second.m_dbProfitOpenTD;
			doc << "PNL2" << itPNL.second.m_dbProfitCloseTD;
			doc << "PNL3" << itPNL.second.m_dbProfitPositionYD;
			doc << bsoncxx::builder::stream::close_document;
	}
	doc << bsoncxx::builder::stream::close_document;
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	HTP_LOG << Insert(g_szMdbTbl[MDB_TRD_ACCOUNT], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	return 0;
}
//MDB存储交易线程的委托日志
int CMongoDB::OnTdOrderLog(shared_ptr<const CThostFtdcInputOrderFieldEx> stField) //交易
{
	document doc = document{};
	doc
		<< "BrokerID" << stField->data()->BrokerID
		<< "InvestorID" << stField->data()->InvestorID
		<< "OrderRef" << stField->data()->OrderRef
		<< "UserID" << stField->data()->UserID
		<< "OrderPriceType" << stField->data()->OrderPriceType
		<< "Direction" << stField->data()->Direction
		<< "CombOffsetFlag" << stField->data()->CombOffsetFlag
		<< "CombHedgeFlag" << stField->data()->CombHedgeFlag
		<< "LimitPrice" << stField->data()->LimitPrice
		<< "VolumeTotalOriginal" << stField->data()->VolumeTotalOriginal
		<< "TimeCondition" << stField->data()->TimeCondition
		<< "GTDDate" << stField->data()->GTDDate
		<< "VolumeCondition" << stField->data()->VolumeCondition
		<< "MinVolume" << stField->data()->MinVolume
		<< "ContingentCondition" << stField->data()->ContingentCondition
		<< "StopPrice" << stField->data()->StopPrice
		<< "ForceCloseReason" << stField->data()->ForceCloseReason
		<< "IsAutoSuspend" << stField->data()->IsAutoSuspend
		<< "RequestID" << stField->data()->RequestID
		<< "ExchangeID" << stField->data()->ExchangeID
		<< "AccountID" << stField->data()->AccountID
		<< "ClientID" << stField->data()->ClientID
		<< "symbol" << stField->data()->InstrumentID
		<< "TargetID" << stField->TargetID().c_str()
		<< "FromRef" << stField->FromRef().c_str();
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	HTP_LOG << Insert(g_szMdbTbl[MDB_TRD_ORDERLOG], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	if (m_cRisk->Ctrl(stField->data()->InstrumentID).bFollow)
		HTP_LOG << Insert(g_szMdbTbl[MDB_FOLLOW], g_szMdbTbl[MDB_TRD_ORDERLOG], doc);
	return 0;
}
//MDB存储交易线程的委托回报
int CMongoDB::OnTdOrder(shared_ptr < const CThostFtdcOrderField> stField) //交易
{
	//char szOrderID[64] = { 0 };
	//sprintf_s(szOrderID, "%d_%d_%s", stField->FrontID, stField->SessionID, stField->OrderRef); //自然日加时分秒
	document doc = document{};
	doc
		<< "symbol" << stField->InstrumentID
		<< "exchange" << stField->ExchangeID
		<< "OrderSysID" << stField->OrderSysID
		<< "OrderRef" << stField->OrderRef
		<< "type" << stField->OrderPriceType
		<< "direction" << stField->Direction
		<< "offset" << stField->CombOffsetFlag
		<< "price" << stField->LimitPrice
		<< "volume" << stField->VolumeTotalOriginal
		<< "traded" << stField->VolumeTraded
		<< "status" << stField->OrderStatus
		<< "InsertTime" << stField->InsertTime;
		//<< "ActiveTime" << stField->ActiveTime
		//<< "SuspendTime" << stField->SuspendTime
		//<< "UpdateTime" << stField->UpdateTime;
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	HTP_LOG << Insert(g_szMdbTbl[MDB_TRD_ORDER], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	if (m_cRisk->Ctrl(stField->InstrumentID).bFollow)
		HTP_LOG << Insert(g_szMdbTbl[MDB_FOLLOW], g_szMdbTbl[MDB_TRD_ORDER], doc);
	return 0;
}
//MDB存储交易线程的成交回报
int CMongoDB::OnTdTrade(shared_ptr<const CThostFtdcTradeField> stField) //交易
{
	document doc = document{};
	doc
		<< "symbol" << stField->InstrumentID
		<< "exchange" << stField->ExchangeID
		<< "OrderSysID" << stField->OrderSysID
		<< "OrderRef" << stField->OrderRef
		<< "TradeId" << stField->TradeID
		<< "direction" << stField->Direction
		<< "offset" << stField->OffsetFlag
		<< "price" << stField->Price
		<< "volume" << stField->Volume
		<< "time" << stField->TradeTime;
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	HTP_LOG << Insert(g_szMdbTbl[MDB_TRD_TRADE], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	if (m_cRisk->Ctrl(stField->InstrumentID).bFollow)
		HTP_LOG << Insert(g_szMdbTbl[MDB_FOLLOW], g_szMdbTbl[MDB_TRD_TRADE], doc);
	return 0;
}
//MDB存储交易线程的持仓更新
int CMongoDB::OnTdPosition(const CThostFtdcInvestorPositionField& stField, const SHtpGD& stGD) //交易
{
	document doc = document{};
	doc
		<< "symbol" << stField.InstrumentID
		<< "volume" << stField.Position
		<< "CloseProfit" << stField.CloseProfit
		<< "PositionProfit" << stField.PositionProfit
		<< "direction" << stField.PosiDirection
		<< "yd_volume" << stField.YdPosition
		<< "td_volume" << stField.TodayPosition
		<< "OpenVolume" << stField.OpenVolume
		<< "CloseVolume" << stField.CloseVolume
		<< "MarginRateByMoney" << stField.MarginRateByMoney
		<< "MarginRateByVolume" << stField.MarginRateByVolume;
	doc << "VolOTD" << ((THOST_FTDC_PD_Long == stField.PosiDirection) ? stGD.VolOBTD : stGD.VolOSTD);
	doc << "VolCTD" << ((THOST_FTDC_PD_Long == stField.PosiDirection) ? stGD.VolCSTD : stGD.VolCBTD);
	doc << "VolCYD" << ((THOST_FTDC_PD_Long == stField.PosiDirection) ? stGD.VolCSYD : stGD.VolCBYD);
	doc << "VolCUD" << ((THOST_FTDC_PD_Long == stField.PosiDirection) ? stGD.VolCSUD : stGD.VolCBUD);
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	HTP_LOG << Insert(g_szMdbTbl[MDB_TRD_POSITION], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	if (m_cRisk->Ctrl(stField.InstrumentID).bFollow)
		HTP_LOG << Insert(g_szMdbTbl[MDB_FOLLOW], g_szMdbTbl[MDB_TRD_POSITION], doc);

	if (m_cRisk->GetEnable(REN_SNAPSHOT_POSITION)) //持仓快照使能
	{
		options::update opt;
		opt.upsert(true);
		document set = document{};
		set << "$set" << doc;
		document key = document{};
		key
			<< "symbol" << stField.InstrumentID
			<< "direction" << stField.PosiDirection;
		HTP_LOG << update(g_szMdbTbl[MDB_RUNTIME], g_szMdbTbl[MDB_TRD_POSITION] , key, set, opt);
	}
	return 0;
}
//MDB加载历史K线信息，用于后台重启后延续上一次的K线计算
int CMongoDB::LoadLineK(CCtpLineK& refCtpLineK)
{
	m_cRisk->SysCnt(RSC_SYN_LINEK)++;
	if (refCtpLineK.m_InstrumentID[0] < '0') { return 0; }
	//HTP_LOG << HTP_MSG(string("CMongoDB::LoadLineK ").append(refCtpLineK.m_InstrumentID), 13841);
	options::find opt;
	opt.sort(make_document(kvp("SysTime", -1))).limit(1); //逆序插入时间查最新
	document key = document{};
	key << "symbol" << refCtpLineK.m_InstrumentID;
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_MKT_LINEK]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_MKT_LINEK]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_MKT_LINEK]]->acquire();
		auto& cursor = (*acEntry)[g_szMdbTbl[MDB_MKT_LINEK]][HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD().c_str()].find(key.view(), opt);
		if (cursor.begin() == cursor.end()) { return 0; }
		for (auto& vw : cursor)
		{
			if (vw["datetime"].type() != type::k_string) { continue; }
			string strTime(vw["datetime"].get_string().value);
			if (strTime.size() < 14) { continue; }
			if (vw["interval"].type() != type::k_double) { continue; }
			if (vw.find("TIME_HMSL") == vw.end()) { continue; }
			if (vw.find("NTU_YMD") == vw.end()) { continue; }
			refCtpLineK.m_ctNature.SetYMD(vw["NTU_YMD"].get_int32().value).SetHMSL(vw["TIME_HMSL"].get_int32().value);
			if (vw.find("TRD_YMD") == vw.end()) { continue; }
			refCtpLineK.m_ctTrading.SetYMD(vw["TRD_YMD"].get_int32().value).SetHMSL(vw["TIME_HMSL"].get_int32().value);
			eLineKType nDoCnt = eLineKType(0);
			for (; nDoCnt < LKT_ENDF; nDoCnt = eLineKType(nDoCnt + 1))
			{
				if (vw.find(LINEK_TYPE(nDoCnt)) == vw.end()) { break; }
				if (vw[LINEK_TYPE(nDoCnt)].type() != type::k_document) { break; }
				auto linek = vw[LINEK_TYPE(nDoCnt)].get_document().value;
				if (linek.begin() == linek.end()) { break; }
				if (linek["prcVwap"].type() != type::k_double) { break; }
				SCtpLineK& refLineK = (refCtpLineK.m_stLineK[nDoCnt]);
				refLineK.prcVwap = linek["prcVwap"].get_double().value;
				refLineK.prcTwap = linek["prcTwap"].get_double().value;
				refLineK.prcVwapSum = linek["prcVwapSum"].get_double().value;
				refLineK.prcTwapSum = linek["prcTwapSum"].get_double().value;
				refLineK.OpenPrice = linek["OpenPrice"].get_double().value;
				refLineK.HighestPrice = linek["HighestPrice"].get_double().value;
				refLineK.LowestPrice = linek["LowestPrice"].get_double().value;
				refLineK.ClosePrice = linek["ClosePrice"].get_double().value;
				refLineK.OpenInterest = linek["OpenInterest"].get_double().value;
				refLineK.Volume = linek["Volume"].get_int32().value;
				//refLineK.prcVolume = linek["prcVolume"].get_int32().value;
				refLineK.prcVolumeSum = linek["prcVolumeSum"].get_int32().value;
				//额外的信息20210902
				refLineK.bidVolSum = linek["bidVolSum"].get_double().value; //盘口bid1量加总
				refLineK.askVolSum = linek["askVolSum"].get_double().value; //盘口ask1量加总
				refLineK.bidPrcVSum = linek["bidPrcVSum"].get_double().value; //盘口bid1价加权成交量和
				refLineK.askPrcVSum = linek["askPrcVSum"].get_double().value; //盘口ask1价加权成交量和
				refLineK.SpreadSum = linek["SpreadSum"].get_double().value; //1档盘口(askPrcT - bidPrcT) 和
				refLineK.bidPrcV = linek["bidPrcV"].get_double().value; //盘口bid1价加权成交量
				refLineK.askPrcV = linek["askPrcV"].get_double().value; //盘口ask1价加权成交量
				refLineK.Spread = linek["Spread"].get_double().value; //1档盘口(askPrcT - bidPrcT) 按时间平均
				refLineK.OiPrev = linek["OiPrev"].get_double().value; //当前bar的第一个tick的 open interest
				refLineK.OiChangeSum = linek["OiChangeSum"].get_double().value; //tick 里面oichg 加总
				refLineK.OiCloseSum = linek["OiCloseSum"].get_double().value; //tick 里面oiClose 加总
				refLineK.OiOpenSum = linek["OiOpenSum"].get_double().value; //tick 里面oiOpen 加总
				refLineK.prcCount = linek["prcCount"].get_int32().value;
			}
			return nDoCnt;
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_MKT_LINEK]).append(":").append(e.what()), HTP_CID_FAIL|1017);
		return -1;
	}
	return 0;
}
//MDB加载K线数据，用于软件重启后K线延续上一次的计算
//此接口存在aggregate统计效率与二次逻辑等复杂问题而放弃使用，目前使用LoadLineK接口
int CMongoDB::LoadLineKMap(map<CFastKey, CCtpLineK, CFastKey>& mapLineK)
{
	HTP_LOG << HTP_MSG("CMongoDB::LoadLineK", 13841);
	int nLoadCnt = 0;
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_MKT_LINEK]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_MKT_LINEK]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		document groupDoc = document{};
		groupDoc << "_id" << "$symbol";
		groupDoc << "exchange" << make_document(kvp("$first", "$exchange"));
		groupDoc << "datetime" << make_document(kvp("$first", "$datetime"));
		groupDoc << "TRD_YMD" << make_document(kvp("$first", "$TRD_YMD")); //总量
		groupDoc << "NTU_YMD" << make_document(kvp("$first", "$NTU_YMD")); //总量
		groupDoc << "TIME_HMSL" << make_document(kvp("$first", "$TIME_HMSL")); //总量
		//groupDoc << "TIME_HOUR" << make_document(kvp("$first", "$TIME_HOUR")); //总量
		//groupDoc << "TIME_MINUTE" << make_document(kvp("$first", "$TIME_MINUTE")); //总量
		//groupDoc << "xxxxxx" << make_document(kvp("$first", "$xxxxxx")); //总量
		groupDoc << "interval" << make_document(kvp("$first", "$interval")); //

		int nTitle = 0;
		for (eLineKType i = eLineKType(0); i < LKT_ENDF; i = eLineKType(i + 1))
		{
			for (eLineKData j = eLineKData(0); j < LKD_ENDF; j = eLineKData(j + 1)){}
				//groupDoc << BSON_LINEK("", i, j) << make_document(kvp("$first", BSON_LINEK("$", i, j))); //
		}
		mongocxx::pipeline stages;
		stages.sort(make_document(kvp("datetime", -1))).group(groupDoc.view()); //聚合字段为排序后第一个 datetime逆序 UpdateTime
		pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_MKT_LINEK]]->acquire();
		auto& cursor = (*acEntry)[g_szMdbTbl[MDB_MKT_LINEK]][HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD().c_str()].aggregate(stages);
		if (cursor.begin() == cursor.end()) { return 0; }
		for (auto& vw : cursor)
		{
			if (vw["datetime"].type() != type::k_string) { continue; }
			if (vw["interval"].type() != type::k_double) { continue; }
			string strID(vw["_id"].get_string().value);
			string strExid(vw["exchange"].get_string().value);
			string strTime(vw["datetime"].get_string().value);
			if (strTime.size() < 14) { continue; }
			//TThostFtdcVolumeType	Volume = vw["volume"].get_int32().value; //实际量
			if (strID.size() > (sizeof(TThostFtdcInstrumentIDType) - 1)) { continue; }
			CCtpLineK& refCtpLineK = mapLineK[strID];
			std::memcpy(refCtpLineK.m_InstrumentID, strID.c_str(), strID.size());
			refCtpLineK.m_InstrumentID[strID.size()] = '\0';
			if (strExid.size() > (sizeof(TThostFtdcExchangeIDType) - 1)) { continue; }
			std::memcpy(refCtpLineK.m_ExchangeID, strExid.c_str(), strExid.size());
			refCtpLineK.m_ExchangeID[strExid.size()] = '\0';
			TThostFtdcDateType stYMD = { 0 }; TThostFtdcTimeType stHM = { 0 };
			std::memcpy(stYMD, strTime.substr(0, 8).c_str(), 8);
			std::memcpy(stHM, strTime.substr(9, 5).c_str(), 5);
			refCtpLineK.m_ctNature.SetYMD(stYMD).SetHM(stHM);
			m_cthLock.lock();
			refCtpLineK.m_ctTrading.SetYMD(HTP_SYS.GetTime(CTP_TIME_TRADING).GetYMD()).SetHM(stHM);
			m_cthLock.unlock();
			int nTitle = 0;
			for (eLineKType i = eLineKType(0); i < LKT_ENDF; i = eLineKType(i + 1))
			{
				//auto at = vw[BSON_LINEK("", i, 0)];
				//if (!at.raw()) { continue; }
				//if (vw[BSON_LINEK("", i, 0)].type() != type::k_double) { continue; }
				//SCtpLineK& refLineK = (refCtpLineK.m_stLineK[i]);
				//nTitle=0; refLineK.prcVwap = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.prcTwap = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.prcVwapSum = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.prcTwapSum = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.OpenPrice = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.HighestPrice = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.LowestPrice = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.ClosePrice = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.OpenInterest = vw[BSON_LINEK("", i, nTitle)].get_double().value;
				//nTitle++; refLineK.Volume = vw[BSON_LINEK("", i, nTitle)].get_int32().value;
				//nTitle++; //refLineK.prcVolume = vw[BSON_LINEK("", i, nTitle)].get_int32().value;
				//nTitle++; refLineK.prcVolumeSum = vw[BSON_LINEK("", i, nTitle)].get_int32().value;
				//nTitle++; refLineK.prcCount = vw[BSON_LINEK("", i, nTitle)].get_int32().value;
			}
			nLoadCnt++;
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_MKT_LINEK]).append(":").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return nLoadCnt;
}
//MDB存储行情线程的K线数据
//采用BULK读写降低CPU负荷，但造成了相对固定的500MS延迟
//快照功能的UPDATE操作占用了很大一部分CPU
int CMongoDB::OnLineK(CCycleQueue<CCtpLineK>& cqLineK)
{
	if (cqLineK.Size() < 1) { return -1; }
	try
	{
		CTimeCalc ctpTimeTD = HTP_SYS.GetTime(CTP_TIME_TRADING);
		if (m_mapPool.find(g_szMdbTbl[MDB_MKT_LINEK]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_MKT_LINEK]).append("\\").append(ctpTimeTD.GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		if (m_mapPool.find(g_szMdbTbl[MDB_FOLLOW]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_FOLLOW]).append("\\").append(g_szMdbTbl[MDB_MKT_LINEK]).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		if (m_mapPool.find(g_szMdbTbl[MDB_RUNTIME]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_RUNTIME]).append("\\").append(g_szMdbTbl[MDB_MKT_LINEK]).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		pool::entry acEntryINSERT = m_mapPool[g_szMdbTbl[MDB_MKT_LINEK]]->acquire();
		auto bulkINSERT = (*acEntryINSERT)[g_szMdbTbl[MDB_MKT_LINEK]][ctpTimeTD.GetStrYMD().c_str()].create_bulk_write();
		pool::entry acEntryFOLLOW = m_mapPool[g_szMdbTbl[MDB_FOLLOW]]->acquire();
		auto bulkFOLLOW = (*acEntryFOLLOW)[g_szMdbTbl[MDB_FOLLOW]][g_szMdbTbl[MDB_MKT_LINEK]].create_bulk_write();
		pool::entry acEntryUPDATE = m_mapPool[g_szMdbTbl[MDB_RUNTIME]]->acquire();
		auto bulkUPDATE = (*acEntryUPDATE)[g_szMdbTbl[MDB_RUNTIME]][g_szMdbTbl[MDB_MKT_LINEK]].create_bulk_write();
		int nCountINSERT = 0;
		int nCountFOLLOW = 0;
		int nCountUPDATE = 0;
		CCtpLineK ctpLineK;
		while (cqLineK.Front(ctpLineK))
		{
			cqLineK.Pop(); //被替换时引用-1
			if ('\0' == ctpLineK.m_InstrumentID[0]) { continue; }
			if (ctpLineK.m_ctTrading.m_nDay != ctpTimeTD.m_nDay) { continue; } //过滤过期TICK
			HTP_LOG << "K";
			document doc = document{};
			doc << "symbol" << ctpLineK.m_InstrumentID;
			doc << "exchange" << ctpLineK.m_ExchangeID;
			doc << "datetime" << ctpLineK.m_ctNature.GetStrYMDHM().c_str();
			doc << "TRD_YMD" << ctpLineK.m_ctTrading.GetYMD(); //用于排序
			doc << "NTU_YMD" << ctpLineK.m_ctNature.GetYMD(); //用于排序
			doc << "TIME_HMSL" << ctpLineK.m_ctNature.GetHMSL(); //用于排序
			//doc << "TIME_HOUR" << ctpLineK.m_ctNature.m_nHour; //用于排序
			//doc << "TIME_MINUTE" << ctpLineK.m_ctNature.m_nMinute; //用于排序
			doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());//用于同步本地时钟，通过本地时钟逆向推出交易所时钟
			//doc << "volume" << ctpLineK.m_stLineK[LKT_MINUTE].Volume; //总量
			doc << "interval" << 1.0f;
			for (eLineKType i = eLineKType(0); i < LKT_ENDF; i = eLineKType(i + 1))
			{
				const SCtpLineK& refLineK = (ctpLineK.m_stLineK[i]);
				doc << LINEK_TYPE(i) << bsoncxx::builder::stream::open_document; //按K线分页
				doc << "prcVwap" << refLineK.prcVwap;
				doc << "prcTwap" << refLineK.prcTwap;
				doc << "prcVwapSum" << refLineK.prcVwapSum;
				doc << "prcTwapSum" << refLineK.prcTwapSum;
				doc << "OpenPrice" << refLineK.OpenPrice;
				doc << "HighestPrice" << refLineK.HighestPrice;
				doc << "LowestPrice" << refLineK.LowestPrice;
				doc << "ClosePrice" << refLineK.ClosePrice;
				doc << "OpenInterest" << refLineK.OpenInterest;
				doc << "Volume" << refLineK.Volume;
				//doc << "prcVolume" << refLineK.prcVolume;
				doc << "prcVolumeSum" << refLineK.prcVolumeSum;
				doc << "prcCount" << refLineK.prcCount;
				//额外的信息20210902
				doc << "bidVolSum" << refLineK.bidVolSum;//盘口bid1量加总
				doc << "askVolSum" << refLineK.askVolSum;//盘口ask1量加总
				doc << "bidPrcVSum" << refLineK.bidPrcVSum;//盘口bid1价加权成交量和
				doc << "askPrcVSum" << refLineK.askPrcVSum;//盘口ask1价加权成交量和
				doc << "SpreadSum" << refLineK.SpreadSum;//1档盘口(askPrcT-bidPrcT)和
				doc << "bidPrcV" << refLineK.bidPrcV;//盘口bid1价加权成交量
				doc << "askPrcV" << refLineK.askPrcV;//盘口ask1价加权成交量
				doc << "Spread" << refLineK.Spread;//1档盘口(askPrcT-bidPrcT)按时间平均
				doc << "OiPrev" << refLineK.OiPrev;//当前bar的第一个tick的openinterest
				doc << "OiChangeSum" << refLineK.OiChangeSum;//tick里面oichg加总
				doc << "OiCloseSum" << refLineK.OiCloseSum;//tick里面oiClose加总
				doc << "OiOpenSum" << refLineK.OiOpenSum;//tick里面oiOpen加总
				doc << bsoncxx::builder::stream::close_document;
			}
			if (m_cRisk->GetEnable(REN_MARKET)) //行情使能
			{
				nCountINSERT++; bulkINSERT.append(mongocxx::model::insert_one(doc.view())); //bulk写入队列
			}
			if (m_cRisk->Ctrl(ctpLineK.m_InstrumentID).bFollow)
			{
				nCountFOLLOW++; bulkFOLLOW.append(mongocxx::model::insert_one(doc.view())); //bulk写入队列 
			}
			if (m_cRisk->GetEnable(REN_SNAPSHOT_LINEK)) //行情K线快照
			{
				document key = document{};
				key << "symbol" << ctpLineK.m_InstrumentID;
				document set = document{};
				set << "$set" << doc;
				mongocxx::model::update_one upsert_op(key.view(), set.view());
				upsert_op.upsert(true);
				nCountUPDATE++;  bulkUPDATE.append(upsert_op); //bulk写入队列
			}
		}
		stdx::optional<result::bulk_write> nResult;
		if (nCountINSERT > 0)
		{
			nResult = bulkINSERT.execute();
			if ((!nResult) || (nResult->inserted_count() != nCountINSERT))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnLineK WARN: ").append("bulkINSERT"), HTP_CID_FAIL | 1018);
			}
		}
		if (nCountFOLLOW > 0)
		{
			nResult = bulkFOLLOW.execute();
			if ((!nResult) || (nResult->inserted_count() != nCountFOLLOW))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnLineK WARN: ").append("bulkFOLLOW"), HTP_CID_FAIL | 1018);
			}
		}
		if (nCountUPDATE > 0)
		{
			nResult = bulkUPDATE.execute();
			if ((!nResult) || ((nResult->upserted_count()+nResult->modified_count()) != nCountUPDATE))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnLineK WARN: ").append("bulkUPDATE"), HTP_CID_FAIL | 1018);
			}
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string("CMongoDB::OnLineK ERROR: ").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return 0;
}
//MDB存储行情线程的TICK数据
//采用BULK读写降低CPU负荷，但造成了相对固定的100MS延迟
//快照功能的UPDATE操作占用了很大一部分CPU
int CMongoDB::OnTick(CCycleQueue<shared_ptr<const CCtpTick>>& cqTick)
{
	if (cqTick.Size() < 1) { return -1; }
	try
	{
		CTimeCalc ctpTimeTD = HTP_SYS.GetTime(CTP_TIME_TRADING);
		if (m_mapPool.find(g_szMdbTbl[MDB_MKT_TICK]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_MKT_TICK]).append("\\").append(ctpTimeTD.GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		if (m_mapPool.find(g_szMdbTbl[MDB_FOLLOW]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_FOLLOW]).append("\\").append(g_szMdbTbl[MDB_MKT_TICK]).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		if (m_mapPool.find(g_szMdbTbl[MDB_RUNTIME]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_RUNTIME]).append("\\").append(g_szMdbTbl[MDB_MKT_TICK]).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		pool::entry acEntryINSERT = m_mapPool[g_szMdbTbl[MDB_MKT_TICK]]->acquire();
		auto bulkINSERT = (*acEntryINSERT)[g_szMdbTbl[MDB_MKT_TICK]][ctpTimeTD.GetStrYMD().c_str()].create_bulk_write();
		pool::entry acEntryFOLLOW = m_mapPool[g_szMdbTbl[MDB_FOLLOW]]->acquire();
		auto bulkFOLLOW = (*acEntryFOLLOW)[g_szMdbTbl[MDB_FOLLOW]][g_szMdbTbl[MDB_MKT_TICK]].create_bulk_write();
		pool::entry acEntryUPDATE = m_mapPool[g_szMdbTbl[MDB_RUNTIME]]->acquire();
		auto bulkUPDATE = (*acEntryUPDATE)[g_szMdbTbl[MDB_RUNTIME]][g_szMdbTbl[MDB_MKT_TICK]].create_bulk_write();
		int nCountINSERT = 0;
		int nCountFOLLOW = 0;
		int nCountUPDATE = 0;
		shared_ptr<const CCtpTick> ctpTick = NULL;
		while (cqTick.Front(ctpTick))
		{
			cqTick.Pop(); //被替换时引用-1
			if (NULL == ctpTick) { continue; }
			//if (ctpTick->m_ctTrading.m_nDay != ctpTimeTD.m_nDay) { continue; } //过滤过期TICK
			HTP_LOG << ">";
			document doc = document{};
			doc
				<< "symbol" << ctpTick->m_stTick.InstrumentID
				<< "exchange" << ctpTick->m_stTick.ExchangeID
				<< "datetime" << ctpTick->m_ctNature.GetStrYMDHMSL().c_str()
				<< "tradeDate" << (int64_t)ctpTick->m_ctTrading.GetYMD()
				<< "volume" << ctpTick->m_stTick.Volume
				<< "open_interest" << ctpTick->m_stTick.OpenInterest
				<< "last_price" << ctpTick->m_stTick.LastPrice
				<< "PreSettlementPrice" << ctpTick->m_stTick.PreSettlementPrice
				<< "last_volume" << ctpTick->m_stTick.Volume
				<< "limit_up" << ctpTick->m_stTick.UpperLimitPrice
				<< "limit_down" << ctpTick->m_stTick.LowerLimitPrice
				<< "open_price" << ctpTick->m_stTick.OpenPrice
				<< "high_price" << ctpTick->m_stTick.HighestPrice
				<< "low_price" << ctpTick->m_stTick.LowestPrice
				<< "pre_close" << ctpTick->m_stTick.PreClosePrice
				<< "bid_price_1" << ctpTick->m_stTick.BidPrice1
				<< "bid_price_2" << ctpTick->m_stTick.BidPrice2
				<< "bid_price_3" << ctpTick->m_stTick.BidPrice3
				<< "bid_price_4" << ctpTick->m_stTick.BidPrice4
				<< "bid_price_5" << ctpTick->m_stTick.BidPrice5
				<< "ask_price_1" << ctpTick->m_stTick.AskPrice1
				<< "ask_price_2" << ctpTick->m_stTick.AskPrice2
				<< "ask_price_3" << ctpTick->m_stTick.AskPrice3
				<< "ask_price_4" << ctpTick->m_stTick.AskPrice4
				<< "ask_price_5" << ctpTick->m_stTick.AskPrice5
				<< "bid_volume_1" << ctpTick->m_stTick.BidVolume1
				<< "bid_volume_2" << ctpTick->m_stTick.BidVolume2
				<< "bid_volume_3" << ctpTick->m_stTick.BidVolume3
				<< "bid_volume_4" << ctpTick->m_stTick.BidVolume4
				<< "bid_volume_5" << ctpTick->m_stTick.BidVolume5
				<< "ask_volume_1" << ctpTick->m_stTick.AskVolume1
				<< "ask_volume_2" << ctpTick->m_stTick.AskVolume2
				<< "ask_volume_3" << ctpTick->m_stTick.AskVolume3
				<< "ask_volume_4" << ctpTick->m_stTick.AskVolume4
				<< "ask_volume_5" << ctpTick->m_stTick.AskVolume5
				<< "OiChange" << ctpTick->m_stTick.OiChange
				<< "VolChange" << ctpTick->m_stTick.VolChange
				<< "OiOpen" << ctpTick->m_stTick.OiOpen
				<< "OiClose" << ctpTick->m_stTick.OiClose;
			if (m_cRisk->GetEnable(REN_MARKET)) //行情使能
			{
				nCountINSERT++; bulkINSERT.append(mongocxx::model::insert_one(doc.view())); //bulk写入队列
			}
			if (m_cRisk->Ctrl(ctpTick->m_stTick.InstrumentID).bFollow)
			{
				nCountFOLLOW++; bulkFOLLOW.append(mongocxx::model::insert_one(doc.view())); //bulk写入队列 
			}
			if (m_cRisk->GetEnable(REN_SNAPSHOT_TICK)&&(ctpTick->m_ctTrading.m_nYear>2020)) //TICK的交易日期有经过修正，属于实时行情
			{
				document key = document{};
				key << "symbol" << ctpTick->m_stTick.InstrumentID;
				document set = document{};
				set << "$set" << doc;
				mongocxx::model::update_one upsert_op(key.view(), set.view());
				upsert_op.upsert(true);
				nCountUPDATE++;  bulkUPDATE.append(upsert_op); //bulk写入队列
			}
		}
		stdx::optional<result::bulk_write> nResult;
		if (nCountINSERT > 0)
		{
			nResult = bulkINSERT.execute();
			if ((!nResult) || (nResult->inserted_count() != nCountINSERT))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnTick WARN: ").append("bulkINSERT"), HTP_CID_FAIL | 1018);
			}
		}
		if (nCountFOLLOW > 0)
		{
			nResult = bulkFOLLOW.execute();
			if ((!nResult) || (nResult->inserted_count() != nCountFOLLOW))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnTick WARN: ").append("bulkFOLLOW"), HTP_CID_FAIL | 1018);
			}
		}
		if (nCountUPDATE > 0)
		{
			nResult = bulkUPDATE.execute();
			if ((!nResult) || (nResult->matched_count() != nCountUPDATE))
			{
				HTP_LOG << HTP_MSG(string("CMongoDB::OnTick WARN: ").append("bulkUPDATE"), HTP_CID_FAIL | 1018);
			}
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string("CMongoDB::OnTick ERROR: ").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return 0;
}
int CMongoDB::SendEmail(const CHtpMsg& cMsg)
{
	vector<string> vecEmail = m_UserCfg->GetEmail();
	if (vecEmail.size() < 1) { return -1; }
	string strTitle;
	string strMsg;
	document doc = document{};
	strTitle = string("HTP").append(HTP_SYS.GetTime(CTP_TIME_NET).GetStrYMD()).c_str();
	strMsg = string(m_UserCfg->GetLogin().UserID).append(": ").append(cMsg.m_strMsg).c_str();
	doc
		<< "CID" << (int64_t)HTP_CID_ERRORID(cMsg.m_CID)
		<< "UserID" << BOOST_STR(m_UserCfg->GetLogin().UserID)
		<< "Title" << strTitle.c_str()
		<< "Msg" << strMsg.c_str();
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	doc << "Email" << bsoncxx::builder::stream::open_document;
	for (int i=0; i < vecEmail.size(); i++)
	{
		doc << BOOST_STR(vecEmail[i].c_str()) << i;
	}
	doc << bsoncxx::builder::stream::close_document;
	HTP_LOG << Insert(g_szMdbTbl[MDB_EMAIL], HTP_SYS.GetTime(CTP_TIME_NET).GetStrYMD(), doc);
	return 0;
}
//MDB存储运行过程中的日志，用于回溯分析，相比于写入的TXT文件，MDB具有统计与遍历优势
int CMongoDB::PushLogToDB(const CHtpMsg& cMsg)
{
	document doc = document{};
	doc 
		<< "type" << (int64_t)HTP_CID_ERRORID(cMsg.m_CID)
		<< "UserID" << BOOST_STR(m_UserCfg->GetLogin().UserID)
		<< "msg" << cMsg.m_strMsg.c_str();
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	string strDate = (HTP_SYS.GetTime(CTP_TIME_TRADING).m_nYear > 2020)
		? HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD() : HTP_SYS.GetTime(CTP_TIME_NET).GetStrYMD();
	HTP_LOG << Insert(g_szMdbTbl[MDB_LOG], strDate, doc);
	return 0;
}
int CMongoDB::LoadTimeTD()
{
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_BASIC]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_BASIC]).append("\\").append("TRADE_TIME_TBL").append("-NOT FIND POOL"), 1041);
			return -1;
		}
		options::find opt;
		document key = document{};
		pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_BASIC]]->acquire();
		auto& cursor = (*acEntry)[g_szMdbTbl[MDB_BASIC]]["TRADE_TIME_TBL"].find_one(key.view(), opt); //只获取一个
		if (!cursor.has_value())
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_BASIC]).append("\\").append("TRADE_TIME_TBL").append("-NOT FIND DATA"), 1041);
			return -1;
		}
		auto& vwFind = cursor.get();
		CTimeTD ctmTimeTD;
		for (auto& itTime : vwFind)
		{
			if (itTime.type() != type::k_array) { continue; }
			auto& vecTimeTD = m_mapTimeTD[string(itTime.key().data())];
			vecTimeTD.clear();
			auto& aryTime = itTime.get_array().value;
			for (auto& itElmt : aryTime)
			{
				if (itElmt.type() != type::k_string) { continue; }
				string strTime(itElmt.get_string().value);
				if (strTime.size() > 11) 
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTimeTD strTime OUT SIZE", HTP_CID_FAIL | 1477);
					return -1;
				}
				size_t nSliper = strTime.find('-');
				if (nSliper < 4)
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTimeTD strTime NOT FIND[-]", HTP_CID_FAIL | 1477);
					return -1;
				}
				int nIdx = 0;
				CTPTIME nHmStart = 0;
				for (; nIdx < strTime.size(); nIdx++)
				{
					if (strTime.at(nIdx) == '-') { break; }
					if (strTime.at(nIdx) < '0' || strTime.at(nIdx)> '9') { continue; }
					nHmStart *= 10;
					nHmStart += strTime.at(nIdx) - '0';
				}
				CTPTIME nHmEnd = 0;
				for (nIdx++; nIdx < strTime.size(); nIdx++)
				{
					if (strTime.at(nIdx) == '-') { break; }
					if (strTime.at(nIdx) < '0' || strTime.at(nIdx) > '9') { continue; }
					nHmEnd *= 10;
					nHmEnd += strTime.at(nIdx) - '0';
				}
				if (nHmEnd < 0 || nHmStart < 0)
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTimeTD strTime Parse Faild", HTP_CID_FAIL | 1477);
					return -1;
				}
				ctmTimeTD.ctmStart.Clear();
				if (nHmStart > 2359)
				{
					ctmTimeTD.ctmStart.SetHM(2359);
					ctmTimeTD.ctmStart.m_nSecond = 59;
					ctmTimeTD.ctmStart.m_nMilSec = 999;
				}
				else
				{
					ctmTimeTD.ctmStart.SetHM(nHmStart);
					ctmTimeTD.ctmStart.m_nSecond = 0;
					ctmTimeTD.ctmStart.m_nMilSec = 0;
				}
				ctmTimeTD.ctmEnd.Clear();
				if (nHmEnd > 2359)
				{
					ctmTimeTD.ctmEnd.SetHM(2359);
					ctmTimeTD.ctmEnd.m_nSecond = 59;
					ctmTimeTD.ctmEnd.m_nMilSec = 999;
				}
				else
				{
					ctmTimeTD.ctmEnd.SetHM(nHmEnd);
					ctmTimeTD.ctmEnd.m_nSecond = 0;
					ctmTimeTD.ctmEnd.m_nMilSec = 0;
				}
				vecTimeTD.push_back(ctmTimeTD);
				HTP_LOG << string("[").append(itTime.key().data()).append("@")
					.append(ctmTimeTD.ctmStart.GetStrHMSL()).append("-")
					.append(ctmTimeTD.ctmEnd.GetStrHMSL()).append("]");
			}
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_BASIC]).append(":").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return 0;
}
//创建TARGET,生成目标计划
int CMongoDB::LoadTarget(CTargetCtrl& cTargetCtrl)//获取目标持仓
{
	unique_lock<mutex> lock(m_lockTarget);
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_TARGET_PLAN]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_TARGET_PLAN]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		options::find opt;
		opt.sort(make_document(kvp("InsertTime", -1))).limit(1); //逆序插入时间查最新
		document key = document{};
		key << "accountid" << cTargetCtrl.GetUserID();
		try
		{
			pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_TARGET_PLAN]]->acquire();
			auto& cursor = (*acEntry)[g_szMdbTbl[MDB_TARGET_PLAN]][HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD().c_str()].find(key.view(), opt);
			if (cursor.begin() == cursor.end())
			{
				return 0;
			}
			auto& mapTarget = cTargetCtrl.NewTarget(); //获取最新target内存
			mapTarget.clear(); //清除旧数据
			LONG64  llnTime = 20210101;
			string strTargetID = "";
			for (auto& vw : cursor)
			{
				if (vw.find("InsertTime") == vw.end()) 
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget InsertTime NOT FIND", HTP_CID_FAIL | 7860); return -1;
				}
				if (vw["InsertTime"].type() != type::k_int64)
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget InsertTime NOT INT64 TYPE", HTP_CID_FAIL | 7860); return -1;
				}
				llnTime = vw["InsertTime"].get_int64().value;
				if (llnTime <= cTargetCtrl.GetTimeStamp()) { return 0; } //时间缀没更新则不进行target刷新
				if (vw.find("accountid") == vw.end()) 
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget accountid NOT FIND", HTP_CID_FAIL | 7860); return -1;
				}
				if (vw["accountid"].type() != type::k_string) 
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget accountid NOT STR TYPE", HTP_CID_FAIL | 7860); return -1;
				}
				string strUserID(vw["accountid"].get_string().value);
				if (strUserID != cTargetCtrl.GetUserID())//账号不对
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget accountid CHECK FAIL", HTP_CID_FAIL | 7860); return -1;
				}
				if (vw.find("TargetPlanId") == vw.end())
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget TargetPlanId NOT FIND", HTP_CID_FAIL | 7860); return -1;
				}
				if (vw["TargetPlanId"].type() != type::k_string)
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget TargetPlanId NOT STR TYPE", HTP_CID_FAIL | 7860); return -1;
				}
				strTargetID = string(vw["TargetPlanId"].get_string().value);
				if(strTargetID.empty())
				{
					HTP_LOG << HTP_MSG("CMongoDB::LoadTarget TargetPlanId Value IS NULL", HTP_CID_FAIL | 7860); return -1;
				}
				for (const auto& itView : vw)
				{
					if (itView.type() != type::k_document) { continue; }
					const auto& vwTarget = itView.get_document().view();
					if (vwTarget.find("lastQty") == vwTarget.end()) 
					{
						HTP_LOG << HTP_MSG("CMongoDB::LoadTarget lastQty NOT FIND", HTP_CID_FAIL | 7860); return -1;
					}
					auto& ptrTarget = mapTarget[string(itView.key())]; //存入对应合约的TARGET数据
					if (!ptrTarget) { ptrTarget = make_unique<CTarget>(); }
					ptrTarget->Clear();
					ptrTarget->m_strID = string(itView.key());
					ptrTarget->m_ctmCurrent = HTP_SYS.GetTime(CTP_TIME_NET);
					ptrTarget->m_ctmTD = HTP_SYS.GetTime(CTP_TIME_TRADING);
					auto itFindOT = vwTarget.find("orderTime"); //获取期望的下单时间
					if ((itFindOT != vwTarget.end()) && (itFindOT->get_int64() > 20210101010101)) //时间有效
					{
						int64 llOrderTime = itFindOT->get_int64();
						ptrTarget->m_ctmTdStart.SetYMD(int(llOrderTime / 1000000));
						ptrTarget->m_ctmTdStart.SetHMS(int(llOrderTime % 1000000));
					}
					else
					{
						ptrTarget->m_ctmTdStart = ptrTarget->m_ctmCurrent; //如果时间无效则使用当前时间为起始时间
					}
					if (!m_FuncGetPosNet) 
					{
						HTP_LOG << HTP_MSG("CMongoDB::LoadTarget m_FuncGetPosNet IS NULL", HTP_CID_FAIL | 7860); return -1;
					}
					//获取净持仓失败，可能是对消异常导致的，必须停下来检查持仓计算
					if (false == m_FuncGetPosNet(ptrTarget->m_strID, ptrTarget->m_volLastQty)) 
					{
						HTP_LOG << HTP_MSG(string("CMongoDB::LoadTarget m_FuncGetPosNet ERROR->")
							.append(ptrTarget->m_strID), HTP_CID_FAIL | 7860);
						return -1;
					}
					//ptrTarget->m_volLastQty = 0; //20210917该来自交易实时 vwTarget["lastQty"].get_int32();
					ptrTarget->m_volTarget = vwTarget["targetQty"].get_int32();
					ptrTarget->m_strExchange = string(vwTarget["exchange"].get_string().value);
					ptrTarget->m_strProduct = string(vwTarget["contractClass"].get_string().value);
					auto itFindTTD = m_mapTimeTD.find(ptrTarget->m_strProduct); //获取合约品种的交易时间
					if (itFindTTD == m_mapTimeTD.end())
					{
						HTP_LOG << HTP_MSG("CMongoDB::LoadTarget m_mapTimdTD NOT FIND", HTP_CID_FAIL | 7860); return -1;
					}
					ptrTarget->m_vecTimeTD = itFindTTD->second; //传递全天交易时间
					ptrTarget->m_volTradeQty = ptrTarget->m_volTarget - ptrTarget->m_volLastQty;//vwTarget["tradeQty"].get_int32();
					ptrTarget->m_nFreqMS = vwTarget["freq"].get_int32();
					ptrTarget->m_dbTimeAdj = vwTarget["TimeAdj"].get_double();
					if (vwTarget.find("VolK") != vwTarget.end()) //有才给，否则使用默认值进行计算
						ptrTarget->m_dbVolK = vwTarget["VolK"].get_double();
					if (vwTarget.find("VolPowK") != vwTarget.end())
						ptrTarget->m_dbVolPowK = vwTarget["VolPowK"].get_double();
					auto findType = vwTarget.find("algoType");
					if (findType == vwTarget.end())
					{
						HTP_LOG << HTP_MSG("CMongoDB::LoadTarget algoType NOT FIND", HTP_CID_FAIL | 7860); return -1;
					}
					if (findType->type() != type::k_document)
					{
						HTP_LOG << HTP_MSG("CMongoDB::LoadTarget algoType NOT DOC TYPE", HTP_CID_FAIL | 7860); return -1;
					}
					auto& vwType = findType->get_document().view();
					for (int i = 0; i < AGMD_ENDF; i++) //获取TARGET类型
					{
						ptrTarget->m_bEnable[i] = false;
						auto itFind = vwType.find(g_strAlgoMD[i]);
						if (itFind == vwType.end()) { continue; }
						ptrTarget->m_nParamK[i] = itFind->get_int32();
						ptrTarget->m_bEnable[i] = (0 < ptrTarget->m_nParamK[i]);
					}
				}
			}
			document docAll = document{};
			map<string, document> mapPlan;
			mapPlan.clear();
			//TARGET生成
			docAll << "accountid" << cTargetCtrl.GetUserID();
			docAll << "FromStmp" << (int64_t)llnTime;
			docAll << "TargetID" << strTargetID.c_str();
			docAll << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
			int nIdx = 0;
			for (auto& itTarget : mapTarget)
			{
				if (itTarget.second)
				{
					CHtpMsg cMsg = itTarget.second->Create(mapPlan[itTarget.first.Key()]);
					docAll << BOOST_STR(itTarget.first.Key()) << mapPlan[itTarget.first.Key()];
					nIdx++;
					if (HTP_CID_RTFLAG(cMsg.m_CID) != HTP_CID_OK)
					{
						HTP_LOG << cMsg; continue;
					}
				}
			}
			cTargetCtrl.SetTimeStamp(llnTime);
			cTargetCtrl.SetTargetID(strTargetID);
			cTargetCtrl.CheckOut(); //切换到最新TARGET
			HTP_LOG << Insert(g_szMdbTbl[MDB_TARGET], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), docAll);
			return (int)mapTarget.size();
		}
		catch (const std::exception& e)
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_TARGET_PLAN]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append(":").append(e.what()), HTP_CID_FAIL | 1012);
			return -1;
		}
		return 1;
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_TARGET_PLAN]).append(":").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return 0;
}

////获取当前客户需要需要执行的订单TARGET
////TARGET的实现是基于合约级的TICK时间触发机制，其主要触发延迟来源于网络延迟与TICK推送延迟
////TARGET支持盘中运行时刻更新，约10S中检查一次是否有最新TARGET，通过排序检查时间缀判断是否为最新
//int CMongoDB::GetTarget(CTargetCtrl& cTargetCtrl)//获取目标持仓
//{
//	try
//	{
//		if (m_mapPool.find(g_szMdbTbl[MDB_TARGET]) == m_mapPool.end()) //找不到连接池
//		{
//			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_TARGET]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append("-NOT FIND POOL"), 1041);
//			return -1;
//		}
//		options::find opt;
//		opt.sort(make_document(kvp("insertTime", -1))).limit(1); //逆序插入时间查最新
//		document key = document{};
//		key << "accountid" << cTargetCtrl.GetUserID();
//		map<CFastKey, CTarget, CFastKey> mapTarget; mapTarget.clear();
//		try
//		{
//			//opt.cursor_type(mongocxx::cursor::type::k_tailable); //可尾游标
//			//opt.no_cursor_timeout();
//			pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_TARGET]]->acquire();
//			auto& cursor = (*acEntry)[g_szMdbTbl[MDB_TARGET]][HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD().c_str()].find(key.view(), opt);
//			if (cursor.begin() == cursor.end())
//			{
//				return 0;
//			}
//			for (auto& vw : cursor)
//			{
//				if (vw.find("insertTime") == vw.end()) { return 0; }
//				if (vw["insertTime"].type() != type::k_date) { return 0; }
//				std::chrono::system_clock::time_point time = vw["insertTime"].get_date();
//				std::time_t nowTime = chrono::system_clock::to_time_t(time);//转换为 std::time_t 格式 
//				char str_time[64];
//				strftime(str_time, sizeof(str_time), "%Y-%m-%d:%H:%M:%S", localtime(&nowTime));
//				string strTimeStamp(str_time);
//				if (strTimeStamp == cTargetCtrl.GetTimeStamp()) { return 0; }
//				if (vw.find("OrderPlan") == vw.end()) { return 0; }
//				cTargetCtrl.SetTimeStamp(strTimeStamp);
//				//----------------------------------------------------
//				auto OrderPlan = vw["OrderPlan"];
//				if (OrderPlan.type() != type::k_document) { return 0; }
//				auto OrderPlanTbl = OrderPlan.get_document().value;
//				SCtpTarget stTarget = { 0 };
//				for (const auto& Plan : OrderPlanTbl)
//				{
//					string strTime(Plan.key());
//					if (Plan.length() < 1) { continue; }
//					if (!ToValue(stTarget, strTime)) { continue; } //时间转换失败
//					if (Plan.type() != type::k_document) { continue; }
//					auto PlanTbl = Plan.get_document().value;
//					for (const auto& Target : PlanTbl)
//					{
//						if (Target.length() < 1) { continue; }
//						switch (Target.type())
//						{
//						case type::k_int32: stTarget.Position = (TThostFtdcVolumeType)Target.get_int32().value; break;
//						case type::k_int64: stTarget.Position = (TThostFtdcVolumeType)Target.get_int64().value; break;
//						case type::k_double: stTarget.Position = (TThostFtdcVolumeType)Target.get_double().value; break;
//						default: break;
//						}
//						mapTarget[Target.key().data()].Push(stTarget);
//					}
//				}
//			}
//		}
//		catch (const std::exception& e)
//		{
//			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_TARGET]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append(":").append(e.what()), HTP_CID_FAIL | 1012);
//			return -1;
//		}
//		if (mapTarget.size() < 1) { return 0; }
//		for (auto it = mapTarget.begin(); it != mapTarget.end(); it++) //排序
//		{
//			it->second.Sort();
//		}
//		cTargetCtrl.SynTarget(mapTarget); //同步TARGET
//		return 1;
//	}
//	catch (const std::exception& e)
//	{
//		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_TARGET]).append(":").append(e.what()), HTP_CID_FAIL | 1017);
//		return -1;
//	}
//	return 0;
//}

//MDB加载风控控制器，提供全局功能开关，计数限制，合约级功能开关
//具体有哪些风控功能可右键查看相关宏定义，这里不进行详细说明
int CMongoDB::LoadRiskCtrl()
{
	//HTP_LOG << HTP_MSG(string("CMongoDB::LoadRisk ").append(refRisk.m_InstrumentID), 13841);
	static int64_t nllSysTimeLast = 0;
	options::find opt;
	opt.sort(make_document(kvp("SysTime", -1))).limit(1); //逆序插入时间查最新
	document key = document{};
	key << "UserID" << m_UserCfg->GetLogin().UserID;
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_RISKCTRL]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_RISKCTRL]).append("\\").append(m_UserCfg->GetLogin().UserID).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_RISKCTRL]]->acquire();
		auto& cursor = (*acEntry)[g_szMdbTbl[MDB_RISKCTRL]][m_UserCfg->GetLogin().UserID].find(key.view(), opt);
		if (cursor.begin() == cursor.end()) //找不到则写入预配置模板
		{
			document doc = document{};
			doc << "UserID" << m_UserCfg->GetLogin().UserID;
			nllSysTimeLast = HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS();
			doc << "SysTime" << (int64_t)nllSysTimeLast;
			doc << BOOST_STR("ENABLE") << bsoncxx::builder::stream::open_document; //按合约分页
			for (int i = 0; i < REN_ENDF; i++)
			{
				doc << BOOST_STR(g_strRiskEN[i]) << m_cRisk->GetEnable(eRiskEnable(i));
			}
			doc << bsoncxx::builder::stream::close_document;
			doc << BOOST_STR("LIMIT") << bsoncxx::builder::stream::open_document; //按合约分页
			doc << BOOST_STR("nOrderDaySum") << int32_t(1000); //
			doc << BOOST_STR("nCancelDaySum") << int32_t(100); //
			doc << BOOST_STR("nOrderMinSum") << int32_t(100); //
			doc << BOOST_STR("nOrderVolMax") << int32_t(1000); //
			doc << BOOST_STR("nLongPosMax") << int32_t(1000); //
			doc << BOOST_STR("nShortPosMax") << int32_t(1000); //
			doc << bsoncxx::builder::stream::close_document;
			doc << BOOST_STR("CTRL") << bsoncxx::builder::stream::open_document; //按合约分页
			doc << BOOST_STR("ABC123") << bsoncxx::builder::stream::open_document; //按合约分页
			doc << BOOST_STR("bFollow") << true; //
			doc << BOOST_STR("bStopOpen") << true; //
			doc << BOOST_STR("bStopTrading") << true; //
			doc << bsoncxx::builder::stream::close_document;
			doc << bsoncxx::builder::stream::close_document;
			HTP_LOG << Insert(g_szMdbTbl[MDB_RISKCTRL], m_UserCfg->GetLogin().UserID, doc);
			return 0;
		}
		for (auto& vw : cursor)
		{
			if (vw.find("SysTime") == vw.end()) { break; }
			if (vw["SysTime"].type() != type::k_int64) { break; }
			int64_t llnSysTime = vw["SysTime"].get_int64().value;
			if (llnSysTime == nllSysTimeLast) { break; }
			nllSysTimeLast = llnSysTime;
			if (vw.find("UserID") == vw.end()) { break; }
			if (vw["UserID"].type() != type::k_string) { break; }
			if (vw.find("ENABLE") == vw.end()) { break; }
			if (vw["ENABLE"].type() != type::k_document) { break; }
			const auto& enable = vw["ENABLE"].get_document().value;
			for (int i = 0; i < REN_ENDF; i++)
			{
				if (enable.find(g_strRiskEN[i]) != enable.end())
					m_cRisk->Enable(eRiskEnable(i)) = enable[g_strRiskEN[i]].get_bool().value;
				else
					m_cRisk->Enable(eRiskEnable(i)) = true;
				HTP_LOG << HTP_MSG(string("CMongoDB::LoadRiskCtrl->").append(g_strRiskEN[i])
					.append("-").append((m_cRisk->GetEnable(eRiskEnable(i))) ? "TRUE" : "FALSE"), HTP_CID_WARN | 7890);
			}
			if (vw.find("LIMIT") == vw.end()) { break; }
			if (vw["LIMIT"].type() != type::k_document) { break; }
			const auto& limit = vw["LIMIT"];
			SCtpRiskLimit& refLimit = m_cRisk->Limit();
			refLimit.nOrderDaySum = limit["nOrderDaySum"].get_int32().value;
			refLimit.nCancelDaySum = limit["nCancelDaySum"].get_int32().value;
			refLimit.nOrderMinSum = limit["nOrderMinSum"].get_int32().value;
			refLimit.nOrderVolMax = limit["nOrderVolMax"].get_int32().value;
			refLimit.nLongPosMax = limit["nLongPosMax"].get_int32().value;
			refLimit.nShortPosMax = limit["nShortPosMax"].get_int32().value;
			if (vw.find("CTRL") == vw.end()) { break; }
			if (vw["CTRL"].type() != type::k_document) { break; }
			const auto& ctrl = vw["CTRL"].get_document().value;
			int nLoadCnt = 0;
			for (auto& dc : ctrl)
			{
				if (dc.type() != type::k_document) { continue; }
				string strKey(dc.key().data());
				SCtpRiskCtrl& refCtrl = m_cRisk->Ctrl(strKey);
				refCtrl.bFollow = dc["bFollow"].get_bool().value;
				refCtrl.bStopOpen = dc["bStopOpen"].get_bool().value;
				refCtrl.bStopTrading = dc["bStopTrading"].get_bool().value;
				nLoadCnt++;
			}
			return nLoadCnt;
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_RISKCTRL]).append(":").append(e.what()), HTP_CID_FAIL | 1017);
		return -1;
	}
	return 0;
}
//MDB加载风控计数器，结合风控控制器进行相关逻辑控制
//具体有哪些风控功能可右键查看相关宏定义，这里不进行详细说明
int CMongoDB::LoadRiskCnt()
{
	//HTP_LOG << HTP_MSG(string("CMongoDB::LoadRisk ").append(refRisk.m_InstrumentID), 13841);
	options::find opt;
	opt.sort(make_document(kvp("SysTime", -1))).limit(1); //逆序插入时间查最新
	document key = document{};
	key << "UserID" << m_UserCfg->GetLogin().UserID;
	try
	{
		if (m_mapPool.find(g_szMdbTbl[MDB_TASK]) == m_mapPool.end()) //找不到连接池
		{
			HTP_LOG << CHtpMsg(string(g_szMdbTbl[MDB_TASK]).append("\\").append(HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD()).append("-NOT FIND POOL"), 1041);
			return -1;
		}
		pool::entry acEntry = m_mapPool[g_szMdbTbl[MDB_TASK]]->acquire();
		auto& cursor = (*acEntry)[g_szMdbTbl[MDB_TASK]][HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD().c_str()].find(key.view(), opt);
		if (cursor.begin() == cursor.end()) { return 0; }
		int nLoadCnt = 0;
		for (auto& vw : cursor)
		{
			if (vw.find("UserID") == vw.end()) { break; }
			if (vw["UserID"].type() != type::k_string) { break; }
			for (auto& cnt : vw)
			{
				if (cnt.type() != type::k_document) { continue; }
				string strKey(cnt.key().data());
				SCtpRiskCnt& refCnt = m_cRisk->Cnt(strKey);
				refCnt.nOrderDaySum = cnt["nOrderDaySum"].get_int32().value;
				refCnt.nCancelDaySum = cnt["nCancelDaySum"].get_int32().value;
				refCnt.nOrderMinSum = cnt["nOrderMinSum"].get_int32().value;
				refCnt.nOrderVolMax = cnt["nOrderVolMax"].get_int32().value;
				refCnt.nLongPosMax = cnt["nLongPosMax"].get_int32().value;
				refCnt.nShortPosMax = cnt["nShortPosMax"].get_int32().value;
				nLoadCnt++;
			}
			return nLoadCnt;
		}
	}
	catch (const std::exception& e)
	{
		HTP_LOG << HTP_MSG(string(g_szMdbTbl[MDB_TASK]).append(":").append(e.what()), HTP_CID_FAIL|1017);
		return -1;
	}
	return 0;
}
//更新TARGET任务进度，输出包含部分风控计数用于参考
int CMongoDB::UpdateTask(const map<CFastKey, SCtpTaskRatio, CFastKey>& mapRatio)
{
	//HTP_LOG << "CMongoDB::UpdateTargetRatio" << HTP_ENDL;
	int nIdCnt = 0;
	document doc = document{};
	doc << "UserID" << m_UserCfg->GetLogin().UserID;
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	for (auto& it : mapRatio)
	{
		nIdCnt++;
		SCtpRiskCnt& stRiskCnt = m_cRisk->Cnt(it.first);
		doc << BOOST_STR(it.first.Key()) << bsoncxx::builder::stream::open_document; //按合约分页
		doc << "nTgtAll" << (int64)it.second.nTgtAll;
		doc << "nTgtDone" << (int64)it.second.nTgtDone;
		doc << "nTgtNotDo" << (int64)it.second.nTgtNotDo;
		doc << "fTgtRatioDone" << it.second.fTgtRatioDone*100.0;
		doc << "fTgtRatioNotDo" << it.second.fTgtRatioNotDo*100.0;
		doc << "nVlmAll" << (int64)it.second.nVlmAll;
		doc << "nVlmDone" << (int64)it.second.nVlmDone;
		doc << "nVlmNotDo" << (int64)it.second.nVlmNotDo;
		doc << "fVlmRatioDone" << it.second.fVlmRatioDone * 100.0;
		doc << "fVlmRatioNotDo" << it.second.fVlmRatioNotDo * 100.0;
		doc << "nOrderDaySum" << (int32)stRiskCnt.nOrderDaySum;
		doc << "nCancelDaySum" << (int32)stRiskCnt.nCancelDaySum;
		doc << "nOrderMinSum" << (int32)stRiskCnt.nOrderMinSum;
		doc << "nOrderVolMax" << (int32)stRiskCnt.nOrderVolMax;
		doc << "nLongPosMax" << (int32)stRiskCnt.nLongPosMax;
		doc << "nShortPosMax" << (int32)stRiskCnt.nShortPosMax;
		doc << bsoncxx::builder::stream::close_document;
	}
	HTP_LOG << Insert(g_szMdbTbl[MDB_TASK], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	return nIdCnt;
}
//MDB更新合约信息，包括相关的手续费，交易线程初始化时调用的
//注意：如果交易线程初始化时刻没有相关持仓，则接口限制默认不返回相关的手续费信息
//盘中运行时刻更新手续费信息功能尚未添加
int CMongoDB::UpdateContract(const map<CFastKey,  CThostFtdcInstrumentField, CFastKey>& mapInstrument,
	const map<CFastKey,  CThostFtdcInstrumentCommissionRateField, CFastKey>& m_mapCommission) //合约信息
{
	HTP_LOG << "CMongoDB::UpdateContract" << HTP_ENDL;
	options::update opt;
	opt.upsert(true);
	int nIdCnt = 0;
	for (auto& it : mapInstrument)
	{
		nIdCnt++;
		const CThostFtdcInstrumentField& stInstrument = it.second;
		//if(stInstrument.ProductClass != THOST_FTDC_PC_Futures) { continue; } //规定为期货类型
		document key = document{};
		document doc = document{};
		key << "symbol" << stInstrument.InstrumentID;
		doc << "$set"
			<< bsoncxx::builder::stream::open_document
			<< "symbol" << stInstrument.InstrumentID
			<< "name" << ASCII2UTF_8(string(stInstrument.InstrumentName)).c_str()
			<< "exchange" << stInstrument.ExchangeID
			<< "product" << stInstrument.ProductClass
			<< "size" << stInstrument.VolumeMultiple
			<< "pricetick" << stInstrument.PriceTick
			<< "min_volume" << stInstrument.MinLimitOrderVolume
			<< "LongMarginRatio" << stInstrument.LongMarginRatio
			<< "ShortMarginRatio" << stInstrument.ShortMarginRatio
			<< "PriceTick" << stInstrument.PriceTick;
		auto itCommission = m_mapCommission.find(GetKind(it.second.InstrumentID));
		if (itCommission != m_mapCommission.end())
		{
			doc << "OpenRatioByVolume" << itCommission->second.OpenRatioByVolume
				<< "OpenRatioByMoney" << itCommission->second.OpenRatioByMoney
				<< "CloseRatioByMoney" << itCommission->second.CloseRatioByMoney
				<< "CloseRatioByVolume" << itCommission->second.CloseRatioByVolume
				<< "CloseTodayRatioByMoney" << itCommission->second.CloseTodayRatioByMoney
				<< "CloseTodayRatioByVolume" << itCommission->second.CloseTodayRatioByVolume;
		}
		doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
		doc << bsoncxx::builder::stream::close_document;
		HTP_LOG << update(g_szMdbTbl[MDB_CONTRACT], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), key, doc, opt);
	}
	return nIdCnt;
}
//MDB更新订单延迟，主要用作统计下单与成交延迟
int CMongoDB::OnTdTimeSheet(shared_ptr<const SHtpTimeSheet> stTimeSheet) //时间缀
{
	document doc = document{};
	doc
		<< "symbol" << stTimeSheet->InstrumentID
		<< "OrderRef" << stTimeSheet->OrderRef;
	doc << "SysTime" << (int64_t)(HTP_SYS.GetTime(CTP_TIME_NET).GetYMDHMS());
	for (int i = 0; i < HTS_ENDF; i++)
	{
		doc << BOOST_STR(g_strHTS[i]) << (int64_t)stTimeSheet->TBL[i];
	}
	HTP_LOG << Insert(g_szMdbTbl[MDB_HTS], HTP_SYS.GetTime(CTP_TIME_TRADING).GetStrYMD(), doc);
	return 0;
}
