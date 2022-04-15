/* 
 * Mpeg Layer-1 audio decoder 
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ... 
 *
 */
#include "mpg123.h"
#include "getbits.h"
#include "options.h"

#ifdef AUTOPC
//static int downsampleRate = DOWNSAMPLE;
#else
extern DownSampleRate g_DownSampleRate;
//static int downsampleRate = g_DownSampleRate;
#endif

void I_step_one(unsigned int balloc[], unsigned int scale_index[2][SBLIMIT],struct frame *fr)
{
  unsigned int *ba=balloc;
  unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int i;
    int jsbound = fr->jsbound;
    for (i=0;i<jsbound;i++) { 
      *ba++ = getbits_fast(4);
      *ba++ = getbits_fast(4);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      *ba++ = getbits_fast(4);

    ba = balloc;

    for (i=0;i<jsbound;i++) {
      if ((*ba++))
        *sca++ = getbits_fast(6);
      if ((*ba++))
        *sca++ = getbits_fast(6);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      if ((*ba++)) {
        *sca++ =  getbits_fast(6);
        *sca++ =  getbits_fast(6);
      }
  }
  else {
    int i;
    for (i=0;i<SBLIMIT;i++)
      *ba++ = getbits_fast(4);
    ba = balloc;
    for (i=0;i<SBLIMIT;i++)
      if ((*ba++))
        *sca++ = getbits_fast(6);
  }
}

void I_step_two(real fraction[2][SBLIMIT],unsigned int balloc[2*SBLIMIT],
	unsigned int scale_index[2][SBLIMIT],struct frame *fr)
{
  int i,n;
  int smpb[2*SBLIMIT]; /* values: 0-65535 */
  int *sample;
  register unsigned int *ba;
  register unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int jsbound = fr->jsbound;
    register real *f0 = fraction[0];
    register real *f1 = fraction[1];
    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++)  {
      if ((n = *ba++))
        *sample++ = getbits(n+1);
      if ((n = *ba++))
        *sample++ = getbits(n+1);
    }
    for (i=jsbound;i<SBLIMIT;i++) 
      if ((n = *ba++))
        *sample++ = getbits(n+1);

    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++) {
      if((n=*ba++))
        *f0++ = MULT_REAL(REAL_INT( ((-1)<<n) + (*sample++) + 1), muls[n+1][*sca++]);
      else
        *f0++ = REAL_ZERO;
      if((n=*ba++))
        *f1++ = MULT_REAL(REAL_INT( ((-1)<<n) + (*sample++) + 1), muls[n+1][*sca++]);
      else
        *f1++ = REAL_ZERO;
    }
    for (i=jsbound;i<SBLIMIT;i++) {
      if ((n=*ba++)) {
        real samp = REAL_INT( ((-1)<<n) + (*sample++) + 1);
        *f0++ = MULT_REAL(samp, muls[n+1][*sca++]);
        *f1++ = MULT_REAL(samp, muls[n+1][*sca++]);
      }
      else
        *f0++ = *f1++ = REAL_ZERO;
    }
    for(i=SBLIMIT>>g_DownSampleRate;i<32;i++)
      fraction[0][i] = fraction[1][i] = REAL_ZERO;
  }
  else {
    register real *f0 = fraction[0];
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++)
      if ((n = *ba++))
        *sample++ = getbits(n+1);
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++) {
      if((n=*ba++))
        *f0++ = MULT_REAL(REAL_INT( ((-1)<<n) + (*sample++) + 1), muls[n+1][*sca++]);
      else
        *f0++ = REAL_ZERO;
    }
    for(i=SBLIMIT>>g_DownSampleRate;i<32;i++)
      fraction[0][i] = REAL_ZERO;
  }
}

int do_layer1(struct frame *fr,struct audio_info_struct *ai)
{
  int clip=0;
  int i,stereo = fr->stereo;
  unsigned int balloc[2*SBLIMIT];
  unsigned int scale_index[2][SBLIMIT];
  static real fraction[2][SBLIMIT];
  int single = fr->single;

  if(stereo == 1 || single == 3)
    single = 0;

  I_step_one(balloc,scale_index,fr);

  for (i=0;i<SCALE_BLOCK;i++)
  {
    I_step_two(fraction,balloc,scale_index,fr);

    if(single >= 0)
    {
      clip += (fr->synth_mono)( (real *) fraction[single],pcm_sample+pcm_point);
    }
    else {
        clip += (fr->synth)( (real *) fraction[0],0,pcm_sample+pcm_point);
        clip += (fr->synth)( (real *) fraction[1],1,pcm_sample+pcm_point);
    }
    pcm_point += fr->block_size;

//    if(pcm_point >= audiobufsize)
//		pcm_point = 0;
//      audio_flush(ai);
  }

  return clip;
}


