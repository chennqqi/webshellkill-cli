// Bootstrap.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <string>
#include <strsafe.h>

#include "../Global/detours.h"

#pragma comment (lib, "../Global/detours.lib")
void ErrorExit(void* lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	printf((LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

int main()
{
	char charFilePath[512];

	GetModuleFileName(NULL, charFilePath, _MAX_PATH);
	auto filePath = std::string(charFilePath);
	auto slashPosition = filePath.rfind('\\');
	auto fileName = filePath.substr(slashPosition + 1);
	auto dir = filePath.substr(0, slashPosition);

	auto webShellKillPath = dir + "\\WebShellKill.exe";
	auto dllPath = dir + "\\HookDLL32.dll";

	auto cmdLine = std::string(GetCommandLine());
	auto fileNamePosition = cmdLine.find(fileName);
	cmdLine.replace(fileNamePosition, fileName.length(), "WebShellKill.exe");
	auto cstrCmdLine = const_cast<char*>(cmdLine.c_str());

	
	STARTUPINFOA info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	GetStartupInfo(&info);
	//	info.hStdOutput = stdout;
	info.dwFlags |= STARTF_USESTDHANDLES;
//	info.dwFlags |= CREATE_NEW_CONSOLE;
//	info.dwFlags |= DETACHED_PROCESS;

//	std::cout << webShellKillPath << std::endl;
//	std::cout << cstrCmdLine << std::endl;
//	std::cout << dllPath << std::endl;
	if (!DetourCreateProcessWithDllEx(webShellKillPath.c_str(), cstrCmdLine, NULL, NULL, 0, NULL, NULL, NULL, &info, &processInfo, dllPath.c_str(), NULL)) {
		printf("CreateProcess failed (%d).\n", GetLastError());
		ErrorExit("CreateProcess");
		return 1;
	}
	WaitForSingleObject(processInfo.hProcess, INFINITE);
	return 0;
}