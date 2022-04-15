// Driver.cpp: implementation of the CDriver class.
//
//////////////////////////////////////////////////////////////////////

#include "Driver.h"
#include "Codec.h"
#include "mpg123.h"
#include "MP3SourceInfo.h"

LPCTSTR g_szDriverName = TEXT("MPEG-1 Layer III Audio Decoder");
LPCTSTR g_szDriverManufacturer = TEXT("Interactive Objects, Inc.");
LPCTSTR g_szDriverVersion = TEXT("1.56");
LPCTSTR g_szDriverCopyright = TEXT("Copyright © 1999 Interactive Objects, Inc.");
LPCTSTR g_szDriverFileExtensions = TEXT("MP3 Audio File\0*.MP3\0\0");

//follow rules for lpstrFilter member of OPENFILENAME struct in Win32
const STREAMFORMATEX g_cConversionTable[] =
{	
	{ FORMAT_MP3, 2, 22050, 0, 16, 0 }, //0
	{ FORMAT_MP3, 2, 44100, 0, 16, 0 }, //1
	{ FORMAT_MP3, 1, 44100, 0, 16, 0 }, //2
	{ FORMAT_MP3, 1, 22050, 0, 16, 0 }, //3
	{ FORMAT_PCM, 2, 44100, 0, 16, 0 }, //4
	{ FORMAT_PCM, 2, 22050, 0, 16, 0 }, //5
	{ FORMAT_PCM, 2, 11025, 0, 16, 0 }, //6
	{ FORMAT_PCM, 1, 44100, 0, 16, 0 }, //7
	{ FORMAT_PCM, 1, 22050, 0, 16, 0 }, //8
	{ FORMAT_PCM, 1, 11025, 0, 16, 0 }, //9
};

const int g_cConversionTableIndex[] = 
{ 4,						//Num Src Formats
  0,						//Format for Src 1
  4,						//Num Dst Formats for Src 1
  5, 6, 8, 9,				//Dst Formats for Src 1
  1,						//Format for Src 2
  6,						//Num Dst Formats for Src 2
  4, 5, 6, 7, 8, 9,			//Dst Formats for Src 2
  2,						//Format for Src 3
  2,						//Num Dst Formats for Src 3
  7, 8,						//Dst Formats For Src 3
  3,						//Format for Src 4
  2,						//Num Dst Formats For Src 4
  8, 9						//Dst Formats For Src 4
};

CMP3Codec* CDriver::s_pCodec = NULL;

bool CodecConfiguration(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate& dsr, ChannelType& ct)
{
	switch(dwSrcIndex)
	{
	case 0 :
		{
			switch(dwDstIndex)
			{
			case 0:
				dsr = NO_DOWNSAMPLING;
				ct = BOTH_CHANNELS;
				return true;
			case 1:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = BOTH_CHANNELS;
				return true;
			case 2:
				dsr = NO_DOWNSAMPLING;
				ct = FORCE_MONO;
				return true;
			case 3:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = FORCE_MONO;
				return true;
			}
			break;
		}
	case 1 :
		{
			switch(dwDstIndex)
			{
			case 0:
				dsr = NO_DOWNSAMPLING;
				ct = BOTH_CHANNELS;
				return true;
			case 1:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = BOTH_CHANNELS;
				return true;
			case 2:
				dsr = DOWNSAMPLE_4_TO_1;
				ct = BOTH_CHANNELS;
				return true;
			case 3:
				dsr = NO_DOWNSAMPLING;
				ct = FORCE_MONO;
				return true;
			case 4:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = FORCE_MONO;
				return true;
			case 5:
				dsr = DOWNSAMPLE_4_TO_1;
				ct = FORCE_MONO;
				return true;
			}
			break;
		}
	case 2 :
		{
			switch(dwDstIndex)
			{
			case 0: 
				dsr = NO_DOWNSAMPLING;
				ct = FORCE_MONO;
				return true;
			case 1:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = FORCE_MONO;
				return true;
			}
			break;
		}
	case 3 :
		{
			switch(dwDstIndex)
			{
			case 0:
				dsr = NO_DOWNSAMPLING;
				ct = FORCE_MONO;
				return true;
			case 1:
				dsr = DOWNSAMPLE_2_TO_1;
				ct = FORCE_MONO;
				return true;
			}
			break;
		}
	}
	return false;
}

void CopyStreamFormat(const STREAMFORMATEX* sfx, IDssAudioStreamFormat* pIFormat)
{
	pIFormat->SetBitsPerSample(sfx->wBitsPerSample);
	pIFormat->SetBytesPerSec(sfx->nBytesPerSec);
	pIFormat->SetChannels(sfx->nChannels);
	pIFormat->SetFormatTag(sfx->wFormatTag);
	pIFormat->SetSamplingFrequency(sfx->nSamplesPerSec);
}

bool SFIsEqual(IDssAudioStreamFormat* pIFormat1, IDssAudioStreamFormat* pIFormat2)
{
	// TODO: verify audio stream formats.
/*
	if(sf1->nChannelFilter && sf2->nChannelFilter && 
		(sf1->nChannelFilter != sf2->nChannelFilter) )
		return false;
	else*/
	if(pIFormat1->GetChannels() && pIFormat2->GetChannels() &&
			(pIFormat1->GetChannels() != pIFormat2->GetChannels() ) )
			return false;
	else if(pIFormat1->GetSamplingFrequency() && pIFormat2->GetSamplingFrequency() &&
			(pIFormat1->GetSamplingFrequency() != pIFormat2->GetSamplingFrequency() ) )
			return false;
	else if(pIFormat1->GetBitsPerSample() && pIFormat2->GetBitsPerSample() &&
			(pIFormat1->GetBitsPerSample() != pIFormat2->GetBitsPerSample() ) )
			return false;
	else if(pIFormat1->GetFormatTag() && pIFormat2->GetFormatTag() &&
			(pIFormat1->GetFormatTag() != pIFormat2->GetFormatTag() ) )
			return false;
	else return true;

}

bool SFIsEqual(IDssAudioStreamFormat* pIFormat, const STREAMFORMATEX* sf2)
{
/*
	if(sf1->nChannelFilter && sf2->nChannelFilter && 
		(sf1->nChannelFilter != sf2->nChannelFilter) )
		return false;
	else*/
	if(pIFormat->GetChannels() && sf2->nChannels &&
			(pIFormat->GetChannels() != sf2->nChannels) )
			return false;
	else if(pIFormat->GetSamplingFrequency() && sf2->nSamplesPerSec &&
			(pIFormat->GetSamplingFrequency() != sf2->nSamplesPerSec) )
			return false;
	else if(pIFormat->GetBitsPerSample() && sf2->wBitsPerSample &&
			(pIFormat->GetBitsPerSample() != sf2->wBitsPerSample) )
			return false;
	else if(pIFormat->GetFormatTag() && sf2->wFormatTag &&
			(pIFormat->GetFormatTag() != sf2->wFormatTag) )
			return false;
	else return true;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
HRESULT CDriver::FinalConstruct()
{
	//Construct conversion table
	const int* pTable = g_cConversionTableIndex;
	m_dwNumSrcFormats = static_cast<DWORD>(*pTable++);
	m_pDstFormats = new VecDstFormat[m_dwNumSrcFormats];
	if(m_pDstFormats == NULL)
		return E_OUTOFMEMORY;
	for(DWORD i=0;i<m_dwNumSrcFormats;i++)
	{
		VecDstFormat& fmt = m_pDstFormats[i];
		fmt.pwfxSrc = g_cConversionTable+(*pTable++);
		fmt.dwNumFormats = static_cast<DWORD>(*pTable++);
		fmt.ppwfx = new const STREAMFORMATEX*[fmt.dwNumFormats];
		if(fmt.ppwfx == NULL)
			return E_OUTOFMEMORY;
		for(DWORD j=0;j<fmt.dwNumFormats;j++)
			fmt.ppwfx[j] = g_cConversionTable+(*pTable++);
	}

	//Construct Codec
	//Platform Specific
	s_pCodec = CMP3Codec::CreateInstance(1,0,NO_DOWNSAMPLING,BOTH_CHANNELS);

	//Construct Driver Details
	m_pDetails = new DriverDetails;
	m_pDetails->szCopyright = g_szDriverCopyright;
	m_pDetails->szFileExtensions = g_szDriverFileExtensions;
	m_pDetails->szManufacturer = g_szDriverManufacturer;
	m_pDetails->szName = g_szDriverName;
	m_pDetails->szVersion = g_szDriverVersion;

	return S_OK;
}

void CDriver::FinalRelease()
{
	CUnknown::FinalRelease();

	s_pCodec->Release();
	delete m_pDetails;
	for(DWORD i = 0;i<m_dwNumSrcFormats;i++)
		delete [] m_pDstFormats[i].ppwfx;
	delete [] m_pDstFormats;
}

/////////////////////////////////////////////////////////////////////
//  Querying Source/Destination formats supported by Driver
/////////////////////////////////////////////////////////////////////
HRESULT STDCALL CDriver::GetNumSrcFormats(DWORD* pdwNumSrcFormats)
{
	if(pdwNumSrcFormats == NULL) return E_POINTER;
	*pdwNumSrcFormats = m_dwNumSrcFormats;
	return S_OK;
}

HRESULT STDCALL CDriver::GetSrcFormat(DWORD dwSrcIndex,IDssStreamFormat** ppISrcFormat)
{
	if(ppISrcFormat == NULL)	return E_POINTER;
	if(dwSrcIndex >= m_dwNumSrcFormats)	return E_OUTOFRANGE;

//	HRESULT hr = DssCreateStreamFormat(ppISrcFormat);
	IDssAudioStreamFormat* pIAudioSrcFormat;
	HRESULT hr = DssCreateAudioStreamFormat(&pIAudioSrcFormat);
	if(FAILED(hr)) return hr;

	CopyStreamFormat(m_pDstFormats[dwSrcIndex].pwfxSrc, pIAudioSrcFormat);
	*ppISrcFormat = pIAudioSrcFormat;
	return S_OK;
}		

HRESULT STDCALL  CDriver::GetNumDstFormats(DWORD dwSrcIndex,DWORD* pdwNumDstFormats)
{
	if(pdwNumDstFormats == NULL) return E_POINTER;
	if(dwSrcIndex >= m_dwNumSrcFormats)	return E_OUTOFRANGE;

	*pdwNumDstFormats = m_pDstFormats[dwSrcIndex].dwNumFormats;
	return S_OK;
}

HRESULT STDCALL CDriver::GetDstFormat(DWORD dwSrcIndex,DWORD dwDstIndex,IDssStreamFormat** ppIDstFormat)
{
	if(ppIDstFormat == NULL) return E_POINTER;
	if((dwSrcIndex >= m_dwNumSrcFormats) ||
	   (dwDstIndex >= m_pDstFormats[dwSrcIndex].dwNumFormats))
		return E_OUTOFRANGE;

//	HRESULT hr = DssCreateStreamFormat(ppIDstFormat);
	IDssAudioStreamFormat* pIAudioDstFormat;
	HRESULT hr = DssCreateAudioStreamFormat(&pIAudioDstFormat);
	if(FAILED(hr)) return hr;
		
	CopyStreamFormat(m_pDstFormats[dwSrcIndex].ppwfx[dwDstIndex],pIAudioDstFormat);
	*ppIDstFormat = pIAudioDstFormat;
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////
HRESULT STDCALL CDriver::CreateCodec(IDssStreamFormat* pISrcFormat, IDssStreamFormat* pIDstFormat, IDssCodec** ppICodec) 
{
	if((pISrcFormat == NULL ) ||
	   (pIDstFormat == NULL ) || 
	   (ppICodec == NULL) )
	   return E_POINTER;

	//Check to see if singleton codec is in use
	if(s_pCodec->m_cRef > 1)
		return E_TOO_MANY_OBJECTS;

	// Make sure the source and destination streams are audio streams.
	IDssPtr<IDssAudioStreamFormat> pIAudioSrcFormat;
	if (FAILED(pISrcFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioSrcFormat))))
		return E_INVALID_FORMATS;
	IDssPtr<IDssAudioStreamFormat> pIAudioDstFormat;
	if (FAILED(pIDstFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioDstFormat))))
		return E_INVALID_FORMATS;

	//Check if pwfxSrcFormat is in m_pDstFormats
	DWORD dwSrcIndex, dwDstIndex;
	bool bSrcFound(false), bDstFound(false);
	for(dwSrcIndex = 0;dwSrcIndex<m_dwNumSrcFormats;dwSrcIndex++)
	{
//		if(SFIsEqual(pISrcFormat,m_pDstFormats[dwSrcIndex].pwfxSrc))
		if(SFIsEqual(pIAudioSrcFormat,m_pDstFormats[dwSrcIndex].pwfxSrc))
		{
			bSrcFound = true;
			break;
		}
	}

	if(bSrcFound)
	{
		for(dwDstIndex = 0;dwDstIndex < m_pDstFormats[dwSrcIndex].dwNumFormats;dwDstIndex++)
		{
//			if(SFIsEqual(pIDstFormat,m_pDstFormats[dwSrcIndex].ppwfx[dwDstIndex]))
			if(SFIsEqual(pIAudioDstFormat,m_pDstFormats[dwSrcIndex].ppwfx[dwDstIndex]))
			{
				bDstFound = true;
				break;
			}
		}

		if(bDstFound)
		{
			//Create and Initialize codec for given format.
			DownSampleRate dsr;
			ChannelType ct;

			if(!CodecConfiguration(dwSrcIndex,dwDstIndex,dsr,ct))
				return E_FAIL;

			s_pCodec->SetConfiguration(dwSrcIndex,dwDstIndex,dsr,ct);
			*ppICodec = s_pCodec;
			s_pCodec->AddRef();
			return S_OK;
		}
	}
	
	*ppICodec = NULL;
	return E_INVALID_FORMATS;
}

HRESULT STDCALL CDriver::GetDriverDetails(DriverDetails** ppDetails)
{
	if(ppDetails == NULL) return E_POINTER;
	*ppDetails = m_pDetails;
	return S_OK;
}

FillDictionary(IDssInputStream* pIStream, IDssDictionary* pIDict);

extern int test_read_frame_header(frame* fr,IDssInputStream* pIStream,CMP3SourceInfo* pInfo);

HRESULT STDCALL CDriver::GetFileInfo(IDssInputStream* pInputStream,IDssSourceInfo** ppIInfo)
{
	if(pInputStream == NULL || ppIInfo == NULL) return E_POINTER;
	HRESULT hr;
	struct frame l_fr;
	
	*ppIInfo = NULL;

	hr = pInputStream->Seek(0,soStartOfFile);

	if(FAILED(hr))
		return hr;
	
	CMP3SourceInfo* pInfo = new CMP3SourceInfo;
	hr = InternalCreateInstance(pInfo,ppIInfo);
	if(FAILED(hr))
		return hr;

	int iRes = test_read_frame_header(&l_fr,pInputStream,pInfo);

	if(!iRes)
	{
		//DEBUG2(g_pDebugFile,L"GetFileInfo rfh failed\n");
		delete pInfo;
		return E_FILE_WRONG_FORMAT;
	}

	//TODO: Convert from STREAMFORMAT to IDssStreamFormat
	IDssStreamFormat* pIFormat;
	pInputStream->GetFormat(&pIFormat);
	DWORD dwSrcIndex = -1L;
	
	if(pIFormat->GetFormatTag() == FORMAT_MP3 && pIFormat->GetBitsPerSample() == 16)
	{
		switch(((IDssAudioStreamFormat*)pIFormat)->GetChannels())
		{
		case 1:
			if(pIFormat->GetSamplingFrequency() == 22050)
				dwSrcIndex = 3;
			else if(pIFormat->GetSamplingFrequency() == 44100)
				dwSrcIndex = 2;
			break;
		case 2:
			if(pIFormat->GetSamplingFrequency() == 22050)
				dwSrcIndex = 0;
			else if(pIFormat->GetSamplingFrequency() == 44100)
				dwSrcIndex = 1;
			break;
		}
	}
	pIFormat->Release();		
	
	pInfo->SetSrcIndex(dwSrcIndex);

//	pInputStream->Seek(-128,soEndOfFile);
	
	
	//TODO : Complete ID3 and ID3v2 tag support

	IDssDictionary* pIDict;
	pInfo->GetFileInfoMap(&pIDict);

	PopulateID3Info(pInputStream,pIDict);
	pIDict->Release();

	*ppIInfo = static_cast<IDssSourceInfo*>(pInfo);
	//DEBUG2(g_pDebugFile,L"cmn_GetFileInfo succeeded\n");
	return S_OK;
}

