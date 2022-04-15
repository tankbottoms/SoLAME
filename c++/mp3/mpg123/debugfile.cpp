// DebugFile.cpp: implementation of the CDebugFile class.
//
//////////////////////////////////////////////////////////////////////

#include "DebugFile.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDebugFile::CDebugFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_wsz = new WCHAR[OUTPUTLENGTH];
}

CDebugFile::~CDebugFile()
{
	if(m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	if(m_wsz)
		delete [] m_wsz;
}

bool CDebugFile::Open(LPCWSTR wszFileName)
{
	m_hFile = CreateFileW(wszFileName,
						 GENERIC_WRITE,
						 0,
						 NULL,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL,
						 NULL
						);
	return m_hFile != INVALID_HANDLE_VALUE;
}

void CDebugFile::Output(LPCWSTR wszFormat, ...)
{
	if(m_wsz && (m_hFile != INVALID_HANDLE_VALUE))
	{
		va_list list;
		va_start( list, wszFormat );
		DWORD Bytes;
		
		WriteFile(m_hFile,m_wsz,sizeof(WCHAR)*vswprintf(m_wsz,wszFormat,list),&Bytes,NULL);
		//memset(m_wsz,0,sizeof(WCHAR)*OUTPUTLENGTH);
	}
}
	