// Codec.h: interface for the CCodec class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CODEC_H__1A413A45_45E4_11D3_8D95_0090276149F2__INCLUDED_)
#define AFX_CODEC_H__1A413A45_45E4_11D3_8D95_0090276149F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DssCodec.h"
#include "MP3SourceInfo.h"
#include "mpg123.h"

class CMP3Codec : public CUnknown, public CodecBase
{
friend class CDriver;
private:
	CMP3Codec(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate dsr, ChannelType ct); //Can't construct directly, use CCodec::CreateInstance

public:
	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN(IDssCodec)

	virtual ~CMP3Codec();
	static CMP3Codec* CreateInstance(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate dsr, ChannelType ct);
	HRESULT SetConfiguration(DWORD dwSrcIndex,DWORD dwDstIndex,DownSampleRate dsr,ChannelType ct);
	HRESULT FinalConstruct();
	void FinalRelease();

//Interface Methods
	virtual HRESULT STDCALL Initialize(IDssInputStream*, IDssOutputStream*);
	virtual HRESULT STDCALL Convert(bool bFirstTime);
	virtual HRESULT STDCALL SetFrame(DWORD dwFrame); 
	virtual HRESULT STDCALL SetChapter(DWORD dwChapter);  //For Audible
	virtual HRESULT STDCALL Advise(IDssEvent* pEvent); 
	virtual HRESULT STDCALL SetCallbackRate(OutputStreamEvent event, DWORD dwRate);
	virtual HRESULT STDCALL GetCodecInfo(IDssCodecInfo** ppICodecInfo);

private:
	DownSampleRate m_dsrate;
	ChannelType m_channeltype;
	CMP3CodecInfo* m_pCodecInfo;
	DWORD m_dwSrcIndex;
	DWORD m_dwDstIndex;
};



#endif // !defined(AFX_CODEC_H__1A413A45_45E4_11D3_8D95_0090276149F2__INCLUDED_)
