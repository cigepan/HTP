#pragma once

#include "stdafx.h"
#include "ThostFtdcMdApi.h"
#include "CCycleQueue.h"
#include "CTimeCalc.h"
#include "CJsonObject.hpp"
#include <sstream>
//---------------------------------------------------队列--------------------------------------------------------------------
template <class T>
class CCycleQueueEx1024 : public CCycleQueue<T>
{
public:
	CCycleQueueEx1024() : CCycleQueue(0x000003FF){}
	~CCycleQueueEx1024() {}
};

template <class T>
class CCycleQueueEx2048 : public CCycleQueue<T>
{
public:
	CCycleQueueEx2048() : CCycleQueue(0x000007FF) {}
	~CCycleQueueEx2048() {}
};

template <class T>
class CCycleQueueEx4096 : public CCycleQueue<T>
{
public:
	CCycleQueueEx4096() : CCycleQueue(0x00000FFF) {}
	~CCycleQueueEx4096() {}
};

template <class T>
class CCycleQueueEx8192 : public CCycleQueue<T>
{
public:
	CCycleQueueEx8192() : CCycleQueue(0x00001FFF) {}
	~CCycleQueueEx8192() {}
};
//HTP主进程返回错误ID
enum eHtpMainReturn
{
	HMR_FAIL_INIT = 9000,
	HMR_FAIL_MDB,
	HMR_FAIL_LOG,
	HMR_FAIL_NTP,
	HMR_FAIL_TD,
	HMR_FAIL_MD,
	HMR_FAIL_RISK,
	HMR_CTP_CLOSED,
	HMR_WEEK_CLOSED,
	HMR_SYS_RISK,
	HMR_FAIL_CTP,
	HMR_ENDF
};

//---------------------------------------------------行情--------------------------------------------------------------------
///深度行情
struct SCtpTick
{
	///合约交易状态
	TThostFtdcInstrumentStatusType	InstrumentStatus;
	///持仓量变化
	TThostFtdcLargeVolumeType	OiChange; //与上一个tick的持仓量对比
	///数量变化
	TThostFtdcLargeVolumeType	VolChange; //与上一个TICK对比
	///开仓量
	TThostFtdcLargeVolumeType	OiOpen; //(持仓量变化+数量变化)*0.5
	///平仓量
	TThostFtdcLargeVolumeType	OiClose; //(持仓量变化-数量变化)*0.5

	//TThostFtdcOldExchangeInstIDType	reserve2;
	///最新价
	TThostFtdcPriceType	LastPrice;
	///上次结算价
	TThostFtdcPriceType	PreSettlementPrice;
	///昨收盘
	TThostFtdcPriceType	PreClosePrice;
	/////昨持仓量
	//TThostFtdcLargeVolumeType	PreOpenInterest;
	///今开盘
	TThostFtdcPriceType	OpenPrice;
	///最高价
	TThostFtdcPriceType	HighestPrice;
	///最低价
	TThostFtdcPriceType	LowestPrice;
	///数量
	TThostFtdcVolumeType	Volume;
	///成交金额
	TThostFtdcMoneyType	Turnover;
	///持仓量
	TThostFtdcLargeVolumeType	OpenInterest;
	///今收盘
	TThostFtdcPriceType	ClosePrice;
	///本次结算价
	TThostFtdcPriceType	SettlementPrice;
	///涨停板价
	TThostFtdcPriceType	UpperLimitPrice;
	///跌停板价
	TThostFtdcPriceType	LowerLimitPrice;
	/////昨虚实度
	//TThostFtdcRatioType	PreDelta;
	/////今虚实度
	//TThostFtdcRatioType	CurrDelta;
	///申买价一
	TThostFtdcPriceType	BidPrice1;
	///申买量一
	TThostFtdcVolumeType	BidVolume1;
	///申卖价一
	TThostFtdcPriceType	AskPrice1;
	///申卖量一
	TThostFtdcVolumeType	AskVolume1;
	///申买价二
	TThostFtdcPriceType	BidPrice2;
	///申买量二
	TThostFtdcVolumeType	BidVolume2;
	///申卖价二
	TThostFtdcPriceType	AskPrice2;
	///申卖量二
	TThostFtdcVolumeType	AskVolume2;
	///申买价三
	TThostFtdcPriceType	BidPrice3;
	///申买量三
	TThostFtdcVolumeType	BidVolume3;
	///申卖价三
	TThostFtdcPriceType	AskPrice3;
	///申卖量三
	TThostFtdcVolumeType	AskVolume3;
	///申买价四
	TThostFtdcPriceType	BidPrice4;
	///申买量四
	TThostFtdcVolumeType	BidVolume4;
	///申卖价四
	TThostFtdcPriceType	AskPrice4;
	///申卖量四
	TThostFtdcVolumeType	AskVolume4;
	///申买价五
	TThostFtdcPriceType	BidPrice5;
	///申买量五
	TThostFtdcVolumeType	BidVolume5;
	///申卖价五
	TThostFtdcPriceType	AskPrice5;
	///申卖量五
	TThostFtdcVolumeType	AskVolume5;
	///当日均价
	TThostFtdcPriceType	AveragePrice;
	///交易日
	TThostFtdcDateType	TradingDay;
	///业务日期
	TThostFtdcDateType	ActionDay;
	///最后修改时间
	TThostFtdcTimeType	UpdateTime;
	///最后修改毫秒
	TThostFtdcMillisecType	UpdateMillisec;
	///交易所代码
	TThostFtdcExchangeIDType	ExchangeID;
	///合约代码
	TThostFtdcInstrumentIDType	InstrumentID;
	/////合约在交易所的代码
	//TThostFtdcExchangeInstIDType	ExchangeInstID;
	/////上带价
	//TThostFtdcPriceType	BandingUpperPrice;
	/////下带价
	//TThostFtdcPriceType	BandingLowerPrice;
};

///深度行情K
struct SCtpLineK
{
	TThostFtdcPriceType prcVwap; //量加权平均
	TThostFtdcPriceType prcTwap; //算数价格平均
	TThostFtdcPriceType prcVwapSum; //量加权平均和
	TThostFtdcPriceType prcTwapSum; //算数价格平均和
	/////最新价
	//TThostFtdcPriceType	LastPrice;
	///今开盘
	TThostFtdcPriceType	OpenPrice;
	///最高价
	TThostFtdcPriceType	HighestPrice;
	///最低价
	TThostFtdcPriceType	LowestPrice;
	/////成交金额
	//TThostFtdcMoneyType	Turnover;
	///今收盘
	TThostFtdcPriceType	ClosePrice;
	///持仓量
	TThostFtdcLargeVolumeType	OpenInterest;
	TThostFtdcVolumeType	Volume; //实际量
	TThostFtdcVolumeType	prcVolume; ////增量
	TThostFtdcVolumeType	prcVolumeSum; ////量加
	//额外的信息20210902
	TThostFtdcLargeVolumeType bidVolSum; //盘口bid1量加总
	TThostFtdcLargeVolumeType askVolSum; //盘口ask1量加总
	TThostFtdcLargeVolumeType bidPrcVSum; //盘口bid1价加权成交量和
	TThostFtdcLargeVolumeType askPrcVSum; //盘口ask1价加权成交量和
	TThostFtdcLargeVolumeType SpreadSum; //1档盘口(askPrcT - bidPrcT) 和
	TThostFtdcLargeVolumeType bidPrcV; //盘口bid1价加权成交量
	TThostFtdcLargeVolumeType askPrcV; //盘口ask1价加权成交量
	TThostFtdcLargeVolumeType Spread; //1档盘口(askPrcT - bidPrcT) 按时间平均
	TThostFtdcLargeVolumeType OiPrev; //当前bar的第一个tick的 open interest
	TThostFtdcLargeVolumeType OiChangeSum; //tick 里面oichg 加总
	TThostFtdcLargeVolumeType OiCloseSum; //tick 里面oiClose 加总
	TThostFtdcLargeVolumeType OiOpenSum; //tick 里面oiOpen 加总
	int prcCount; //TICK计数
	bool bIsLastTick; //是否是最后一个TICK
};
enum eCtpOrderModel
{
	CTP_ORDM_DEFAULT = 0,
	CTP_ORDM_ENDF
};
enum eCtpExid
{
	CTP_EXID_INE = 0,
	CTP_EXID_CFFEX,
	CTP_EXID_CZCE,
	CTP_EXID_DCE,
	CTP_EXID_SHFE,
	CTP_EXID_ENDF
};
const char g_strCtpExid[CTP_EXID_ENDF][16] = {
	"INE", "CFFEX", "CZCE", "DCE", "SHFE"
};

enum eCtpTime
{
	CTP_TIME_SYS = 0,
	CTP_TIME_INE,
	CTP_TIME_CFFEX,
	CTP_TIME_CZCE,
	CTP_TIME_DCE,
	CTP_TIME_SHFE,
	CTP_TIME_NET,
	CTP_TIME_TRADING,
	CTP_TIME_NATURE,
	CTP_TIME_DEFAULT,
	CTP_TIME_ENDF
};
const char g_strCtpTime[CTP_TIME_ENDF][16] = {
	"SYS", "INE", "CFFEX", "CZCE", "DCE", "SHFE", "NET","TRADING","NATURE", "DEFAULT"
};

inline eCtpTime GetExID(const TThostFtdcExchangeIDType& ExchangeID)
{
	switch (ExchangeID[0])
	{
	case 'S':
		if (ExchangeID[1] == 'H') { return CTP_TIME_SHFE; }
		else if (ExchangeID[1] == 'Y') { return CTP_TIME_SYS; }
	case 'C':
		if (ExchangeID[1] == 'F') { return CTP_TIME_CFFEX; }
		else if (ExchangeID[1] == 'Z') { return CTP_TIME_CZCE; }
	case 'D':
		if (ExchangeID[1] == 'C') { return CTP_TIME_DCE; }
		else if (ExchangeID[1] == 'E') { return CTP_TIME_DEFAULT; }
	case 'I':
		if (ExchangeID[1] == 'N') { return CTP_TIME_INE; }
	case 'N':
		if (ExchangeID[1] == 'E') { return CTP_TIME_NET; }
	default:
		return CTP_TIME_ENDF;
	}
}
#define CTP_EXID_ISSHFE(exid) (exid[0]=='S'&&exid[2]=='F')
#define CTP_EXID_ISINE(exid) (exid[0]=='I'&&exid[2]=='E')
#define CTP_EXID_ISDCE(exid) (exid[0]=='D'&&exid[2]=='E')
#define CTP_EXID_ISCZCE(exid) (exid[0]=='C'&&exid[2]=='C')
#define CTP_EXID_ISCFFEX(exid) (exid[0]=='C'&&exid[2]=='F')

#define CTP_MIN_PRICE ((double)0.000001)
#define CTP_MAX_PRICE ((double)99999999.999)
#define CTP_PRICE_ISOUT(PRICE) ((PRICE<CTP_MIN_PRICE)||(PRICE>CTP_MAX_PRICE))
#define CTP_MIN_DOUBLE ((double)-999999999999999.999)
#define CTP_MAX_DOUBLE ((double)999999999999999.999)
#define CTP_DOUBLE_ISOUT(DBV) ((DBV<CTP_MIN_DOUBLE)||(DBV>CTP_MAX_DOUBLE))
#define CTP_MIN_INT (-999999999)
#define CTP_MAX_INT (999999999)
#define CTP_INT_ISOUT(NV) ((NV<CTP_MIN_INT)||(NV>CTP_MAX_INT))
//查询接口超时10分钟则认定前置超时，则需要重启CTP
#define HTP_FRONT_TIMEOUTMS 600000
//订单执行超时5分钟，没有收到委托回报则查询委托和成交，并同步持仓
#define HTP_ORDER_TIMEOUTMS 300000
//挂单超时1分钟才能被撤
#define HTP_TRADE_TIMEOUTMS 60000
//挂单至少5S才能被撤重开
#define HTP_CANCEL_LIMITTIME 5000
enum eCtpStatus
{
	CTPST_NULL = 0,
	CTPST_INIT,
	CTPST_FRONTON,
	CTPST_FRONTOFF,
	CTPST_ONAUTHEN,//检查机器
	CTPST_LOGINON,//正在登录
	CTPST_LOGINOFF,
	CTPST_CONFIRM,
	CTPST_ONGETID, //获取信息，包括资金，持仓以及可交易合约等信息
	CTPST_ONCMSRATE, //手续费
	CTPST_ONACCOUNT,
	CTPST_ONQRYORDER,
	CTPST_ONQRYTRADE,
	CTPST_ONQRYPOSITION,
	CTPST_ONQRYTICK,
	CTPST_BUSY, //正在处理中
	CTPST_RUNING, //交易中
	CTPST_PAUSE, //交易停止
	CTPST_FAILED, //故障
	CTPST_ENDF
};
class CCtpTick
{
public:
	CCtpTick()
	{
		memset(&m_stTick, 0, sizeof(SCtpTick));
	}
	CCtpTick(const CThostFtdcDepthMarketDataField& stField)
	{
		Init(stField);
	}
	CCtpTick& Init(const CThostFtdcDepthMarketDataField& stField)
	{
		m_stTick.InstrumentStatus = THOST_FTDC_IS_Continous; //默认标记为连续交易TICK
		memcpy(m_stTick.TradingDay, stField.TradingDay, sizeof(TThostFtdcDateType));
		memcpy(m_stTick.ExchangeID, stField.ExchangeID, sizeof(TThostFtdcExchangeIDType));
		m_stTick.LastPrice = stField.LastPrice;
		m_stTick.PreSettlementPrice = stField.PreSettlementPrice;
		m_stTick.PreClosePrice = stField.PreClosePrice;
		m_stTick.OpenPrice = stField.OpenPrice;
		m_stTick.HighestPrice = stField.HighestPrice;
		m_stTick.LowestPrice = stField.LowestPrice;
		m_stTick.Volume = stField.Volume;
		m_stTick.Turnover = stField.Turnover;
		m_stTick.OpenInterest = stField.OpenInterest;
		m_stTick.ClosePrice = stField.ClosePrice;
		m_stTick.SettlementPrice = stField.SettlementPrice;
		m_stTick.UpperLimitPrice = stField.UpperLimitPrice;
		m_stTick.LowerLimitPrice = stField.LowerLimitPrice;
		memcpy(m_stTick.UpdateTime, stField.UpdateTime, sizeof(TThostFtdcTimeType));
		m_stTick.UpdateMillisec = stField.UpdateMillisec;
		m_stTick.BidPrice1 = stField.BidPrice1;
		m_stTick.BidVolume1 = stField.BidVolume1;
		m_stTick.AskPrice1 = stField.AskPrice1;
		m_stTick.AskVolume1 = stField.AskVolume1;
		m_stTick.BidPrice2 = stField.BidPrice2;
		m_stTick.BidVolume2 = stField.BidVolume2;
		m_stTick.AskPrice2 = stField.AskPrice2;
		m_stTick.AskVolume2 = stField.AskVolume2;
		m_stTick.BidPrice3 = stField.BidPrice3;
		m_stTick.BidVolume3 = stField.BidVolume3;
		m_stTick.AskPrice3 = stField.AskPrice3;
		m_stTick.AskVolume3 = stField.AskVolume3;
		m_stTick.BidPrice4 = stField.BidPrice4;
		m_stTick.BidVolume4 = stField.BidVolume4;
		m_stTick.AskPrice4 = stField.AskPrice4;
		m_stTick.AskVolume4 = stField.AskVolume4;
		m_stTick.BidPrice5 = stField.BidPrice5;
		m_stTick.BidVolume5 = stField.BidVolume5;
		m_stTick.AskPrice5 = stField.AskPrice5;
		m_stTick.AskVolume5 = stField.AskVolume5;
		m_stTick.AveragePrice = stField.AveragePrice;
		memcpy(m_stTick.ActionDay, stField.ActionDay, sizeof(TThostFtdcDateType));
		memcpy(m_stTick.InstrumentID, stField.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
		m_ctNature.SetYMD(m_stTick.ActionDay).SetHMS(m_stTick.UpdateTime).SetMilSec(m_stTick.UpdateMillisec);
		m_ctTrading.SetYMD(m_stTick.TradingDay).SetHMS(m_stTick.UpdateTime).SetMilSec(m_stTick.UpdateMillisec);
		m_stTick.OiChange = 0.0;
		m_stTick.VolChange = 0.0;
		m_stTick.OiOpen = 0.0;
		m_stTick.OiClose = 0.0;
		return *this;
	}
	//计算额外的TICK信息//与上一个TICK的数据进行对比计算
	CCtpTick& CalcTickEx(const CCtpTick& ctpTickLast) 
	{
		m_stTick.OiChange = m_stTick.OpenInterest - ctpTickLast.m_stTick.OpenInterest;
		m_stTick.VolChange = abs(m_stTick.Volume - ctpTickLast.m_stTick.Volume);
		m_stTick.OiOpen = (m_stTick.OiChange + m_stTick.VolChange) * 0.5;
		m_stTick.OiClose = (m_stTick.OiChange - m_stTick.VolChange) * 0.5;
		return *this;
	}
	~CCtpTick(){}
	SCtpTick m_stTick;
	//TThostFtdcVolumeType	prcVolume; ////增量
	//SCtpLineK m_stLineK;
	CTimeCalc m_ctNature;
	CTimeCalc m_ctTrading;
	CCtpTick& operator = (const CCtpTick& ctpTick)
	{
		m_stTick = ctpTick.m_stTick;
		m_ctNature = ctpTick.m_ctNature;
		m_ctTrading = ctpTick.m_ctTrading;
		return *this;
	}
	//CCtpMinute& operator = (const CThostFtdcDepthMarketDataField& stField);
};
//---------------------------------------------------K线--------------------------------------------------------------------
enum eLineKType
{
	//LKT_TICK = 0,
	LKT_MINUTE = 0,
	LKT_HOUR,
	LKT_DAY,
	LKT_SETTLE, //结算价
	LKT_ENDF,
	LKT_NULL0,
	LKT_NULL1,
	LKT_NULL2
};
const char g_strLineKType[LKT_ENDF][16] = { "MINUTE", "HOUR", "DAY", "SETTLE"};
#define LINEK_ISLASTTICK(TYPE, A, B) \
(((LKT_MINUTE == TYPE)&&(A.m_ctTrading.m_nMinute != B.m_ctTrading.m_nMinute))\
|| ((LKT_HOUR == TYPE) && (A.m_ctTrading.m_nHour != B.m_ctTrading.m_nHour))\
|| ((LKT_DAY == TYPE) && (A.m_ctTrading.m_nDay != B.m_ctTrading.m_nDay))) //检测判断K线是否需要重新计数
//K线相关的数据定义，顾名思义，采用BUF管理是为了方便做循环逻辑，时间有限不作计算逻辑的封装
enum eLineKData
{
	LKD_PRCVWAP = 0,
	LKD_PRCTWAP,
	LKD_PRCVWAPSUM,
	LKD_PRCTWAPSUM,
	LKD_OPENPRICE,
	LKD_HIGHPRICE,
	LKD_LOWPRICE,
	LKD_CLOSEPRICE,
	LKD_INTEREST,
	LKD_VOLUME,
	LKD_PRCVOLUME,
	LKD_PRCVOLUMESUM,
	LKD_PRCCOUNT,
	LKD_ENDF
};

#define BSON_LINEK(PRE, X, Y) (boost::string_view(string(PRE).append(g_strLineKType[X]).append("-").append(g_strLineKData[Y]).c_str()))
#define BOOST_STR(STR) (boost::string_view(STR))
#define LINEK_TYPE(X) (boost::string_view(g_strLineKType[X]))

class CCtpLineK
{
public:
	CCtpLineK();
	//CCtpLineK(const CCtpTick& ctpTick);
	~CCtpLineK();
	SCtpLineK m_stLineK[LKT_ENDF];
	TThostFtdcInstrumentIDType	m_InstrumentID;///合约代码
	TThostFtdcExchangeIDType	m_ExchangeID;///交易所代码
	CTimeCalc m_ctNature;
	CTimeCalc m_ctTrading;
	CCtpLineK& Init(eLineKType eType, const SCtpTick& stTick, bool bZeroMode = false);
	CCtpLineK& SetID(const TThostFtdcInstrumentIDType& stID);
	CCtpLineK& operator = (const CCtpLineK& ctpLineK);
	bool Compare(const CCtpLineK& ctpLineK);
	//CCtpMinute& operator = (const CThostFtdcDepthMarketDataField& stField);
};
//---------------------------------------------------TARGET--------------------------------------------------------------------
struct SCtpTarget
{
	CTPTIME nYMD; //年月日
	CTPTIME nHMSL; //时分秒毫
	TThostFtdcVolumeType	Position;//目标数量
};
struct SCtpTaskRatio
{
	size_t nTgtAll; //需要执行的TARGET总数
	size_t nTgtDone; //已经执行了的TARGET数量
	size_t nTgtNotDo;//未执行的TARGET数量
	float fTgtRatioDone; //比率
	float fTgtRatioNotDo; //比率
	TThostFtdcVolumeType nVlmAll; //TARGET时间排序后合约需要交易的量差和
	TThostFtdcVolumeType nVlmDone; //已经执行了的量差和
	TThostFtdcVolumeType nVlmNotDo; //剩余需要执行的量差和
	float fVlmRatioDone;//比率
	float fVlmRatioNotDo;//比率
};

struct SCtpRiskCtrl //风控控制器，键值：合约
{
	atomic_bool bFollow; //跟踪品种交易信息
	atomic_bool bStopOpen; //停止开仓
	atomic_bool bStopTrading; //停止交易
};
struct SCtpRiskCnt //风控计数器, 键值：合约
{
	SCtpRiskCnt()
	{
		nOrderDaySum = 0; //累计挂单数量
		nCancelDaySum = 0; //累计撤单数量
		nOrderMinSum = 0; //累计分钟委托数量
		nOrderVolMax = 0; //单笔报单最大数量
		nLongPosMax = 0; //多头最大持仓
		nShortPosMax = 0; //空头最大持仓
	}
	atomic_int nOrderDaySum; //累计挂单数量
	atomic_int nCancelDaySum; //累计撤单数量
	atomic_int nOrderMinSum; //累计分钟委托数量
	atomic_int nOrderVolMax; //单笔报单最大数量
	atomic_int nLongPosMax; //多头最大持仓
	atomic_int nShortPosMax; //空头最大持仓
};
typedef SCtpRiskCnt SCtpRiskLimit;

struct SCtpTradeCnt //交易计数器, 键值：合约
{
	SCtpTradeCnt()
	{
		lnDayNeedDo = 0; //该交易日需要交易的手数
		lnTimeNeedDo = 0; //该时间点需要完成的手数/Num2Trade
		lnTimeDone = 0; //该时间点已完成的手数/Num2Trade
		lnCancelNum = 0; //累计撤单次数
		lnOrderNum = 0; //累计发单次数
		lnMaxVolume = 0; //最大下单量
		lnMaxPosition = 0; //最大持仓量
		lnOrderFreqMin = 0; //1分钟频率下单次数
	}
	atomic_int lnDayNeedDo; //该交易日需要交易的手数
	atomic_int lnTimeNeedDo; //该时间点需要完成的手数/Num2Trade
	atomic_int lnTimeDone; //该时间点已完成的手数/Num2Trade
	atomic_int lnCancelNum; //累计撤单次数
	atomic_int lnOrderNum; //累计发单次数
	atomic_int lnMaxVolume; //最大下单量
	atomic_int lnMaxPosition; //最大持仓量
	atomic_int lnOrderFreqMin; //1分钟频率下单次数
};
#define HTP_ISFAIL(RSP) ((RSP && RSP->ErrorID!=0 && RSP->ErrorMsg[0] > 0))
#define CH_TO_ULL(CHARP) (*(unsigned long long*)(CHARP))
#define DOUBLE_UPLIMIT (1.7976931348623157e+308)
//#define CC_TO_ULL(CHARP) (*(unsigned long long*)(char*)(CHARP))
#define CQ_SIZE_TICK (0x00003FFF)
#define CQ_SIZE_LINEKTICK (0x00001FFF)
#define CQ_SIZE_LINEK (0x00000FFF)
#define CQ_SIZE_TDTICK (0x00001FFF)
#define CQ_SIZE_TDH (0x00000FFF)
#define CQ_SIZE_TDL (0x000003FF)

#define FUNC_BIND(FUNC, BIND) (FUNC = BIND)
//---------------------------------------------------日志--------------------------------------------------------------------
#define HTP_APDMSG_SIZEX (0x00000000000000FF)
#define HTP_ONEMSG_SIZEX (0x0000000000000FFF)
#define HTP_MSG_SIZEY (0x00000000000000FF)
#define HTP_MSGUTF8_SIZEY (0x00000000000001FF)
#define HTP_CID unsigned long long
#define HTP_CMSG(STR, CID) (SHtpMsg({STR.c_str(),STR.size(), (HTP_CID)CID}))
#define HTP_PMSG(PBUF,PLEN,CID) (SHtpMsg({PBUF, (size_t)PLEN, (HTP_CID)CID}))
//#define HTP_MSG CHtpMsg
#define HTP_ENDL (0x00000000AAAAAAAA)
//CID RTFLAG : 0-3; //0失败，1成功
#define HTP_CID_RTFLAG(CID) (CID&0xF000000000000000)
#define HTP_CID_NULL 0x1000000000000000
#define HTP_CID_OK 0x2000000000000000
#define HTP_CID_FAIL 0x3000000000000000
#define HTP_CID_WARN 0x4000000000000000
//CID DECODE : 4-7; //四种编码: 0ASCII, 1UTF8, 2UNICODE, 3GB2312
#define HTP_CID_DECODE(CID) (CID&0x0F00000000000000)
#define HTP_CID_ISASCII 0x0000000000000000
#define HTP_CID_ISUTF8 0x0100000000000000
//CID COLOR : 8-11; //四种颜色: 0白，1红，2绿，3黄

//CID LOG : 12-15; //0001：控制台输出，0010：日志输出，0100：MDB输出, 1000：网络打印
#define HTP_CID_USE_LOGALL 0x000F000000000000
#define HTP_CID_USE_LOGCOUT 0x0001000000000000
#define HTP_CID_USE_LOGTXT 0x0002000000000000
#define HTP_CID_USE_LOGMDB 0x0004000000000000
#define HTP_CID_USE_LOGNET 0x0008000000000000
#define HTP_CID_LOG(CID) (HTP_CID)(CID&0x000F000000000000)
//控制字
#define HTP_CID_CTRL(CID) (HTP_CID)(CID&0xFFFF000000000000)
//CID NULL : 16-47
//CID ERRORID : 48-63; //自定义错误ID号
#define HTP_CID_ERRORID(CID) (HTP_CID)(CID&0x000000000000FFFF)

class CHtpMsg
{
public:
	CHtpMsg()
	{
		m_strMsg.clear(); m_CID = HTP_CID_NULL;
	}
	CHtpMsg(HTP_CID CID)
	{
		m_strMsg.clear(); m_CID = CID;
	}
	CHtpMsg(const string& strMsg, HTP_CID CID)
	{
		m_strMsg = strMsg; m_CID = CID;
	}
	CHtpMsg(const char* pMsg, HTP_CID CID)
	{
		m_strMsg.clear(); m_strMsg.append(pMsg); m_CID = CID;
	}
	HTP_CID m_CID;
	string m_strMsg;
};
struct SHtpMsg
{
	const char* MSG;
	unsigned long long SIZE;
	HTP_CID CID;
};
inline SHtpMsg HTP_MSG(const string& strMsg, HTP_CID CID) { return HTP_CMSG(strMsg, CID); };
//---------------------------------------------------配置--------------------------------------------------------------------
#ifndef HTP_UserConfig
#define HTP_UserConfig
enum eUserConfig
{
	HTP_UC_NULL = 0,
	HTP_UC_NAME,
	HTP_UC_MDBURL,
	HTP_UC_CMDLIST,
	HTP_UC_TD, HTP_UC_MD, HTP_UC_USERID, HTP_UC_BROKERID,
	HTP_UC_PASSWARD, HTP_UC_APPID, HTP_UC_AUTHCODE, HTP_UC_PRODUCTINFO,
	HTP_UC_ENDF
};
extern const char* g_strUserConfig[HTP_UC_ENDF];
class CUserConfig //不加锁，除了刚启动时读写，运行时刻均为读
{
	CThostFtdcUserLogoutField m_stLogout;
	CThostFtdcReqUserLoginField m_stLogin;
	CThostFtdcReqAuthenticateField m_stAuthenticate;
	CThostFtdcSettlementInfoConfirmField m_stConfirm;
	CThostFtdcQryInvestorPositionField m_stPosition;
	CThostFtdcQryTradingAccountField m_stAccount;
	CThostFtdcQryOrderField m_stOrder;
	CThostFtdcQryTradeField m_stTrade;
	CThostFtdcQryInstrumentCommissionRateField m_stCommission;
	string m_strFrontMD;
	string m_strFrontTD;
	string m_strMDB;
	vector<string> m_vecCmd;
	string m_strExePath;
	string m_strExeFolder;
	SYSTEMTIME m_sysTime;
	string m_strDate;
	vector<string> m_vecEmail;
public:
	CUserConfig();
	~CUserConfig();
	void Clear();
	string Trim(const string& strValue);
	CHtpMsg Load(const string& strFileName, const string& strUcName);
	void SetMDB(const string& strConfig);
	void PushCmd(const string& strConfig);
	void SetFrontMD(const string& strConfig);
	void SetFrontTD(const string& strConfig);
	void SetUserID(const string& strConfig);
	void SetBrokerID(const string& strConfig);
	void SetPassword(const string& strConfig);
	void SetAppID(const string& strConfig);
	void SetAuthCode(const string& strConfig);
	void SetUserProductInfo(const string& strConfig);

	string GetExePath()const { return m_strExePath; }
	string GetExeFolder()const { return m_strExeFolder; }
	string GetTitle() const
	{
		return string(m_strExePath).append("    ").append(m_stLogin.UserID).append("-").append(m_strFrontTD);
	}
	vector<string> GetCmd() const { return m_vecCmd; }
	vector<string> GetEmail() const { return m_vecEmail; }
	string GetMDB() const { return this->m_strMDB; }
	string GetFrontMD() const { return this->m_strFrontMD; }
	string GetFrontTD() const { return this->m_strFrontTD; }
	CThostFtdcUserLogoutField GetLogout() const { return this->m_stLogout; }
	CThostFtdcReqUserLoginField GetLogin() const { return this->m_stLogin; }
	CThostFtdcReqAuthenticateField GetAuthenticate() const { return this->m_stAuthenticate; }
	CThostFtdcSettlementInfoConfirmField GetConfirm() const { return this->m_stConfirm; }
	CThostFtdcQryInvestorPositionField GetPosition() const { return this->m_stPosition; }
	CThostFtdcQryTradingAccountField GetAccount() const { return this->m_stAccount; }
	CThostFtdcQryOrderField GetOrder() const { return this->m_stOrder; }
	CThostFtdcQryTradeField GetTrade() const { return this->m_stTrade; }
	CThostFtdcQryInstrumentCommissionRateField GetCommission() const { return this->m_stCommission; }
};
#endif // !HTP_UserConfig
//---------------------------------------------------时间节点记录/延迟计算--------------------------------------------------------------------
enum eHtpOrderStatus
{
	HTS_INIT = 0,
	//HTS_MD_TICK, //tick的时间
	//HTS_GET_TARGET,//获取到target时间
	HTS_ORD_NEW, //创建订单时间
	HTS_ORD_DOING, //订单执行时间
	HTS_ORD_DONE, //下单接口调用完时间
	HTS_ORDED, //已委托时间
	HTS_TRADED_PART, //部分成交时间
	HTS_CCL_DOING, //撤单时间
	HTS_CCL_DONE, //撤单成功时间
	HTS_TRADED, //已成交时间
	HTS_CANCEL, //撤单成功时间
	HTS_ENDF
};
extern const char* g_strHTS[HTS_ENDF];
struct SHtpTimeSheet
{
	TThostFtdcInstrumentIDType InstrumentID;
	TThostFtdcOrderRefType OrderRef;
	ULONG64 TBL[HTS_ENDF]; //时间缀记录
};
class CHtpOrderManage
{
	mutex m_lock;
	SHtpTimeSheet m_stTimeSheet;
	eHtpOrderStatus m_eStatus; //时间记录状态
public:

	CHtpOrderManage(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef) //默认构造
	{
		unique_lock<mutex> lock(m_lock);
		memset(&m_stTimeSheet, 0, sizeof(SHtpTimeSheet));
		memcpy(m_stTimeSheet.InstrumentID, InstrumentID, sizeof(TThostFtdcInstrumentIDType));
		memcpy(m_stTimeSheet.OrderRef, OrderRef, sizeof(TThostFtdcOrderRefType));
		m_eStatus = HTS_INIT;
		m_stTimeSheet.TBL[HTS_INIT] = GetTickCount64();
	}
	CHtpOrderManage(CHtpOrderManage& cTimeSheet)
	{
		*this = cTimeSheet;
	}
	~CHtpOrderManage(){}
	CHtpOrderManage& operator=(const CHtpOrderManage& cTimeSheet)
	{
		unique_lock<mutex> lock(m_lock);
		memcpy(&m_stTimeSheet, &(cTimeSheet.m_stTimeSheet), sizeof(SHtpTimeSheet));
		m_eStatus = cTimeSheet.m_eStatus;
	}
	bool Set(const eHtpOrderStatus& eStatus) //设置时间缀，有流程顺序限制
	{
		unique_lock<mutex> lock(m_lock);
		if (m_stTimeSheet.TBL[eStatus] == 0)
		{
			m_stTimeSheet.TBL[eStatus] = GetTickCount64();
		}
		if (eStatus > m_eStatus)
		{
			m_eStatus = eStatus;
			return true;
		}
		else
		{
			return false;
		}
	}
	ULONG64 GetTickChange(const eHtpOrderStatus& eStatus) //获取计数改变延迟，用于交易订单和委托超时判断
	{
		unique_lock<mutex> lock(m_lock);
		if (m_stTimeSheet.TBL[eStatus] != 0)
		{
			return (GetTickCount64() - m_stTimeSheet.TBL[eStatus]);
		}
		return 0LL;
	}
	ULONG64  GetDelay(const eHtpOrderStatus& eStatus) //获取延迟，获取交易流程间的延迟
	{
		unique_lock<mutex> lock(m_lock);
		if (m_stTimeSheet.TBL[eStatus] == 0) { return 0LL; }
		if (HTS_ENDF > eStatus && HTS_INIT < eStatus)
		{
			switch (eStatus)
			{
			case HTS_TRADED_PART:
			case HTS_TRADED:
			case HTS_CCL_DOING:
				if (m_stTimeSheet.TBL[HTS_ORD_DONE] == 0) { return 0LL; }
				return (m_stTimeSheet.TBL[eStatus] - m_stTimeSheet.TBL[eHtpOrderStatus(HTS_ORD_DONE)]);
			case HTS_CANCEL:
				if (m_stTimeSheet.TBL[HTS_CCL_DONE] == 0) { return 0LL; }
				return (m_stTimeSheet.TBL[eStatus] - m_stTimeSheet.TBL[eHtpOrderStatus(HTS_CCL_DONE)]);
			default:
				if (m_stTimeSheet.TBL[eHtpOrderStatus(eStatus - 1)] == 0) { return 0LL; }
				return (m_stTimeSheet.TBL[eStatus] - m_stTimeSheet.TBL[eHtpOrderStatus(eStatus - 1)]);
			}
		}
		return 0LL;
	}
	eHtpOrderStatus GetStatus() 
	{ 
		unique_lock<mutex> lock(m_lock); 
		return m_eStatus; 
	} //获取标记状态
};

class CHtpManage
{
	map<CFastKey, map<CFastKey, shared_ptr<CHtpOrderManage>, CFastKey>, CFastKey> m_mapOrder; //键值：合约-> 委托引用
	mutex m_lock;
public:
	CHtpManage();
	~CHtpManage();
	//设置时间缀，有流程顺序限制，不是由HTP通过TARGET触发的订单不会被加载进订单管理
	//也就是不会对通过第三方或手动下单的订单（简称非TARGET单）进行超时撤单管理与持仓计算
	//但非TARGET单成交后依然会刷新到持仓，撤单依然会自动重开
	bool SetStatus(eHtpOrderStatus eStatus, const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef);
	//获取延迟
	shared_ptr<const SHtpTimeSheet>  GetDelay(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef);
	//获取标记状态
	eHtpOrderStatus GetStatus(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef);
	//获取延时或距离相关状态的时间
	ULONG64 GetTickChange(const eHtpOrderStatus& eStatus, const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef);
	//订单管理维护删除
	void Delete(const TThostFtdcInstrumentIDType& InstrumentID, const TThostFtdcOrderRefType& OrderRef);
	//获取挂单,ullTimeOutMS=超时时间
	size_t GetOrderGD(const CFastKey& InstrumentID, vector<CFastKey>& vecGD, ULONG64 ullTimeOutMS = 0);
	////获取超时的挂单,ullTimeOutMS=超时时间，10S调用一次检查
	//size_t GetTimeOutGD(ULONG64 ullTimeOutMS, map<CFastKey, map<CFastKey, shared_ptr<CHtpOrderManage>, CFastKey>, CFastKey>& mapGD);
	//是否有正在运行中的订单，用于判断是否能够订单生成
	int HaveDoing(const TThostFtdcInstrumentIDType& InstrumentID);
};
//挂单数据定义
struct SHtpGD
{
	TThostFtdcVolumeType	VolOBTD; //今买开
	TThostFtdcVolumeType	VolCBTD; //买平今
	TThostFtdcVolumeType	VolCBYD; //买平昨
	TThostFtdcVolumeType  VolCBUD; //未定义的买平
	TThostFtdcVolumeType	VolOSTD; //今卖开
	TThostFtdcVolumeType	VolCSTD; //卖平今
	TThostFtdcVolumeType	VolCSYD; //卖平昨
	TThostFtdcVolumeType  VolCSUD; //未定义的卖平
};
//盈亏金额信息，合约级汇总
struct SHtpPNL
{
	//------------------------------------------------计算产生的数据结果------------------------------------------------
	///今日总开仓盈亏 += (最新价-开仓价)*多空方向*成交量*交易乘数
	TThostFtdcMoneyType	m_dbProfitOpenTD; //今日开仓交易产生的资金盈亏
		///今日总开仓盈亏 += (最新价-开仓价)*多空方向*成交量*交易乘数
	TThostFtdcMoneyType	m_dbProfitCloseTD; //今日平仓交易产生的资金盈亏
	///昨日未平仓合约盈亏 = (最新价-昨结算价)*多空方向*昨剩余量*交易乘数
	TThostFtdcMoneyType	m_dbProfitPositionYD; //昨日持仓产生的盈亏
	//------------------------------------------------计算过程中的数据------------------------------------------------
	TThostFtdcMoneyType m_dbOpenSumK;  //开仓价量和系数，用于与最新价进行计算，成交回报时刻进行增量计算
	TThostFtdcMoneyType m_dbPVSumK;  //开仓量和系数，用于与最新价计算成最新价量和，定时进行全量计算
	TThostFtdcMoneyType m_dbCloseSumK;  //平仓价量和系数，用于与最新价进行计算，成交回报时刻进行增量计算
	TThostFtdcMoneyType m_dbCVSumK;  //平仓量和系数，用于与最新价计算成最新价量和，定时进行全量计算
	TThostFtdcMoneyType m_dbLYDVolK; //昨日净持仓量和系数，多持仓, 用于与最新价计算出昨日持仓盈亏
	TThostFtdcMoneyType m_dbSYDVolK; //昨日净持仓量和系数，空持仓, 用于与最新价计算出昨日持仓盈亏
};

struct SHtpUpdPos
{
	TThostFtdcDirectionType	Direction; ///买卖方向
	TThostFtdcOffsetFlagType	OffsetFlag; ///开平标志
	TThostFtdcVolumeType	Volume; //成交的数量
	TThostFtdcPriceType	Price; //成交价格
	TThostFtdcExchangeIDType	ExchangeID; ///交易所代码
	TThostFtdcInstrumentIDType	InstrumentID; ///合约代码
};
//---------------------------------------------------------订单委托--------------------------------------------------------------------------
class CThostFtdcInputOrderFieldEx
{
	shared_ptr<CThostFtdcInputOrderField> m_ptrData;
	string strTargetID;
	string strFromRef;
public:
	CThostFtdcInputOrderFieldEx() 
	{
		m_ptrData = NULL;
		strTargetID.clear();
		strFromRef.clear();
	}
	CThostFtdcInputOrderFieldEx(shared_ptr<CThostFtdcInputOrderField> ptrOrder, const string& strID, const string& strRef)
	{
		m_ptrData = ptrOrder;
		strTargetID = strID;
		strFromRef = strRef;
	}
	shared_ptr<CThostFtdcInputOrderField> data() const
	{
		return m_ptrData;
	}
	const string& TargetID() const
	{
		return strTargetID;
	}
	const string& FromRef() const
	{
		return strFromRef;
	}
};