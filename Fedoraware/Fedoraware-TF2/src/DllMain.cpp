#include <Windows.h>
#include "Core/Core.h"
#include "Utils/Minidump/Minidump.h"

DWORD WINAPI MainThread(LPVOID lpParam)
{
	//"mss32.dll" being one of the last modules to be loaded
	//So wait for that before proceeding, after it's up everything else should be too
	//Allows us to correctly use autoinject and just start the game.
	while (!GetModuleHandleW(L"mss32.dll") ||
		!GetModuleHandleW(L"ntdll.dll") ||
		!GetModuleHandleW(L"stdshader_dx9.dll") ||
		!GetModuleHandleW(L"materialsystem.dll"))
	{
		Sleep(2000);
	}

	g_Core.Load();

	while (!g_Core.ShouldUnload())
	{
		Sleep(20);
	}

	g_Core.Unload();

#ifndef _DEBUG
	SetUnhandledExceptionFilter(nullptr);
#endif
	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), EXIT_SUCCESS);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
#ifndef _DEBUG
		SetUnhandledExceptionFilter(Minidump::ExceptionFilter);
#endif

		if (const auto hMainThread = CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr))
		{
			CloseHandle(hMainThread);
		}
	}

	return TRUE;
}
