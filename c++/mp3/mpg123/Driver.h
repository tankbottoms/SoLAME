// Driver.h: interface for the CDriver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRIVER_H__C06AF67D_3BA8_11D3_8D91_0090276149F2__INCLUDED_)
#define AFX_DRIVER_H__C06AF67D_3BA8_11D3_8D91_0090276149F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Codec.h"

struct VecDstFormat
{
	const STREAMFORMATEX** ppwfx;
	const STREAMFORMATEX* pwfxSrc;
	DWORD dwNumFormats;
};

class CDriver : public CUnknown, public IDssDriver
{
public:
	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN(IDssDriver)

	CDriver() : m_pDstFormats(NULL), m_dwNumSrcFormats(0) {};
	virtual ~CDriver() {};

	virtual HRESULT FinalConstruct();
	virtual void FinalRelease();

	//Implementaion of IODriver interface
	virtual HRESULT STDCALL GetNumSrcFormats(DWORD* pdwNumSrcFormats);
	virtual HRESULT STDCALL GetSrcFormat(DWORD dwSrcIndex,IDssStreamFormat** ppISrcFormat);
	virtual HRESULT STDCALL GetNumDstFormats(DWORD dwSrcIndex,DWORD* pdwNumDstFormats);
	virtual HRESULT STDCALL GetDstFormat(DWORD dwSrcIndex,DWORD dwDstIndex,IDssStreamFormat** ppIDstFormat);
	virtual HRESULT STDCALL CreateCodec(IDssStreamFormat* pISrcFormat, IDssStreamFormat* pIDstFormat, IDssCodec** ppICodec);

	virtual HRESULT STDCALL GetDriverDetails(DriverDetails** ppDetails);
	virtual HRESULT STDCALL GetFileInfo(IDssInputStream* pInputStream,IDssSourceInfo** ppIInfo);

private:
	static CMP3Codec* s_pCodec;
	DWORD m_dwNumSrcFormats; //Number of source formats in conversion table
	VecDstFormat* m_pDstFormats; //Conversion Table
	DriverDetails* m_pDetails; //Driver constants
};

bool SFIsEqual(IDssAudioStreamFormat* pIFormat, const STREAMFORMATEX* sf2);
bool SFIsEqual(IDssAudioStreamFormat* pIFormat1, IDssAudioStreamFormat* pIFormat2);
bool CodecConfiguration(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate& dsr, ChannelType& ct);

#endif // !defined(AFX_DRIVER_H__C06AF67D_3BA8_11D3_8D91_0090276149F2__INCLUDED_)
