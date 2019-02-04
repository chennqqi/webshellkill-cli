// Bootstrap.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <string>
#include "../Global/detours.h"

#pragma comment (lib, "../Global/detours.lib")


int main()
{
	char charFilePath[512];

	GetModuleFileName(NULL, charFilePath, _MAX_PATH);
	auto filePath = std::string(charFilePath);
	auto slashPosition = filePath.rfind('\\');
	auto fileName = filePath.substr(slashPosition + 1);
	auto dir = filePath.substr(0, slashPosition);

	auto webShellKillPath = dir + "\\WebShellKill.exe";
	auto dllPath = dir + "\\HookDLL.dll";

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

	if (!DetourCreateProcessWithDllA(webShellKillPath.c_str(), cstrCmdLine, NULL, NULL, 0, NULL, NULL, NULL, &info, &processInfo, dllPath.c_str(), NULL)) {
		std::cout << "Create Process Failed!";
	}
	WaitForSingleObject(processInfo.hProcess, INFINITE);
	return 0;
}