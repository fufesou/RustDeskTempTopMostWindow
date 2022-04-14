#include <stdio.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

#pragma comment(lib, "shlwapi.lib")


typedef WINBASEAPI BOOL(WINAPI* CreateProcessHid)(
	_In_opt_ LPCWSTR lpApplicationName,
	_Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOW lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation
	);

void PrintError(const TCHAR* header)
{
	TCHAR msg[256] = { 0, };
	DWORD code = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		msg, (sizeof(msg) / sizeof(wchar_t)), NULL);

	TCHAR buf[1024] = { 0, };
	_sntprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, _T("Failed %s, 0x%x, %s"), header, code, msg);
	_tprintf(buf);
}

BOOL InjectDll(HANDLE hProcess, HANDLE hThread, const TCHAR* path)
{
	size_t strSize = (_tcslen(path) + 1) * sizeof(TCHAR);
	LPVOID pBuf = VirtualAllocEx(hProcess, 0, strSize, MEM_COMMIT, PAGE_READWRITE);
	if (pBuf == NULL)
	{
		PrintError(_T("VirtualAllocEx"));
		return FALSE;
	}

	SIZE_T written;
	if (!WriteProcessMemory(hProcess, pBuf, path, strSize, &written))
	{
		PrintError(_T("WriteProcessMemory"));
		return FALSE;
	}

	HMODULE hmodule = GetModuleHandle(_T("kernel32"));
	if (NULL == hmodule)
	{
		PrintError(_T("GetModuleHandle"));
		return FALSE;
	}

#ifdef _UNICODE
	LPVOID pLoadLibrary = GetProcAddress(hmodule, "LoadLibraryW");
#else
	LPVOID pLoadLibrary = GetProcAddress(hmodule, "LoadLibraryA");
#endif
	if (NULL == pLoadLibrary)
	{
		PrintError(_T("GetProcAddress"));
		return FALSE;
	}

	DWORD APCRet = QueueUserAPC((PAPCFUNC)pLoadLibrary, hThread, (ULONG_PTR)pBuf);
	if (0 == APCRet)
	{
		PrintError(_T("QueueUserAPC"));
		return FALSE;
	}
	return TRUE;
}

BOOL GetExecutableDir(TCHAR* dir, int maxLen)
{
	if (0 == GetModuleFileName(nullptr, dir, maxLen))
	{
		PrintError(_T("GetModuleFileName"));
		return FALSE;
	}
	PathRemoveFileSpec(dir);
	return TRUE;
}

typedef BOOL(WINAPI* SetWindowBand)(HWND hWnd, HWND hwndInsertAfter, DWORD dwBand);

int main(int argc, char* argv[])
{
	TCHAR dir[MAX_PATH] = { 0, };
	if (FALSE == GetExecutableDir(dir, sizeof(dir) / sizeof(dir[0])))
	{
		return 1;
	}

	TCHAR path[MAX_PATH] = { 0, };
	_sntprintf_s(path, sizeof(path) / sizeof(path[0]), _TRUNCATE, _T("%s\\WindowInjection.dll"), dir);

	STARTUPINFO startInfo = { 0 };
	PROCESS_INFORMATION procInfo = { 0 };

	//TCHAR cmdline[MAX_PATH] = { 0, };
	// _sntprintf_s(cmdline, sizeof(cmdline) / sizeof(cmdline[0]), _TRUNCATE, _T("%s\\MiniBroker.exe"), dir);
	TCHAR cmdline[] = L"C:\\Windows\\System32\\RuntimeBroker.exe";

	startInfo.cb = sizeof(startInfo);

	if (CreateProcess(nullptr, cmdline, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startInfo, &procInfo))
	{
		(VOID)InjectDll(procInfo.hProcess, procInfo.hThread, path);
		ResumeThread(procInfo.hThread);
	}
	else
	{
		PrintError(_T("CreateProcess"));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
	const TCHAR* WindowTitle = _T("RustDeskPrivacyWindow");
	HWND hwnd = FindWindow(NULL, WindowTitle);
	if (hwnd == NULL)
	{
		PrintError(_T("FindWindow"));
	}
	else
	{
		printf("now hide window\n");
		ShowWindow(hwnd, SW_HIDE);
		std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));

		printf("now show window\n");
		ShowWindow(hwnd, SW_SHOW);
		std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));

		std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000));
		printf("now destroy window\n");
		PostMessage(hwnd, WM_CLOSE, NULL, NULL);
	}

	return 0;
}
