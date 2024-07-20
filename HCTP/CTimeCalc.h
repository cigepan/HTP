#pragma once
#include <string.h>
#include "ThostFtdcUserApiStruct.h"
#include ".\LIBWin\NTP\VxNtpHelper.h"

#define CTPTIME int
#define CTPTIMETICK int
#define CTPTIMETICK_BIG long long
class CTimeCalc
{

public:
	CTPTIME m_nYear;
	CTPTIME m_nMonth;
	CTPTIME m_nDay;
	CTPTIME m_nHour;
	CTPTIME m_nMinute;
	CTPTIME m_nSecond;
	CTPTIME m_nMilSec;
	CTimeCalc()
	{
		Clear();
	}
	CTimeCalc(const SYSTEMTIME& sysTime)
	{
		SetSysTime(sysTime);
	}
	CTimeCalc(const x_ntp_time_context_t& netTime)
	{
		SetNetTime(netTime);
	}
	CTimeCalc(const TThostFtdcDateType& stYMD, const TThostFtdcTimeType& stHMS, const TThostFtdcMillisecType stMilSec)
	{
		SetYMD(stYMD);
		SetHMS(stHMS);
		SetMilSec(stMilSec);
	}
	void Clear()
	{
		m_nYear = m_nMonth = m_nDay = m_nHour = m_nMinute = m_nSecond = m_nMilSec = -1;
	}
	CTPTIME GetYMD() const
	{
		return (m_nYear * 10000 + m_nMonth * 100 + m_nDay);
	}
	CTimeCalc& SetYMD(CTPTIME ctpYMD)
	{
		m_nDay = ctpYMD % 100; ctpYMD /= 100;
		m_nMonth = ctpYMD % 100; ctpYMD /= 100;
		m_nYear = ctpYMD; return *this;
	}
	CTPTIME GetHM() const
	{
		return (m_nHour * 100 + m_nMinute);
	}
	CTPTIMETICK GetTickHMS() const
	{
		return m_nHour * 3600 + m_nMinute * 60 + m_nSecond;
	}
	CTimeCalc& SetHM(CTPTIME ctpHM)
	{
		m_nMinute = ctpHM % 100; ctpHM /= 100;
		m_nHour = ctpHM; return *this;
	}
	CTPTIME GetHMS() const
	{
		return (m_nHour * 10000 + m_nMinute * 100 + m_nSecond);
	}
	CTimeCalc& SetHMS(CTPTIME ctpHMS)
	{
		m_nSecond = ctpHMS % 100; ctpHMS /= 100;
		m_nMinute = ctpHMS % 100; ctpHMS /= 100;
		m_nHour = ctpHMS; return *this;
	}
	CTPTIME GetHMSL() const
	{
		return (m_nHour * 10000000 + m_nMinute * 100000 + m_nSecond*1000 + m_nMilSec);
	}
	CTimeCalc& SetHMSL(CTPTIME ctpHMSL)
	{
		m_nMilSec = ctpHMSL % 1000; ctpHMSL /= 1000;
		m_nSecond = ctpHMSL % 100; ctpHMSL /= 100;
		m_nMinute = ctpHMSL % 100; ctpHMSL /= 100;
		m_nHour = ctpHMSL; return *this;
	}
	LONG64 GetYMDHMS()
	{
		return (m_nYear * 10000000000 + m_nMonth * 100000000 + m_nDay * 1000000
			+ m_nHour * 10000 + m_nMinute * 100 + m_nSecond);
	}
	CTimeCalc& SetSysTime(const SYSTEMTIME& sysTime)
	{
		m_nYear = sysTime.wYear;
		m_nMonth = sysTime.wMonth;
		m_nDay = sysTime.wDay;
		m_nHour = sysTime.wHour;
		m_nMinute = sysTime.wMinute;
		m_nSecond = sysTime.wSecond;
		m_nMilSec = sysTime.wMilliseconds;
		return *this;
	}
	CTimeCalc& SetNetTime(const x_ntp_time_context_t& netTime)
	{
		m_nYear = netTime.xut_year;
		m_nMonth = netTime.xut_month;
		m_nDay = netTime.xut_day;
		m_nHour = netTime.xut_hour;
		m_nMinute = netTime.xut_minute;
		m_nSecond = netTime.xut_second;
		m_nMilSec = netTime.xut_msec;
		return *this;
	}
	CTimeCalc& SetYMD(const TThostFtdcDateType& stYMD)
	{
		if (stYMD[0] == '\0')
		{
			m_nYear = m_nMonth = m_nDay = -1;
		}
		else
		{
			m_nYear = (stYMD[0] - '0') * 1000 + (stYMD[1] - '0') * 100 + (stYMD[2] - '0') * 10 + (stYMD[3] - '0');
			m_nMonth = (stYMD[4] - '0') * 10 + (stYMD[5] - '0');
			m_nDay = (stYMD[6] - '0') * 10 + (stYMD[7] - '0');
		}
		return *this;
	}
	CTimeCalc& SetHM(const TThostFtdcTimeType& stHM)
	{
		if (stHM[0] == '\0')
		{
			m_nHour = m_nMinute = -1;
		}
		else
		{
			m_nHour = (stHM[0] - '0') * 10 + (stHM[1] - '0');
			m_nMinute = (stHM[3] - '0') * 10 + (stHM[4] - '0');
		}
		return *this;
	}
	CTimeCalc& SetHMS(const TThostFtdcTimeType& stHMS)
	{
		if (stHMS[0] == '\0')
		{
			m_nHour = m_nMinute = m_nSecond = -1;
		}
		else
		{
			m_nHour = (stHMS[0] - '0') * 10 + (stHMS[1] - '0');
			m_nMinute = (stHMS[3] - '0') * 10 + (stHMS[4] - '0');
			m_nSecond = (stHMS[6] - '0') * 10 + (stHMS[7] - '0');
		}
		return *this;
	}
	CTimeCalc& SetMilSec(const TThostFtdcMillisecType& stMilSec)
	{
		m_nMilSec = stMilSec; return *this;
	}
	~CTimeCalc() {}
public:
	bool operator==(const CTimeCalc& cTime)
	{
		return ((m_nMilSec == cTime.m_nMilSec) && (m_nSecond == cTime.m_nSecond) && (m_nMinute == cTime.m_nMinute)
			&& (m_nHour == cTime.m_nHour) && (m_nDay == cTime.m_nDay) && (m_nMonth == cTime.m_nMonth)
			&& (m_nYear == cTime.m_nYear));
	}

	CTimeCalc& operator=(const CTimeCalc& cTime)
	{
		m_nYear = cTime.m_nYear;
		m_nMonth = cTime.m_nMonth;
		m_nDay = cTime.m_nDay;
		m_nHour = cTime.m_nHour;
		m_nMinute = cTime.m_nMinute;
		m_nSecond = cTime.m_nSecond;
		m_nMilSec = cTime.m_nMilSec;
		return *this;
	}
	inline bool isLeapYear(CTPTIME nYear)
	{
		if (nYear % 100 == 0) //是世纪年
		{
			return ((nYear % 400)==0);
		}
		else
		{
			return ((nYear % 4) == 0);
		}
	}
	string GetStrYMDHMSL() const
	{
		return (GetStrYMD() + "-" + GetStrHMSL());
	}
	string GetStrYMDHMS() const
	{
		return (GetStrYMD() + "-" + GetStrHMS());
	}
	string GetStrYMDHM() const
	{
		return (GetStrYMD() + "-" + GetStrHM());
	}
	string GetStrYMD() const
	{
		TThostFtdcDateType stYMD = {0};
		GetYMD(stYMD);
		return string(stYMD);
	}
	string GetStrHMS() const
	{
		TThostFtdcTimeType stHMS = { 0 };
		GetHMS(stHMS);
		return string(stHMS);
	}
	string GetStrHM() const
	{
		TThostFtdcTimeType stHM = { 0 };
		GetHM(stHM);
		return string(stHM);
	}
	string GetStrHMSL() const
	{
		char stHMSL[16] = { 0 };
		sprintf_s(stHMSL, "%02d:%02d:%02d.%03d", m_nHour, m_nMinute, m_nSecond, m_nMilSec);
		return string(stHMSL);
	}
	void GetHMS(TThostFtdcTimeType& stHMS) const
	{
		sprintf_s(stHMS, "%02d:%02d:%02d", m_nHour, m_nMinute, m_nSecond);
	}
	void GetHM(TThostFtdcTimeType& stHM) const
	{
		sprintf_s(stHM, "%02d:%02d", m_nHour, m_nMinute);
	}
	void GetYMD(TThostFtdcDateType& stYMD) const
	{
		sprintf_s(stYMD, "%04d%02d%02d", m_nYear, m_nMonth, m_nDay);
	}
	bool GetNextMinute(TThostFtdcDateType& stYMD, TThostFtdcTimeType& stHMS)
	{
		SetYMD(stYMD); SetHMS(stHMS);
		if ((m_nYear < 1) || (m_nHour < 0))
		{
			return false;
		}
		GetNextMinute();
		GetYMD(stYMD); GetHMS(stHMS);
		return true;
	}
	bool GetPrevMinute(TThostFtdcDateType& stYMD, TThostFtdcTimeType& stHMS)
	{
		SetYMD(stYMD); SetHMS(stHMS);
		if ((m_nYear < 1) || (m_nHour < 0))
		{
			return false;
		}
		GetPrevMinute();
		GetYMD(stYMD); GetHMS(stHMS);
		return true;
	}
	bool GetNextDay(TThostFtdcDateType& stYMD)
	{
		SetYMD(stYMD);
		if (m_nYear < 1) 
		{
			return false;
		}
		GetNextDay();
		GetYMD(stYMD);
		return true;
	}
	bool GetPrevDay(TThostFtdcDateType& stYMD)
	{
		SetYMD(stYMD);
		if (m_nYear < 1)
		{
			return false;
		}
		GetPrevDay();
		GetYMD(stYMD);
		return true;
	}
	bool GetNextMinute(TThostFtdcTimeType& stHMS)
	{
		SetHMS(stHMS);
		if (m_nHour < 0)
		{
			return false;
		}
		GetNextMinute();
		GetHMS(stHMS);
		return true;
	}
	bool GetPrevMinute(TThostFtdcTimeType& stHMS)
	{
		SetHMS(stHMS);
		if (m_nHour < 0)
		{
			return false;
		}
		GetPrevMinute();
		GetHMS(stHMS);
		return true;
	}
	CTimeCalc& GetNextHour() //获取下一小时
	{
		if (++m_nHour > 23)
		{
			m_nHour = 0;
			GetNextDay();
		}
		return *this;
	}
	CTimeCalc& GetPrevHour() //获取上一小时
	{
		m_nMinute = 59;
		if (--m_nHour < 0)
		{
			m_nHour = 23;
			GetPrevDay();
		}
		return *this;
	}
	CTimeCalc& GetNextMinute() //获取下一分钟
	{
		if (++m_nMinute > 59)
		{
			m_nMinute = 0;
			GetNextHour();
		}
		return *this;
	}
	CTimeCalc& GetPrevMinute() //获取上一分钟
	{
		if (--m_nMinute < 0)
		{
			m_nMinute = 59;
			GetPrevHour();
		}
		return *this;
	}
	CTimeCalc& GetPrevDay() //获取明天
	{
		if (m_nYear < 1)
		{
			return *this;
		}
		if (--m_nDay < 1)
		{
			if (--m_nMonth < 1) 
			{
				m_nMonth = 12;
				m_nYear--; //过年
			}
			switch (m_nMonth)
			{
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
			case 12: m_nDay = 31; break;
			case 4:
			case 6:
			case 9:
			case 11:m_nDay = 30; break;
			case 2:
				if (isLeapYear(m_nYear)) {
					m_nDay = 29; //瑞年
				}
				else {
					m_nDay = 28;  //平年
				}
				break;
			default:
				break;
			}
		}
		return *this;
	}
	CTimeCalc& GetNextDay() //获取明天
	{
		if (m_nYear < 1)
		{
			return *this;
		}
		switch (m_nMonth)
		{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12: if (++m_nDay > 31) { m_nDay = 1; m_nMonth++; } break;
		case 4:
		case 6:
		case 9:
		case 11: if (++m_nDay > 30) { m_nDay = 1;  m_nMonth++; } break;
		case 2: 
		if (isLeapYear(m_nYear)) {
			if (++m_nDay > 29) { m_nDay = 1; m_nMonth++; }  //瑞年
		}
		else {
			if (++m_nDay > 28) { m_nDay = 1; m_nMonth++; }  //平年
		}
		break;
		default:
			break;
		}
		if (m_nMonth > 12) {
			m_nMonth = 1;
			m_nYear++; //过年
		}
		return *this;
	}
};
