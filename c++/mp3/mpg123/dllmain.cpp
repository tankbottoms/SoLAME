#include "dsscodec.h"
#include "driver.h"
#include "drivercrypt.h"

CDriver* g_pDriver = NULL;

void ProcessAttach();
void ProcessDetach();
//////////////////////////////////////////////////////////////////////
//
// DLL Entry Point
//
HRESULT GetDriverAuthenticationData(BYTE** ppData,DWORD* pdwDataLen)
{
	return DriverCrypt::GetAuthenticationData(ppData,pdwDataLen);
}
HRESULT AuthenticateDriver(BYTE* pData,DWORD dwDataLen)
{
	return DriverCrypt::Authenticate(pData,dwDataLen);
}

HRESULT GetDriverEntryPoint(IDssDriver** ppIDriver)
{
	if(!DriverCrypt::IsAuthenticated())
	{
		*ppIDriver = NULL;
		return E_DRIVER_NOT_AUTHENTICATED;
	}
	else if(g_pDriver != NULL)
	{
		*ppIDriver = static_cast<IDssDriver*>(g_pDriver);
		g_pDriver->AddRef();
		return S_OK;
	}
	else
	{
		*ppIDriver = NULL;
		return E_FAIL;
	}
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			ProcessAttach();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			ProcessDetach();
			break;
    }
    return TRUE;
}

void ProcessAttach()
{
	//Create Driver
	g_pDriver = new CDriver;
	if(g_pDriver != NULL)
	{
		g_pDriver->AddRef();
		HRESULT hr = g_pDriver->FinalConstruct();
		g_pDriver->Release();
		if(hr != S_OK)
		{
			delete g_pDriver;
			g_pDriver = NULL;
		}
	}
}

void ProcessDetach()
{
	//delete g_pDriver;
	g_pDriver->Release();
	
}