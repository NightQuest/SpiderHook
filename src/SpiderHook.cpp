﻿#include "preCompiled.h"

using ReportFault_type = EFaultRepRetVal (APIENTRY*)(LPEXCEPTION_POINTERS, DWORD);
ReportFault_type real_ReportFault = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH:
		if( ++instances == 1 )
		{
			hInstance = hModule;

			if( !hFaultRep )
			{
				// Load the dll in the system directory, so our calls can be forwarded
				char tmp[MAX_PATH] = { 0 };
				::GetSystemDirectory(tmp, MAX_PATH);
				strcat_s(tmp, "\\faultrep.dll");
				hFaultRep = ::LoadLibrary(tmp);

				// Make sure we were able to load it
				if( !hFaultRep )
					::ExitProcess(1);

				// Create our function pointer
				real_ReportFault = reinterpret_cast<ReportFault_type>(::GetProcAddress(hFaultRep, "ReportFault"));
				if( !real_ReportFault )
					::ExitProcess(1);
			}

			app.OnAttach();
		}
		break;

	case DLL_PROCESS_DETACH:
		if( --instances == 0 )
		{
			if( hFaultRep )
			{
				::FreeLibrary(hFaultRep);
				hFaultRep = nullptr;
				real_ReportFault = nullptr;
			}

			app.OnDetach();
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

EFaultRepRetVal APIENTRY fake_ReportFault(LPEXCEPTION_POINTERS pep, DWORD dwOpt)
{
	// Make sure we have a function to call, and gracefully error otherwise
	if( !real_ReportFault )
	{
		::MessageBox(nullptr, "Calling ReportFault after it was unloaded!", "Error", MB_OK | MB_ICONERROR);
		return EFaultRepRetVal::frrvErr;
	}

	// Call the real ReportFault
	return real_ReportFault(pep, dwOpt);
}