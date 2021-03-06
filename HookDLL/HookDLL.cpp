// HookDLL.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "global.h"
#include "config.h"

#include "../Global/detours.h"
#include "CLI11.hpp"

#include <shlobj.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <io.h>
#include <shellapi.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <iostream>

#pragma comment (lib, "../Global/detours.lib")

// Make detours happy
#pragma comment(linker, "/EXPORT:noname=_noname,@1,NONAME")
extern "C" __declspec(naked) void __cdecl noname (void){}

static auto oldSHBrowseForFolderA = SHBrowseForFolderA;
static auto oldRegQueryValueExA = RegQueryValueExA;
static auto oldCreateWindowExA = CreateWindowExA;
static auto oldShowWindow = ShowWindow;
static auto oldGetCommandLineA = GetCommandLineA;
bool isWindowInitialized = false;

LPITEMIDLIST parsePIDLFromPath(LPCSTR path) {
	OLECHAR szOleChar[MAX_PATH];
	LPSHELLFOLDER lpsfDeskTop;
	LPITEMIDLIST lpifq;
	ULONG ulEaten, ulAttribs;
	HRESULT hres;
	SHGetDesktopFolder(&lpsfDeskTop);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, -1, szOleChar, sizeof(szOleChar));
	hres = lpsfDeskTop->ParseDisplayName(NULL, NULL, szOleChar, &ulEaten, &lpifq, &ulAttribs);
	hres = lpsfDeskTop->Release();
	if (FAILED(hres)) {
		return NULL;
	}	
	return lpifq;
}

LPITEMIDLIST WINAPI hookSHBrowseForFolderA(LPBROWSEINFOA lpbi)
{
	return parsePIDLFromPath(WebShellKillHook::Global::currentIterator->c_str());
}

LSTATUS WINAPI hookRegQueryValueExA(HKEY hKey, LPSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE  lpData, LPDWORD lpcbData) {
	// printf("%s\n", lpValueName);
	// As symbol of window initialized
	if (strcmp(lpValueName, "ALL_Check_dir") == 0) {
		isWindowInitialized = true;
	}
	auto s = Config::get(std::string(lpValueName));
	if (s == "") {
		return oldRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	}
	else {
		if (lpData != nullptr) {
			lpData = (LPBYTE)s.c_str();
		}
		*lpcbData = s.length();
		return ERROR_SUCCESS;
	}	
}

HWND WINAPI hookCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	HWND ret = oldCreateWindowExA(hWndParent == 0 ? WS_EX_TOOLWINDOW : dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (hWndParent == 0) {
		SetWindowLong(ret, GWL_STYLE, 0);
	}
	if (lpWindowName != nullptr) {
		if (strcmp(lpWindowName, "自定义扫描") == 0) {
			WebShellKillHook::Global::event.emit<HWND>(WebShellKillHook::Global::EVENT_GET_BUTTON_HWND, ret);
		}
	}
	return ret;
}

BOOL WINAPI hookShowWindow(HWND hWnd, int nCmdShow) {
	if (isWindowInitialized) {
		WebShellKillHook::Global::event.emit(WebShellKillHook::Global::EVENT_READY);
		isWindowInitialized = false; // do not initialize twice
	}
	return oldShowWindow(hWnd, 0);
}

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
	FILE *f = nullptr;
	auto hntdll = GetModuleHandle("ntdll.dll");
	auto pwine_get_version = (void *)GetProcAddress(hntdll, "wine_get_version");
	if (!pwine_get_version) {
		auto attached = AttachConsole(ATTACH_PARENT_PROCESS);
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);
	}
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void **)&oldSHBrowseForFolderA, hookSHBrowseForFolderA);
	DetourAttach((void **)&oldRegQueryValueExA, hookRegQueryValueExA);
	DetourAttach((void **)&oldGetCommandLineA, hookGetCommandLineA);
	DetourAttach((void **)&oldCreateWindowExA, hookCreateWindowExA);
	DetourAttach((void **)&oldShowWindow, hookShowWindow);
	DetourTransactionCommit();

	Config::initialize();
	WebShellKillHook::Global::initialize();

}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
	if (DetourIsHelperProcess()) {
		return TRUE;
	}

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Hijack();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourTransactionCommit();
	}

	return TRUE;
}
