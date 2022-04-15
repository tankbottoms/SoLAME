// MP3SourceInfo.h: interface for the CMP3SourceInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MP3SOURCEINFO_H__AA2AABBE_4917_11D3_8D96_0090276149F2__INCLUDED_)
#define AFX_MP3SOURCEINFO_H__AA2AABBE_4917_11D3_8D96_0090276149F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Dss.h"


class CMP3SourceInfo : public CUnknown, public IDssSourceInfo
{
public:
	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN(IDssSourceInfo)

	CMP3SourceInfo() {};	
	virtual ~CMP3SourceInfo() {};

	virtual HRESULT FinalConstruct();
	virtual void FinalRelease();

	virtual HRESULT STDCALL NondelegatingQueryInterface(const IID& iid, void** ppv);

	//static HRESULT CreateInstance(IDssSourceInfo** ppIInfo);

	virtual HRESULT SetSrcIndex(DWORD dwSrcIndex);
	virtual HRESULT SetDuration(TimeInfo duration);
	virtual HRESULT SetFrameInfo(FrameInfo info);
	virtual HRESULT SetInputFrameSize(DWORD dwFrameSize);
	virtual HRESULT SetOutputFrameSize(DWORD dwFrameSize);
	virtual HRESULT SetCurrentFrame(DWORD dwFrameNum);
	virtual HRESULT SetNumFrames(DWORD dwNumFrames);

//Interface Methods
	virtual HRESULT STDCALL GetDriver(/*out*/ IDssDriver** ppIDriver);				// c f
	virtual HRESULT STDCALL GetSrcIndex(/*out*/ DWORD* pdwSrcIndex);				// c f		
	virtual HRESULT STDCALL GetDuration(/*in/out*/ TimeInfo* pDuration);			// c f
	virtual HRESULT STDCALL GetFrameInfo(/*in/out*/ FrameInfo* pFrameInfo);			// c f*
	virtual HRESULT STDCALL GetInputFrameSize(DWORD* pdwFrameSize);						// c f
	virtual HRESULT STDCALL GetOutputFrameSize(DWORD* pdwFrameSize);
	virtual HRESULT STDCALL GetFileInfoMap(/*out*/ IDssDictionary** ppIDict);		// c f

protected:
	DWORD m_dwSrcIndex;
	TimeInfo m_Duration;
	FrameInfo m_FrameInfo;
	DWORD m_dwInFrameSize;
	DWORD m_dwOutFrameSize;
	IDssDictionary* m_pIDict;
};

class CMP3CodecInfo : public CMP3SourceInfo, public IDssCodecInfo
{
public:
	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN(IDssCodecInfo)

	CMP3CodecInfo() {};
	~CMP3CodecInfo() {};

	virtual HRESULT STDCALL NondelegatingQueryInterface(const IID& iid, void** ppv);

	virtual HRESULT SetDstIndex(DWORD dwDstIndex);
	virtual HRESULT SetCurrentTimeInfo(TimeInfo CurrentTime);

//Interface Methods
	virtual HRESULT STDCALL GetDstIndex(/*out*/ DWORD* pdwDstIndex);				// c
	virtual HRESULT STDCALL GetCurrentTimeInfo(/*in/out*/ TimeInfo* pCurrentTime);	// c

protected:
	DWORD m_dwDstIndex;
	TimeInfo m_CurrentTime;
};

#endif // !defined(AFX_MP3SOURCEINFO_H__AA2AABBE_4917_11D3_8D96_0090276149F2__INCLUDED_)
