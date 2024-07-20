#include "CMdThread.h"
#include "CLogThread.h"
#include "CTimeCalc.h"

CMdThread::CMdThread()
    : m_cqTick(CQ_SIZE_TICK)
    , m_cqLineKTick(CQ_SIZE_LINEKTICK)
    , m_cqLineK(CQ_SIZE_LINEK)
    , m_smTick(5)
    , m_smLineK(5)
{
    m_mapInstrument.clear();
    m_mapLineK.clear();
    m_pApi = CThostFtdcMdApi::CreateFtdcMdApi(); //
    m_pApi->RegisterSpi(this);
    m_bThRunning = true;
    m_FuncTDTick = NULL;
    m_FuncTDLineKGet = NULL;
    m_FuncTDLineKSet = NULL;
    m_pthTaskTick = NULL;
    m_pthTaskLineK = NULL;
    for (int i = 0; i < CTPST_ENDF; i++)
    {
        m_bStatus[CTPST_ENDF] = false;
    }
    m_eStatus = CTPST_NULL;
    m_MDB = NULL;
    m_UserCfg = NULL;
    m_cRisk = NULL;
}
CMdThread::~CMdThread()
{
    m_bThRunning = false; //设置停止运行标志
    m_smTick.Release();
    m_smLineK.Release();

    if (m_pthTaskTick)
    {
        m_pthTaskTick->join(); //等待线程退出
        delete m_pthTaskTick; //释放线程空间
    }
    if (m_pthTaskLineK)
    {
        m_pthTaskLineK->join(); //等待线程退出
        delete m_pthTaskLineK; //释放线程空间
    }

    if (m_pApi)
    {
        m_pApi->Release();
        //m_pApi->Join(); //等待行情线程退出
    }
}

//shared_ptr<const CCtpTick> CMdThread::GetFastMarket(const TThostFtdcInstrumentIDType& InstrumentID)
//{
//    unique_lock<mutex> lock(m_mapLock);
//    return m_mapTick[InstrumentID]; //引用+1
//}

//分钟级行情处理线程
void CMdThread::ThreadTick()
{
    HTP_LOG << "CMdThread::ThreadTick" << HTP_ENDL;
    
    while (m_bThRunning)
    {
        m_smTick.Wait(100); //100MS-BULK写入一次
        m_MDB->OnTick(m_cqTick); //TICK行情写入MONGO
    }
    HTP_LOG << "CMdThread::ThreadTick END" << HTP_ENDL;
}
//K线处理
void CMdThread::ThreadLineK()
{
    HTP_LOG << "CMdThread::ThreadLineK" << HTP_ENDL;
    shared_ptr<const CCtpTick> ctpTick = NULL;
    ULONG64 llnTime = GetTickCount64();
    while (m_bThRunning)
    {
        m_smLineK.Wait(300); //BULK处理延迟
        while (m_cqLineKTick.Front(ctpTick))
        {
            m_cqLineKTick.Pop();
            if (NULL == ctpTick) { continue; }
            CalcLineK(*ctpTick); //计算K线
        }
        m_MDB->OnLineK(m_cqLineK); //行情K线写入MONGO
        if ((GetTickCount64() - llnTime) > 60000) //大于1分钟
        {
            llnTime = GetTickCount64();
            LineKWay(); //处理过期行情
        }
    }
    HTP_LOG << "CMdThread::ThreadLineK END" << HTP_ENDL;
}

int CMdThread::Start(shared_ptr<const CUserConfig> Config, const map<CFastKey, CThostFtdcInstrumentField, CFastKey>& mapInstrument)
{
    HTP_LOG << "CMdThread::Start" << HTP_ENDL;
    if (!Config) { return -1; }
    if (NULL == m_pApi) { return -1; }  //API注册失败或异常
    m_nRequestID = (int)HTP_SYS.GetTime(CTP_TIME_NET).GetHMSL();
    m_UserCfg = Config;
    m_mapInstrument = mapInstrument;
    unique_lock<mutex> lock(m_pthLock);
    if (!m_pthTaskTick)
        m_pthTaskTick = new thread(&CMdThread::ThreadTick, this);
    if (!m_pthTaskLineK)
        m_pthTaskLineK = new thread(&CMdThread::ThreadLineK, this);
    //1. 初始化
    m_pApi->RegisterFront((char*)m_UserCfg->GetFrontMD().c_str());
    m_pApi->Init();
    m_eStatus = eCtpStatus::CTPST_INIT;
    return 0;
}
int CMdThread::Stop()
{
    HTP_LOG << "CMdThread::Stop" << HTP_ENDL;
    if (NULL == m_pApi) { return -1; }  //API注册失败或异常
    if (!m_bStatus[CTPST_LOGINON]) { return -1; }
    CThostFtdcUserLogoutField req = m_UserCfg->GetLogout();
    int iResult = m_pApi->ReqUserLogout(&req, ++m_nRequestID);
    if (iResult != 0) {
        return iResult; //失败
    }
    return 0;
}
///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void CMdThread::OnFrontConnected() 
{
    m_bStatus[CTPST_FRONTON] = true;
    HTP_LOG << "CMdThread::OnFrontConnected" << HTP_ENDL;
    //2. 登录
    CThostFtdcReqUserLoginField req = m_UserCfg->GetLogin();
    int iResult = m_pApi->ReqUserLogin(&req, ++m_nRequestID);
    m_cRisk->SysCnt(RSC_LOGIN)++;
    if (iResult != 0) 
    {
        HTP_LOG << "CMdThread::ReqUserLogin: Failed" << HTP_ENDL;
    }
};

///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
///@param nReason 错误原因
///        0x1001 网络读失败
///        0x1002 网络写失败
///        0x2001 接收心跳超时
///        0x2002 发送心跳失败
///        0x2003 收到错误报文
void CMdThread::OnFrontDisconnected(int nReason) 
{
    m_bStatus[CTPST_FRONTON] = false;
    m_bStatus[CTPST_LOGINON] = false;
    HTP_LOG << "CMdThread::OnFrontDisconnected" <<nReason << HTP_ENDL;
};

///心跳超时警告。当长时间未收到报文时，该方法被调用。
///@param nTimeLapse 距离上次接收报文的时间
void CMdThread::OnHeartBeatWarning(int nTimeLapse) 
{
    m_bStatus[CTPST_FRONTON] = false;
    m_bStatus[CTPST_LOGINON] = false;
    HTP_LOG << "CMdThread::OnHeartBeatWarning" << nTimeLapse << HTP_ENDL;
};

///登录请求响应
void CMdThread::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
    HTP_LOG << "CMdThread::OnRspUserLogin" << HTP_ENDL;
   
    if (HTP_ISFAIL(pRspInfo)) //报错
    {
        HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
    }
    if (!pRspUserLogin) { return; }
    m_eStatus = eCtpStatus::CTPST_LOGINON;
    m_bStatus[CTPST_LOGINON] = true;
    m_pthLock.lock();
    m_stLogin = *pRspUserLogin;
    m_pthLock.unlock();

    //strcpy_s(m_stTradingDay, m_stLogin.TradingDay); //获取交易日
    int nCount = 0; //行情订阅计数
    char* ppInstrumentID[INT_MAX_MKTLOOK+1] = {0};
    for (auto it = m_mapInstrument.begin(); it != m_mapInstrument.end(); it++)
    {
        //if (it->second.ProductClass != THOST_FTDC_PC_Futures) { continue; } //规定为期货类型
        ppInstrumentID[INT_MAX_MKTLOOK&nCount] = it->second.InstrumentID;
        nCount++;
    }
    nCount = (INT_MAX_MKTLOOK > nCount) ? nCount : INT_MAX_MKTLOOK;
    int iResult = m_pApi->SubscribeMarketData(ppInstrumentID, nCount); //订阅行情
    if (iResult != 0) 
    {
        HTP_LOG << "CMdThread::SubscribeMarketData: Failed" << HTP_ENDL;
        return; //失败
    }
};

///登出请求响应
void CMdThread::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
    HTP_LOG << "CMdThread::OnRspUserLogout" << HTP_ENDL;
    if (HTP_ISFAIL(pRspInfo)) //报错
    {
        HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
    }
    if (!pUserLogout) { return; }
    m_bStatus[CTPST_LOGINON] = false;
    //m_pApi->Release(); //释放
};

///错误应答
void CMdThread::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
    HTP_LOG << "CMdThread::OnRspError" << HTP_ENDL;
    if (HTP_ISFAIL(pRspInfo)) //报错
    {
        HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
    }
};
//
///订阅行情应答
void CMdThread::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
{
    //HTP_LOG << "CMdThread::OnRspSubMarketData" << HTP_ENDL;
    if (HTP_ISFAIL(pRspInfo)) //报错
    {
        HTP_LOG << HTP_MSG(pRspInfo->ErrorMsg, pRspInfo->ErrorID); return;
    }
    if (!pSpecificInstrument) { return; }
    m_pthLock.lock();
    if ((bIsLast) && m_eStatus != eCtpStatus::CTPST_RUNING)
    {
        m_eStatus = eCtpStatus::CTPST_RUNING;
        m_bStatus[CTPST_RUNING] = true;
        HTP_LOG << "--------------------------------------------HTP MD RUNNING--------------------------------------------" << HTP_ENDL;
    }
    m_pthLock.unlock();
}
//
/////取消订阅行情应答
//void CMdThread::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {};
//
/////订阅询价应答
//void CMdThread::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {};
//
/////取消订阅询价应答
//void CMdThread::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {};
//
///深度行情通知
void CMdThread::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) 
{
    //-------------------------------------------------------TICK处理-------------------------------------------------------
    auto ctpTick = make_shared<CCtpTick>(*pDepthMarketData);
    memcpy(ctpTick->m_stTick.ExchangeID, m_mapInstrument[ctpTick->m_stTick.InstrumentID].ExchangeID, sizeof(TThostFtdcExchangeIDType));
    //时间修正
    HTP_SYS.SetTime(ctpTick);
    if (!HTP_SYS.TimeStand(ctpTick))//时间修正 //引用+1-1，过滤过期TICK
    {
        ctpTick->m_ctTrading.Clear();
        m_cqTick.Push(ctpTick); //未被休整的TICK将也被存储
        HTP_LOG << "TM!";  
        return; //时间休整中
    }
    //初始化，过滤量差异常
    m_mapLock.lock();
    auto& itTick = m_mapTick[pDepthMarketData->InstrumentID];
    if ((itTick)&&(itTick->m_stTick.Volume > pDepthMarketData->Volume)) //检测量差
    {
        ctpTick->m_ctTrading.Clear();
        m_cqTick.Push(ctpTick); //未被休整的TICK将也被存储
        m_mapLock.unlock();
        HTP_LOG << "TV!" << itTick->m_stTick.InstrumentID; //量差异常
        return;
    }
    if (itTick) { ctpTick->CalcTickEx(*itTick); } //计算额外TICK信息，如果有前一个TICK的话
    itTick = ctpTick; //更新引用
    m_mapLock.unlock();
    if (m_cRisk->GetEnable(REN_TDRADE)) //行情使能
    {
        m_FuncTDTick(ctpTick); //交易推送，线程采用回调的方式以避免可能的重复引用
    }
    //if (m_cRisk->GetEnable(REN_MARKET)) //行情使能
    //{
        m_cqTick.Push(ctpTick); //引用+1, 队列长度外被替换-1
        m_cqLineKTick.Push(ctpTick); //用于计算行情K线
    //}
};
//集中竞价方法，规定的时间推送集合竞价订单
void CMdThread::LineKWay() //定时1分钟刷一次，超过10分钟的TICK算作超时，需要将LINEK的数据PUSH到MDB
{
    if (false == m_cRisk->GetEnable(REN_MARKET)) //行情使能
    {
        return;
    }
    CTimeCalc ctpTime = HTP_SYS.GetTime(CTP_TIME_NET);
    CTPTIME nMinuteX = 0;
    for (const auto& itID : m_mapInstrument) //开始补TICK
    {
        m_mapLock.lock();
        auto& itFind = m_mapTick.find(itID.first); //获取行情TICK
        if ((itFind == m_mapTick.end()) 
            || (!itFind->second)
            || (THOST_FTDC_IS_Closed==itFind->second->m_stTick.InstrumentStatus))
        {
            m_mapLock.unlock(); continue; //没有行情则无需补TICK
        } 
        shared_ptr<CCtpTick> ctpTick = make_shared<CCtpTick>(*itFind->second); //复制当前行情TICK
        m_mapLock.unlock();
        if (!ctpTick) { continue; }
        nMinuteX = abs(ctpTime.m_nMinute - ctpTick->m_ctNature.m_nMinute);
        if (nMinuteX < 10 || nMinuteX > 50) { continue; }//十分钟内的不过期不处理
        m_mapLock.lock();
        itFind->second->m_stTick.InstrumentStatus = THOST_FTDC_IS_Closed; //标记收盘
        m_mapLock.unlock();
        //超过10分钟的TICK算作超时，需要将LINEK的数据PUSH到MDB，补TICK
        ctpTick->m_ctNature.GetNextMinute(); //此处的篡改已充分考虑到行情已被使用
        ctpTick->m_ctNature.GetNextMinute();
        ctpTick->m_ctNature.GetNextMinute();
        ctpTick->m_ctTrading.SetHMS(ctpTick->m_ctNature.GetHMS()); //交易日期不进行改变
        HTP_LOG << string("K+").append(ctpTick->m_stTick.InstrumentID).c_str(); //补TICK日志
        m_cqLineKTick.Push(ctpTick); //PUSH到MDB用于计算行情K线
    }
}
//计算行情K线
int CMdThread::CalcLineK(const CCtpTick& ctpTick) //TICK驱动
{
    //unique_lock<mutex> lock(m_lockLineK);
    if (ctpTick.m_stTick.OpenInterest < 1) { return -1; }
    eLineKType eLoopIdx = eLineKType(0); //分钟开始
    auto itFind = m_mapLineK.find(ctpTick.m_stTick.InstrumentID);
    CCtpLineK& refctpLineK = m_mapLineK[ctpTick.m_stTick.InstrumentID]; //只在此局部使用则可以不加锁
    memcpy(refctpLineK.m_InstrumentID, ctpTick.m_stTick.InstrumentID, sizeof(TThostFtdcInstrumentIDType));
    memcpy(refctpLineK.m_ExchangeID, ctpTick.m_stTick.ExchangeID, sizeof(TThostFtdcExchangeIDType));
    //K线内存数据校验比对，主要目的是为了防止K线数据被篡改
    //因为K线计算采用增量计算的方式，需要使用上一步的计算结果
    //所以一定要确保计算过程中不能被莫名的篡改导致出错，提升程序的健壮性
    if (itFind != m_mapLineK.end()) //已经初始化则不进行数据同步，但要进行内存校验
    {
        if (m_FuncTDLineKGet) //内存校验，不一致也需要同步
        {
            CCtpLineK mapLineKBack;
            m_FuncTDLineKGet(mapLineKBack.SetID(ctpTick.m_stTick.InstrumentID));
            if (!refctpLineK.Compare(mapLineKBack)) //内存校验不一致
            {
                HTP_LOG << string("KCMP!").append(ctpTick.m_stTick.InstrumentID).c_str();
                goto LINEK_SYNDB; //不一致也需要同步
            }
        }
        goto LINEK_LOOP; //直接进入K线计算循环
    }
LINEK_SYNDB:
    if (0 < m_MDB->LoadLineK(refctpLineK))//同步MDB数据到内存
    {
        m_cRisk->SysCnt(RSC_SYN_LINEK)++;
        HTP_LOG << "+" << refctpLineK.m_InstrumentID;
    }
    //refctpLineK.Init(eLineKType(0), ctpTick.m_stTick); //LKT_MINUTE 必须初始化
    for (eLineKType i = eLineKType(0); i < LKT_ENDF; i = eLineKType(i + 1))
    {
        refctpLineK.m_stLineK[i].bIsLastTick = LINEK_ISLASTTICK(i, refctpLineK, ctpTick);
        if (refctpLineK.m_stLineK[i].bIsLastTick)//不在同一分钟或者同一小时不进行累加，否则使用历史值
        {
            refctpLineK.Init(i, ctpTick.m_stTick, true); //计算数据初始化
        }
    }
    eLoopIdx = LKT_ENDF; goto LINEK_END;
    //K线计算循环，复用同一种逻辑，可将计算逻辑封装并通过继承的方式实现计算差异，由于时间有限，不做逻辑分隔与封装
LINEK_LOOP:
    SCtpLineK& refLineK = refctpLineK.m_stLineK[eLoopIdx];
    if ((refLineK.Volume > ctpTick.m_stTick.Volume) && (ctpTick.m_stTick.Volume > 0))
    {
        m_cRisk->SysCnt(RSC_VOLX)++;
        HTP_LOG << "KV!" << refctpLineK.m_InstrumentID; //量差异常
        return -1;
    }
    if (CTP_PRICE_ISOUT(ctpTick.m_stTick.LastPrice))
    {
        HTP_LOG << "KP!" << refctpLineK.m_InstrumentID; //异常
        return -1;
    }
    TThostFtdcPriceType	AskPrice1 = CTP_PRICE_ISOUT(ctpTick.m_stTick.AskPrice1) ? ctpTick.m_stTick.LastPrice : ctpTick.m_stTick.AskPrice1;
    TThostFtdcPriceType	BidPrice1 = CTP_PRICE_ISOUT(ctpTick.m_stTick.BidPrice1) ? ctpTick.m_stTick.LastPrice : ctpTick.m_stTick.BidPrice1;
    if ((LKT_SETTLE == eLoopIdx) && (CTP_EXID_ISCFFEX(ctpTick.m_stTick.ExchangeID))) //中金所结算价 TXXX:14:15-15:15
    {
        if (((ctpTick.m_stTick.InstrumentID[0] == 'T') && ((ctpTick.m_ctNature.GetHM() > 1414) && (ctpTick.m_ctNature.GetHM() < 1516)))
            || ((ctpTick.m_stTick.InstrumentID[0] != 'T') && ((ctpTick.m_ctNature.GetHM() > 1359) && (ctpTick.m_ctNature.GetHM() < 1501))))
        {

        }
        else //中金所不在时间范围不计算结算价，其他交易所全天=结算价
        {
            refctpLineK.Init(eLoopIdx, ctpTick.m_stTick, true);
            goto LINEK_END;
        }
    }
    //-------------------------------------------------------计算K线-------------------------------------------------------
    refctpLineK.m_stLineK[eLoopIdx].bIsLastTick = LINEK_ISLASTTICK(eLoopIdx, refctpLineK, ctpTick);
    if (refctpLineK.m_stLineK[eLoopIdx].bIsLastTick) //TICK切换
    {
        if (LKT_MINUTE == eLoopIdx)//计算完成，分钟改变触发保存
        {
            m_cqLineK.Push(refctpLineK);
            m_cRisk->Cnt(ctpTick.m_stTick.InstrumentID).nOrderMinSum = 0; //分钟计数清零
        }
        else if (LKT_DAY == eLoopIdx) //交易日变更，新的一天则全部初始化
        {
            for (eLineKType i = eLineKType(0); i < LKT_ENDF; i = eLineKType(i + 1))
            {
                refctpLineK.Init(i, ctpTick.m_stTick, true); //相关计数重置为0
                refctpLineK.m_stLineK[eLoopIdx].bIsLastTick = true;
            }
            eLoopIdx = LKT_ENDF; goto LINEK_END;
        }
        else{}
        CTPTIME nMinuteX = abs(refctpLineK.m_ctNature.m_nMinute - ctpTick.m_ctNature.m_nMinute);
        CTPTIME nHourX = abs(refctpLineK.m_ctNature.m_nHour - ctpTick.m_ctNature.m_nHour);
        //TICK数量较少的边界TICK或者前后不超过一分钟的TICK将会被初始化，否则重置为0
        //由于盘中网络波动导致K线跨越超1分钟的K线数据将会被丢掉
        if ((nHourX == 0 || nHourX == 1 || nHourX == 23) && (nMinuteX == 1 || nMinuteX == 59))
        {
            //分钟差距为正常一分钟会被延续增量，否则初始化为0
            //原因是不需要把分钟外的数据累加到某分钟内造成K线的波动，丢掉的TICK不在进行K线增量计算
            refctpLineK.Init(eLoopIdx, ctpTick.m_stTick, false); goto LINEK_END; //初始化相关计数
        }
        else
        {
            refctpLineK.Init(eLoopIdx, ctpTick.m_stTick, true); goto LINEK_END; //相关计数重置为0
        }
    }
    refLineK.prcVolume = ctpTick.m_stTick.Volume - refLineK.Volume; //量差
    refLineK.Volume = ctpTick.m_stTick.Volume; //备份量
    refLineK.HighestPrice = max(ctpTick.m_stTick.LastPrice, refLineK.HighestPrice);
    refLineK.LowestPrice = min(ctpTick.m_stTick.LastPrice, refLineK.LowestPrice);
    refLineK.ClosePrice = ctpTick.m_stTick.LastPrice;
    refLineK.OpenInterest = ctpTick.m_stTick.OpenInterest;
    refLineK.prcVwapSum += ctpTick.m_stTick.LastPrice * refLineK.prcVolume; //量加权平均和
    refLineK.prcTwapSum += ctpTick.m_stTick.LastPrice; //算数价格平均和
    refLineK.prcVolumeSum += refLineK.prcVolume; ////量加
    refLineK.bidVolSum += ctpTick.m_stTick.BidVolume1; //盘口bid1量加总
    refLineK.askVolSum += ctpTick.m_stTick.AskVolume1; //盘口ask1量加总
    refLineK.bidPrcVSum += (BidPrice1 * ctpTick.m_stTick.BidVolume1); //盘口bid1价加权成交量和
    refLineK.askPrcVSum += (AskPrice1 * ctpTick.m_stTick.AskVolume1); //盘口ask1价加权成交量和
    refLineK.SpreadSum += (AskPrice1 - BidPrice1); //1档盘口(askPrcT - bidPrcT) 和
    //refLineK.OiPrev; //当前bar的第一个tick的 open interest
    refLineK.OiChangeSum += ctpTick.m_stTick.OiChange; //tick 里面oichg 加总
    refLineK.OiCloseSum += ctpTick.m_stTick.OiClose; //tick 里面oiClose 加总
    refLineK.OiOpenSum += ctpTick.m_stTick.OiOpen; //tick 里面oiOpen 加总
    refLineK.prcCount++;
    if (CTP_DOUBLE_ISOUT(refLineK.prcVwapSum)
        || CTP_DOUBLE_ISOUT(refLineK.prcVolumeSum)
        || CTP_DOUBLE_ISOUT(refLineK.prcTwapSum)
        || CTP_DOUBLE_ISOUT(refLineK.bidPrcVSum)
        || CTP_DOUBLE_ISOUT(refLineK.askPrcVSum)
        || CTP_DOUBLE_ISOUT(refLineK.SpreadSum))
    {
        HTP_LOG << "KSUM!" << refctpLineK.m_InstrumentID; //异常
        refctpLineK.Init(eLoopIdx, ctpTick.m_stTick, true); goto LINEK_END;//相关计数重置为0
    }
    if (refLineK.prcVolumeSum > 0 && refLineK.prcVwapSum != 0.0)
        refLineK.prcVwap = refLineK.prcVwapSum / refLineK.prcVolumeSum;
    if (refLineK.prcCount > 0 && refLineK.prcTwapSum != 0.0)
        refLineK.prcTwap = refLineK.prcTwapSum / refLineK.prcCount;
    if (refLineK.prcCount > 0 && refLineK.bidPrcVSum != 0.0)
        refLineK.bidPrcV = refLineK.bidPrcVSum / refLineK.bidVolSum; //盘口bid1价加权成交量
    if (refLineK.prcCount > 0 && refLineK.askPrcVSum != 0.0)
        refLineK.askPrcV = refLineK.askPrcVSum / refLineK.askVolSum; //盘口ask1价加权成交量
    if (refLineK.prcCount > 0 && refLineK.SpreadSum != 0.0)
        refLineK.Spread = refLineK.SpreadSum / refLineK.prcCount; //1档盘口(askPrcT - bidPrcT) 按时间平均
LINEK_END: 
    eLoopIdx = eLineKType(eLoopIdx + 1);
    if (eLoopIdx < LKT_ENDF) { goto LINEK_LOOP; }
    refctpLineK.m_ctNature = ctpTick.m_ctNature; //更新为最新时钟
    refctpLineK.m_ctTrading = ctpTick.m_ctTrading;
    if (m_FuncTDLineKSet) //保存备份，用于下一次计算K线前内存比对
    {
        m_FuncTDLineKSet(refctpLineK);
    }
    return 1; //返回完成
}
//
/////分价表通知
//void CMdThread::OnRtnMBLMarketData(CThostFtdcMBLMarketDataField* pMBLMarketData) {};
//
/////询价通知
//void CMdThread::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) {};