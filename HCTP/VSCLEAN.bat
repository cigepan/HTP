@echo off
echo �������obj pch idb pdb ncb opt plg res sbr ilk suo�ļ������Ե�......
pause
del /f /s /q .\*.obj
del /f /s /q .\*.pch
del /f /s /q .\*.idb
del /f /s /q .\*.pdb
del /f /s /q .\*.ncb 
del /f /s /q .\*.opt 
del /f /s /q .\*.plg
del /f /s /q .\*.res
del /f /s /q .\*.sbr
del /f /s /q .\*.ilk
del /f /s /q .\*.aps
del /f /s /q .\*.log
rd /s /q .vs\
rd /s /q x64\
echo ����ļ���ɣ�
echo. & pause