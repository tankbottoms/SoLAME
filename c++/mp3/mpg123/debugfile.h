// DebugFile.h: interface for the CDebugFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUGFILE_H__7F0DEF46_A989_11D2_86DF_00A0C9777D43__INCLUDED_)
#define AFX_DEBUGFILE_H__7F0DEF46_A989_11D2_86DF_00A0C9777D43__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define OUTPUTLENGTH 256
#include <windows.h>
#include <stdarg.h>

class CDebugFile  
{
	HANDLE m_hFile;
	WCHAR*  m_wsz;
public:
	CDebugFile();
	virtual ~CDebugFile();
	bool Open(LPCWSTR wszFileName);
	void Output(LPCWSTR wszFormat, ...);
};

#endif // !defined(AFX_DEBUGFILE_H__7F0DEF46_A989_11D2_86DF_00A0C9777D43__INCLUDED_)
