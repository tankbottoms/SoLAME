// MP3SourceInfo.cpp: implementation of the CMP3SourceInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "MP3SourceInfo.h"
#include "Driver.h"
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// CMP3SourceInfo Implementation
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
HRESULT CMP3SourceInfo::FinalConstruct()
{
	return DssCreateDictionary(&m_pIDict);
}

void CMP3SourceInfo::FinalRelease()
{
	CUnknown::FinalRelease();
	m_pIDict->Release();
}

/*
HRESULT CMP3SourceInfo::CreateInstance(IDssSourceInfo** ppIInfo)
{
	CMP3SourceInfo* pInfo = new CMP3SourceInfo;
	return ::InternalCreateInstance(pInfo,ppIInfo);
}
*/

HRESULT CMP3SourceInfo::SetSrcIndex(DWORD dwSrcIndex)
{
	m_dwSrcIndex = dwSrcIndex;
	return S_OK;
}

HRESULT CMP3SourceInfo::SetDuration(TimeInfo duration)
{
	m_Duration = duration;
	return S_OK;
}

HRESULT CMP3SourceInfo::SetFrameInfo(FrameInfo info)
{
	m_FrameInfo = info;
	return S_OK;
}

HRESULT CMP3SourceInfo::SetInputFrameSize(DWORD dwFrameSize)
{
	m_dwInFrameSize = dwFrameSize;
	return S_OK;
}

HRESULT CMP3SourceInfo::SetOutputFrameSize(DWORD dwFrameSize)
{
	m_dwOutFrameSize = dwFrameSize;
	return S_OK;
}


HRESULT CMP3SourceInfo::SetCurrentFrame(DWORD dwFrameNum)
{
	m_FrameInfo.CurrentFrame = dwFrameNum;
	return S_OK;
}

HRESULT CMP3SourceInfo::SetNumFrames(DWORD dwNumFrames)
{
	m_FrameInfo.NumFrames = dwNumFrames;
	return S_OK;
}


extern CDriver* g_pDriver;
//Interface Methods
HRESULT STDCALL CMP3SourceInfo::GetDriver(/*out*/ IDssDriver** ppIDriver)
{
	if(ppIDriver == NULL) return E_POINTER;
	*ppIDriver = g_pDriver;
	if(g_pDriver)
	{
		g_pDriver->AddRef();
		return S_OK;
	}
	else
		return E_UNEXPECTED;
}

HRESULT STDCALL CMP3SourceInfo::GetSrcIndex(/*out*/ DWORD* pdwSrcIndex)
{
	if(pdwSrcIndex == NULL) return E_POINTER;
	*pdwSrcIndex = m_dwSrcIndex;
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::GetDuration(/*in/out*/ TimeInfo* pDuration)
{
	if(pDuration == NULL) return E_POINTER;
	*pDuration = m_Duration;
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::GetFrameInfo(/*in/out*/ FrameInfo* pFrameInfo)
{
	if(pFrameInfo == NULL) return E_POINTER;
	*pFrameInfo = m_FrameInfo;
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::GetInputFrameSize(DWORD* pdwFrameSize)
{
	if(pdwFrameSize == NULL) return E_POINTER;
	*pdwFrameSize = m_dwInFrameSize;
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::GetOutputFrameSize(DWORD* pdwFrameSize)
{
	if(pdwFrameSize == NULL) return E_POINTER;
	*pdwFrameSize = m_dwOutFrameSize;
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::GetFileInfoMap(/*out*/ IDssDictionary** ppIDict)
{
	if(ppIDict == NULL) return E_POINTER;
	*ppIDict = m_pIDict;
	m_pIDict->AddRef();
	return S_OK;
}

HRESULT STDCALL CMP3SourceInfo::NondelegatingQueryInterface(const IID& iid, void **ppv)
{
	if(IsEqualGuid(iid,IID_IDssSourceInfo))
	{
		IDssSourceInfo* pI = static_cast<IDssSourceInfo*>(this);
		pI->AddRef();
		*ppv = pI;
		return S_OK;
	}
	else
		return CUnknown::NondelegatingQueryInterface(iid,ppv);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// CMP3CodecInfo Implementation
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
HRESULT CMP3CodecInfo::SetDstIndex(DWORD dwDstIndex)
{
	m_dwDstIndex = dwDstIndex;
	return S_OK;
}

HRESULT CMP3CodecInfo::SetCurrentTimeInfo(TimeInfo CurrentTime)
{
	m_CurrentTime = CurrentTime;
	return S_OK;
}

HRESULT STDCALL CMP3CodecInfo::GetDstIndex(/*out*/ DWORD* pdwDstIndex)
{
	if(pdwDstIndex == NULL) return E_POINTER;
	*pdwDstIndex = m_dwDstIndex;
	return S_OK;
}

HRESULT STDCALL CMP3CodecInfo::GetCurrentTimeInfo(/*in/out*/ TimeInfo* pCurrentTime)
{
	if(pCurrentTime == NULL) return E_POINTER;
	*pCurrentTime = m_CurrentTime;
	return S_OK;
}


HRESULT STDCALL CMP3CodecInfo::NondelegatingQueryInterface(const IID& iid, void **ppv)
{
	if(IsEqualGuid(iid,IID_IDssSourceInfo))
	{
		IDssSourceInfo* pI = static_cast<IDssSourceInfo*>(this);
		pI->AddRef();
		*ppv = pI;
		return S_OK;
	}
	else if(IsEqualGuid(iid,IID_IDssCodecInfo))
	{
		IDssCodecInfo* pI = static_cast<IDssCodecInfo*>(this);
		pI->AddRef();
		*ppv = pI;
		return S_OK;
	}
	else
		return CUnknown::NondelegatingQueryInterface(iid,ppv);
}
