#include <vcl.h>
#include <windows.h>
#include "cepluginsdk.h"


ExportedFunctions mmt;
DWORD_PTR OldPid = 0;
HANDLE hThread = 0;


HANDLE __stdcall MyRemoteThread
(
HANDLE                 hProcess,
LPSECURITY_ATTRIBUTES  lpThreadAttributes,
SIZE_T                 dwStackSize,
LPTHREAD_START_ROUTINE lpStartAddress,
LPVOID                 lpParameter,
DWORD_PTR              dwCreationFlags,
LPDWORD                lpThreadId
)
{
DWORD_PTR fakeStartAdr = (DWORD_PTR)GetModuleHandle("ntdll.dll");

HANDLE hThread = CreateRemoteThread(hProcess,lpThreadAttributes,dwStackSize,(LPTHREAD_START_ROUTINE)fakeStartAdr,lpParameter,CREATE_SUSPENDED,lpThreadId);
if(hThread)
{

CONTEXT ctx;
ctx.ContextFlags = CONTEXT_INTEGER;

GetThreadContext(hThread,&ctx);

#ifdef _WIN64
ctx.Rcx = (DWORD_PTR)lpStartAddress;
#else
ctx.Eax = (DWORD_PTR)lpStartAddress;
#endif

SetThreadContext(hThread,&ctx);
ResumeThread(hThread);
return hThread;
}
return 0;
}


void PatcherThread()
{
while(true)
{
if(*(DWORD_PTR*)(mmt.OpenedProcessID) != OldPid)
{
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,*(DWORD_PTR*)(mmt.OpenedProcessID));
if(hProcess)
{
DWORD_PTR pAddrContext = (DWORD_PTR)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtGetContextThread");
DWORD_PTR pAddrQuery = (DWORD_PTR)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryObject");

#ifdef _WIN64
BYTE pRet[2] = {0xC3,0x90};
pAddrContext += 0x12;
pAddrQuery   += 0x12;
WriteProcessMemory(hProcess,(PVOID)pAddrContext,pRet,2,0);
WriteProcessMemory(hProcess,(PVOID)pAddrQuery,pRet,2,0);
pAddrContext += 0x03;
pAddrQuery   += 0x03;
WriteProcessMemory(hProcess,(PVOID)pAddrContext,pRet,2,0);
WriteProcessMemory(hProcess,(PVOID)pAddrQuery,pRet,2,0);
#else
pAddrContext += 0x0A;
pAddrQuery   += 0x0A;
BYTE pRet1[3] = {0xC2,0x08,0x00};
BYTE pRet2[3] = {0xC2,0x14,0x00};
WriteProcessMemory(hProcess,(PVOID)pAddrContext,pRet1,3,0);
WriteProcessMemory(hProcess,(PVOID)pAddrQuery,pRet2,3,0);
#endif
OldPid = *(DWORD_PTR*)(mmt.OpenedProcessID);
CloseHandle(hProcess);
}
}
Sleep(1000);
}
}


void __stdcall mainmenuplugin(void)
{
if(!hThread)
{
hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)PatcherThread,0,0,0);
mmt.ShowMessage("Thread Baslatildi! Tekrar Durdurmak Icin Buraya Tiklayin!");
}
else
{
TerminateThread(hThread,0);
hThread = 0;
mmt.ShowMessage("Thread Durduruldu! Tekrar Baslatmak Icin Buraya Tiklayin!");
}
}
BOOL APIENTRY DllMain( HANDLE hModule,
					   DWORD_PTR  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

    return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall CEPlugin_GetVersion(PPluginVersion pv , int sizeofpluginversion)
{
	pv->version = CESDK_VERSION;
	pv->pluginname = "mmtylcn27";
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall CEPlugin_InitializePlugin(PExportedFunctions ef , int pluginid)
{
	MAINMENUPLUGIN_INIT init1;

	PVOID pAdr = ef->CreateRemoteThread;
	*(DWORD_PTR*)(pAdr) = (DWORD_PTR)&MyRemoteThread;

	init1.name="Mmt CE Plugin";
	init1.callbackroutine=mainmenuplugin;
	ef->RegisterFunction(pluginid, ptMainMenu, &init1);

	mmt = *ef;
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall CEPlugin_DisablePlugin(void)
{
	return TRUE;
}
