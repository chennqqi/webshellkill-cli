#include "pch.h"
#include <iostream>
#include <string>
#include <cstring>

#include <easyhook.h>

int main(int argc, char* argv[])
{
	DWORD processId;
	STARTUPINFO stStartUpInfo;
	memset(&stStartUpInfo, 0, sizeof(stStartUpInfo));
	stStartUpInfo.cb = sizeof(stStartUpInfo);

	PROCESS_INFORMATION stProcessInfo;
	memset(&stProcessInfo, 0, sizeof(stProcessInfo));
	CreateProcess(
		".\\WebShellKill.exe",
		NULL,
		NULL,
		NULL,
		false,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&stStartUpInfo,
		&stProcessInfo);
	processId = stProcessInfo.dwProcessId;
	// Sleep(100);
	WaitForInputIdle(stProcessInfo.hProcess, INFINITE);

	WCHAR* dllToInject = (WCHAR*)(void*) L"HookDLL.dll";
	wprintf(L"Attempting to inject: %s\n\n", dllToInject);

	// Inject dllToInject into the target process Id, passing 
	// freqOffset as the pass through data.
	NTSTATUS nt = RhInjectLibrary(
		processId,   // The process to inject into
		0,           // ThreadId to wake up upon injection
		EASYHOOK_INJECT_DEFAULT,
		dllToInject, // 32-bit
		NULL,    // 64-bit not provided
		NULL, // data to send to injected DLL entry point
		0// size of data to send
	);

	if (nt != 0)
	{
		printf("RhInjectLibrary failed with error code = %d\n", nt);
		PWCHAR err = RtlGetLastErrorString();
		std::wcout << err << "\n";
	}
	else
	{
		std::wcout << L"Library injected successfully.\n";
	}

	return 0;
}