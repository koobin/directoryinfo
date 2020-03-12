
//#include "stdafx.h"
//#include <WS2tcpip.h>
#include <Windows.h>
#include <direct.h>
#include <math.h>
#include <tchar.h>
#include <time.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <IPHlpApi.h>
#include "GenericHelper.h"

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "shlwapi.lib")


BOOL SystemShutdown(UINT uFlags)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return(FALSE);

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
		&tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
		(PTOKEN_PRIVILEGES)NULL, 0);

	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	// Shut down the system and force all applications to close. 

	if (!ExitWindowsEx(uFlags, 0))
		return FALSE;

	return TRUE;
}

// 查找进程主窗口的回调函数  
BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam)
{
	ProcessWindow *pProcessWindow = (ProcessWindow *)lParam;

	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);

	// 判断是否是指定进程的主窗口  
	if (pProcessWindow->dwProcessId == dwProcessId && IsWindowVisible(hWnd) && 
		(GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX) && GetParent(hWnd) == NULL)
	{
		pProcessWindow->hwndWindow = hWnd;

		return FALSE;
	}

	return TRUE;
}

string_t binToHex(const unsigned char* buffer, int size)
{
	string_t s;

	TCHAR digit[4];

	unsigned char* p = (unsigned char*)buffer;

	for (int index = 0; index < size; index++)
	{
		_stprintf_s(digit, _T("%02x "), *p++);
		s += digit;
	}

	return s;
}

// DWORD GetTimeGap32(DWORD dwBefor)
// {
// 	DWORD dwCur = ::timeGetTime();
// 	return (dwCur - dwBefor);
// }

//************************************************************
//FILETIME, SYSTEMTIME 与 time_t 相互转换 

//#####SYSTEMTIME 与 FILETIME相互转换##### 
//可以使用系统函数
//FileTimeToSystemTime(&ftcreate,&stcreate);  

//参数：
//(lpFileTime As FILETIME, lpSystemTime As SYSTEMTIME) 
//说明 
//根据一个FILETIME结构的内容，装载一个SYSTEMTIME结构 
//返回值 
//Long，非零表示成功，零表示失败。会设置GetLastError 
//参数表 
//参数 类型及说明 
//lpFileTime FILETIME，包含了文件时间的一个结构 
//lpSystemTime SYSTEMTIME，用于装载系统时间信息的一个结构

//#####SYSTEMTIME 与 time_t相互转换#####

//#### Time_tToSystemTime ####
void TimetToSystemTime(time_t t, LPSYSTEMTIME pst)
{
	FILETIME ft;
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32);

	FileTimeToSystemTime(&ft, pst);
}

//#### FileTimeToTime_t ####
void  FileTimeToTime_t(FILETIME  ft, time_t  *t)
{
	//LONGLONG  ll;  

	ULARGE_INTEGER            ui;
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;

	//ll            =  (LONGLONG(ft.dwHighDateTime)<<32)  +  ft.dwLowDateTime;  

	*t = ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
}

//#### SystemTimeToTime_t ####
void SystemTimeToTime_t(SYSTEMTIME st, time_t *pt)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	FileTimeToTime_t(ft, pt);
}
//********************************************************************/


SYSTEMTIME TimetToSystemTime(__time64_t t, bool toLocal)
{
	if (toLocal)
	{
		struct tm gm;
		_localtime64_s(&gm, &t);
		t = _mkgmtime64(&gm);
	}
	FILETIME ft;
	SYSTEMTIME pst;
	LONGLONG nLL = Int32x32To64(t, 10000000) + 116444736000000000;

	ft.dwLowDateTime = (DWORD)nLL;
	ft.dwHighDateTime = (DWORD)(nLL >> 32);

	FileTimeToSystemTime(&ft, &pst);

	return pst;
}

/* **SYSTEMTIME转time_t */

__time64_t SystemTimeToTimet(const SYSTEMTIME& st, bool toUTC)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	LONGLONG nLL;
	ULARGE_INTEGER ui;

	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;
	nLL = ((LONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;

	__time64_t pt = (long)((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);

	if (toUTC)
	{
		struct tm gm;
		_gmtime64_s(&gm, &pt);
		pt = _mktime64(&gm);
	}

	return pt;
}

bool SeparateInt64(INT64 llSize, int& GB, int& MB, int& kB, int& Byte)
{
	if (llSize < 0) return false;

	GB	=	int(llSize/1000000000);
	MB	= int((llSize-INT64(GB)*1000000000)/1000000);
	kB	=	int((llSize-INT64(GB)*1000000000-MB*1000000)/1000);
	Byte = int((llSize-INT64(GB)*1000000000-MB*1000000-kB*1000));

	return true;
}

bool IntegerToString(INT64 llSize, LPTSTR strOut, DWORD nSize)
{
	if (nSize < 20)  return false;

	int ret = -1;
	int GB		= int(llSize/1000000000);
	int MB		= int(llSize/1000000%1000);
	int kB		= int(llSize/1000%1000);
	int Byte	= int(llSize%1000);
	if (GB > 0)
	{
		ret = _stprintf_s(strOut, nSize, _T("%d,%03d,%03d,%03d"), GB, MB, kB, Byte);
	} 
	else if (MB > 0)
	{
		ret = _stprintf_s(strOut, nSize, _T("%d,%03d,%03d"), MB, kB, Byte);
	}
	else if (kB > 0)
	{
		ret = _stprintf_s(strOut, nSize, _T("%d,%03d"), kB, Byte);
	}
	else
	{
		ret = _stprintf_s(strOut, nSize, _T("%d"), Byte);
	}
	return (ret != -1);
}

double ArrayMax(double* arr, int len, int* retIdx)
{
	double dmax = std::numeric_limits<double>::min();
	int idx = -1;
	for (int i = 1; i < len; i++)
	{
		if (arr[i] > dmax)
		{
			dmax = arr[i]; idx = i;
		}
	}
	if (retIdx) *retIdx = idx;

	return dmax;
}

bool EnumHostIPv4Addr(std::vector<std::string>& ipvec)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	bool bRet = false;

	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO) * 3);
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO) * 3;

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		ipvec.clear();
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			//if (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
			ipvec.emplace_back(pAdapter->IpAddressList.IpAddress.String);
			pAdapter = pAdapter->Next;
		}
		bRet = true;
	}
	else {
		bRet = false;
	}
	free(pAdapterInfo);
	return bRet;
}

// LPCTSTR FindHostIPv4Addr(const std::vector<std::string>& ipvec, bool biggest)
// {
// 	if (ipvec.size() == 0)   return nullptr;
// 
// 	static TCHAR ipaddr[16] = { 0 };
// 	int idx = -1, num = 0; DWORD dwIP = 0;
// 	IN_ADDR addr;
// 
// 	for each (auto& var in ipvec)
// 	{
// 		++num;
// 		inet_pton(AF_INET, var.c_str(), &addr);
// 		ULONG ul = ntohl(addr.S_un.S_addr);
// 		if ((num == 1) || (biggest && ul > dwIP) || (!biggest && ul < dwIP))
// 		{
// 			dwIP = ul;
// 			idx = num - 1;
// 		}
// 	}
// 	addr.S_un.S_addr = htonl(dwIP);
// 
// 	if (InetNtop(AF_INET, &addr, ipaddr, 16))
// 		return ipaddr;
// 	else
// 		return nullptr;
// }
