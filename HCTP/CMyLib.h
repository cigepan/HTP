#pragma once

#include "stdafx.h"

using namespace std;

//控制台事件
BOOL CtrlHandler(DWORD fdwCtrlType);

std::string ASCII2UTF_8(std::string& strAsciiCode);
//utf8 ת Unicode  
std::wstring Utf82Unicode(const std::string& utf8string);


//unicode תΪ ascii  
std::string WideByte2Acsi(std::wstring& wstrcode);





//utf-8 ת ascii  
std::string UTF_82ASCII(std::string& strUtf8Code);


///////////////////////////////////////////////////////////////////////  


//ascii ת Unicode  
std::wstring Acsi2WideByte(std::string& strascii);


//Unicode ת Utf8  
std::string Unicode2Utf8(const std::wstring& widestring);


//ascii ת Utf8  
std::string ASCII2UTF_8(std::string& strAsciiCode);

inline string GetKind(const char* InstrumentID)
{
	string strKind("");
	for (int i = 0; InstrumentID[i] != '\0'; i++)
	{
		if (InstrumentID[i] > '9')
			strKind.push_back(InstrumentID[i]);
		else
			return strKind;
	}
	return strKind;
}
