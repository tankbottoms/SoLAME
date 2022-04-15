/*
 * mpg123 defines 
 * used source: musicout.h from mpegaudio package
 */

#ifndef MP123_H_
#define MP123_H_

#include <windows.h>
#include <math.h>
#include "Dss.h"
#include "real.h"
#include "MP3SourceInfo.h"

#define ADD_ID3	  //Support ID3 tags at the end of MP3 files

/* [Petteri] For some reason Watcom math.h doesn't define these: */
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT2
#  define M_SQRT2 1.41421356237309504880
#endif

enum DownSampleRate
{			
	NO_DOWNSAMPLING = 0,
	DOWNSAMPLE_2_TO_1,
	DOWNSAMPLE_4_TO_1
//	DOWNSAMPLE_BEST_FIT
};

#define BOTH_CHANNELS -1
#define LEFT_CHANNEL 0
#define RIGHT_CHANNEL 1
#define FORCE_MONO 3

typedef int ChannelType;

#include "audio.h"

#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

//Used to initialize audiobufsize, afterwards,
//recalculated in set_synth_functions() whenever
//the file, channels, or downsampling rate change.
#define AUDIOBUFSIZE (128*SSLIMIT) /* ed: changed to multiple of frame size */

#define         FALSE                   0
#define         TRUE                    1

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

struct al_table 
{
  short bits;
  short d;
};

struct frame {
    struct al_table *alloc;
    int (*synth)(real *,int,unsigned char *);
    int (*synth_mono)(real *,unsigned char *);
    int stereo;
    int jsbound;
    int single;
    int II_sblimit;
    int lsf;
    int mpeg25;
    int header_change;
    int block_size;
    int lay;
	int (*do_layer)(struct frame *fr,struct audio_info_struct *);
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
	int framesize;
};


/* ------ Declarations from "common.c" ------ */

extern void audio_flush(struct audio_info_struct *);
extern void (*catchsignal(int signum, void(*handler)()))();

extern char *strndup(const char *src, int num);

extern void set_pointer(long);

extern unsigned char *pcm_sample;
extern int pcm_point;
extern int audiobufsize;

struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      real *full_gain[3];
      real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};
extern void UpdateTimeInfo(TimeInfo& ti);
extern int open_stream(IDssInputStream* pIStream);
extern void close_stream(void);
extern void read_frame_init (void);
extern int read_frame(struct frame *fr);
//extern int read_frame_header(struct frame *fr,HANDLE hFile);
extern int GenerateFrameTable(CMP3CodecInfo *pMP3Info);
#ifdef ADD_ID3
extern HRESULT PopulateID3Info(IDssInputStream* pIStream,IDssDictionary* pIDict);
#endif
extern int SetFrame(long FrameNum);

extern void play_frame(int init,struct frame *fr);
extern int do_layer3(struct frame *fr,struct audio_info_struct *);
extern int do_layer2(struct frame *fr,struct audio_info_struct *);
extern int do_layer1(struct frame *fr,struct audio_info_struct *);
extern int synth_1to1 (real *,int,unsigned char *);
#ifdef PENTIUM_OPT
extern int synth_1to1_pent (real *,int,unsigned char *);
#endif
extern int synth_1to1_8bit (real *,int,unsigned char *);
extern int synth_2to1 (real *,int,unsigned char *);
extern int synth_2to1_8bit (real *,int,unsigned char *);
extern int synth_4to1 (real *,int,unsigned char *);
extern int synth_4to1_8bit (real *,int,unsigned char *);
extern int synth_1to1_mono (real *,unsigned char *);
extern int synth_1to1_mono2stereo (real *,unsigned char *);
extern int synth_1to1_8bit_mono (real *,unsigned char *);
extern int synth_1to1_8bit_mono2stereo (real *,unsigned char *);
extern int synth_2to1_mono (real *,unsigned char *);
extern int synth_2to1_mono2stereo (real *,unsigned char *);
extern int synth_2to1_8bit_mono (real *,unsigned char *);
extern int synth_2to1_8bit_mono2stereo (real *,unsigned char *);
extern int synth_4to1_mono (real *,unsigned char *);
extern int synth_4to1_mono2stereo (real *,unsigned char *);
extern int synth_4to1_8bit_mono (real *,unsigned char *);
extern int synth_4to1_8bit_mono2stereo (real *,unsigned char *);
extern void rewindNbits(int bits);
extern int  hsstell(void);
extern void set_pointer(long);
extern void huffman_decoder(int ,int *);
extern void huffman_count1(int,int *);

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables(long scale);
extern void make_conv16to8_table(int);

extern void control_sajber(struct frame *fr);
extern void control_tk3play(struct frame *fr);

extern unsigned char *conv16to8;
extern long freqs[7];
extern real muls[27][64];
extern real decwin[512+32];
extern real *pnts[5];


#endif	// MP123_H_
