#include "CMyData.h"
#ifdef _SHOW_HTS
#include "CLogThread.h"
#endif // _SHOW_HTS


//----------------------------------------------K线数据结构--------------------------------------------
CCtpLineK::CCtpLineK()
{
	memset(m_stLineK, 0, (sizeof(SCtpLineK) * LKT_ENDF));
	memset(m_InstrumentID, 0, sizeof(TThostFtdcInstrumentIDType));
	memset(m_ExchangeID, 0, sizeof(TThostFtdcExchangeIDType));
}
//CCtpLineK::CCtpLineK(const CCtpTick& ctpTick)
//{
//	*this = ctpTick;
//}
CCtpLineK::~CCtpLineK() {}
bool CCtpLineK::Compare(const CCtpLineK& ctpLineK) //K线数据的内存比较功能，用于内存篡改检测，提高程序健壮性
{
	return ((0 == memcmp(m_stLineK, ctpLineK.m_stLineK, (sizeof(SCtpLineK) * LKT_ENDF)))
		&& (m_ctNature == ctpLineK.m_ctNature)
		&& (m_ctTrading == ctpLineK.m_ctTrading));
}
CCtpLineK& CCtpLineK::operator = (const CCtpLineK& ctpLineK)
{
	memcpy(m_stLineK, ctpLineK.m_stLineK, (sizeof(SCtpLineK) * LKT_ENDF));
	memcpy(m_InstrumentID, ctpLineK.m_InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	memcpy(m_ExchangeID, ctpLineK.m_ExchangeID, sizeof(TThostFtdcExchangeIDType));
	m_ctNature = ctpLineK.m_ctNature;
	m_ctTrading = ctpLineK.m_ctTrading;
	return *this;
}
CCtpLineK& CCtpLineK::SetID(const TThostFtdcInstrumentIDType& stID)
{
	memcpy(m_InstrumentID, stID, sizeof(TThostFtdcInstrumentIDType));
	return *this;
}
//K线计算过程中的数据初始化，在分/时/日时间跳变时需要将增量计算数据初始化
CCtpLineK& CCtpLineK::Init(eLineKType eType, const SCtpTick& stTick, bool bZeroMode)
{
	SCtpLineK& stLineK = m_stLineK[eType];
	//stLineK.bIsLastTick = false;
	//stLineK.LastPrice = stTick.LastPrice;
	stLineK.OpenPrice = stTick.LastPrice;
	stLineK.HighestPrice = stTick.LastPrice;
	stLineK.LowestPrice = stTick.LastPrice;
	//stLineK.Turnover = stTick.Turnover;
	stLineK.OpenInterest = stTick.OpenInterest;
	stLineK.ClosePrice = stTick.LastPrice;
	stLineK.OiPrev = stTick.OpenInterest; //当前bar的第一个tick的 open interest
	if (bZeroMode)
	{
		stLineK.prcCount = 0;
		stLineK.prcVolume = 0;////TICK量差
		stLineK.prcVolumeSum = 0; ////量差加
		stLineK.prcVwap = 0.0; //量加权平均
		stLineK.prcTwap = 0.0; //算数价格平均
		stLineK.prcVwapSum = 0.0; //量加权平均和
		stLineK.prcTwapSum = 0.0; //算数价格平均和
		//额外的
		stLineK.bidVolSum = 0.0; //盘口bid1量加总
		stLineK.askVolSum = 0.0; //盘口ask1量加总
		stLineK.bidPrcVSum = 0.0; //盘口bid1价加权成交量和
		stLineK.askPrcVSum = 0.0; //盘口ask1价加权成交量和
		stLineK.SpreadSum = 0.0; //1档盘口(askPrcT - bidPrcT) 和
		stLineK.bidPrcV = 0.0; //盘口bid1价加权成交量
		stLineK.askPrcV = 0.0; //盘口ask1价加权成交量
		stLineK.Spread = 0.0; //1档盘口(askPrcT - bidPrcT) 按时间平均
		stLineK.OiChangeSum = 0.0; //tick 里面oichg 加总
		stLineK.OiCloseSum = 0.0; //tick 里面oiClose 加总
		stLineK.OiOpenSum = 0.0; //tick 里面oiOpen 加总
	}
	else
	{
		stLineK.prcCount = 1;
		stLineK.prcVolume = stTick.Volume - stLineK.Volume;////TICK量差
		stLineK.prcVolumeSum = stLineK.prcVolume; ////量差加
		if (stLineK.prcVolume > 0) //量差大于0，应用最新，否则使用上次
		{
			stLineK.prcVwap = stTick.LastPrice; //量加权平均
			stLineK.prcTwap = stTick.LastPrice; //算数价格平均
		}
		else //使用上次的
		{
			if (stLineK.prcVwap == 0.0) { stLineK.prcVwap = stTick.LastPrice; } //当天第一个
			if (stLineK.prcTwap == 0.0) { stLineK.prcTwap = stTick.LastPrice; } //当天第一个
		}
		stLineK.prcVwapSum = stLineK.prcVwap * stLineK.prcVolumeSum; //量加权平均和
		stLineK.prcTwapSum = stLineK.prcTwap * stLineK.prcCount; //算数价格平均和
		//额外的
		stLineK.bidVolSum = stTick.BidVolume1; //盘口bid1加总
		stLineK.askVolSum = stTick.AskVolume1; //盘口ask1加总
		stLineK.bidPrcV = stTick.BidPrice1; //盘口bid1加权成交量
		stLineK.askPrcV = stTick.AskPrice1; //盘口ask1加权成交量
		stLineK.Spread = stTick.AskPrice1 - stTick.BidPrice1; //1档盘口(askPrcT - bidPrcT) 按时间平均
		stLineK.bidPrcVSum = stTick.BidPrice1 * stTick.BidVolume1; //盘口bid1价加权成交量和
		stLineK.askPrcVSum = stTick.AskPrice1 * stTick.AskVolume1; //盘口ask1价加权成交量和
		stLineK.SpreadSum = stLineK.Spread * stLineK.prcCount; //1档盘口(askPrcT - bidPrcT) 和
		stLineK.OiChangeSum = stTick.OiChange; //tick 里面oichg 加总
		stLineK.OiCloseSum = stTick.OiClose; //tick 里面oiClose 加总
		stLineK.OiOpenSum = stTick.OiOpen; //tick 里面oiOpen 加总
	}
	stLineK.Volume = stTick.Volume; //实际量
	return *this;
}
//void CtpToValue(CCtpLineK& ctpTo, CCtpTick& ctpFrom)
//{
//	memcpy(ctpTo.TradingDay, ctpFrom.TradingDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.ExchangeID, ctpFrom.ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	ctpTo.LastPrice = ctpFrom.LastPrice;
//	ctpTo.OpenPrice = ctpFrom.OpenPrice;
//	ctpTo.HighestPrice = ctpFrom.HighestPrice;
//	ctpTo.LowestPrice = ctpFrom.LowestPrice;
//	ctpTo.Volume = ctpFrom.Volume;
//	ctpTo.Turnover = ctpFrom.Turnover;
//	ctpTo.OpenInterest = ctpFrom.OpenInterest;
//	ctpTo.ClosePrice = ctpFrom.ClosePrice;
//	memcpy(ctpTo.UpdateTime, ctpFrom.UpdateTime, sizeof(TThostFtdcTimeType));
//	ctpTo.UpdateMillisec = ctpFrom.UpdateMillisec;
//	memcpy(ctpTo.ActionDay, ctpFrom.ActionDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.InstrumentID, ctpFrom.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//}
//void CtpToValue(CCtpLineK& ctpTo, CThostFtdcDepthMarketDataField& ctpFrom)
//{
//	memcpy(ctpTo.TradingDay, ctpFrom.TradingDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.ExchangeID, ctpFrom.ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	ctpTo.LastPrice = ctpFrom.LastPrice;
//	ctpTo.OpenPrice = ctpFrom.OpenPrice;
//	ctpTo.HighestPrice = ctpFrom.HighestPrice;
//	ctpTo.LowestPrice = ctpFrom.LowestPrice;
//	ctpTo.Volume = ctpFrom.Volume;
//	ctpTo.Turnover = ctpFrom.Turnover;
//	ctpTo.OpenInterest = ctpFrom.OpenInterest;
//	ctpTo.ClosePrice = ctpFrom.ClosePrice;
//	memcpy(ctpTo.UpdateTime, ctpFrom.UpdateTime, sizeof(TThostFtdcTimeType));
//	ctpTo.UpdateMillisec = ctpFrom.UpdateMillisec;
//	memcpy(ctpTo.ActionDay, ctpFrom.ActionDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.InstrumentID, ctpFrom.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//}
//
//void CtpToValue(CCtpDayTime& ctpTo, CCtpTick& ctpFrom)
//{
//	memcpy(ctpTo.InstrumentID, ctpFrom.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	memcpy(ctpTo.NatureDay, ctpFrom.ActionDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.TradingDay, ctpFrom.TradingDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.UpdateTime, ctpFrom.UpdateTime, sizeof(TThostFtdcTimeType));
//}
//void CtpToValue(CCtpDayTime& ctpTo, CCtpLineK& ctpFrom)
//{
//	memcpy(ctpTo.InstrumentID, ctpFrom.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	memcpy(ctpTo.NatureDay, ctpFrom.ActionDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.TradingDay, ctpFrom.TradingDay, sizeof(TThostFtdcDateType));
//	memcpy(ctpTo.UpdateTime, ctpFrom.UpdateTime, sizeof(TThostFtdcTimeType));
//}
//----------------------------------------------配置数据结构--------------------------------------------
const char* g_strUserConfig[HTP_UC_ENDF] = {
	"HTP_UC_NULL",
	"HTP_UC_NAME",
	"HTP_UC_MDBURL",
	"HTP_UC_CMDLIST",
	"HTP_UC_TD", "HTP_UC_MD", "HTP_UC_USERID", "HTP_UC_BROKERID",
	"HTP_UC_PASSWARD", "HTP_UC_APPID", "HTP_UC_AUTHCODE", "HTP_UC_PRODUCTINFO"
};
CUserConfig::CUserConfig()
{
	Clear();
};

CUserConfig::~CUserConfig()
{

};
void CUserConfig::Clear()
{
	memset(&m_stLogout, 0, sizeof(m_stLogout));
	memset(&m_stLogin, 0, sizeof(m_stLogin));
	memset(&m_stAuthenticate, 0, sizeof(m_stAuthenticate));
	memset(&m_stConfirm, 0, sizeof(m_stConfirm));
	memset(&m_stPosition, 0, sizeof(m_stPosition));
	memset(&m_stAccount, 0, sizeof(m_stAccount));
	memset(&m_stOrder, 0, sizeof(m_stOrder));
	memset(&m_stTrade, 0, sizeof(m_stTrade));
	memset(&m_stCommission, 0, sizeof(m_stCommission));
	memset(&m_stConfirm, 0, sizeof(m_stConfirm));
	m_strFrontMD.clear();
	m_strFrontTD.clear();
	m_strFrontMD.clear();
	m_strFrontTD.clear();
	m_strMDB.clear();
	m_strExePath.clear();
	m_strExeFolder.clear();
	m_strDate.clear();
	m_vecCmd.clear();
	m_vecEmail.clear();
}
//过于过滤去除JSON库解析后的字符串数据，以便能被正确地填充到各线程接口调用的地方
string CUserConfig::Trim(const string& strValue)
{
	string strTrim;
	for (auto& it : strValue)
	{
		if ((it == '\r') || (it == '\n') || (it == '\"')) { continue; }
		strTrim.push_back(it);
	}
	return strTrim;
}
//加载本地JSON配置，JSON库来源于第三方相对精简，需要做一些检测与处理机制确保数据的正确性
CHtpMsg CUserConfig::Load(const string& strFileName, const string& strUcName)
{
	char exeFullPath[MAX_PATH]; // Full path 
	GetModuleFileName(NULL, exeFullPath, MAX_PATH); //配置文件默认为软件根目录下的
	m_strExePath = exeFullPath;    // Get full path of the file
	m_strExeFolder = m_strExePath.substr(0, m_strExePath.find_last_of("\\"));
	GetLocalTime(&m_sysTime);
	if (strFileName.empty()) { return CHtpMsg("JSON 配置文件路径为空值！", HTP_CID_FAIL| HTP_CID_ISUTF8); }
	string strFile = m_strExeFolder+("\\")+(strFileName)+(".json");
	std::ifstream fin(strFile);
	std::stringstream ssContent;
	if (false == fin.good()) { return CHtpMsg(string("JSON文件打开失败").append(strFile), HTP_CID_FAIL | HTP_CID_ISUTF8); }
	ssContent << fin.rdbuf();
	fin.close();
	string strConfig;
	try
	{
		neb::CJsonObject oJson;
		if (false == oJson.Parse(ssContent.str())) //解析JSON配置
		{ return CHtpMsg("JSON 配置文件解析失败", HTP_CID_FAIL | HTP_CID_ISUTF8); }
		for (int i = 0; i < oJson["EMAIL"].GetArraySize(); i++)
		{
			auto& jsonEmail = oJson["EMAIL"][i];
			string strValue = Trim(jsonEmail.ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[i]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			m_vecEmail.push_back(strValue);
		}
		for (int i = 0; i < oJson[strFileName].GetArraySize(); i++)
		{
			auto& jsonConfig = oJson[strFileName][i];
			string strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_NAME]].ToString());
			if (strValue != strUcName) { continue; }
			strConfig.append(strValue).append("\r\n");
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_MDBURL]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_MDBURL]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetMDB(strValue);
			for (int j = 0; j < jsonConfig[g_strUserConfig[HTP_UC_CMDLIST]].GetArraySize(); j++)
			{
				strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_CMDLIST]][j].ToString());
				if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[i]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
				strConfig.append(strValue).append("\r\n");
				PushCmd(strValue);
			}
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_TD]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_TD]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetFrontTD(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_MD]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_MD]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetFrontMD(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_USERID]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_USERID]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetUserID(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_BROKERID]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_BROKERID]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetBrokerID(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_PASSWARD]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_PASSWARD]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			//strConfig.append(strValue).append("\r\n");
			SetPassword(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_APPID]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_APPID]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetAppID(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_AUTHCODE]].ToString());
			if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_AUTHCODE]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetAuthCode(strValue);
			strValue = Trim(jsonConfig[g_strUserConfig[HTP_UC_PRODUCTINFO]].ToString());
			//if (strValue.empty()) { return CHtpMsg(string("JSON 配置字段解析失败->").append(g_strUserConfig[HTP_UC_PRODUCTINFO]), HTP_CID_FAIL | HTP_CID_ISUTF8); }
			strConfig.append(strValue).append("\r\n");
			SetUserProductInfo(strValue);
			return CHtpMsg(string("JSON配置加载完成").append(strConfig), HTP_CID_OK | HTP_CID_ISUTF8);
		}
		return CHtpMsg("JSON 配置加载失败，找不到指定项！", HTP_CID_FAIL | HTP_CID_ISUTF8);
	}
	catch (...)
	{
		return CHtpMsg(string("JSON 数据解析错误"), HTP_CID_FAIL | HTP_CID_ISUTF8);
	}
	return CHtpMsg("JSON 配置加载失败", HTP_CID_FAIL | HTP_CID_ISUTF8);
}
void CUserConfig::SetMDB(const string& strConfig)
{
	m_strMDB = strConfig;
}
void CUserConfig::PushCmd(const string& strConfig)
{
	m_vecCmd.push_back(strConfig);
}
void CUserConfig::SetFrontMD(const string& strConfig)
{
	m_strFrontMD = strConfig;
}
void CUserConfig::SetFrontTD(const string& strConfig)
{
	m_strFrontTD = strConfig;
}
void CUserConfig::SetUserID(const string& strConfig)
{
	strcpy_s(m_stLogout.UserID, strConfig.c_str());
	strcpy_s(m_stLogin.UserID, strConfig.c_str());
	strcpy_s(m_stAuthenticate.UserID, strConfig.c_str());
	strcpy_s(m_stConfirm.InvestorID, strConfig.c_str());
	strcpy_s(m_stPosition.InvestorID, strConfig.c_str());
	strcpy_s(m_stAccount.InvestorID, strConfig.c_str());
	strcpy_s(m_stOrder.InvestorID, strConfig.c_str());
	strcpy_s(m_stTrade.InvestorID, strConfig.c_str());
	strcpy_s(m_stCommission.InvestorID, strConfig.c_str());
}
void CUserConfig::SetBrokerID(const string& strConfig)
{
	strcpy_s(m_stLogout.BrokerID, strConfig.c_str());
	strcpy_s(m_stLogin.BrokerID, strConfig.c_str());
	strcpy_s(m_stAuthenticate.BrokerID, strConfig.c_str());
	strcpy_s(m_stConfirm.BrokerID, strConfig.c_str());
	strcpy_s(m_stPosition.BrokerID, strConfig.c_str());
	strcpy_s(m_stAccount.BrokerID, strConfig.c_str());
	strcpy_s(m_stOrder.BrokerID, strConfig.c_str());
	strcpy_s(m_stTrade.BrokerID, strConfig.c_str());
	strcpy_s(m_stCommission.BrokerID, strConfig.c_str());
}
void CUserConfig::SetPassword(const string& strConfig)
{
	//strcpy_s(m_stLogout.Password, strConfig.c_str());
	strcpy_s(m_stLogin.Password, strConfig.c_str());
	//strcpy_s(m_stAuthenticate.Password, strConfig.c_str());
	//strcpy_s(m_stConfirm.Password, strConfig.c_str());
}
void CUserConfig::SetAppID(const string& strConfig)
{
	//strcpy_s(m_stLogout.AppID, strConfig.c_str());
	//strcpy_s(m_stLogin.AppID, strConfig.c_str());
	strcpy_s(m_stAuthenticate.AppID, strConfig.c_str());
	//strcpy_s(m_stConfirm.AppID, strConfig.c_str());
}
void CUserConfig::SetAuthCode(const string& strConfig)
{
	//strcpy_s(m_stLogout.AuthCode, strConfig.c_str());
	//strcpy_s(m_stLogin.AuthCode, strConfig.c_str());
	strcpy_s(m_stAuthenticate.AuthCode, strConfig.c_str());
	//strcpy_s(m_stConfirm.AuthCode, strConfig.c_str());
}
void CUserConfig::SetUserProductInfo(const string& strConfig)
{
	//strcpy_s(m_stLogout.UserProductInfo, strConfig.c_str());
	strcpy_s(m_stLogin.UserProductInfo, strConfig.c_str());
	strcpy_s(m_stAuthenticate.UserProductInfo, strConfig.c_str());
	//strcpy_s(m_stConfirm.UserProductInfo, strConfig.c_str());
}
//交易过程中的状态标记与延迟统计
//用于交易订单管理
const char* g_strHTS[HTS_ENDF] = {
		"HTS_INIT",
		//MD_TICK, //tick的时间
		//GET_TARGET,//获取到target时间
		"ORD_NEW", //创建订单时间
		"ORD_DOING", //订单执行时间
		"ORD_DONE", //下单接口调用完时间
		"ORDED", //已委托时间
		"TRD_PART", //部分成交时间
		"CCL_DOING", //撤单时间
		"CCL_DONE", //撤单成功时间
		"TRADED", //已成交时间
		"CANCEL" //撤单成功时间
};
//-------------------------------------------------------订单管理------------------------------------------------------------------
CHtpManage::CHtpManage() { m_mapOrder.clear(); }
CHtpManage::~CHtpManage() {}
//设置时间缀，有流程顺序限制
//但非TARGET单成交后依然会刷新到持仓，撤单依然会自动重开
bool CHtpManage::SetStatus(eHtpOrderStatus eStatus, const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef)
{
#ifdef _SHOW_HTS
	HTP_LOG << HTP_MSG(string("HTS: ").append(InstrumentID).append(" ")
		.append(OrderRef).append(" ")
		.append(g_strHTS[eStatus]), HTP_CID_OK | 1888);
#endif // _SHOW_HTS
	unique_lock<mutex> lock(m_lock);
	auto& stOrder = m_mapOrder[InstrumentID][OrderRef];
	if (!stOrder)//任意产生的订单都参与管理
	{
		stOrder = make_shared<CHtpOrderManage>(InstrumentID, OrderRef);
	}
	return stOrder->Set(eStatus);
}
//获取延迟
shared_ptr<const SHtpTimeSheet>  CHtpManage::GetDelay(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef)
{
	shared_ptr<SHtpTimeSheet> stSheet = make_shared<SHtpTimeSheet>();
	memcpy(stSheet->InstrumentID, InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	memcpy(stSheet->OrderRef, OrderRef, sizeof(TThostFtdcOrderRefType));
	unique_lock<mutex> lock(m_lock);
	auto& mapOrder = m_mapOrder[InstrumentID];
	auto itOrder = mapOrder.find(OrderRef);
	if (itOrder != mapOrder.end())
	{
		for (int i = 0; i < HTS_ENDF; i++)
		{
			stSheet->TBL[i] = itOrder->second->GetDelay(eHtpOrderStatus(i));
		}
	}
	return stSheet;
}
//获取标记状态
eHtpOrderStatus CHtpManage::GetStatus(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef)
{
	unique_lock<mutex> lock(m_lock);
	auto& mapOrder = m_mapOrder[InstrumentID];
	auto itOrder = mapOrder.find(OrderRef);
	if (itOrder != mapOrder.end())
		return itOrder->second->GetStatus();
	else
		return HTS_INIT;
}
//获取延时或距离相关状态的时间
ULONG64 CHtpManage::GetTickChange(const eHtpOrderStatus& eStatus, const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef)
{
	unique_lock<mutex> lock(m_lock);
	auto& mapOrder = m_mapOrder[InstrumentID];
	auto itOrder = mapOrder.find(OrderRef);
	if (itOrder != mapOrder.end())
		return itOrder->second->GetTickChange(eStatus);
	else
		return 0LL;
}
//订单管理维护删除
void CHtpManage::Delete(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef)
{
	unique_lock<mutex> lock(m_lock);
	m_mapOrder[InstrumentID].erase(OrderRef);
}
//获取挂单,ullTimeOutMS=超时时间
size_t CHtpManage::GetOrderGD(const CFastKey& InstrumentID, vector<CFastKey>& vecGD, ULONG64 ullTimeOutMS)
{
	m_lock.lock();
	const auto mapOrder = m_mapOrder[InstrumentID]; //一级map需传值，在挂单较多的情况下一定程度上影响效率
	m_lock.unlock();
	if (mapOrder.size() < 1) { return 0; }
	vecGD.clear();
	for (const auto& itOrder : mapOrder)
	{
		if (!itOrder.second) { continue; }
		if ((itOrder.second->GetStatus() > HTS_ORD_DONE)
			&& (itOrder.second->GetStatus() < HTS_TRADED)
			&& (itOrder.second->GetTickChange(HTS_ORDED) >= ullTimeOutMS))
		{
			vecGD.push_back(itOrder.first); //已下单但还未完全成交的属于挂单
		}
	}
	return vecGD.size();
}
////获取超时的挂单,ullTimeOutMS=超时时间，10S调用一次检查
//size_t CHtpManage::GetTimeOutGD(ULONG64 ullTimeOutMS, map<CFastKey, map<CFastKey, shared_ptr<CHtpOrderManage>, CFastKey>, CFastKey>& mapGD)
//{
//	m_lock.lock();
//	const auto mapOrder = m_mapOrder; //订单快照，在交易合约不多的情况下可以用此方法遍历
//	m_lock.unlock();
//	mapGD.clear();
//	for (const auto& itOrder : mapOrder)
//	{
//		for (const auto& itGD : itOrder.second)
//		{
//			if (!itGD.second) { continue; }
//			if ((itGD.second->GetStatus() > HTS_ORD_DONE) 
//				&& (itGD.second->GetStatus() < HTS_TRADED)
//				&& (itGD.second->GetTickChange(HTS_ORDED) > ullTimeOutMS)) //已下单但还未完全成交并超时的挂单
//			{
//				mapGD[itOrder.first][itGD.first] = itGD.second;
//			}
//		}
//	}
//	return mapGD.size();
//}
//是否有正在运行中的订单，用于判断是否能够订单生成
int CHtpManage::HaveDoing(const TThostFtdcInstrumentIDType& InstrumentID)
{
	m_lock.lock();
	const auto mapOrder = m_mapOrder[InstrumentID];
	m_lock.unlock();
	for (const auto& itOrder : mapOrder)
	{
		if (!itOrder.second) { continue; }
		eHtpOrderStatus eStatus = itOrder.second->GetStatus();
		if (eStatus < HTS_ORD_DONE || eStatus == HTS_CCL_DOING)
		{
			return 1; //属于在本地队列中还未被执行了的将不会参与持仓与TARGET计算
		}
		else if (eStatus == HTS_ORD_DONE
			|| eStatus == HTS_CCL_DONE)  //属于已委托中但一直没有产生委托回报的
		{
			if (HTP_FRONT_TIMEOUTMS < itOrder.second->GetTickChange(eStatus))
			{
				return -2;  //超时太长，有可能前置前置掉线导致的
			}
			if (HTP_ORDER_TIMEOUTMS < itOrder.second->GetTickChange(eStatus))
			{
				return -1;  //一直没有产生委托回报但委托已超过下单时间的
			}
			return 1;
		}
	}
	return 0; //没有阻塞
}


