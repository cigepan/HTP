@echo off
set dbip=192.168.3.199
set dbport=7801
rem set /p dbip=�����������IP��
rem if "%dbip%" == "" (
rem set dbip=0.0.0.0
rem ) 
rem set /p dbport=������������˿ڣ�
rem if "%dbport%" == "" (
rem set dbport=27017
rem ) 
goto THREAD
:THREAD
echo ����MONGO������: %dbip% %dbport%
set "workpath=D:\MongoDB\%dbip%_%dbport%"
echo �����ļ�·����%workpath%
title=%workpath%
md %workpath%
mongod.exe --wiredTigerCacheSizeGB=4 --bind_ip %dbip% --port %dbport% --dbpath %workpath%
goto THREAD
