// Driver.h: interface for the Driver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRIVER_H__AF572B83_6C7D_4234_823C_FE37387FF658__INCLUDED_)
#define AFX_DRIVER_H__AF572B83_6C7D_4234_823C_FE37387FF658__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <wtypes.h>

class DriverCrypt
{
public:
static HRESULT GetAuthenticationData(BYTE** ppData,DWORD* pdwDataLen);
static HRESULT Authenticate(BYTE* pData,DWORD dwDataLen);
static bool IsAuthenticated();
};

#endif // !defined(AFX_DRIVER_H__AF572B83_6C7D_4234_823C_FE37387FF658__INCLUDED_)
