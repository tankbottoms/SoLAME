#include "wincehelper.h"

bool IsEof(HANDLE hFile)
{
	DWORD dwFileSize;
	DWORD dwTmp;
	dwFileSize = GetFileSize(hFile,&dwTmp);
	return (SetFilePointer(hFile,0,NULL,FILE_CURRENT) + 1) < dwFileSize;
}