@echo off
set HTPLOG=%date% %time%: 开始启动HTP控制台程序
rem 默认配置
set CONFIG_DEFAULT=A9
rem 备用账号相关配置
set CONFIG_BACKUP=A2
rem 当前配置
set CONFIG_RUN=%CONFIG_DEFAULT%
rem 自动循环模式，根据结果确定是否需要自动循环重启，0退出，不再次启动，1再次启动，2采用备用账号再次启动
set LOOP_MODE=0
:HTP_LOOP
set HTPLOG=本次运行的配置：%CONFIG_RUN%
echo %date% %time% %HTPLOG% >>HTPLOG.txt 
HCTP.exe %CONFIG_RUN%
set HTPLOG=运行返回结果：%errorlevel%
echo %HTPLOG%
echo %date% %time% %HTPLOG% >>HTPLOG.txt 
if %errorlevel%==9000 (
	set LOOP_MODE=1
	set HTPLOG=返回类型：HMR_FAIL_INIT，主程序MAIN初始化错误，自动重新初始化
) else if %errorlevel%==9001 (
	set LOOP_MODE=1
	set HTPLOG=返回类型：HMR_FAIL_MDB，MONGODB初始化错误，请检查MONGODB连接与相关配置
) else if %errorlevel%==9002 (
	set LOOP_MODE=1
	set HTPLOG=返回类型：HMR_FAIL_LOG，日志线程初始化错误，请检查磁盘是否已满
) else if %errorlevel%==9003 (
	set LOOP_MODE=1
	set HTPLOG=返回类型：HMR_FAIL_NTP，网络时钟初始化失败，请检查网络连接
) else if %errorlevel%==9004 (
	set LOOP_MODE=2
	set HTPLOG=返回类型：HMR_FAIL_TD，交易线程初始化失败，将启用备用账号！请检查该CTP交易前置地址是否正常，然后检查账号配置相关，也有可能账号被风控导致登录不上或者不满足交易所监管的机器配置，多账户重复登录，或者一个IP登录过多账户，或者防火墙原因
) else if %errorlevel%==9005 (
	set LOOP_MODE=2
	set HTPLOG=返回类型：HMR_FAIL_MD，行情线程初始化失败，将启用备用账号！请检查该CTP行情前置地址是否正常或检查账号配置相关，基本上前置连接的网络有问题，因为行情登录不需要账户ID，或者防火墙原因。
) else if %errorlevel%==9006 (
	set LOOP_MODE=0
	set HTPLOG=返回类型：HMR_FAIL_RISK，风控线程参数初始化失败，一般是MDB加载风控参数失败导致的，有可能的字段缺失或字段不满足要求的格式导致加载出错
) else if %errorlevel%==9007 (
	set LOOP_MODE=0
	set HTPLOG=返回类型：HMR_CTP_CLOSED，CTP收盘时间，正常退出自动化脚本，下一次将通过windows任务计划运行该脚本
) else if %errorlevel%==9008 (
	set LOOP_MODE=0
	set HTPLOG=返回类型：HMR_WEEK_CLOSED，CTP节假日收盘时间，正常退出自动化脚本，下一次将通过windows任务计划运行该脚本
) else if %errorlevel%==9009 (
	set LOOP_MODE=1
	set HTPLOG=返回类型：HMR_SYS_RISK，系统进程风控产生的异常退出重启机制，一般由于网络原因导致各种超时，具体请检查运行日志
) else if %errorlevel%==9010 (
	set LOOP_MODE=2
	set HTPLOG=返回类型：HMR_FAIL_CTP，将启用备用账号，原因是该CTP前置PING不通，或者当前不是交易时间
) else (
	set LOOP_MODE=1
	set HTPLOG=返回类型：未定义，未知错误，自动化程序异常崩溃导致的，可能内存占用过多导致的
)
echo %HTPLOG%
echo %date% %time% %CONFIG_RUN% %HTPLOG% >>HTPLOG.txt
if %CONFIG_RUN%==%CONFIG_BACKUP% (
	echo %date% %time% %CONFIG_RUN% 使用备用账号登录失败，不再进行登录尝试，请检查交易所连接 >>HTPLOG.txt
	goto exit
)
if %LOOP_MODE%==0 (
	goto exit
) else if %LOOP_MODE%==1 (
	rem set CONFIG_RUN=%CONFIG_DEFAULT%
	goto HTP_LOOP
) else if %LOOP_MODE%==2 (
	set CONFIG_RUN=%CONFIG_BACKUP%
	goto HTP_LOOP
) else (
	goto HTP_LOOP
)
goto exit
rem echo %time%: 程序异常退出 >>HTPLOG.txt
rem set HTPLOG=%time%: 重新启动HTP控制台程序
rem 检查时间
rem for /f "tokens=2" %%I in ('echo %date% ^|find " "') do set week=%%I 
rem echo %date%
rem echo %week%
rem echo %time%
rem if %week%==周六 (
rem 	goto DAY6 
rem )
rem if %week%==周日 (
rem 	goto DAY7 
rem )
rem goto HTP_LOOP
rem :DAY6
rem if %time:~0,2% GTR 6 (
rem 	echo %time%: 不在交易时间 >>HTPLOG.txt
rem 	goto exit
rem ) else (
rem 	echo %time%: 交易时间重启 >>HTPLOG.txt
rem 	goto HTP_LOOP
rem )
rem :DAY7
rem echo %time%: 不在交易时间 >>HTPLOG.txt
rem goto exit
