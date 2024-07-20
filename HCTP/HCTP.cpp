// HCTP.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "stdafx.h"
#include "CMongoDB.h"
#include "CHpTcpServer.h"
#include "CHpTcpClient.h"
#include "CMdThread.h"
#include "CTdThread.h"
#include "CHtpSystem.h"
#include "CMyLib.h"
#include "CLogThread.h"
#include "CTimeCalc.h"

using namespace std;
int main(int argc, char* argv[])
{
    for(int i=0; i<argc; i++)
        cout << *(argv + i) << endl;
    //--------------------------------------------设置控制台全局事件--------------------------------------------
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    //--------------------------------------------加载配置--------------------------------------------
    auto& CONFIG = make_shared<CUserConfig>();
    string strInput;
    CHtpMsg stMsg;
    eHtpMainReturn eReturn = HMR_FAIL_INIT;
    unique_ptr<CTdThread> ptrTD = NULL;
    unique_ptr<CMdThread> ptrMD = NULL;
    while(true) 
    {
        Sleep(500);
        if (argc < 2)
        {
            cout << UTF_82ASCII(string("请输入需要加载的配置项：")) << endl;
            strInput.clear(); cin >> strInput;
        }
        else 
        {
            strInput = string(*(argv + 1));
        }
        cout << UTF_82ASCII(string("开始加载配置：").append(strInput)) << endl;
        CONFIG->Clear();
        stMsg = CONFIG->Load("CONFIG", strInput);
        if (HTP_CID_DECODE(stMsg.m_CID) == HTP_CID_ISUTF8)
            cout << UTF_82ASCII(stMsg.m_strMsg) << endl;
        else
            cout << (stMsg.m_strMsg) << endl;
        if (HTP_CID_RTFLAG(stMsg.m_CID) == HTP_CID_OK)
        {
            if (argc > 1) { break; } //带参数运行不进行防呆
            cout << UTF_82ASCII(string("请输入OK确认配置")) << endl;
            strInput.clear(); cin >> strInput;
            if(strInput == "OK"){ break; }
            cout << UTF_82ASCII(string("此次加载的配置无效，需要重新加载！")) << endl;
        }
    }
    auto RISK = make_shared<CRiskThread>();
    RISK->Start(CONFIG);
    //--------------------------------------------初始化MD--------------------------------------------
    auto MDB = make_shared<CMongoDB>();
    MDB->m_cRisk = RISK; //风控引用 + 1
    cout << UTF_82ASCII(string("开始初始化MONGODB")) << endl;
    stMsg = MDB->Start(CONFIG);
    if (HTP_CID_RTFLAG(stMsg.m_CID) != HTP_CID_OK) 
    {
        cout << UTF_82ASCII(string("初始化MONGODB失败")) << endl;
        cout << stMsg.m_strMsg << stMsg.m_CID << endl;
        eReturn = HMR_FAIL_MDB;
        goto MAIN_STOP;
    }
    if (0 > MDB->LoadTimeTD()) 
    {
        cout << UTF_82ASCII(string("警告：获取合约交易时间列表失败，账户将不能进行策略订单生成和执行下单")) << endl;
    }
    //--------------------------------------------启动日志--------------------------------------------
    HTP_LOG.m_MDB = MDB; //MDB引用 + 1
    HTP_LOG.Start(CONFIG);
    HTP_LOG << stMsg;
    if (HTP_CID_RTFLAG(stMsg.m_CID) != HTP_CID_OK) { return int(HMR_FAIL_MDB); }
    HTP_LOG << "------------START HTP V3.10------------" << HTP_ENDL;
    //--------------------------------------------初始化时钟--------------------------------------------
    HTP_LOG << HTP_MSG("初始化网络时钟！", HTP_CID_FAIL|HTP_CID_ISUTF8);
    if (!HTP_SYS.SynNetTime())
    {
        HTP_LOG << HTP_MSG("初始化网络时钟失败，请检查网络连接！", HTP_CID_FAIL|HTP_CID_ISUTF8|HTP_CID_USE_LOGNET);
        eReturn = HMR_FAIL_NTP;
        goto MAIN_STOP;
    }
    HTP_SYS.Start(CONFIG);
    //if (HTP_SYS.GetTime(CTP_TIME_NET).m_nHour > 4 && HTP_SYS.GetTime(CTP_TIME_NET).m_nHour < 8) //早盘退出，交易日过8点后需要重启
    //{
    //    //HTP_LOG << HTP_MSG("非交易时间，进程自动退出", HTP_CID_WARN | HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET);
    //    eReturn = HMR_WEEK_CLOSED;
    //    goto MAIN_STOP;
    //}
    //检查交易所是否开盘
    HTP_LOG << HTP_MSG("正在检测CTP状态！", HTP_CID_WARN | HTP_CID_ISUTF8);
    if (0 != HTP_SYS.GetPingMS())
    {
        HTP_LOG << HTP_MSG("检测CTP不在状态，请确认今天是否开盘，如果开盘请及时修复该启动问题，否则忽略该尝试警告！"
            , HTP_CID_FAIL | HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET);
        eReturn = HMR_FAIL_CTP;
        goto MAIN_STOP;
    }
    HTP_LOG << HTP_MSG("检测CTP状态通过", HTP_CID_WARN | HTP_CID_ISUTF8);
    //----------------------------------启动交易线程------------------------------------------
    ptrTD = make_unique<CTdThread>();
    if (!ptrTD) 
    {
        HTP_LOG << HTP_MSG("HTP进程已退出，初始化交易线程内存失败，需要检查内存是否占用过高！", HTP_CID_WARN | HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET);
        eReturn = HMR_FAIL_TD; goto MAIN_STOP; 
    }
    ptrTD->m_cRisk = RISK; //风控引用 + 1
    ptrTD->m_MDB = MDB; //MDB引用 + 1
    if (0 != ptrTD->Start(CONFIG)) //启动不成功则
    {
        HTP_LOG << HTP_MSG("交易线程账号登录失败，请检查账号配置相关！", HTP_CID_FAIL|HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET);
        eReturn = HMR_FAIL_TD;
        goto MAIN_STOP;
    }
    if (!ptrTD || !ptrTD->GetStatus(CTPST_RUNING)) 
    { 
        HTP_LOG << HTP_MSG("HTP进程已退出，初始化状态标志不对，请检查初始化流程异常", HTP_CID_WARN | HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET);
        eReturn = HMR_FAIL_TD; goto MAIN_STOP; 
    } //登录不成功或其他情况
    ptrMD = make_unique<CMdThread>();
    ptrMD->m_cRisk = RISK; //风控引用 + 1
    ptrMD->m_MDB = MDB; //MDB引用 + 1
    //--------------------------------------------设置行情回调-----------------------------------------
    //同级别的线程之间使用回调进行交互，避免循环引用
    FUNC_BIND(ptrMD->m_FuncTDTick, (bind(&CTdThread::OnTick, ptrTD.get(), std::placeholders::_1)));
    FUNC_BIND(ptrMD->m_FuncTDLineKGet, (bind(&CTdThread::GetLineK, ptrTD.get(), std::placeholders::_1)));
    FUNC_BIND(ptrMD->m_FuncTDLineKSet, (bind(&CTdThread::SetLineK, ptrTD.get(), std::placeholders::_1)));
    FUNC_BIND(MDB->m_FuncGetPosNet, (bind(&CTdThread::GetPosNet, ptrTD.get(), std::placeholders::_1, std::placeholders::_2)));
    MDB->LoadRiskCnt();//加载风控计数
    if (0 == MDB->UpdateContract(ptrTD->GetMapInstrument(), ptrTD->GetMapCommissionRate()))
    {
        HTP_LOG << HTP_MSG("没有获取到任何合约相关信息，请检查初始化流程与交易前置属性！", HTP_CID_ISUTF8 | HTP_CID_USE_LOGNET); //
        eReturn = HMR_FAIL_TD;
        goto MAIN_STOP;
    }
    //----------------------------------启动行情线程------------------------------------------
    ptrMD->Start(CONFIG, ptrTD->GetMapInstrument()); //启动行情线程，行情线程无需账号密码
    DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND); //屏蔽按钮
    SetWindowText(GetConsoleWindow(), CString(CONFIG->GetTitle().c_str()));
    CtrlHandler(CTRL_C_EVENT);
    CTPTIME nTime;
    while (true)
    {
        Sleep(10000);
        nTime = HTP_SYS.GetTime(CTP_TIME_NET).GetHMS();
        if ((nTime > INT_CTPPM_CLOSE && nTime < INT_CTPPM_OPEN)
            || (nTime > INT_CTPAM_CLOSE && nTime < INT_CTPAM_OPEN))//收盘后需要退出运行
        {
            eReturn = HMR_CTP_CLOSED;
            goto MAIN_STOP;
        }
        //if (HTP_SYS.GetTime(CTP_TIME_NET).m_nHour > 4 && HTP_SYS.GetTime(CTP_TIME_NET).m_nHour < 8) //早盘退出，交易日过8点后需要重启
        //{
        //    eReturn = HMR_WEEK_CLOSED;
        //    goto MAIN_STOP;
        //}
        //if (0 != HTP_SYS.GetPingMS())
        //{
        //    HTP_LOG << HTP_MSG("初始化PING交易所前置失败，请检查网络连接或当前是否处于交易时间", HTP_CID_FAIL | HTP_CID_ISUTF8);
        //    eReturn = HMR_FAIL_CTP;
        //    goto MAIN_STOP;
        //}
        for (int i = 0; i < RSC_ENDF; i++)
        {
            if (RISK->SysCnt(eRiskSysCnt(i)) > g_nSysCntLimit[i])
            {
                HTP_LOG << HTP_MSG(string("MAIN PROCESS CHECK RISK SYSCNT OUT->").append(g_strSysCnt[i]), HTP_CID_FAIL | HTP_CID_USE_LOGNET | 4700);
                eReturn = HMR_SYS_RISK;
                goto MAIN_STOP;
            }
        }
    }
MAIN_STOP:
    CtrlHandler(CTRL_CLOSE_EVENT);
    if(ptrMD)
        ptrMD->Stop();
    if(ptrTD)
        ptrTD->Stop();
    Sleep(1000);
    HTP_LOG.Stop();
    HTP_SYS.Stop();
    //AppendMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND, ""); //屏蔽按钮
    return (int)eReturn;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
