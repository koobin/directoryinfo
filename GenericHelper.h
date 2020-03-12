/*!
\ 一般工具函数集合
\ version: v1.0.1
\ last modified: 2020.03.11
\ author: koobin
*/

#ifndef _COMMON_HELPER_
#define _COMMON_HELPER_


#if _MSC_VER > 1000
#pragma once
#endif

#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <limits>

#ifdef _UNICODE
typedef std::wstring string_t;
#else
typedef std::string string_t;
#endif


struct ProcessWindow
{
	DWORD dwProcessId;
	HWND hwndWindow;
};

BOOL SystemShutdown(UINT uFlags);

// 查找进程主窗口的回调函数  
BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam);

string_t binToHex(const unsigned char* buffer, int size);

void TimetToSystemTime(time_t t, LPSYSTEMTIME pst);

void FileTimeToTime_t(FILETIME ft, time_t *t);

void SystemTimeToTime_t(SYSTEMTIME st, time_t *pt);

SYSTEMTIME	TimetToSystemTime(__time64_t t, bool toLocal);

__time64_t		SystemTimeToTimet(const SYSTEMTIME& st, bool toUTC);

bool SeparateInt64(INT64 llSize, int& GB, int& MB, int& kB, int& Byte);

bool IntegerToString(INT64 llSize, LPTSTR strOut, DWORD nSize);

double ArrayMax(double* arr, int len, int* retIdx);

bool EnumHostIPv4Addr(std::vector<std::string>& ipvec);

//LPCTSTR FindHostIPv4Addr(const std::vector<std::string>& ipvec, bool biggest);

#endif  //_COMMON_HELPER_