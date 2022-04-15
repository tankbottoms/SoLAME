/* [Petteri] Get write(): */
#include <windows.h>

#include "Dss.h"
#include "mpg123.h"
#include "tables.h"
#include "options.h"
#include <tchar.h>

#include "AutoProfile.h"
#include "debug.h"

#ifdef ADD_ID3
#include "genre.h"
#include "id3_tag.h"
#include <excpt.h>
#undef try			// annoying #define in excpt.h
#endif

//#define MP3_DEBUG
#ifdef MP3_DEBUG
#include "DebugFile.h"
CDebugFile *g_pDebugFile = NULL;
void CreateFile() 
{
	if(!g_pDebugFile)
	{
		g_pDebugFile = new CDebugFile;
		g_pDebugFile->Open(L"\\MP3.txt");
	}
}
#define DEBUG2(x,y) CreateFile(); x->Output(y)
#define DEBUG3(x,y,z) CreateFile(); x->Output(y,z)
#define DEBUG4(x,y,z,a) CreateFile(); x->Output(y,z,a)
#define DEBUG5(x,y,z,a,b) CreateFile(); x->Output(y,z,a,b)

#else
#define DEBUG2(x,y) 
#define DEBUG3(x,y,z)
#define DEBUG4(x,y,z,a)
#define DEBUG5(x,y,z,a,b)

#endif

long g_FrameNum = 0;

/* max = 1728 */
#define MAXFRAMESIZE 1792

#define SKIP_JUNK 1

int head_read(unsigned char *hbuf,unsigned long *newhead,IDssInputStream * pIStream);
int head_check(unsigned long newhead);
int read_frame_header(frame* fr,IDssInputStream* pIStream);

struct frame fr;
long id3v2offset=0;

int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long freqs[7] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 };

extern int bitindex;
unsigned char *wordpointer;

static int fsize=0,fsizeold=0,ssize;
static unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
static unsigned char *bsbuf=bsspace[1],*bsbufold;
static int bsnum=0;

struct ibuf {
	struct ibuf *next;
	struct ibuf *prev;
	unsigned char *buf;
	unsigned char *pnt;
	int len;
	/* skip,time stamp */
};

struct ibuf ibufs[2];
struct ibuf *cibuf;
int ibufnum=0;

unsigned char *pcm_sample = NULL;
int pcm_point = 0;

//static FILE *filept;
//static int filept_opened;
static unsigned long firsthead=0;
static unsigned long oldhead = 0;
static bool s_bTryResynch = true;
//static HANDLE s_hFile;
static IDssInputStream* s_pIStream;
int get_input(void* bp, unsigned int size,IDssInputStream* pIStream);

static double compute_bpf(struct frame *fr)
{
	double bpf;

        switch(fr->lay) {
                case 1:
                        bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
                        bpf *= 12000.0 * 4.0;
                        bpf /= freqs[fr->sampling_frequency] <<(fr->lsf);
                        break;
                case 2:
                case 3:
                        bpf = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];                        bpf *= 144000;
                        bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
                        break;
                default:
                        bpf = 1.0;
        }

	return bpf;
}

static double compute_tpf(struct frame *fr)
{
	static int bs[4] = { 0,384,1152,1152 };
	double tpf;

	tpf = (double) bs[fr->lay];
	tpf /= freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

#ifdef ADD_ID3
struct ID3FrameIDStrings
{
ID3_FrameID		id;
TCHAR*			szFrameID;
};

static	ID3FrameIDStrings	ID3_FrameIDStrings[] =
{
	{	ID3FID_ENCODEDBY,			TEXT("Encoded by")				},
	{	ID3FID_ORIGALBUM,			TEXT("Original Album")			},
	{	ID3FID_PUBLISHER,			TEXT("Publisher")				},
	{	ID3FID_ENCODERSETTINGS,		TEXT("Encoder Settings")		},
	{	ID3FID_ORIGFILENAME,		TEXT("Original Filename")		},
	{	ID3FID_LANGUAGE,			TEXT("Language")				},
	{	ID3FID_PARTINSET,			TEXT("Part in set")				},
	{	ID3FID_DATE,				TEXT("Date")					},
	{	ID3FID_TIME,				TEXT("Time")					},
	{	ID3FID_RECORDINGDATES,		TEXT("Recording Dates")			},
	{	ID3FID_MEDIATYPE,			TEXT("Media Type")				},
	{	ID3FID_FILETYPE,			TEXT("File Type")				},
	{	ID3FID_NETRADIOSTATION,		TEXT("Internet Radio Station")	},
	{	ID3FID_NETRADIOOWNER,		TEXT("Internet Radio Owner")	},
	{	ID3FID_LYRICIST,			TEXT("Lyricist")				},
	{	ID3FID_ORIGARTIST,			TEXT("Original Artist")			},
	{	ID3FID_ORIGLYRICIST,		TEXT("Original Lyricist")		},
	{	ID3FID_CONTENTGROUP,		TEXT("Content Group")			},
	{	ID3FID_TITLE,				TEXT("Title")					},
	{	ID3FID_SUBTITLE,			TEXT("Subtitle")				},
	{	ID3FID_LEADARTIST,			TEXT("Artist")					},
	{	ID3FID_BAND,				TEXT("Band")					},
	{	ID3FID_CONDUCTOR,			TEXT("Conductor")				},
	{	ID3FID_MIXARTIST,			TEXT("Mix Artist")				},
	{	ID3FID_ALBUM,				TEXT("Album")					},
	{	ID3FID_YEAR,				TEXT("Year")					},
	{	ID3FID_COMPOSER,			TEXT("Composer")				},
	{	ID3FID_COPYRIGHT,			TEXT("Copyright")				},
	{	ID3FID_CONTENTTYPE,			TEXT("Genre")					},
	{	ID3FID_TRACKNUM,			TEXT("Track Number")			},
	{	ID3FID_USERTEXT,			TEXT("User Text")				},
	{	ID3FID_COMMENT,				TEXT("Comment")					},
	{	ID3FID_UNSYNCEDLYRICS,		TEXT("Lyrics")					},
	// URL Frames
	{	ID3FID_WWWAUDIOFILE,		TEXT("Audio File Information")	},
	{	ID3FID_WWWARTIST,			TEXT("Artist Information")		},
	{	ID3FID_WWWAUDIOSOURCE,		TEXT("Audio Source Information")},
	{	ID3FID_WWWCOMMERCIALINFO,	TEXT("Commercial Information")	},
	{	ID3FID_WWWCOPYRIGHT,		TEXT("Copyright Information")	},
	{	ID3FID_WWWPUBLISHER,		TEXT("Publisher Information")	},
	{	ID3FID_WWWPAYMENT,			TEXT("Licensing Information")	},
	{	ID3FID_WWWRADIOPAGE,		TEXT("Radio Web Site")			},
	{	ID3FID_WWWUSER,				TEXT("Web Site")				},
	//no frame info
	{   ID3FID_NOFRAME,				TEXT("")						}
};


TCHAR* GetFrameIDText(ID3_FrameID fid)
{
	ID3FrameIDStrings* f = ID3_FrameIDStrings;
	while(f->id != ID3FID_NOFRAME)
	{
		if(f->id == fid)
			return f->szFrameID;
		f++;
	}
	return NULL;
}

static bool s_bID3Exception = false;

int ID3_Exception ( DWORD dwException )
{
    if(dwException >= 0xE0000000)
	{
		//ID3 Error was thrown....run exception handler
		s_bID3Exception = true;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

HRESULT PopulateID3Info(IDssInputStream* pIStream,IDssDictionary* pIDict)
{
	if(pIStream == NULL || pIDict == NULL) return E_POINTER;

	ID3_Tag* pt = new ID3_Tag;
	ID3_Tag& t = *pt;
	int frames;
	TCHAR buf[1024];

	__try
	{
		//t.Link(TEXT("bdlll.mp3"));
		t.Link(pIStream);
		frames = t.NumFrames();
		for(int i=0;i<frames;i++)
		{
			ID3_Frame* f = t.GetFrameNum(i);
			f->Field(ID3FN_TEXT).Get(buf,1024);
			if(f->frameID == ID3FID_CONTENTTYPE)
			{
				if(buf[0] == L'(')
				{
					buf[0] = L' ';
					TCHAR* p = buf+1;
					while(_istdigit(*p))
						p++;
					
					*p = L' ';
					
#ifdef UNICODE
					int genre_index = _wtoi(buf+1);
#else
					int genre_index = atoi(buf+1);
#endif
					if(genre_index < genre_count)
						_tcscpy(buf,genre_table[genre_index]);
					else
					{
						buf[0] = L'(';
						*p = L')';
					}
				}
			}
			pIDict->AddEntry(GetFrameIDText(f->frameID),buf);
		}
	}
	__except( ID3_Exception( GetExceptionCode() ) )
	{
	}

	delete pt;

	return S_OK;
}
/*	
	//Get ID3 Information
	pIStream->Seek(-128,soEndOfFile);

	char buf[128];
	DWORD BytesRead;
	pIStream->Read((unsigned char*)buf,128,&BytesRead);
	
	if(buf[0] == 'T' && buf[1] =='A' && buf[2] == 'G')
	{
		TCHAR sz[31];

		struct id3tag *tag = (struct id3tag *) buf;				
		if(tag->genre < genre_count)
			pIDict->AddEntry(TEXT("Genre"),genre_table[(int)(tag->genre)]);
		sz[30] = '\0';
		
#if defined(UNICODE) && (defined(WIN32) || defined(_WIN32_WCE))
		MultiByteToWideChar(CP_ACP,0,tag->album,30,sz,30);
#else
		_tcsncpy(sz,tag->album,30);
#endif
		FixupId3String(sz,30);
		if(*sz)
			pIDict->AddEntry(TEXT("Album"),sz);
		
#if defined(UNICODE) && (defined(WIN32) || defined(_WIN32_WCE))
		MultiByteToWideChar(CP_ACP,0,tag->title,30,sz,30);
#else
		_tcsncpy(sz,tag->title,30);
#endif
		FixupId3String(sz,30);
		if(*sz)
			pIDict->AddEntry(TEXT("Title"),sz);

#if defined(UNICODE) && (defined(WIN32) || defined(_WIN32_WCE))
		MultiByteToWideChar(CP_ACP,0,tag->artist,30,sz,30);
#else
		_tcsncpy(sz,tag->artist,30);
#endif
		FixupId3String(sz,30);
		if(*sz)
			pIDict->AddEntry(TEXT("Artist"),sz);
		
#if defined(UNICODE) && (defined(WIN32) || defined(_WIN32_WCE))
		MultiByteToWideChar(CP_ACP,0,tag->comment,30,sz,30);
#else
		_tcsncpy(sz,tag->comment,30);
#endif
		FixupId3String(sz,30);
		if(*sz)
			pIDict->AddEntry(TEXT("Comment"),sz);
		
#if defined(UNICODE) && (defined(WIN32) || defined(_WIN32_WCE))
		MultiByteToWideChar(CP_ACP,0,tag->year,4,sz,4);
#else
		_tcsncpy(sz,tag->year,4);
#endif
		sz[4] = '\0';
		FixupId3String(sz,4);
		if(*sz)
			pIDict->AddEntry(TEXT("Copyright"),sz);
		return S_OK;
	}
	return E_FAIL;
}
*/
#endif


long g_NumFrames;
long g_FrameSize;
int GenerateFrameTable(CMP3CodecInfo *pMP3Info)
{
	HRESULT hr;
	DWORD FileSize;
	DWORD FilePointer = 0;
	
	hr = s_pIStream->FileSize(&FileSize);
	if(FAILED(hr))
	{
		DEBUG2(g_pDebugFile,L"File size determination failed\n");
		return 0;
	}
	
	//Count Frames
	hr = s_pIStream->Seek(0,soStartOfFile);
	if(FAILED(hr))
	{
		DEBUG2(g_pDebugFile,L"File seek failed\n");
		return 0;
	}

	oldhead = 0;
	firsthead = 0;
	s_bTryResynch = false;
	DEBUG2(g_pDebugFile,L"GenerateFrameTable\n");
	int iRes = read_frame_header(&fr,s_pIStream);
	s_bTryResynch = true;
	if(!iRes)
	{
		DEBUG2(g_pDebugFile,L"GenerateFrameTable failed\n");
		return 0;
	}

	double bpf = compute_bpf(&fr);
	double tpf = compute_tpf(&fr);

	g_FrameSize = fr.framesize;
	
	pMP3Info->SetInputFrameSize(g_FrameSize);
	pMP3Info->SetOutputFrameSize(4608); //TODO : figure out better way of determining this.

	g_NumFrames = (long)((double)(FileSize-id3v2offset) / bpf);//fr.framesize;

	FrameInfo fi = { 0, g_NumFrames, 0, 1 };
	pMP3Info->SetFrameInfo(fi);
	

	//pMP3Info->bitrate = tabsel_123[fr.lsf][fr.lay][fr.bitrate_index];
	// ************
	// Ed's changes
	IDssAudioStreamFormat* pIFormat;
	if (FAILED(DssCreateAudioStreamFormat(&pIFormat)))
		return 0;
//	s_pIStream->GetFormat(&pIFormat);
	pIFormat->SetBytesPerSec((int)(bpf/tpf));
	s_pIStream->SetFormat(pIFormat);
	pIFormat->Release();

	TimeInfo ti = { 0, 0, 0, 0, 0};

	ti.dwMilliseconds = (long)(1000 * tpf * g_NumFrames);
	UpdateTimeInfo(ti);
	pMP3Info->SetDuration(ti);
	
#ifdef ADD_ID3
	IDssDictionary* pIDict;
	pMP3Info->GetFileInfoMap(&pIDict);
	PopulateID3Info(s_pIStream,pIDict);
	pIDict->Release();
#endif //ADD_ID3

	s_pIStream->Seek(0,soStartOfFile);
	SetFrame(0);
	DEBUG2(g_pDebugFile,L"GenerateFrameTable succeeded\n");
	return 1;

}

void UpdateTimeInfo(TimeInfo& ti)
{
	DWORD t;
	ti.Hours = (BYTE)(ti.dwMilliseconds / (1000 * 60 * 60));
	t = ti.dwMilliseconds % (1000 * 60 * 60);
	ti.Minutes = (BYTE)(t / (1000 * 60));
	t = t % (1000 * 60);
	ti.Seconds = (BYTE)(t / (1000));
	ti.Milliseconds = (WORD)(t % 1000);
}


extern DownSampleRate g_DownSampleRate;
extern void reinit_1to1();
extern void reinit_2to1();
extern void reinit_hybrid();
extern void reinit_dct();

int SetFrame(long FrameNum)
{
	if(s_pIStream == NULL)
		return 0;

	DEBUG3(g_pDebugFile,L"::SetFrame %d\r\n",FrameNum);

	if(FrameNum < 0 || FrameNum > g_NumFrames)
	{
		DEBUG2(g_pDebugFile,L"::SetFrame - FrameNum Out of Range\r\n");
		return 0;
	}

	g_FrameNum = FrameNum;
	
	DWORD FilePointer;
	unsigned char hbuf[8];
	unsigned long header;
	
	FilePointer = FrameNum*g_FrameSize+id3v2offset;
	HRESULT hr = s_pIStream->Seek(FilePointer,soStartOfFile);
	if(FAILED(hr))
	{
		DEBUG2(g_pDebugFile,L"::SetFrame - SFP Out of Range\r\n");
		return 0;
	}

	if(!head_read(hbuf,&header,s_pIStream))
	{
		DEBUG2(g_pDebugFile,L"::SetFrame - Couldn't read file\r\n");
		return 0;
	}
	while(!head_check(header))
	{
		s_pIStream->Seek(-3,soCurrent);
		if(!head_read(hbuf,&header,s_pIStream))
		{
			DEBUG2(g_pDebugFile,L"::SetFrame - Couldn't read file\r\n");
			return 0;
		}
	}
	s_pIStream->Seek(-4,soCurrent);

	//pcm_point = 0;
	DEBUG3(g_pDebugFile,L"::SetFrame - Frame set to %d!!\r\n",FrameNum);

//#define CLEAR_BUFFERS
#ifdef CLEAR_BUFFERS
	fsize=0;
//	fsizeold=0;

//	memset(bsspace, 0, MAXFRAMESIZE + 512);
	bsbuf = bsspace[1];
//	bsbufold = bsbuf;	
	bsnum = 0;

//	fr.header_change = 1;
	firsthead=0;
	oldhead = 0;

	bitindex = 0;
	wordpointer = (unsigned char *) bsbuf;

///*
	//reinit_1to1();
	//reinit_2to1();
	//reinit_dct();
///*
	//reinit_hybrid();
//init_layer3(g_DownSampleRate);
//*/
#else
	oldhead = header;
#endif

	return 1;
}


static void get_II_stuff(struct frame *fr)
{
  static int translate[3][2][16] = 
   { { { 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 } ,
       { 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 } } ,
     { { 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 } ,
       { 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 } } ,
     { { 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 } ,
       { 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 } } };

  int table,sblim;
  static struct al_table *tables[5] = 
       { alloc_0, alloc_1, alloc_2, alloc_3 , alloc_4 };
  static int sblims[5] = { 27 , 30 , 8, 12 , 30 };

  if(fr->lsf)
    table = 4;
  else
    table = translate[fr->sampling_frequency][2-fr->stereo][fr->bitrate_index];
  sblim = sblims[table];

  fr->alloc = tables[table];
  fr->II_sblimit = sblim;
}


void read_frame_init (void)
{
	oldhead = 0;
	firsthead = 0;
}

#define HDRCMPMASK 0xfffffd00

int head_read(unsigned char *hbuf,unsigned long *newhead,IDssInputStream* pIStream)
{
//	if(fread(hbuf,1,4,filept) != 4)
	if (get_input(hbuf, 4,pIStream) != 4)
	{
		DEBUG2(g_pDebugFile,L"head_read failed\n");
		return FALSE;
	}

	*newhead = ((unsigned long) hbuf[0] << 24) |
	           ((unsigned long) hbuf[1] << 16) |
	           ((unsigned long) hbuf[2] << 8)  |
	            (unsigned long) hbuf[3];
	return TRUE;
}

int head_check(unsigned long head) 
{
	if( (head & 0xffe00000) != 0xffe00000)
		return FALSE;
	if(!((head>>17)&3))
		return FALSE;
	if( ((head>>12)&0xf) == 0xf)
		return FALSE;
	if( ((head>>10)&0x3) == 0x3 )
		return FALSE;
	DEBUG2(g_pDebugFile,L"head_check succeeded\n");
	return TRUE;
}



int test_read_frame_header(frame* fr,IDssInputStream* pIStream,CMP3SourceInfo* pInfo)
{
  unsigned long newhead;
  unsigned char hbuf[8];

	pIStream->Seek(0,soStartOfFile);

	//Check for ID3 tags
	unsigned char buf[10];
	DWORD dwBytesRead;
	pIStream->Read(buf,10,&dwBytesRead);
	if(buf[0] == 0x49 && buf[1] == 0x44 && buf[2] == 0x33)
	{
		//ID3v2 header
//		DWORD dwTmp;
		
		id3v2offset = (static_cast<DWORD>(buf[6]) << 21 ) | 
					  (static_cast<DWORD>(buf[7]) << 14) | 
					  (static_cast<DWORD>(buf[8]) << 7) | 
					  static_cast<DWORD>(buf[9]);
		id3v2offset += 10;
	}
	else
		id3v2offset = 0;

	pIStream->Seek(id3v2offset,soStartOfFile);

	if(!head_read(hbuf,&newhead,pIStream))
	{
		return FALSE;
	}

    fr->header_change = 1;

	int i;

	//search in 8 bit steps through the first 2.5k
	for(i = 0;i<2560;i++)
	{
		if(head_check(newhead))
			break;
		
		//Shift header and get next byte
		memmove(hbuf,hbuf+1,3);
		if(get_input(hbuf+3,1,pIStream) != 1)
			return 0;
		
		newhead <<= 8;
		newhead |= hbuf[3];
	}
	if(i == 2560)
	{
//  	fprintf(stderr,"Giving up searching valid MPEG header\n");	
		return 0;
	}
		/* 
		 * should we check, whether a new frame starts at the next
		 * expected position? (some kind of read ahead)
		 * We could implement this easily, at least for files.
		 */
//	NKDbgPrintfW(L"rfh Found header!\n");

    if( (newhead & 0xffe00000) != 0xffe00000) {
		DEBUG3(g_pDebugFile,L"rfh Illegal MPEG Header %x\n",newhead);
        // Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.
        // newhead, ftell(filept)-4

//		NKDbgPrintfW(L"rfh exit - Illegal Header, no resynch\n");
        return (0);
    }

	pIStream->GetCurrentPosition(&id3v2offset);
    
    if( newhead & (1<<20) ) {
      fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
      fr->mpeg25 = 0;
    }
    else {
      fr->lsf = 1;
      fr->mpeg25 = 1;
    }

//	NKDbgPrintfW(L"rfh fr->lsf = %d\n",fr->lsf);
//	NKDbgPrintfW(L"rfh fr->mpeg25 = %d\n",fr->mpeg25);

	//bTryResync? TODO
          /* If "tryresync" is true, assume that certain
             parameters do not change within the stream! */
      fr->lay = 4-((newhead>>17)&3);
      fr->bitrate_index = ((newhead>>12)&0xf);
      if( ((newhead>>10)&0x3) == 0x3) {
//		  NKDbgPrintfW(L"rfh exit - Stream Error\n");
		  return 0;
//        fprintf(stderr,"Stream error\n");
//	TODO: Exit gracefully
//        exit(1);
	  }

    if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
      }
    else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
    
	fr->error_protection = ((newhead>>16)&0x1)^0x1;

    if(fr->mpeg25) /* allow Bitrate change for 2.5 ... */
      fr->bitrate_index = ((newhead>>12)&0xf);

    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

//	NKDbgPrintfW(L"rfh fr->lay = %d\n",fr->lay);
//	NKDbgPrintfW(L"rfh fr->bitrate_index = %d\n",fr->bitrate_index);
//	NKDbgPrintfW(L"rfh fr->padding = %d\n",fr->padding);
//	NKDbgPrintfW(L"rfh fr->extension = %d\n",fr->extension);
//	NKDbgPrintfW(L"rfh fr->mode = %d\n",fr->mode);
//	NKDbgPrintfW(L"rfh fr->mode_ext = %d\n",fr->mode_ext);
//	NKDbgPrintfW(L"rfh fr->copyright = %d\n",fr->copyright);
//	NKDbgPrintfW(L"rfh fr->original = %d\n",fr->original);
//	NKDbgPrintfW(L"rfh fr->emphasis = %d\n",fr->emphasis);
//	NKDbgPrintfW(L"rfh fr->stereo = %d\n",fr->stereo);

    if(!fr->bitrate_index)
    {
//      fprintf(stderr,"Free format not supported.\n");
//	  NKDbgPrintfW(L"rfh exit - Free format not supported\n");
      return (0);
    }


    switch(fr->lay)
    {
      case 1:
/*
		fr->do_layer = do_layer1;
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? 
                         (fr->mode_ext<<2)+4 : 32;
*/
        fr->framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
        break;
      case 2:
/*
		fr->do_layer = do_layer2;
//        get_II_stuff(fr);
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
                         (fr->mode_ext<<2)+4 : fr->II_sblimit;
*/
        fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        break;
      case 3:
/*		fr->do_layer = do_layer3;
        if(fr->lsf)
          ssize = (fr->stereo == 1) ? 9 : 17;
        else
          ssize = (fr->stereo == 1) ? 17 : 32;
        if(fr->error_protection)
          ssize += 2;
*/
        fr->framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
        fr->framesize = fr->framesize + fr->padding - 4;
        break; 
      default:
//        fprintf(stderr,"Sorry, unknown layer type.\n"); 
//		NKDbgPrintfW(L"rfh exit - Unknown layer type %d\n",fr->lay);
        return (0);
    }

//  NKDbgPrintfW(L"rfh fr->framesize = %d\n",fr->framesize);
//  NKDbgPrintfW(L"rfh fr->header_change = %d\n",fr->header_change);
//  NKDbgPrintfW(L"rfh exit - succeeded\n");
  
	IDssAudioStreamFormat* pIFormat;
	DssCreateAudioStreamFormat(&pIFormat);

	
	DWORD FileSize;
	pIStream->FileSize(&FileSize);
	
	double bpf = compute_bpf(fr);
	double tpf = compute_tpf(fr);

	FrameInfo fi = { 0, 1, 0, 0 };
	fi.NumFrames = (long)((double)(FileSize-id3v2offset)/bpf);
	pInfo->SetFrameInfo(fi);

	//pMP3Info->bitrate = tabsel_123[fr.lsf][fr.lay][fr.bitrate_index];
	pIFormat->SetBytesPerSec((DWORD)(bpf/tpf));
	pIFormat->SetBitsPerSample(16);
	pIFormat->SetChannels(fr->stereo);
	pIFormat->SetSamplingFrequency(freqs[fr->sampling_frequency]);
	pIFormat->SetFormatTag(FORMAT_MP3);

	TimeInfo duration = {0, 0, 0, 0, 0};
	duration.dwMilliseconds = (DWORD)(1000*tpf*fi.NumFrames);
	UpdateTimeInfo(duration);
	pInfo->SetDuration(duration);
	pInfo->SetInputFrameSize(fr->framesize);

	pInfo->SetOutputFrameSize(4608); //TODO : Figure out better way of doing this.
	
	pIStream->SetFormat(pIFormat);
	pIFormat->Release();
	return 1;
}


int read_frame_header(frame* fr,IDssInputStream* pIStream)
{
  static unsigned long newhead;
  unsigned char hbuf[8];

  DEBUG2(g_pDebugFile,L"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
  DEBUG2(g_pDebugFile,L"read_frame_header begin\n");
read_again:
  DEBUG2(g_pDebugFile,L"rfh label read_again:\n");
	if(!head_read(hbuf,&newhead,pIStream))
	{
		DEBUG2(g_pDebugFile,L"rfh exit - head_read failed\n");
		return FALSE;
	}

  if(oldhead != newhead || !oldhead)
  {
    fr->header_change = 1;

init_resync:
	DEBUG2(g_pDebugFile,L"rfh label init_resync:\n");
//#ifdef SKIP_JUNK
	if(!firsthead && !head_check(newhead) ) 
	{
		int i;

//		fprintf(stderr,"Junk at the beginning\n");
		/* I even saw RIFF headers at the beginning of MPEG streams ;( */

		if(newhead == ('R'<<24)+('I'<<16)+('F'<<8)+'F') {
			DEBUG2(g_pDebugFile,L"rfh RIFF header\n");
			char buf[68];
//			fprintf(stderr,"Skipped RIFF header\n");
			get_input(buf, 68,pIStream);
			goto read_again;
		}
		/* search in 32 bit steps through the first 2K */
		for(i=0;i<2560;i++) 
		{
			if(head_check(newhead))
				break;
		
			memmove(hbuf,hbuf+1,3);
			if(get_input(hbuf+3,1,pIStream) != 1)
			{
				DEBUG2(g_pDebugFile,L"rfh exit - get_input failed\n");
				return 0;
			}
			
			newhead <<= 8;
			newhead |= hbuf[3];
		}
		if(i == 2560)
		{
//			fprintf(stderr,"Giving up searching valid MPEG header\n");
			return 0;
		}
		/* 
		 * should we check, whether a new frame starts at the next
		 * expected position? (some kind of read ahead)
		 * We could implement this easily, at least for files.
		 */
	}
//#endif
	DEBUG2(g_pDebugFile,L"rfh Found header!\n");

    if( (newhead & 0xffe00000) != 0xffe00000) 
	{
		DEBUG3(g_pDebugFile,L"rfh Illegal MPEG Header %x\n",newhead);
        // Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.
        // newhead, ftell(filept)-4
		if(s_bTryResynch)
		{
			/* Read more bytes until we find something that looks
				   reasonably like a valid header.  This is not a
				   perfect strategy, but it should get us back on the
				   track within a short time (and hopefully without
				   too much distortion in the audio output).  */
			do {
			  memmove (&hbuf[0], &hbuf[1], 7);
			  if (get_input(&hbuf[3], 1,pIStream) != 1)
				return 0;

			  /* This is faster than combining newhead from scratch */
			  newhead = ((newhead << 8) | hbuf[3]) & 0xffffffff;

			  if (!oldhead)
				goto init_resync;       /* "considered harmful", eh? */

			} while ((newhead & HDRCMPMASK) != (oldhead & HDRCMPMASK)
				  && (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK));
		}
		else //s_bTryResynch = false
		{
			DEBUG2(g_pDebugFile,L"rfh exit - Illegal Header, no resynch\n");
			return (0);
		}
    }
    
	if (!firsthead)
      firsthead = newhead;

	    if( newhead & (1<<20) ) {
      fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
      fr->mpeg25 = 0;
    }
    else {
      fr->lsf = 1;
      fr->mpeg25 = 1;
    }

	DEBUG3(g_pDebugFile,L"rfh fr->lsf = %d\n",fr->lsf);
	DEBUG3(g_pDebugFile,L"rfh fr->mpeg25 = %d\n",fr->mpeg25);

	//bTryResync? TODO
    if (!oldhead) {
          /* If "tryresync" is true, assume that certain
             parameters do not change within the stream! */
      fr->lay = 4-((newhead>>17)&3);
      fr->bitrate_index = ((newhead>>12)&0xf);
      if( ((newhead>>10)&0x3) == 0x3) {
		  DEBUG2(g_pDebugFile,L"rfh exit - Stream Error\n");
		  return 0;
//        fprintf(stderr,"Stream error\n");
//	TODO: Exit gracefully
//        exit(1);
      }
      if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
      }
      else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
      fr->error_protection = ((newhead>>16)&0x1)^0x1;
    }

    if(fr->mpeg25) /* allow Bitrate change for 2.5 ... */
      fr->bitrate_index = ((newhead>>12)&0xf);

    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    oldhead = newhead;

	DEBUG3(g_pDebugFile,L"rfh fr->lay = %d\n",fr->lay);
	DEBUG3(g_pDebugFile,L"rfh fr->bitrate_index = %d\n",fr->bitrate_index);
	DEBUG3(g_pDebugFile,L"rfh fr->padding = %d\n",fr->padding);
	DEBUG3(g_pDebugFile,L"rfh fr->extension = %d\n",fr->extension);
	DEBUG3(g_pDebugFile,L"rfh fr->mode = %d\n",fr->mode);
	DEBUG3(g_pDebugFile,L"rfh fr->mode_ext = %d\n",fr->mode_ext);
	DEBUG3(g_pDebugFile,L"rfh fr->copyright = %d\n",fr->copyright);
	DEBUG3(g_pDebugFile,L"rfh fr->original = %d\n",fr->original);
	DEBUG3(g_pDebugFile,L"rfh fr->emphasis = %d\n",fr->emphasis);
	DEBUG3(g_pDebugFile,L"rfh fr->stereo = %d\n",fr->stereo);

    if(!fr->bitrate_index)
    {
//      fprintf(stderr,"Free format not supported.\n");
	  DEBUG2(g_pDebugFile,L"rfh exit - Free format not supported\n");
      return (0);
    }

    switch(fr->lay)
    {
      case 1:
		fr->do_layer = do_layer1;
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? 
                         (fr->mode_ext<<2)+4 : 32;
        fr->framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
        break;
      case 2:
		fr->do_layer = do_layer2;
        get_II_stuff(fr);
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
                         (fr->mode_ext<<2)+4 : fr->II_sblimit;
        fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        break;
      case 3:
		fr->do_layer = do_layer3;
        if(fr->lsf)
          ssize = (fr->stereo == 1) ? 9 : 17;
        else
          ssize = (fr->stereo == 1) ? 17 : 32;
        if(fr->error_protection)
          ssize += 2;
          fr->framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
          fr->framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
          fr->framesize = fr->framesize + fr->padding - 4;
        break; 
      default:
//        fprintf(stderr,"Sorry, unknown layer type.\n"); 
		DEBUG3(g_pDebugFile,L"rfh exit - Unknown layer type %d\n",fr->lay);
        return (0);
    }
  }
  else
    fr->header_change = 0;
 

  DEBUG3(g_pDebugFile,L"rfh fr->framesize = %d\n",fr->framesize);
  DEBUG3(g_pDebugFile,L"rfh fr->header_change = %d\n",fr->header_change);

  DEBUG2(g_pDebugFile,L"rfh exit - succeeded\n");
  return 1;
}

int read_frame(struct frame *fr)
{
  int l;

/////////////
/////////////

  if(!read_frame_header(fr,s_pIStream))
	  return 0;
  /*
	reading info for layer 3
  */
  fsizeold=fsize;	/* for Layer3 */
  bsbufold = bsbuf;	
  bsbuf = bsspace[bsnum]+512;
  bsnum = (bsnum + 1) & 1;

  /*
  only read in what frame size we know is next...
  */
  fsize = fr->framesize;
 
  if( (l=get_input(bsbuf, fsize,s_pIStream)) != fsize)
  {
    if(l <= 0)
      return 0;
    memset(bsbuf+l,0,fsize-l);
  }

  bitindex = 0;
  wordpointer = (unsigned char *) bsbuf;
  return 1;
}

/* open the device to read the bit stream from it */

//int open_stream(const TCHAR* bs_filenam,int fd)
int open_stream(IDssInputStream* pIStream)
{
	close_stream();
    
	read_frame_init();

/*	s_hFile = CreateFile(bs_filenam, 
						GENERIC_READ, 
						FILE_SHARE_READ, 
						NULL, 
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);
	
	return (s_hFile == INVALID_HANDLE_VALUE) ? 0 : 1;
*/
	s_pIStream = pIStream;
	s_pIStream->AddRef();
	return 1;
}

/*close the device containing the bit stream after a read process*/

void close_stream(void)
{
//	if (s_hFile != INVALID_HANDLE_VALUE)
//		CloseHandle(s_hFile);
	if(s_pIStream) s_pIStream->Release();
	s_pIStream = NULL;
}

int get_input(void* bp, unsigned int size,IDssInputStream * pIStream)
{	
DWORD dwBytesRead;

	//bWrks = ReadFile(hFile, bp, size, &dwBytesRead, NULL);
	pIStream->Read((unsigned char*)bp,size,&dwBytesRead);
/*
#ifdef DEBUG
	DWORD dwError;
	if (dwBytesRead != size && bWrks == TRUE)
	{
		Message("Bytes read(%d)-asked(%d)\n", dwBytesRead, size);								
		if (bWrks && dwBytesRead == 0)
			Message("ReadFile found EOF");
		else				
			Message("Error ReadFile");		
	}
	else
	{	
		dwError = GetLastError();
		if (dwError == ERROR_HANDLE_EOF)
			Message("Pointer passed EOF.");	
	}

	Message("Read from file: %d.\n", dwBytesRead);
#endif 
*/
	return dwBytesRead;
}


void set_pointer(long backstep)
{
  wordpointer = bsbuf + ssize - backstep;
  if (backstep)
    memcpy(wordpointer,bsbufold+fsizeold-backstep,backstep);
  bitindex = 0; 
}
