// HookDLL.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "WebShellKillData.h"
#include "WebShellKillConfig.h"
#include <string>
#include <iostream>
#include <shlobj.h>
#include <Windows.h>
#include <tlhelp32.h>
#include "detours.h"
#include <shellapi.h>
#include "CLI11.hpp"
#pragma comment (lib, "detours.lib")

WebShellKillData data;

LPITEMIDLIST ParsePidlFromPath(LPCSTR path) {
	OLECHAR szOleChar[MAX_PATH];
	LPSHELLFOLDER lpsfDeskTop;
	LPITEMIDLIST lpifq;
	ULONG ulEaten, ulAttribs;
	HRESULT hres;
	SHGetDesktopFolder(&lpsfDeskTop);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, -1, szOleChar, sizeof(szOleChar));
	hres = lpsfDeskTop->ParseDisplayName(NULL, NULL, szOleChar, &ulEaten, &lpifq, &ulAttribs);
	hres = lpsfDeskTop->Release();
	if (FAILED(hres))
		return NULL;
	return lpifq;
}

LRESULT CALLBACK hookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	return data.hookCallback(nCode, wParam, lParam);
}

static LPITEMIDLIST(WINAPI *oldSHBrowseForFolderA)(LPBROWSEINFOA lpbi) = SHBrowseForFolderA;
LPITEMIDLIST WINAPI hookSHBrowseForFolderA(LPBROWSEINFOA lpbi)
{
	LPITEMIDLIST pList = ParsePidlFromPath("D:\\Website\\3");
	HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, hookCallback, nullptr, GetCurrentThreadId());
	return pList;
}

static LSTATUS(WINAPI *oldRegQueryValueExA)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) = RegQueryValueExA;
LSTATUS WINAPI hookRegQueryValueExA(HKEY hKey, LPSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE  lpData, LPDWORD lpcbData) {
	printf("%s\n", lpValueName);
	return oldRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}


static LPTSTR(WINAPI *oldGetCommandLineA)() = GetCommandLineA;
// v2.0.9, 001329FB. Don't let it get argv
LPTSTR WINAPI hookGetCommandLineA()
{
	LPTSTR ret = new char[MAX_PATH];
	LPWSTR *szArglist;
	int nArgs;
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	sprintf_s(ret, lstrlenW(szArglist[0]) + 1, "%ls", szArglist[0]);
	return ret;
}

void Hijack() {

	data.initialize();
	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		AllocConsole();
	}
	freopen("CONOUT$", "w", stdout);
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void **)&oldSHBrowseForFolderA, hookSHBrowseForFolderA);
	DetourAttach((void **)&oldRegQueryValueExA, hookRegQueryValueExA);
	DetourAttach((void **)&oldGetCommandLineA, hookGetCommandLineA);
	DetourTransactionCommit();
	
	WebShellKillConfig::initialize();

	return;
}
