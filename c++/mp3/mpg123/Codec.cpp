// Codec.cpp: implementation of the CCodec class.
//
//////////////////////////////////////////////////////////////////////

#include "mpg123.h"
#include "options.h"
#include "getbits.h"
#include <mmreg.h>
#include "Codec.h"
#include "Driver.h"

#define DEBUG2(a,b)
#define DEBUG3(a,b,c)
#define DEBUG4(a,b,c,d)
#define DEBUG5(a,b,c,d,e)
#define TIMESTAMP(a)

//////////////////////////////////////////////////////////////////////
//
// Naughty global data
//
extern long g_NumFrames;
extern CDriver* g_pDriver;
extern long g_FrameNum;
extern struct frame fr;

static bool g_bDownSampleBestFit = false;
DownSampleRate g_DownSampleRate = NO_DOWNSAMPLING;
static int force_mono = 0;
int audiobufsize = AUDIOBUFSIZE;

struct audio_info_struct g_ai;

DWORD g_dwFrameProgressRate = 0;
DWORD g_dwFrameProgressStatus = 1;
DWORD g_dwWaveformVisRate = 0;
DWORD g_dwWaveformVisStatus = 1;
DWORD g_dwSpectralVisRate = 0;
DWORD g_dwSpectralVisStatus = 1;
CodecEventInfo* g_pEventInfo = NULL;

DWORD g_dwMaxBufferTime = 0;
DWORD g_dwFramePlayTime = 0;
//////////////////////////////////////////////////////////////////////
//
// Function Prototypes
//
void set_synth_functions(struct frame *fr);
void DoFrameNotify(long frame);
void DoWaveNotify(short* pcm_sample,int samplesperchannel,int channels);
void DoSpectralNotify(real* bandPtrL, real* bandPtrR, int sblimit, int sslimit);
void CreateEventInfo();
void DestroyCodecEventInfo(CodecEventInfo* p);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMP3Codec::CMP3Codec(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate dsr, ChannelType ct) :
m_dwSrcIndex(dwSrcIndex), 
m_dwDstIndex(dwDstIndex), 
m_dsrate(dsr), 
m_channeltype(ct)
{
}

CMP3Codec::~CMP3Codec() {}

#define TEST_MAXIMUM_PLAY
#undef TEST_MAXIMUM_PLAY
#define TEST_MAXIMUM_PLAY_BUFFERS
//#define WRITE_MAXIMUM_PLAY
#define DOWNSAMPLE_4_TO_1_INTERPOLATION

#define FIX_POP
//#undef FIX_POP

#ifdef TEST_MAXIMUM_PLAY
#include "Debugfile.h"
#ifdef WRITE_MAXIMUM_PLAY
CDebugFile g_DebugFile;
#endif
#endif

HRESULT CMP3Codec::FinalConstruct()
{
#if defined(TEST_MAXIMUM_PLAY) && defined(WRITE_MAXIMUM_PLAY)
#ifdef UNDER_CE
	g_DebugFile.Open(L"\\maxplay.txt");
#else
	g_DebugFile.Open(L"D:\\maxplay.txt");
#endif //UNDER_CE
#endif //TEST_MAXIMUM_PLAY && WRITE_MAXIMUM_PLAY
	
	//Create CMP3CodecInfo class	
	m_pCodecInfo = new CMP3CodecInfo;
	IDssCodecInfo* pInfo;
	HRESULT hr = InternalCreateInstance(m_pCodecInfo,&pInfo);
	m_spICodecInfo = pInfo;

#ifdef TEST_MAXIMUM_PLAY
	make_decode_tables(OUTSCALE);
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(0);  //dummy argument for compatibility
#endif

	return hr;
}

void CMP3Codec::FinalRelease()
{
	CUnknown::FinalRelease();
	
	close_stream();

	delete [] pcm_sample;

	if(m_pCodecInfo)
		m_pCodecInfo->Release();
	//Call MP3_Destroy
}

CMP3Codec* CMP3Codec::CreateInstance(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate dsr, ChannelType ct)
{
	CMP3Codec* pCodec = new CMP3Codec(dwSrcIndex,dwDstIndex,dsr,ct);
	if(pCodec != NULL)
	{
		pCodec->AddRef();
		HRESULT hr = pCodec->FinalConstruct();
		pCodec->Release();
		if(FAILED(hr))
		{
			delete pCodec;
			pCodec = NULL;
		}
	}
	return pCodec;
}

HRESULT CMP3Codec::SetConfiguration(DWORD dwSrcIndex, DWORD dwDstIndex, DownSampleRate dsr, ChannelType ct)
{
	m_spIInStream.Release();
	m_spIOutStream.Release();

	m_dwSrcIndex = dwSrcIndex;
	m_dwDstIndex = dwDstIndex;
	m_dsrate = dsr; 
	m_channeltype = ct;
	return S_OK;
}

HRESULT CMP3Codec::SetChapter(DWORD dwChapter)
{
	return (dwChapter == 0) ? S_OK : E_OUTOFRANGE; 
}

HRESULT CMP3Codec::SetCallbackRate(OutputStreamEvent event, DWORD dwRate)
{
	switch(event)
	{
	case oseFrameProgress :
		g_dwFrameProgressRate = dwRate;
		return S_OK;
	case oseWaveformVisualization :
		g_dwWaveformVisRate = dwRate;
		return S_OK;
	case oseSpectralVisualization :
		g_dwSpectralVisRate = dwRate;
		return S_OK;
	}
	return E_FAIL;
}

HRESULT STDCALL CMP3Codec::GetCodecInfo(IDssCodecInfo** ppICodecInfo)
{
	if(ppICodecInfo == NULL) return E_POINTER;
	*ppICodecInfo = m_spICodecInfo;
	if(m_spICodecInfo)
		m_spICodecInfo->AddRef();
	return S_OK;
}

HRESULT STDCALL CMP3Codec::Advise(IDssEvent* pEvent)
{
	if(pEvent == NULL) return E_POINTER;

	IDssCodecEvent* pICodecEvent;
	if (FAILED(pEvent->QueryInterface(IID_IDssCodecEvent, reinterpret_cast<void**>(&pICodecEvent))))
		return E_INVALID_EVENT;
	if(m_spIOutStream)
		m_spIOutStream->Advise(pEvent);
	m_spINotify = pICodecEvent;

	// TODO: Um, should a ref be added?
	pICodecEvent->Release();

	return S_OK;
}

HRESULT STDCALL CMP3Codec::Initialize(IDssInputStream* pIInStream, IDssOutputStream* pIOutStream)
{
	if(pIInStream == NULL || pIOutStream == NULL) return E_POINTER;

	bool bRes;

	m_spIInStream.Release();
	m_spIOutStream.Release();
	close_stream();

	IDssStreamFormat *pIFormat;
	pIInStream->GetFormat(&pIFormat);
	IDssAudioStreamFormat *pIAudioFormat;
	if FAILED(pIFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioFormat)))
	{
		pIFormat->Release();
		return E_INVALID_FORMATS;
	}
	pIFormat->Release();
	
	IDssPtr<IDssStreamFormat> pISrcFormat;
	g_pDriver->GetSrcFormat(m_dwSrcIndex,&pISrcFormat);
	IDssAudioStreamFormat *pIAudioSrcFormat;
	if FAILED(pIFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioSrcFormat)))
	{
		pIAudioFormat->Release();
		return E_INVALID_FORMATS;
	}

//	bRes = SFIsEqual(pIAudioSrcFormat,pISrcFormat);
	bRes = SFIsEqual(pIAudioFormat,pIAudioSrcFormat);
	pIAudioSrcFormat->Release();

	if(!bRes)
	{
		pIAudioSrcFormat->Release();
		return E_INVALID_FORMATS;
	}


	pIOutStream->GetFormat(&pIFormat);
	if FAILED(pIFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioFormat)))
	{
		pIAudioSrcFormat->Release();
		pIFormat->Release();
		return E_INVALID_FORMATS;
	}
	pIFormat->Release();
	IDssPtr<IDssStreamFormat> pIDstFormat;
	g_pDriver->GetDstFormat(m_dwSrcIndex,m_dwDstIndex,&pIDstFormat);
	IDssAudioStreamFormat *pIAudioDstFormat;
	if FAILED(pIDstFormat->QueryInterface(IID_IDssAudioStreamFormat, reinterpret_cast<void**>(&pIAudioDstFormat)))
	{
		pIAudioSrcFormat->Release();
//		pISrcFormat->Release();
		return E_INVALID_FORMATS;
	}
//	bRes = SFIsEqual(pIFormat,pIDstFormat);
	bRes = SFIsEqual(pIAudioFormat,pIAudioDstFormat);
	pIAudioFormat->Release();

	if(!bRes)
	{
//		pISrcFormat->Release();
//		pIDstFormat->Release();
		pIAudioSrcFormat->Release();
		pIAudioDstFormat->Release();
		return E_INVALID_FORMATS;
	}

	m_spIInStream = pIInStream;
	m_spIOutStream = pIOutStream;

	if(m_spINotify)
		pIOutStream->Advise(m_spINotify);

	g_DownSampleRate = m_dsrate;
	force_mono = (m_channeltype < 0) ? 0 : 1;
	fr.stereo = 2; /*just for now*/
	fr.single = m_channeltype;
	fr.synth = synth_1to1;
	g_ai.channels = pIAudioDstFormat->GetChannels();
	g_ai.rate = pIAudioDstFormat->GetSamplingFrequency();
	g_ai.bits = pIAudioDstFormat->GetBitsPerSample();
	g_ai.format = AUDIO_FORMAT_SIGNED_16;
	
#ifndef TEST_MAXIMUM_PLAY
	//Called during FinalConstruct when TEST_MAXIMUM_PLAY is defined
	make_decode_tables(OUTSCALE);
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(g_DownSampleRate);
#endif
	//Place MP3_INitialize Here;

	DEBUG2(g_pDebugFile,L"MP3_Initialize begin\n");
	DEBUG3(g_pDebugFile,L"Initialize - file %s\n",filename);

	open_stream(pIInStream);

	if(!GenerateFrameTable(m_pCodecInfo))
	{
		DEBUG2(g_pDebugFile,L"Initialize - file wrong format\n");
		close_stream();
		return E_FILE_WRONG_FORMAT;
	}

	DEBUG2(g_pDebugFile,L"Initialize - initialize succeeded\n");

	//Setup proper Audio information
	if(fr.stereo != 2)   //mono MP3
		force_mono = 1;

	//Determine buffer size per frame decoded
	audiobufsize = 4608;

#ifdef TEST_MAXIMUM_PLAY
	g_dwFramePlayTime = (audiobufsize * 1000)/(2*44100*fr.stereo);
	g_dwMaxBufferTime = 16*g_dwFramePlayTime;
#ifdef WRITE_MAXIMUM_PLAY
	g_DebugFile.Output(L"FramePlayTime = %d\n",g_dwFramePlayTime);
	g_DebugFile.Output(L"Max BufferTime = %d\n",g_dwMaxBufferTime);
#endif
	//Mark: Testing interpolation
	g_DownSampleRate = DOWNSAMPLE_2_TO_1; //DOWNSAMPLE_4_TO_1;
	//g_DownSampleRate = DOWNSAMPLE_4_TO_1;
#endif
	//Destination waveformat info used for g_ai

	set_synth_functions(&fr);
	pcm_point = 0;
	if(!pcm_sample)
		pcm_sample = new BYTE[audiobufsize];

	return S_OK;
}



extern real buffs[2][2][0x110];

HRESULT STDCALL CMP3Codec::Convert(bool bFirstTime)
{
	bool bEOF(false);
#ifdef TEST_MAXIMUM_PLAY
#define PLAY_HOLD 0
#define PLAY_HIGHER_QUALITY -1
#define PLAY_LOWER_QUALITY 1
	static int dwBufferTime = 100;	
	static int MaxPlayToDo = PLAY_HOLD;
	static int sCounter = 16;

#ifndef TEST_MAXIMUM_PLAY_BUFFERS
	DWORD dwStartTime;
	DWORD dwEndTime;

	dwStartTime = GetTickCount();
#endif //TEST_MAXIMUM_PLAY_BUFFERS
#endif //TEST_MAXIMUM_PLAY

#ifdef FIX_POP
	static int SilenceData = 0;
#endif

	DEBUG2(g_pDebugFile,L"Decode begin\r\n",);
	// Need to check to see if EOF
	if(!read_frame(&fr))
	{
		TIMESTAMP(g_pDebugFile);
		DEBUG2(g_pDebugFile,L"Couldn't read frame\r\n");
		bEOF = true;
	}

	DEBUG3(g_pDebugFile,L"Decode - g_ai.channels = %d\n",g_ai.channels);
	DEBUG3(g_pDebugFile,L"Decode - g_ai.bits = %d\n",g_ai.bits);
	DEBUG3(g_pDebugFile,L"Decode - g_ai.rate = %d\n",g_ai.rate);
	DEBUG3(g_pDebugFile,L"Decode - g_ai.format = %d\n",g_ai.format);

	pcm_point = 0;
	if (fr.error_protection) 
	{
		getbits(16); /* crc */
	}

	//Populate pEventInfo or set to NULL if no events for this frame.

#ifdef TEST_MAXIMUM_PLAY
	if(sCounter)
	{
		sCounter--;
	}
	else if(MaxPlayToDo == PLAY_HIGHER_QUALITY)
	{
		switch(g_DownSampleRate)
		{
		case NO_DOWNSAMPLING:
			break;
		case DOWNSAMPLE_2_TO_1:
			g_DownSampleRate = NO_DOWNSAMPLING;
			break;
		case DOWNSAMPLE_4_TO_1:
			g_DownSampleRate = DOWNSAMPLE_2_TO_1;
			break;
		}
#ifdef WRITE_MAXIMUM_PLAY
		g_DebugFile.Output(L"%d (%d)\n",g_DownSampleRate,g_FrameNum);
#endif //WRITE_MAXIMUM_PLAY
		dwBufferTime = 300;
		set_synth_functions(&fr);
		sCounter = 4;
	}
	else if(MaxPlayToDo == PLAY_LOWER_QUALITY)
	{
		switch(g_DownSampleRate)
		{
		case NO_DOWNSAMPLING:
			g_DownSampleRate = DOWNSAMPLE_2_TO_1;
			break;
		case DOWNSAMPLE_2_TO_1:
			//g_DownSampleRate = DOWNSAMPLE_4_TO_1;
			break;
		case DOWNSAMPLE_4_TO_1:
			break;
		}
#ifdef WRITE_MAXIMUM_PLAY
		g_DebugFile.Output(L"%d (%d)\n",g_DownSampleRate,g_FrameNum);
#endif //WRITE_MAXIMUM_PLAY
		dwBufferTime = 150;
		set_synth_functions(&fr);
		sCounter = 4;
	}

#endif //TEST_MAXIMUM_PLAY

	int clip = (fr.do_layer)(&fr,&g_ai);

#ifdef TEST_MAXIMUM_PLAY
#ifndef TEST_MAXIMUM_PLAY_BUFFERS
	dwEndTime = GetTickCount();
	DWORD duration = dwEndTime - dwStartTime;
	dwBufferTime += g_dwFramePlayTime - duration;

#ifdef WRITE_MAXIMUM_PLAY
	if(sCounter == 3)
		g_DebugFile.Output(L"(%d) %d/%d\n",dwBufferTime,duration,g_DownSampleRate);
#endif //WRITE_MAXIMUM_PLAY
#endif //TEST_MAXIMUM_PLAY_BUFFERS

#ifdef TEST_MAXIMUM_PLAY_BUFFERS
	if(!sCounter)
	{
		DWORD dwNumBuffers,dwBuffersFilled;
		m_spIOutStream->GetBufferInfo(&dwBuffersFilled,&dwNumBuffers);
		if(dwBuffersFilled > (dwNumBuffers/2))
			MaxPlayToDo = PLAY_HIGHER_QUALITY;
		else if(dwBuffersFilled <= (dwNumBuffers/4))
			MaxPlayToDo = PLAY_LOWER_QUALITY;
		else
			MaxPlayToDo = PLAY_HOLD;
	}
#else
	if(dwBufferTime > 400)
	{
#ifdef WRITE_MAXIMUM_PLAY
		//g_DebugFile.Output(L"Request Raise playback quality\n");
#endif	//WRITE_MAXIMUM_PLAY
		//Mark: Commented out for 4_TO_1 downsampling
		MaxPlayToDo = PLAY_HIGHER_QUALITY;
	}
	else if(dwBufferTime <100)
	{
#ifdef WRITE_MAXIMUM_PLAY
		//g_DebugFile.Output(L"Request Lower playback quality\n");
#endif  //WRITE_MAXIMUM_PLAY
		MaxPlayToDo = PLAY_LOWER_QUALITY;
	}
	else
		MaxPlayToDo = PLAY_HOLD;
#endif //TEST_MAXIMUM_PLAY_BUFFERS

	if(g_DownSampleRate == DOWNSAMPLE_2_TO_1)
	{
		//need to upsample contents of pcm_sample
		short* pDownSample = (short*)(pcm_sample + pcm_point) - 1;
		if(fr.stereo == 2)
		{
			//stereo
			short* pUpSampleRight = (short*)(pcm_sample+audiobufsize) - 1;
			short* pUpSampleLeft = pUpSampleRight - 1;
			while(pDownSample >= (short*)pcm_sample)
			{
				*pUpSampleRight = *pDownSample;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample--;
				pUpSampleRight -= 2;
				*pUpSampleLeft =  *pDownSample;
				pUpSampleLeft -= 2;
				*pUpSampleLeft = *pDownSample--;
				pUpSampleLeft -= 2;
			}
		}
		else
		{
			//mono
			short* pUpSample = (short*)(pcm_sample + audiobufsize) - 1;
			while(pDownSample >= (short*)pcm_sample)
			{
				*pUpSample-- = *pDownSample;
				*pUpSample-- = *pDownSample--;
			}
		}
		pcm_point *= 2;
	}
	else if(g_DownSampleRate == DOWNSAMPLE_4_TO_1)
	{
		//need to upsample contents of pcm_sample
		short* pDownSample = (short*)(pcm_sample + pcm_point) - 1;
		if(fr.stereo == 2)
		{
			//stereo
			short* pUpSampleRight = (short*)(pcm_sample+audiobufsize) - 1;
			short* pUpSampleLeft = pUpSampleRight - 1;
			while(pDownSample >= (short*)pcm_sample)
			{
#undef DOWNSAMPLE_4_TO_1_INTERPOLATION
#ifdef DOWNSAMPLE_4_TO_1_INTERPOLATION				
#if 1
				short NextSampleRight, NextSampleLeft, PrevSampleRight, PrevSampleLeft;							

				NextSampleLeft = *pDownSample--;
				NextSampleRight = *pDownSample;				
				
				pDownSample++;
				
				PrevSampleLeft = *pDownSample++;
				PrevSampleRight = *pDownSample;
				
				pDownSample--;				
				
				short increment = (*pDownSample - PrevSampleRight)/4;				
				short DownSampleCopy;

				// Make incremental copy
				DownSampleCopy = *pDownSample;

				*pUpSampleRight = DownSampleCopy;
				pUpSampleRight -= 2;									
				
				DownSampleCopy += increment;
				*pUpSampleRight = DownSampleCopy;
				pUpSampleRight -= 2;		
				
				DownSampleCopy += increment;
				*pUpSampleRight = DownSampleCopy;
				pUpSampleRight -= 2;		
				
				DownSampleCopy += increment;
				*pUpSampleRight = DownSampleCopy;				
				*pDownSample--;
				pUpSampleRight -= 2;				
				
				increment = (*pDownSample - PrevSampleLeft)/4;				

				// Get new value for the left channel
				DownSampleCopy = *pDownSample;
				
				*pUpSampleLeft =  DownSampleCopy;
				pUpSampleLeft -= 2;						
				DownSampleCopy += increment;

				*pUpSampleLeft =  DownSampleCopy;
				pUpSampleLeft -= 2;										
				DownSampleCopy += increment;

				*pUpSampleLeft =  DownSampleCopy;
				pUpSampleLeft -= 2;			
				DownSampleCopy += increment;

				*pUpSampleLeft = DownSampleCopy;				
				*pDownSample--;
				pUpSampleLeft -= 2;
#else
				short i;
				i = (*pDownSample - *(pDownSample-2)) >> 2;
				*pUpSampleRight = *pDownSample--;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample + 3*i;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample + (i << 1);
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample + i;
				pUpSampleRight -= 2;
				i = (*pDownSample - *(pDownSample-2)) >> 2;
				*pUpSampleLeft =  *pDownSample--;
				pUpSampleLeft -= 2;
				*pUpSampleLeft =  *pDownSample + 3*i;
				pUpSampleLeft -= 2;
				*pUpSampleLeft =  *pDownSample + (i << 1);
				pUpSampleLeft -= 2;
				*pUpSampleLeft = *pDownSample + i;
				pUpSampleLeft -= 2;
#endif
#else
				*pUpSampleRight = *pDownSample;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample;
				pUpSampleRight -= 2;
				*pUpSampleRight = *pDownSample--;
				pUpSampleRight -= 2;
				*pUpSampleLeft =  *pDownSample;
				pUpSampleLeft -= 2;
				*pUpSampleLeft =  *pDownSample;
				pUpSampleLeft -= 2;
				*pUpSampleLeft =  *pDownSample;
				pUpSampleLeft -= 2;
				*pUpSampleLeft = *pDownSample--;
				pUpSampleLeft -= 2;
#endif
			}
		}
		else
		{
			//mono
			short* pUpSample = (short*)(pcm_sample + audiobufsize) - 1;
			while(pDownSample >= (short*)pcm_sample)
			{
				*pUpSample-- = *pDownSample;
				*pUpSample-- = *pDownSample;				
				*pUpSample-- = *pDownSample;
				*pUpSample-- = *pDownSample--;
			}
		}
		pcm_point *= 4;
	}

#endif

	if(bEOF)
	{
		DEBUG2(g_pDebugFile,L"At end of file\n");
		DestroyCodecEventInfo(g_pEventInfo);
		g_pEventInfo = NULL;
		return S_FALSE;
	}

	DoFrameNotify(g_FrameNum);

	DEBUG3(g_pDebugFile,L"Decode end %d\r\n",pcm_point);

	DWORD dwBytesWritten;
	if(pcm_point)
	{

	//Bad Matt, Bad.....this causes static.  Only turn on for certain OEMs.
	/*
	if(pcm_point < audiobufsize)
		{
			//Add silence data to end of file;
			short* p = (short*)pcm_sample;
			p += pcm_point/2;
			memset(p,0,audiobufsize-pcm_point);
			pcm_point += 2;
		}
	*/

		if(bFirstTime)
		{
			SilenceData = 10;
			//try blanking out the first frame
			//memset(pcm_sample,0,pcm_point);
		}

		if(SilenceData)
		{
			memset(pcm_sample,0,pcm_point);
			SilenceData--;
		}

		m_spIOutStream->Write(pcm_sample,pcm_point,g_pEventInfo,&dwBytesWritten);
		g_pEventInfo = NULL;
	}
	else
	{
		//Destroy g_pEventInfo if it exists;
		DestroyCodecEventInfo(g_pEventInfo);
		g_pEventInfo = NULL;
	}
	
	if(bEOF)
	{
		DEBUG2(g_pDebugFile,L"At end of file\n");
		return S_FALSE;
	}

	++g_FrameNum;
	m_pCodecInfo->SetCurrentFrame(g_FrameNum);

	return S_OK;
}

HRESULT STDCALL CMP3Codec::SetFrame(DWORD dwFrame)
{
	if(!m_spIInStream) return E_UNEXPECTED;
	if(::SetFrame(dwFrame))
	{
		DEBUG2(g_pDebugFile,L"MP3_SetFrame - SetFrame succeeded\r\n");
		return S_OK;
	}
	else
	{
		DEBUG2(g_pDebugFile,L"MP3_SetFrame - SetFrame failed\r\n");
		return E_FAIL;
	}
}

///////////////////////////////////////////////////////////////////
//
// Stuff to include into class in future
//
void set_synth_functions(struct frame *fr)
{
	typedef int (*func)(real *,int,unsigned char *);
	typedef int (*func_mono)(real *,unsigned char *);

	static func funcs[2][3] = { 
		{ synth_1to1, synth_2to1, synth_4to1 } ,
		{ synth_1to1_8bit, synth_2to1_8bit, synth_4to1_8bit } 
	};

    static func_mono funcs_mono[2][2][3] = {    
        { { synth_1to1_mono2stereo ,
            synth_2to1_mono2stereo ,
            synth_4to1_mono2stereo } ,
          { synth_1to1_8bit_mono2stereo ,
            synth_2to1_8bit_mono2stereo ,
            synth_4to1_8bit_mono2stereo } } ,
        { { synth_1to1_mono ,
            synth_2to1_mono ,
            synth_4to1_mono } ,
          { synth_1to1_8bit_mono ,
            synth_2to1_8bit_mono ,
            synth_4to1_8bit_mono } }
    };


	fr->synth = funcs[FORCE_8_BIT][g_DownSampleRate];
	fr->synth_mono = funcs_mono[force_mono][FORCE_8_BIT][g_DownSampleRate];
	fr->block_size = 128 >> (force_mono + FORCE_8_BIT + g_DownSampleRate);


#if FORCE_8_BIT == 1
	ai.format = AUDIO_FORMAT_ULAW_8;
	make_conv16to8_table(ai.format);
#endif
}

void CreateEventInfo()
{
	if(g_pEventInfo == NULL)
	{
		g_pEventInfo = new CodecEventInfo;
		g_pEventInfo->pFrame = NULL;
		g_pEventInfo->pSpectral = NULL;
		g_pEventInfo->pWaveform = NULL;
	}
}

void DestroyCodecEventInfo(CodecEventInfo* p)
{
	if(p)
	{
		delete [] p->pWaveform;
		delete [] p->pSpectral;
		delete p->pFrame;
		delete p;
	}
}

void DoSpectralNotify(real* bandPtrL, real* bandPtrR, int dim1, int dim2)
{
	if(g_dwSpectralVisRate)
	{
		if(g_dwSpectralVisStatus >= g_dwSpectralVisRate)
		{
			g_dwSpectralVisStatus = 1;
			CreateEventInfo();			
			int size = dim1*dim2;
			g_pEventInfo->dwSpectralDim1 = dim1;
			g_pEventInfo->dwSpectralDim2 = dim2;
			g_pEventInfo->dwSpectralElementsPerChannel = size;
			g_pEventInfo->Channels = (bandPtrR == NULL) ? 1 : 2;
			int arrsize = size;
			if(bandPtrR)
				arrsize *= 2;
			g_pEventInfo->pSpectral = new float[arrsize];
			float* p = g_pEventInfo->pSpectral;
			int i;
			for(i=0;i<size;i++)
			{
				*p = (float)(*bandPtrL);
				p++;
				bandPtrL++;
			}
			if(bandPtrR)
			{
				for(i=0;i<size;i++)
				{
					*p = (float)(*bandPtrR);
					p++;
					bandPtrR++;
				}
			}
		}
		else 
			++g_dwSpectralVisStatus;
	}
}

void DoWaveNotify(short* pcm_sample,int samplesperchannel,int channels)
{
	if(g_dwWaveformVisRate)
	{
		if(g_dwWaveformVisStatus >= g_dwWaveformVisRate)
		{
			g_dwWaveformVisStatus = 1;
			CreateEventInfo();
			int arrsize = samplesperchannel*channels;
			g_pEventInfo->pWaveform = new short[arrsize];
			g_pEventInfo->Channels = channels;
			g_pEventInfo->dwWaveformElementsPerChannel = samplesperchannel;
			//convert pcm_sample from [samplesperchannel][channels] to [channels][samplesperchannel]
			if(channels == 1)
			{
				memcpy(g_pEventInfo->pWaveform,pcm_sample,samplesperchannel);
			}
			else if(channels == 2)
			{
				short* pcm_left = g_pEventInfo->pWaveform;
				short* pcm_right = g_pEventInfo->pWaveform + samplesperchannel;
				for(int i=0;i<samplesperchannel;i++)
				{
					*pcm_left = *pcm_sample;
					pcm_sample++;
					pcm_left++;
					*pcm_right = *pcm_sample;
					pcm_sample++;
					pcm_right++;
				}
			}
		}	
		else
			++g_dwWaveformVisStatus;
	}
}


void DoFrameNotify(long frame)
{
	if(g_dwFrameProgressRate)
	{
		if(g_dwFrameProgressStatus >= g_dwFrameProgressRate)
		{
			g_dwFrameProgressStatus = 1;
			CreateEventInfo();
			g_pEventInfo->pFrame = new FrameInfo;
			g_pEventInfo->pFrame->CurrentFrame = frame;
			g_pEventInfo->pFrame->CurrentChapter = 0;
			g_pEventInfo->pFrame->NumChapter = 1;
			g_pEventInfo->pFrame->NumFrames = g_NumFrames;
		}
		else
			++g_dwFrameProgressStatus;
	}
}


