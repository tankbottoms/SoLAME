#include "layer3_helper.h"

//#define MP3_DEBUG
#undef MP3_DEBUG

#if defined(MP3_DEBUG)
#include "DebugFile.h"
extern CDebugFile *g_pDebugFile;
//CDebugFile *g_pDebugLastPlayed;
#define DEBUG2(x,y) x->Output(y)
#define DEBUG3(x,y,z) x->Output(y,z)
#define DEBUG4(x,y,z,a) x->Output(y,z,a)
#define DEBUG5(x,y,z,a,b) x->Output(y,z,a,b)
#define DEBUG6(x,y,z,a,b,c) x->Output(y,z,a,b,c)
#else
#define DEBUG2(x,y) 
#define DEBUG3(x,y,z)
#define DEBUG4(x,y,z,a)
#define DEBUG5(x,y,z,a,b)
#define DEBUG6(x,y,z,a,b,c)
#endif // MP3_DEBUG


extern void dct36(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf);
extern void III_antialias(real xr[SBLIMIT][SSLIMIT],struct gr_info_s *gr_info);
extern void III_i_stereo(real xr_buf[2][SBLIMIT][SSLIMIT],int *scalefac, struct gr_info_s *gr_info,int sfreq,int ms_stereo,int lsf);

/*
 * new DCT12
 */

#include "options.h"
#include "AutoProfile.h"

static void dct12(real *in,real *rawout1,real *rawout2,register real *wi,register real *ts)
{
#ifdef PROFILE
	FunctionProfiler fp(L"dct12");
#endif

#define DCT12_PART1 \
             in5 = in[5*3];  \
     in5 += (in4 = in[4*3]); \
     in4 += (in3 = in[3*3]); \
     in3 += (in2 = in[2*3]); \
     in2 += (in1 = in[1*3]); \
     in1 += (in0 = in[0*3]); \
                             \
     in5 += in3; in3 += in1; \
                             \
     MULTEQ_REAL(in2, COS6_1); \
     MULTEQ_REAL(in3, COS6_1); \

#define DCT12_PART2 \
     in0 += MULT_REAL(in4, COS6_2); \
                          \
     in4 = in0 + in2;     \
     in0 -= in2;          \
                          \
     in1 += MULT_REAL(in5, COS6_2); \
                          \
     in5 = MULT_REAL((in1 + in3), tfcos12[0]); \
     in1 = MULT_REAL((in1 - in3), tfcos12[2]); \
                         \
     in3 = in4 + in5;    \
     in4 -= in5;         \
                         \
     in2 = in0 + in1;    \
     in0 -= in1;


   {
     real in0,in1,in2,in3,in4,in5;
     register real *out1 = rawout1;
     ts[SBLIMIT*0] = out1[0]; ts[SBLIMIT*1] = out1[1]; ts[SBLIMIT*2] = out1[2];
     ts[SBLIMIT*3] = out1[3]; ts[SBLIMIT*4] = out1[4]; ts[SBLIMIT*5] = out1[5];
 
     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = MULT_REAL((in1 - in5), tfcos12[1]);
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       ts[(17-1)*SBLIMIT] = out1[17-1] + MULT_REAL(tmp0, wi[11-1]);
       ts[(12+1)*SBLIMIT] = out1[12+1] + MULT_REAL(tmp0, wi[6+1]);
       ts[(6 +1)*SBLIMIT] = out1[6 +1] + MULT_REAL(tmp1, wi[1]);
       ts[(11-1)*SBLIMIT] = out1[11-1] + MULT_REAL(tmp1, wi[5-1]);
     }

     DCT12_PART2

     ts[(17-0)*SBLIMIT] = out1[17-0] + MULT_REAL(in2, wi[11-0]);
     ts[(12+0)*SBLIMIT] = out1[12+0] + MULT_REAL(in2, wi[6+0]);
     ts[(12+2)*SBLIMIT] = out1[12+2] + MULT_REAL(in3, wi[6+2]);
     ts[(17-2)*SBLIMIT] = out1[17-2] + MULT_REAL(in3, wi[11-2]);

     ts[(6+0)*SBLIMIT]  = out1[6+0] + MULT_REAL(in0, wi[0]);
     ts[(11-0)*SBLIMIT] = out1[11-0] + MULT_REAL(in0, wi[5-0]);
     ts[(6+2)*SBLIMIT]  = out1[6+2] + MULT_REAL(in4, wi[2]);
     ts[(11-2)*SBLIMIT] = out1[11-2] + MULT_REAL(in4, wi[5-2]);
  }

  in++;

  {
     real in0,in1,in2,in3,in4,in5;
     register real *out2 = rawout2;
 
     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = MULT_REAL((in1 - in5), tfcos12[1]);
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       out2[5-1] = MULT_REAL(tmp0, wi[11-1]);
       out2[0+1] = MULT_REAL(tmp0, wi[6+1]);
       ts[(12+1)*SBLIMIT] += MULT_REAL(tmp1, wi[1]);
       ts[(17-1)*SBLIMIT] += MULT_REAL(tmp1, wi[5-1]);
     }

     DCT12_PART2

     out2[5-0] = MULT_REAL(in2, wi[11-0]);
     out2[0+0] = MULT_REAL(in2, wi[6+0]);
     out2[0+2] = MULT_REAL(in3, wi[6+2]);
     out2[5-2] = MULT_REAL(in3, wi[11-2]);

     ts[(12+0)*SBLIMIT] += MULT_REAL(in0, wi[0]);
     ts[(17-0)*SBLIMIT] += MULT_REAL(in0, wi[5-0]);
     ts[(12+2)*SBLIMIT] += MULT_REAL(in4, wi[2]);
     ts[(17-2)*SBLIMIT] += MULT_REAL(in4, wi[5-2]);
  }

  in++; 

  {
     real in0,in1,in2,in3,in4,in5;
     register real *out2 = rawout2;
     out2[12]=out2[13]=out2[14]=out2[15]=out2[16]=out2[17]=REAL_ZERO;

     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = MULT_REAL((in1 - in5), tfcos12[1]);
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       out2[11-1] = MULT_REAL(tmp0, wi[11-1]);
       out2[6 +1] = MULT_REAL(tmp0, wi[6+1]);
       out2[0+1] += MULT_REAL(tmp1, wi[1]);
       out2[5-1] += MULT_REAL(tmp1, wi[5-1]);
     }

     DCT12_PART2

     out2[11-0] = MULT_REAL(in2, wi[11-0]);
     out2[6 +0] = MULT_REAL(in2, wi[6+0]);
     out2[6 +2] = MULT_REAL(in3, wi[6+2]);
     out2[11-2] = MULT_REAL(in3, wi[11-2]);

     out2[0+0] += MULT_REAL(in0, wi[0]);
     out2[5-0] += MULT_REAL(in0, wi[5-0]);
     out2[0+2] += MULT_REAL(in4, wi[2]);
     out2[5-2] += MULT_REAL(in4, wi[5-2]);
  }
}

/*
 * III_hybrid
 */

static real block[2][2][SBLIMIT*SSLIMIT] = { { { REAL_ZERO, } } };
static int blc[2]={0,0};

static real hybridIn[2][SBLIMIT][SSLIMIT];
static real hybridOut[2][SSLIMIT][SBLIMIT];

void reinit_hybrid()
{
	memset(block, 0, 2*2*SBLIMIT*SSLIMIT*sizeof(real));
	memset(hybridIn, 0, 2*SBLIMIT*SSLIMIT*sizeof(real));
	memset(hybridOut, 0, 2*SBLIMIT*SSLIMIT*sizeof(real));
	blc[0] = 0;
	blc[1] = 0;
}
 
static void III_hybrid(real fsIn[SBLIMIT][SSLIMIT],real tsOut[SSLIMIT][SBLIMIT],
   int ch,struct gr_info_s *gr_info)
{
#ifdef PROFILE
   FunctionProfiler fp(L"III_hybrid");
#endif

   real *tspnt = (real *) tsOut;
//   static real block[2][2][SBLIMIT*SSLIMIT] = { { { REAL_ZERO, } } };
//   static int blc[2]={0,0};
   real *rawout1,*rawout2;
   int bt;
   int sb = 0;

   {
     int b = blc[ch];
     rawout1=block[b][ch];
     b=-b+1;
     rawout2=block[b][ch];
     blc[ch] = b;
   }

  
   if(gr_info->mixed_block_flag) {
     sb = 2;
     dct36(fsIn[0],rawout1,rawout2,win[0],tspnt);
     dct36(fsIn[1],rawout1+18,rawout2+18,win1[0],tspnt+1);
     rawout1 += 36; rawout2 += 36; tspnt += 2;
   }
 
   bt = gr_info->block_type;
   if(bt == 2) {
     for (; sb<(int)gr_info->maxb; sb+=2,tspnt+=2,rawout1+=36,rawout2+=36) {
       dct12(fsIn[sb],rawout1,rawout2,win[2],tspnt);
       dct12(fsIn[sb+1],rawout1+18,rawout2+18,win1[2],tspnt+1);
     }
   }
   else {
     for (; sb<(int)gr_info->maxb; sb+=2,tspnt+=2,rawout1+=36,rawout2+=36) {
       dct36(fsIn[sb],rawout1,rawout2,win[bt],tspnt);
       dct36(fsIn[sb+1],rawout1+18,rawout2+18,win1[bt],tspnt+1);
     }
   }

   for(;sb<SBLIMIT;sb++,tspnt++) {
     int i;
     for(i=0;i<SSLIMIT;i++) {
       tspnt[i*SBLIMIT] = *rawout1++;
       *rawout2++ = REAL_ZERO;
     }
   }
}

void DoSpectralNotify(real* bandPtrL, real* bandPtrR, int sblimit, int sslimit);
void DoWaveNotify(short* pcm_sample,int samplesperchannel,int channels);
void DoFrameNotify(long frame);


/*
 * main layer3 handler
 */
#define SCALEFACTOR_PER_CHANNEL
//#undef SCALEFACTOR_PER_CHANNEL

int do_layer3(struct frame *fr,struct audio_info_struct *ai)
{
#ifdef PROFILE
  FunctionProfiler fp(L"do_layer3");
#endif
#ifdef SCALEFACTOR_PER_CHANNEL
  static int scalefacs[2][39]; /* max 39 for short[13][3] mode, mixed: 38, long: 22 */ 
#else
  int scalefacs[39]; /* max 39 for short[13][3] mode, mixed: 38, long: 22 */
#endif
  int gr, ch, ss,clip=0;
  struct III_sideinfo sideinfo;
  int stereo = fr->stereo;
  int single = fr->single;
  int ms_stereo,i_stereo;
  int sfreq = fr->sampling_frequency;
  int stereo1,granules;

  if(stereo == 1) {
    stereo1 = 1;
    single = 0;
  }
  else if(single >= 0)
    stereo1 = 1;
  else
    stereo1 = 2;

  ms_stereo = (fr->mode == MPG_MD_JOINT_STEREO) && (fr->mode_ext & 0x2);
  i_stereo = (fr->mode == MPG_MD_JOINT_STEREO) && (fr->mode_ext & 0x1);

  if(fr->lsf) {
    granules = 1;
    III_get_side_info_2(&sideinfo,stereo,ms_stereo,sfreq,single);
  }
  else {
    granules = 2;
    III_get_side_info_1(&sideinfo,stereo,ms_stereo,sfreq,single);
  }

  set_pointer(sideinfo.main_data_begin);

  for (gr=0;gr<granules;gr++) 
  {
//	static real hybridIn[2][SBLIMIT][SSLIMIT];
//	static real hybridOut[2][SSLIMIT][SBLIMIT];

    {
      struct gr_info_s *gr_info = &(sideinfo.ch[0].gr[gr]);
      long part2bits;
#ifdef SCALEFACTOR_PER_CHANNEL
      if(fr->lsf)
        part2bits = III_get_scale_factors_2(scalefacs[0],gr_info,0);
      else
        part2bits = III_get_scale_factors_1(scalefacs[0],gr_info);
      if(III_dequantize_sample(hybridIn[0], scalefacs[0],gr_info,sfreq,part2bits))
        return clip;
#else
      if(fr->lsf)
        part2bits = III_get_scale_factors_2(scalefacs,gr_info,0);
      else
        part2bits = III_get_scale_factors_1(scalefacs,gr_info);
      if(III_dequantize_sample(hybridIn[0], scalefacs,gr_info,sfreq,part2bits))
        return clip;
#endif
    }
    if(stereo == 2) {
      struct gr_info_s *gr_info = &(sideinfo.ch[1].gr[gr]);
      long part2bits;
#ifdef SCALEFACTOR_PER_CHANNEL
      if(fr->lsf) 
        part2bits = III_get_scale_factors_2(scalefacs[1],gr_info,i_stereo);
      else
        part2bits = III_get_scale_factors_1(scalefacs[1],gr_info);

      if(ms_stereo) {
        if(III_dequantize_sample_ms(hybridIn,scalefacs[1],gr_info,sfreq,part2bits))
          return clip;
      }
      else {
        if(III_dequantize_sample(hybridIn[1],scalefacs[1],gr_info,sfreq,part2bits))
          return clip;
      }

      if(i_stereo)
        III_i_stereo(hybridIn,scalefacs[1],gr_info,sfreq,ms_stereo,fr->lsf);
#else
      if(fr->lsf) 
        part2bits = III_get_scale_factors_2(scalefacs,gr_info,i_stereo);
      else
        part2bits = III_get_scale_factors_1(scalefacs,gr_info);

      if(ms_stereo) {
        if(III_dequantize_sample_ms(hybridIn,scalefacs,gr_info,sfreq,part2bits))
          return clip;
      }
      else {
        if(III_dequantize_sample(hybridIn[1],scalefacs,gr_info,sfreq,part2bits))
          return clip;
      }

      if(i_stereo)
        III_i_stereo(hybridIn,scalefacs,gr_info,sfreq,ms_stereo,fr->lsf);
#endif

      if(ms_stereo || i_stereo || (single == 3) ) {
        if(gr_info->maxb > sideinfo.ch[0].gr[gr].maxb) 
          sideinfo.ch[0].gr[gr].maxb = gr_info->maxb;
        else
          gr_info->maxb = sideinfo.ch[0].gr[gr].maxb;
      }

      switch(single) {
        case 3:
          {
            register int i;
            register real *in0 = (real *) hybridIn[0],*in1 = (real *) hybridIn[1];
            for(i=0;i<(int)(SSLIMIT*gr_info->maxb);i++,in0++)
              *in0 = (*in0 + *in1++); /* *0.5 done by pow-scale */ 
          }
          break;
        case 1:
          {
            register int i;
            register real *in0 = (real *) hybridIn[0],*in1 = (real *) hybridIn[1];
            for(i=0;i<(int)(SSLIMIT*gr_info->maxb);i++)
              *in0++ = *in1++;
          }
          break;
      }
    }

	DoSpectralNotify((real*)hybridIn[0],(real*)hybridIn[1],SBLIMIT,SSLIMIT);

    for(ch=0;ch<stereo1;ch++) {
      struct gr_info_s *gr_info = &(sideinfo.ch[ch].gr[gr]);
      III_antialias(hybridIn[ch],gr_info);
      III_hybrid(hybridIn[ch], hybridOut[ch], ch,gr_info);
    }

	DEBUG2(g_pDebugFile,L"=======DoLayer3========\n");

    for(ss=0;ss<SSLIMIT;ss++) {
      if(single >= 0) {
        clip += (fr->synth_mono)(hybridOut[0][ss],pcm_sample+pcm_point);
      }
      else {
        clip += (fr->synth)(hybridOut[0][ss],0,pcm_sample+pcm_point);
        clip += (fr->synth)(hybridOut[1][ss],1,pcm_sample+pcm_point);
      }
      pcm_point += fr->block_size;
	  DEBUG4(g_pDebugFile,L"\tpcm_point = %d, audiobufsize = %d\n",pcm_point,audiobufsize);
//      if(pcm_point >= audiobufsize)
//		  pcm_point = 0;
//        audio_flush(ai);
    }
  }

  if(single>=0)
	  DoWaveNotify((short*)pcm_sample,pcm_point/2,1);
  else
	  DoWaveNotify((short*)pcm_sample,pcm_point/4,2);

  
  return clip;
}
