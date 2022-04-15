/* 
 * Mpeg Layer-3 audio decoder 
 * --------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp.
 * All rights reserved. See also 'README'
 *
 * - I'm currently working on that .. needs a few more optimizations,
 *   though the code is now fast enough to run in realtime on a 100Mhz 486
 * - a few personal notes are in german .. 
 *
 * used source: 
 *   mpeg1_iis package
 */

#include <stdlib.h>


#include "mpg123.h"
#include "huffman.h"

#include "getbits.h"
#include "layer3.h"

#include "options.h"
#include "AutoProfile.h"

extern DownSampleRate g_DownSampleRate;

#define INITLAYER3_TABLES //If defined, uses pre-generated tables instead of making them
							//on the fly in init_layer3

#ifdef INITLAYER3_TABLES
#include "initlayer3.cpp"
#else
#ifdef HEAPIFY
real *ispow;
#else
real ispow[8207];
#endif	// HEAPIFY
real aa_ca[8],aa_cs[8];
real COS1[12][6];
real win[4][36];
real win1[4][36];
#ifdef HEAPIFY
real *gainpow2;
#else
real gainpow2[256+118+4];
#endif	// HEAPIFY
real COS9[9];
real COS6_1,COS6_2;
real tfcos36[9];
real tfcos12[3];
#ifdef NEW_DCT9
real cos9[3],cos18[3];
#endif

//int *longLimit[9][23];

//int longLimit[9][23]; 
//int shortLimit[9][23];

int longLimit[3][9][23];
int shortLimit[3][9][23];



int mapbuf0[9][152];
int mapbuf1[9][156];
int mapbuf2[9][44];

unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

real tan1_1[16],tan2_1[16],tan1_2[16],tan2_2[16];
real pow1_1[2][16],pow2_1[2][16],pow1_2[2][16],pow2_2[2][16];
#endif //INITLAYER3_TABLES

int *map[9][3];
int *mapend[9][3];

/* 
 * init tables for layer-3 
 */
struct bandInfoStruct bandInfo[9] = { 

/* MPEG 1.0 */
 { {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
   {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
   {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
   {4,4,4,4,6,8,10,12,14,18,22,30,56} } ,

 { {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
   {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
   {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
   {4,4,4,4,6,6,10,12,14,16,20,26,66} } ,

 { {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576} ,
   {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26} ,
   {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3} ,
   {4,4,4,4,6,8,12,16,20,26,34,42,12} }  ,

/* MPEG 2.0 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
   {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
   {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
   {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,52,64,70,76,36 } ,
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3} ,
   {4,4,4,6,8,10,12,14,18,24,32,44,12 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
   {4,4,4,6,8,10,12,14,18,24,30,40,18 } } ,
/* MPEG 2.5 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576},
   {12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2},
   {0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576},
   {8,8,8,12,16,20,24,28,36,2,2,2,26} } ,
};

#ifdef INITLAYER3_TABLES
void init_layer3(int down_samp)
{
  for(int j=0;j<9;j++)
  {
   struct bandInfoStruct *bi = &bandInfo[j];
   int *mp;

   mp = map[j][0] = mapbuf0[j];
   mapend[j][0] = mp + 192;
	
   mp = map[j][1] = mapbuf1[j];
   mapend[j][1] = mp + 160;

   mp = map[j][2] = mapbuf2[j];
   mapend[j][2] = mp + 44;
  }

	return;
}
#else
void init_layer3(int down_samp)
{
  int i,j,k,l;

#ifdef HEAPIFY
  gainpow2 = new real [256+118+4];
  //table pre-calculated in layer3tables.cpp
#endif	// HEAPIFY
  for(i=-256;i<118+4;i++)
    gainpow2[i+256] = REAL_FLOAT(pow((double)2.0,-0.25 * (double) (i+210) ));


#ifdef HEAPIFY
  ispow = new real[8207];
#endif	// HEAPIFY

  for(i=0;i<8207;i++)
//    ispow[i] = pow((double)i,(double)4.0/3.0);
    ispow[i] = REAL_FLOAT(pow((double)i,(double)4.0/3.0));

  for (i=0;i<8;i++)
  {
    static double Ci[8]={-0.6f,-0.535f,-0.33f,-0.185f,-0.095f,-0.041f,-0.0142f,-0.0037f};
    double sq=(double)sqrt(1.0f+Ci[i]*Ci[i]);
    aa_cs[i] = REAL_FLOAT(1.0/sq);
    aa_ca[i] = REAL_FLOAT(Ci[i]/sq);
  }

  for(i=0;i<18;i++)
  {
    win[0][i]    = win[1][i]    = REAL_FLOAT(0.5 * sin( M_PI / 72.0 * (double) (2*(i+0) +1) ) / cos ( M_PI * (double) (2*(i+0) +19) / 72.0 ));
    win[0][i+18] = win[3][i+18] = REAL_FLOAT(0.5 * sin( M_PI / 72.0 * (double) (2*(i+18)+1) ) / cos ( M_PI * (double) (2*(i+18)+19) / 72.0 ));
  }
  for(i=0;i<6;i++)
  {
    win[1][i+18] = REAL_FLOAT(0.5 / cos ( M_PI * (double) (2*(i+18)+19) / 72.0 ));
    win[3][i+12] = REAL_FLOAT(0.5 / cos ( M_PI * (double) (2*(i+12)+19) / 72.0 ));
    win[1][i+24] = REAL_FLOAT(0.5 * sin( M_PI / 24.0 * (double) (2*i+13) ) / cos ( M_PI * (double) (2*(i+24)+19) / 72.0 ));
    win[1][i+30] = win[3][i] = REAL_ZERO;
    win[3][i+6 ] = REAL_FLOAT(0.5 * sin( M_PI / 24.0 * (double) (2*i+1) )  / cos ( M_PI * (double) (2*(i+6 )+19) / 72.0 ));
  }

  for(i=0;i<9;i++)
    COS9[i] = REAL_FLOAT(cos( M_PI / 18.0 * (double) i));

  for(i=0;i<9;i++)
    tfcos36[i] = REAL_FLOAT(0.5 / cos ( M_PI * (double) (i*2+1) / 36.0 ));
  for(i=0;i<3;i++)
    tfcos12[i] = REAL_FLOAT(0.5 / cos ( M_PI * (double) (i*2+1) / 12.0 ));

  COS6_1 = REAL_FLOAT(cos( M_PI / 6.0 * (double) 1));
  COS6_2 = REAL_FLOAT(cos( M_PI / 6.0 * (double) 2));

#ifdef NEW_DCT9
  cos9[0] = cos(1.0*M_PI/9.0);
  cos9[1] = cos(5.0*M_PI/9.0);
  cos9[2] = cos(7.0*M_PI/9.0);
  cos18[0] = cos(1.0*M_PI/18.0);
  cos18[1] = cos(11.0*M_PI/18.0);
  cos18[2] = cos(13.0*M_PI/18.0);
#endif

  for(i=0;i<12;i++)
  {
    win[2][i]  = REAL_FLOAT(0.5 * sin( M_PI / 24.0 * (double) (2*i+1) ) / cos ( M_PI * (double) (2*i+7) / 24.0 ));
    for(j=0;j<6;j++)
      COS1[i][j] = REAL_FLOAT(cos( M_PI / 24.0 * (double) ((2*i+7)*(2*j+1)) ));
  }

  for(j=0;j<4;j++) {
    static int len[4] = { 36,36,12,36 };
    for(i=0;i<len[j];i+=2)
//      win1[j][i] = + win[j][i];
      win1[j][i] = win[j][i];
    for(i=1;i<len[j];i+=2)
      win1[j][i] = - win[j][i];
  }

  for(i=0;i<16;i++)
  {
    double t = (double)tan( (double) i * M_PI / 12.0 );
    tan1_1[i] = REAL_FLOAT(t / (1.0+t));
    tan2_1[i] = REAL_FLOAT(1.0 / (1.0 + t));
    tan1_2[i] = REAL_FLOAT(M_SQRT2 * t / (1.0+t));
    tan2_2[i] = REAL_FLOAT(M_SQRT2 / (1.0 + t));

    for(j=0;j<2;j++) {
      double base = (double)pow(2.0,-0.25*(j+1.0));
      double p1=1.0,p2=1.0;
      if(i > 0) {
        if( i & 1 )
          p1 = (double)pow(base,(i+1.0)*0.5);
        else
          p2 = (double)pow(base,i*0.5);
      }
      pow1_1[j][i] = REAL_FLOAT(p1);
      pow2_1[j][i] = REAL_FLOAT(p2);
      pow1_2[j][i] = REAL_FLOAT(M_SQRT2 * p1);
      pow2_2[j][i] = REAL_FLOAT(M_SQRT2 * p2);
    }
  }

  for(j=0;j<9;j++)
  {
   struct bandInfoStruct *bi = &bandInfo[j];
   int *mp;
   int cb,lwin;
   int *bdf;

   mp = map[j][0] = mapbuf0[j];
   bdf = bi->longDiff;
   for(i=0,cb = 0; cb < 8 ; cb++,i+=*bdf++) {
     *mp++ = (*bdf) >> 1;
     *mp++ = i;
     *mp++ = 3;
     *mp++ = cb;
   }
   bdf = bi->shortDiff+3;
   for(cb=3;cb<13;cb++) {
     int l = (*bdf++) >> 1;
     for(lwin=0;lwin<3;lwin++) {
       *mp++ = l;
       *mp++ = i + lwin;
       *mp++ = lwin;
       *mp++ = cb;
     }
     i += 6*l;
   }
   mapend[j][0] = mp;

   mp = map[j][1] = mapbuf1[j];
   bdf = bi->shortDiff+0;
   for(i=0,cb=0;cb<13;cb++) {
     int l = (*bdf++) >> 1;
     for(lwin=0;lwin<3;lwin++) {
       *mp++ = l;
       *mp++ = i + lwin;
       *mp++ = lwin;
       *mp++ = cb;
     }
     i += 6*l;
   }
   mapend[j][1] = mp;

   mp = map[j][2] = mapbuf2[j];
   bdf = bi->longDiff;
   for(cb = 0; cb < 22 ; cb++) {
     *mp++ = (*bdf++) >> 1;
     *mp++ = cb;
   }
   mapend[j][2] = mp;

  }

  for(k=0;k<3;k++)
  {
	for(j=0;j<9;j++) 
	{
		for(i=0;i<23;i++) {
		longLimit[k][j][i] = (bandInfo[j].longIdx[i] - 1 + 8) / 18 + 1;
		//if(longLimit[k][j][i] > (SBLIMIT >> down_samp) )
	    //    longLimit[k][j][i] = SBLIMIT >> down_samp;
		if(longLimit[k][j][i] > (SBLIMIT >> k) )
			longLimit[k][j][i] = SBLIMIT >> k;
		}
		for(i=0;i<14;i++) {
		  shortLimit[k][j][i] = (bandInfo[j].shortIdx[i] - 1) / 18 + 1;
		  //if(shortLimit[k][j][i] > (SBLIMIT >> down_samp) )
		  //shortLimit[k][j][i] = SBLIMIT >> down_samp;
		  if(shortLimit[k][j][i] > (SBLIMIT >> k))
			 shortLimit[k][j][i] = SBLIMIT >> k;
		}
	}
  }

  for(i=0;i<5;i++) {
    for(j=0;j<6;j++) {
      for(k=0;k<6;k++) {
        int n = k + j * 6 + i * 36;
        i_slen2[n] = i|(j<<3)|(k<<6)|(3<<12);
      }
    }
  }
  for(i=0;i<4;i++) {
    for(j=0;j<4;j++) {
      for(k=0;k<4;k++) {
        int n = k + j * 4 + i * 16;
        i_slen2[n+180] = i|(j<<3)|(k<<6)|(4<<12);
      }
    }
  }
  for(i=0;i<4;i++) {
    for(j=0;j<3;j++) {
      int n = j + i * 3;
      i_slen2[n+244] = i|(j<<3) | (5<<12);
      n_slen2[n+500] = i|(j<<3) | (2<<12) | (1<<15);
    }
  }

  for(i=0;i<5;i++) {
    for(j=0;j<5;j++) {
      for(k=0;k<4;k++) {
        for(l=0;l<4;l++) {
          int n = l + k * 4 + j * 16 + i * 80;
          n_slen2[n] = i|(j<<3)|(k<<6)|(l<<9)|(0<<12);
        }
      }
    }
  }

  for(i=0;i<5;i++) {
    for(j=0;j<5;j++) {
      for(k=0;k<4;k++) {
        int n = k + j * 4 + i * 20;
        n_slen2[n+400] = i|(j<<3)|(k<<6)|(1<<12);
      }
    }
  }
}
#endif //INITLAYER3_TABLES
/*
 * read additional side information
 */
void III_get_side_info_1(struct III_sideinfo *si,int stereo,
 int ms_stereo,long sfreq,int single)
{
#ifdef PROFILE
   FunctionProfiler fp(L"III_get_side_info_1");
#endif

   int ch, gr;
   int powdiff = (single == 3) ? 4 : 0;

   si->main_data_begin = getbits(9);
   if (stereo == 1)
     si->private_bits = getbits_fast(5);
   else 
     si->private_bits = getbits_fast(3);

   for (ch=0; ch<stereo; ch++) {
       si->ch[ch].gr[0].scfsi = -1;
       si->ch[ch].gr[1].scfsi = getbits_fast(4);
   }

   for (gr=0; gr<2; gr++) 
   {
     for (ch=0; ch<stereo; ch++) 
     {
       register struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);

       gr_info->part2_3_length = getbits(12);
       gr_info->big_values = getbits(9);
       if(gr_info->big_values > 288) {
//          fprintf(stderr,"big_values too large!\n");
          gr_info->big_values = 288;
       }
       gr_info->pow2gain = gainpow2+256 - getbits_fast(8) + powdiff;
       if(ms_stereo)
         gr_info->pow2gain += 2;
       gr_info->scalefac_compress = getbits_fast(4);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
       if(get1bit()) 
       {
         int i;
         gr_info->block_type = getbits_fast(2);
         gr_info->mixed_block_flag = get1bit();
         gr_info->table_select[0] = getbits_fast(5);
         gr_info->table_select[1] = getbits_fast(5);
         /*
          * table_select[2] not needed, because there is no region2,
          * but to satisfy some verifications tools we set it either.
          */
         gr_info->table_select[2] = 0;
         for(i=0;i<3;i++)
           gr_info->full_gain[i] = gr_info->pow2gain + (getbits_fast(3)<<3);

         if(gr_info->block_type == 0) {
//           fprintf(stderr,"Blocktype == 0 and window-switching == 1 not allowed.\n");
//	TODO: Exit gracefully
//           exit(1);
         }
         /* region_count/start parameters are implicit in this case. */       
         gr_info->region1start = 36>>1;
         gr_info->region2start = 576>>1;
       }
       else 
       {
         int i,r0c,r1c;
         for (i=0; i<3; i++)
           gr_info->table_select[i] = getbits_fast(5);
         r0c = getbits_fast(4);
         r1c = getbits_fast(3);
         gr_info->region1start = bandInfo[sfreq].longIdx[r0c+1] >> 1 ;
         gr_info->region2start = bandInfo[sfreq].longIdx[r0c+1+r1c+1] >> 1;

		 //Matt/Ed - quick fix to handle cases when MP3 file goes beyond our
		 //bandInfo table.  In that case, use the regular 576 >> 1 for region2start.
		 if(r0c + r1c + 1 + 1 > 22)
			 gr_info->region2start = 576 >> 1;

         gr_info->block_type = 0;
         gr_info->mixed_block_flag = 0;
       }
       gr_info->preflag = get1bit();
       gr_info->scalefac_scale = get1bit();
       gr_info->count1table_select = get1bit();
     }
   }
}

/*
 * Side Info for MPEG 2.0 / LSF
 */
void III_get_side_info_2(struct III_sideinfo *si,int stereo,
 int ms_stereo,long sfreq,int single)
{
#ifdef PROFILE
   FunctionProfiler fp(L"III_get_side_info_2");
#endif

   int ch;
   int powdiff = (single == 3) ? 4 : 0;

   si->main_data_begin = getbits_fast(8);
   if (stereo == 1)
     si->private_bits = get1bit();
   else 
     si->private_bits = getbits_fast(2);

   for (ch=0; ch<stereo; ch++) 
   {
       register struct gr_info_s *gr_info = &(si->ch[ch].gr[0]);

       gr_info->part2_3_length = getbits(12);
       gr_info->big_values = getbits(9);
       if(gr_info->big_values > 288) {
//         fprintf(stderr,"big_values too large!\n");
         gr_info->big_values = 288;
       }
       gr_info->pow2gain = gainpow2+256 - getbits_fast(8) + powdiff;
       if(ms_stereo)
         gr_info->pow2gain += 2;
       gr_info->scalefac_compress = getbits(9);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
       if(get1bit()) 
       {
         int i;
         gr_info->block_type = getbits_fast(2);
         gr_info->mixed_block_flag = get1bit();
         gr_info->table_select[0] = getbits_fast(5);
         gr_info->table_select[1] = getbits_fast(5);
         /*
          * table_select[2] not needed, because there is no region2,
          * but to satisfy some verifications tools we set it either.
          */
         gr_info->table_select[2] = 0;
         for(i=0;i<3;i++)
           gr_info->full_gain[i] = gr_info->pow2gain + (getbits_fast(3)<<3);

         if(gr_info->block_type == 0) {
//           fprintf(stderr,"Blocktype == 0 and window-switching == 1 not allowed.\n");
//	TODO: Exit gracefully
//           exit(1);
         }
         /* region_count/start parameters are implicit in this case. */       
/* check this again! */
         if(gr_info->block_type == 2)
           gr_info->region1start = 36>>1;
         else
           gr_info->region1start = 54>>1;
         gr_info->region2start = 576>>1;
/* check this for 2.5 and sfreq=8 */
       }
       else 
       {
         int i,r0c,r1c;
         for (i=0; i<3; i++)
           gr_info->table_select[i] = getbits_fast(5);
         r0c = getbits_fast(4);
         r1c = getbits_fast(3);
         gr_info->region1start = bandInfo[sfreq].longIdx[r0c+1] >> 1 ;
         gr_info->region2start = bandInfo[sfreq].longIdx[r0c+1+r1c+1] >> 1;
         gr_info->block_type = 0;
         gr_info->mixed_block_flag = 0;
       }
       gr_info->scalefac_scale = get1bit();
       gr_info->count1table_select = get1bit();
   }
}

#define NEW_SCALEFACTORS
//#undef NEW_SCALEFACTORS
/*
 * read scalefactors
 */
int III_get_scale_factors_1(int *scf,struct gr_info_s *gr_info)
{
#ifdef PROFILE
   FunctionProfiler fp(L"III_get_scale_factors_1");
#endif

   static unsigned char slen[2][16] = {
     {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
     {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
   };
   int numbits;
   int num0 = slen[0][gr_info->scalefac_compress];
   int num1 = slen[1][gr_info->scalefac_compress];

    if (gr_info->block_type == 2) 
    {
      int i=18;
      numbits = (num0 + num1) * 18;

      if (gr_info->mixed_block_flag) {
         for (i=8;i;i--)
           *scf++ = getbits_fast(num0);
         i = 9;
         numbits -= num0; /* num0 * 17 + num1 * 18 */
      }

      for (;i;i--)
        *scf++ = getbits_fast(num0);
      for (i = 18; i; i--)
        *scf++ = getbits_fast(num1);
      *scf++ = 0; *scf++ = 0; *scf++ = 0; /* short[13][0..2] = 0 */
    }
    else 
    {
      int i;
      int scfsi = gr_info->scfsi;

      if(scfsi < 0) { /* scfsi < 0 => granule == 0 */
         for(i=11;i;i--)
           *scf++ = getbits_fast(num0);
         for(i=10;i;i--)
           *scf++ = getbits_fast(num1);
         numbits = (num0 + num1) * 10 + num0;
#ifdef NEW_SCALEFACTORS
		 *scf++ = 0;
#endif
      }
      else {
        numbits = 0;
        if(!(scfsi & 0x8)) {
          //for (i=6;i;i--)
		  for(i=0;i<6;i++)
            *scf++ = getbits_fast(num0);
          numbits += num0 * 6;
        }
        else {
#ifdef NEW_SCALEFACTORS
			scf+=6;
#else
          *scf++ = 0; *scf++ = 0; *scf++ = 0;  /* set to ZERO necessary? */
          *scf++ = 0; *scf++ = 0; *scf++ = 0;
#endif
        }

        if(!(scfsi & 0x4)) {
          //for (i=5;i;i--)
		  for(i=0;i<5;i++)
            *scf++ = getbits_fast(num0);
          numbits += num0 * 5;
        }
        else {
#ifdef NEW_SCALEFACTORS
			scf+=5;
#else
			*scf++ = 0; *scf++ = 0; *scf++ = 0;  /* set to ZERO necessary? */
            *scf++ = 0; *scf++ = 0;
#endif
        }

        if(!(scfsi & 0x2)) {
          //for(i=5;i;i--)
		  for(i=0;i<5;i++)
            *scf++ = getbits_fast(num1);
          numbits += num1 * 5;
        }
        else {
#ifdef NEW_SCALEFACTORS
			scf+=5;
#else
			*scf++ = 0; *scf++ = 0; *scf++ = 0;  /* set to ZERO necessary? */
            *scf++ = 0; *scf++ = 0;
#endif
        }

        if(!(scfsi & 0x1)) {
          //for (i=5;i;i--)
		  for(i=0;i<5;i++)
            *scf++ = getbits_fast(num1);
          numbits += num1 * 5;
        }
        else {
#ifdef NEW_SCALEFACTORS
			scf+=5;
#else
          *scf++ = 0; *scf++ = 0; *scf++ = 0;  /* set to ZERO necessary? */
          *scf++ = 0; *scf++ = 0;
#endif		
        }
#ifdef NEW_SCALEFACTORS
		*scf++ = 0;  /* no l[21] in original sources */	
#endif
      }
#ifndef NEW_SCALEFACTORS
	  *scf++ = 0;  /* no l[21] in original sources */
#endif
    }
    return numbits;
}

int III_get_scale_factors_2(int *scf,struct gr_info_s *gr_info,int i_stereo)
{
#ifdef PROFILE
  FunctionProfiler fp(L"III_get_scale_factors_2");
#endif

  unsigned char *pnt;
  int i,j;
  unsigned int slen;
  int n = 0;
  int numbits = 0;

  static unsigned char stab[3][6][4] = {
   { { 6, 5, 5,5 } , { 6, 5, 7,3 } , { 11,10,0,0} ,
     { 7, 7, 7,0 } , { 6, 6, 6,3 } , {  8, 8,5,0} } ,
   { { 9, 9, 9,9 } , { 9, 9,12,6 } , { 18,18,0,0} ,
     {12,12,12,0 } , {12, 9, 9,6 } , { 15,12,9,0} } ,
   { { 6, 9, 9,9 } , { 6, 9,12,6 } , { 15,18,0,0} ,
     { 6,15,12,0 } , { 6,12, 9,6 } , {  6,18,9,0} } }; 

  if(i_stereo) /* i_stereo AND second channel -> do_layer3() checks this */
    slen = i_slen2[gr_info->scalefac_compress>>1];
  else
    slen = n_slen2[gr_info->scalefac_compress];

  gr_info->preflag = (slen>>15) & 0x1;

  n = 0;  
  if( gr_info->block_type == 2 ) {
    n++;
    if(gr_info->mixed_block_flag)
      n++;
  }

  pnt = stab[n][(slen>>12)&0x7];

  for(i=0;i<4;i++) {
    int num = slen & 0x7;
    slen >>= 3;
    if(num) {
      for(j=0;j<pnt[i];j++)
        *scf++ = getbits_fast(num);
      numbits += pnt[i] * num;
    }
    else {
      for(j=0;j<pnt[i];j++)
        *scf++ = 0;
    }
  }
  
  n = (n << 1) + 1;
  for(i=0;i<n;i++)
    *scf++ = 0;

  return numbits;
}

static int pretab1[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
static int pretab2[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*
 * don't forget to apply the same changes to III_dequantize_sample_ms() !!! 
 */
int III_dequantize_sample(real xr[SBLIMIT][SSLIMIT],int *scf,
   struct gr_info_s *gr_info,int sfreq,int part2bits)
{
#ifdef PROFILE
  FunctionProfiler fp(L"III_dequantize_sample");
#endif

  int shift = 1 + gr_info->scalefac_scale;
  real *xrpnt = (real *) xr;
  int l[3],l3;
  int part2remain = gr_info->part2_3_length - part2bits;
  int *me;

  {
    int bv       = gr_info->big_values;
    int region1  = gr_info->region1start;
    int region2  = gr_info->region2start;

    l3 = ((576>>1)-bv)>>1;   
/*
 * we may lose the 'odd' bit here !! 
 * check this later again 
 */
    if(bv <= region1) {
      l[0] = bv; l[1] = 0; l[2] = 0;
    }
    else {
      l[0] = region1;
      if(bv <= region2) {
        l[1] = bv - l[0];  l[2] = 0;
      }
      else {
        l[1] = region2 - l[0]; l[2] = bv - region2;
      }
    }
  }
 
  if(gr_info->block_type == 2) {
    /*
     * decoding with short or mixed mode BandIndex table 
     */
    int i,max[4];
    int step=0,lwin=0,cb=0;
    register real v = REAL_ZERO;
    register int *m,mc;

    if(gr_info->mixed_block_flag) {
      max[3] = -1;
      max[0] = max[1] = max[2] = 2;
      m = map[sfreq][0];
      me = mapend[sfreq][0];
    }
    else {
      max[0] = max[1] = max[2] = max[3] = -1;
      /* max[3] not really needed in this case */
      m = map[sfreq][1];
      me = mapend[sfreq][1];
    }

    mc = 0;
    for(i=0;i<2;i++) {
      int lp = l[i];
      struct newhuff *h = ht+gr_info->table_select[i];
      for(;lp;lp--,mc--) {
        register int x,y;
        if( (!mc) ) {
          mc = *m++;
          xrpnt = ((real *) xr) + (*m++);
          lwin = *m++;
          cb = *m++;
          if(lwin == 3) {
            v = gr_info->pow2gain[(*scf++) << shift];
            step = 1;
          }
          else {
            v = gr_info->full_gain[lwin][(*scf++) << shift];
            step = 3;
          }
        }
        {
          register short *val = h->table;
          while((y=*val++)<0) {
            if (get1bit())
              val -= y;
            part2remain--;
          }
          x = y >> 4;
          y &= 0xf;
        }
        if(x == 15) {
          max[lwin] = cb;
          part2remain -= h->linbits+1;
          x += getbits(h->linbits);
          if(get1bit())
            *xrpnt = MULT_REAL(-ispow[x], v);
          else
            *xrpnt =  MULT_REAL(ispow[x], v);
        }
        else if(x) {
          max[lwin] = cb;
          if(get1bit())
            *xrpnt = MULT_REAL(-ispow[x], v);
          else
            *xrpnt =  MULT_REAL(ispow[x], v);
          part2remain--;
        }
        else
          *xrpnt = REAL_ZERO;
        xrpnt += step;
        if(y == 15) {
          max[lwin] = cb;
          part2remain -= h->linbits+1;
          y += getbits(h->linbits);
          if(get1bit())
            *xrpnt = MULT_REAL(-ispow[y], v);
          else
            *xrpnt =  MULT_REAL(ispow[y], v);
        }
        else if(y) {
          max[lwin] = cb;
          if(get1bit())
            *xrpnt = MULT_REAL(-ispow[y], v);
          else
            *xrpnt =  MULT_REAL(ispow[y], v);
          part2remain--;
        }
        else
          *xrpnt = REAL_ZERO;
        xrpnt += step;
      }
    }
    for(;l3 && (part2remain > 0);l3--) {
      struct newhuff *h = htc+gr_info->count1table_select;
      register short *val = h->table,a;

      while((a=*val++)<0) {
        part2remain--;
        if(part2remain < 0) {
          part2remain++;
          a = 0;
          break;
        }
        if (get1bit())
          val -= a;
      }

      for(i=0;i<4;i++) {
        if(!(i & 1)) {
          if(!mc) {
            mc = *m++;
            xrpnt = ((real *) xr) + (*m++);
            lwin = *m++;
            cb = *m++;
            if(lwin == 3) {
              v = gr_info->pow2gain[(*scf++) << shift];
              step = 1;
            }
            else {
              v = gr_info->full_gain[lwin][(*scf++) << shift];
              step = 3;
            }
          }
          mc--;
        }
        if( (a & (0x8>>i)) ) {
          max[lwin] = cb;
          part2remain--;
          if(part2remain < 0) {
            part2remain++;
            break;
          }
          if(get1bit()) 
            *xrpnt = -v;
          else
            *xrpnt = v;
        }
        else
          *xrpnt = REAL_ZERO;
        xrpnt += step;
      }
    }
 
    while( m < me ) {
      if(!mc) {
        mc = *m++;
        xrpnt = ((real *) xr) + *m++;
        if( (*m++) == 3)
          step = 1;
        else
          step = 3;
        m++; /* cb */
      }
      mc--;
      *xrpnt = REAL_ZERO;
      xrpnt += step;
      *xrpnt = REAL_ZERO;
      xrpnt += step;
/* we could add a little opt. here:
 * if we finished a band for window 3 or a long band
 * further bands could copied in a simple loop without a
 * special 'map' decoding
 */
    }

    gr_info->maxband[0] = max[0]+1;
    gr_info->maxband[1] = max[1]+1;
    gr_info->maxband[2] = max[2]+1;
    gr_info->maxbandl = max[3]+1;

    {
      int rmax = max[0] > max[1] ? max[0] : max[1];
      rmax = (rmax > max[2] ? rmax : max[2]) + 1;
      gr_info->maxb = rmax ? shortLimit[g_DownSampleRate][sfreq][rmax] : longLimit[g_DownSampleRate][sfreq][max[3]+1];
    }

  }
  else {
	/*
     * decoding with 'long' BandIndex table (block_type != 2)
     */
    int *pretab = gr_info->preflag ? pretab1 : pretab2;
    int i,max = -1;
    int cb = 0;
    register int *m = map[sfreq][2];
    register real v = REAL_ZERO;
    register int mc = 0;

	/*
     * long hash table values
     */
    for(i=0;i<3;i++) {
      int lp = l[i];
      struct newhuff *h = ht+gr_info->table_select[i];

      for(;lp;lp--,mc--) {
        int x,y;

        if(!mc) {
          mc = *m++;
          v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
          cb = *m++;
        }
        {
          register short *val = h->table;
          while((y=*val++)<0) {
            if (get1bit())
              val -= y;
            part2remain--;
          }
          x = y >> 4;
          y &= 0xf;
        }
        if (x == 15) {
          max = cb;
          part2remain -= h->linbits+1;
          x += getbits(h->linbits);
          if(get1bit())
            *xrpnt++ = MULT_REAL(-ispow[x], v);
          else
            *xrpnt++ =  MULT_REAL(ispow[x], v);
        }
        else if(x) {
          max = cb;
          if(get1bit())
            *xrpnt++ = MULT_REAL(-ispow[x], v);
          else
            *xrpnt++ =  MULT_REAL(ispow[x], v);
          part2remain--;
        }
        else
          *xrpnt++ = REAL_ZERO;

        if (y == 15) {
          max = cb;
          part2remain -= h->linbits+1;
          y += getbits(h->linbits);
          if(get1bit())
            *xrpnt++ = MULT_REAL(-ispow[y], v);
          else
            *xrpnt++ =  MULT_REAL(ispow[y], v);
        }
        else if(y) {
          max = cb;
          if(get1bit())
            *xrpnt++ = MULT_REAL(-ispow[y], v);
          else
            *xrpnt++ =  MULT_REAL(ispow[y], v);
          part2remain--;
        }
        else
          *xrpnt++ = REAL_ZERO;
      }
    }

	/*
     * short (count1table) values
     */
    for(;l3 && (part2remain > 0);l3--) {
      struct newhuff *h = htc+gr_info->count1table_select;
      register short *val = h->table,a;

      while((a=*val++)<0) {
        part2remain--;
        if(part2remain < 0) {
          part2remain++;
          a = 0;
          break;
        }
        if (get1bit())
          val -= a;
      }

      for(i=0;i<4;i++) {
        if(!(i & 1)) {
          if(!mc) {
            mc = *m++;
            cb = *m++;
            v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
          }
          mc--;
        }
        if ( (a & (0x8>>i)) ) {
          max = cb;
          part2remain--;
          if(part2remain < 0) {
            part2remain++;
            break;
          }
          if(get1bit())
            *xrpnt++ = -v;
          else
            *xrpnt++ = v;
        }
        else
          *xrpnt++ = REAL_ZERO;
      }
    }

	/* 
     * zero part
     */
    for(i=(&xr[SBLIMIT][0]-xrpnt)>>1;i;i--) {
      *xrpnt++ = REAL_ZERO;
      *xrpnt++ = REAL_ZERO;
    }

    gr_info->maxbandl = max+1;
    gr_info->maxb = longLimit[g_DownSampleRate][sfreq][gr_info->maxbandl];
  }

  while( part2remain > 16 ) {
    getbits(16); /* Dismiss stuffing Bits */
    part2remain -= 16;
  }
  if(part2remain > 0)
    getbits(part2remain);
  else if(part2remain < 0) {
//    fprintf(stderr,"mpg123: Can't rewind stream by %d bits!\n",-part2remain);
    return 1; /* -> error */
  }
  return 0;
}



int III_dequantize_sample_ms(real xr[2][SBLIMIT][SSLIMIT],int *scf,
   struct gr_info_s *gr_info,int sfreq,int part2bits)
{
#ifdef PROFILE
  FunctionProfiler fp(L"III_dequantize_sample_ms");
#endif

  int shift = 1 + gr_info->scalefac_scale;
  real *xrpnt = (real *) xr[1];
  real *xr0pnt = (real *) xr[0];
  int l[3],l3;
  int part2remain = gr_info->part2_3_length - part2bits;
  int *me;

  {
    int bv       = gr_info->big_values;
    int region1  = gr_info->region1start;
    int region2  = gr_info->region2start;

    l3 = ((576>>1)-bv)>>1;   
/*
 * we may lose the 'odd' bit here !! 
 * check this later gain 
 */
    if(bv <= region1) {
      l[0] = bv; l[1] = 0; l[2] = 0;
    }
    else {
      l[0] = region1;
      if(bv <= region2) {
        l[1] = bv - l[0];  l[2] = 0;
      }
      else {
        l[1] = region2 - l[0]; l[2] = bv - region2;
      }
    }
  }
 
  if(gr_info->block_type == 2) {
    int i,max[4];
    int step=0,lwin=0,cb=0;
    register real v = REAL_ZERO;
    register int *m,mc = 0;

    if(gr_info->mixed_block_flag) {
      max[3] = -1;
      max[0] = max[1] = max[2] = 2;
      m = map[sfreq][0];
      me = mapend[sfreq][0];
    }
    else {
      max[0] = max[1] = max[2] = max[3] = -1;
      /* max[3] not really needed in this case */
      m = map[sfreq][1];
      me = mapend[sfreq][1];
    }

    for(i=0;i<2;i++) {
      int lp = l[i];
      struct newhuff *h = ht+gr_info->table_select[i];
      for(;lp;lp--,mc--) {
        int x,y;

        if(!mc) {
          mc = *m++;
          xrpnt = ((real *) xr[1]) + *m;
          xr0pnt = ((real *) xr[0]) + *m++;
          lwin = *m++;
          cb = *m++;
          if(lwin == 3) {
            v = gr_info->pow2gain[(*scf++) << shift];
            step = 1;
          }
          else {
            v = gr_info->full_gain[lwin][(*scf++) << shift];
            step = 3;
          }
        }
        {
          register short *val = h->table;
          while((y=*val++)<0) {
            if (get1bit())
              val -= y;
            part2remain--;
          }
          x = y >> 4;
          y &= 0xf;
        }
        if(x == 15) {
          max[lwin] = cb;
          part2remain -= h->linbits+1;
          x += getbits(h->linbits);
          if(get1bit()) {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt = *xr0pnt + a;
            *xr0pnt -= a;
          }
          else {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt = *xr0pnt - a;
            *xr0pnt += a;
          }
        }
        else if(x) {
          max[lwin] = cb;
          if(get1bit()) {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt = *xr0pnt + a;
            *xr0pnt -= a;
          }
          else {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt = *xr0pnt - a;
            *xr0pnt += a;
          }
          part2remain--;
        }
        else
          *xrpnt = *xr0pnt;
        xrpnt += step;
        xr0pnt += step;

        if(y == 15) {
          max[lwin] = cb;
          part2remain -= h->linbits+1;
          y += getbits(h->linbits);
          if(get1bit()) {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt = *xr0pnt + a;
            *xr0pnt -= a;
          }
          else {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt = *xr0pnt - a;
            *xr0pnt += a;
          }
        }
        else if(y) {
          max[lwin] = cb;
          if(get1bit()) {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt = *xr0pnt + a;
            *xr0pnt -= a;
          }
          else {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt = *xr0pnt - a;
            *xr0pnt += a;
          }
          part2remain--;
        }
        else
          *xrpnt = *xr0pnt;
        xrpnt += step;
        xr0pnt += step;
      }
    }

    for(;l3 && (part2remain > 0);l3--) {
      struct newhuff *h = htc+gr_info->count1table_select;
      register short *val = h->table,a;

      while((a=*val++)<0) {
        part2remain--;
        if(part2remain < 0) {
          part2remain++;
          a = 0;
          break;
        }
        if (get1bit())
          val -= a;
      }

      for(i=0;i<4;i++) {
        if(!(i & 1)) {
          if(!mc) {
            mc = *m++;
            xrpnt = ((real *) xr[1]) + *m;
            xr0pnt = ((real *) xr[0]) + *m++;
            lwin = *m++;
            cb = *m++;
            if(lwin == 3) {
              v = gr_info->pow2gain[(*scf++) << shift];
              step = 1;
            }
            else {
              v = gr_info->full_gain[lwin][(*scf++) << shift];
              step = 3;
            }
          }
          mc--;
        }
        if( (a & (0x8>>i)) ) {
          max[lwin] = cb;
          part2remain--;
          if(part2remain < 0) {
            part2remain++;
            break;
          }
          if(get1bit()) {
            *xrpnt = *xr0pnt + v;
            *xr0pnt -= v;
          }
          else {
            *xrpnt = *xr0pnt - v;
            *xr0pnt += v;
          }
        }
        else
          *xrpnt = *xr0pnt;
        xrpnt += step;
        xr0pnt += step;
      }
    }
 
    while( m < me ) {
      if(!mc) {
        mc = *m++;
        xrpnt = ((real *) xr[1]) + *m;
        xr0pnt = ((real *) xr[0]) + *m++;
        if(*m++ == 3)
          step = 1;
        else
          step = 3;
        m++; /* cb */
      }
      mc--;
      *xrpnt = *xr0pnt;
      xrpnt += step;
      xr0pnt += step;
      *xrpnt = *xr0pnt;
      xrpnt += step;
      xr0pnt += step;
/* we could add a little opt. here:
 * if we finished a band for window 3 or a long band
 * further bands could copied in a simple loop without a
 * special 'map' decoding
 */
    }

    gr_info->maxband[0] = max[0]+1;
    gr_info->maxband[1] = max[1]+1;
    gr_info->maxband[2] = max[2]+1;
    gr_info->maxbandl = max[3]+1;

    {
      int rmax = max[0] > max[1] ? max[0] : max[1];
      rmax = (rmax > max[2] ? rmax : max[2]) + 1;
      gr_info->maxb = rmax ? shortLimit[g_DownSampleRate][sfreq][rmax] : longLimit[g_DownSampleRate][sfreq][max[3]+1];
    }
  }
  else {
    int *pretab = gr_info->preflag ? pretab1 : pretab2;
    int i,max = -1;
    int cb = 0;
    register int mc=0,*m = map[sfreq][2];
    register real v = REAL_ZERO;

    for(i=0;i<3;i++) {
      int lp = l[i];
      struct newhuff *h = ht+gr_info->table_select[i];

      for(;lp;lp--,mc--) {
        int x,y;
        if(!mc) {
          mc = *m++;
          cb = *m++;
          v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
        }
        {
          register short *val = h->table;
          while((y=*val++)<0) {
            if (get1bit())
              val -= y;
            part2remain--;
          }
          x = y >> 4;
          y &= 0xf;
        }
        if (x == 15) {
          max = cb;
          part2remain -= h->linbits+1;
          x += getbits(h->linbits);
          if(get1bit()) {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt++ = *xr0pnt + a;
            *xr0pnt++ -= a;
          }
          else {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt++ = *xr0pnt - a;
            *xr0pnt++ += a;
          }
        }
        else if(x) {
          max = cb;
          if(get1bit()) {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt++ = *xr0pnt + a;
            *xr0pnt++ -= a;
          }
          else {
            real a = MULT_REAL(ispow[x], v);
            *xrpnt++ = *xr0pnt - a;
            *xr0pnt++ += a;
          }
          part2remain--;
        }
        else
          *xrpnt++ = *xr0pnt++;

        if (y == 15) {
          max = cb;
          part2remain -= h->linbits+1;
          y += getbits(h->linbits);
          if(get1bit()) {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt++ = *xr0pnt + a;
            *xr0pnt++ -= a;
          }
          else {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt++ = *xr0pnt - a;
            *xr0pnt++ += a;
          }
        }
        else if(y) {
          max = cb;
          if(get1bit()) {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt++ = *xr0pnt + a;
            *xr0pnt++ -= a;
          }
          else {
            real a = MULT_REAL(ispow[y], v);
            *xrpnt++ = *xr0pnt - a;
            *xr0pnt++ += a;
          }
          part2remain--;
        }
        else
          *xrpnt++ = *xr0pnt++;
      }
    }

    for(;l3 && (part2remain > 0);l3--) {
      struct newhuff *h = htc+gr_info->count1table_select;
      register short *val = h->table,a;

      while((a=*val++)<0) {
        part2remain--;
        if(part2remain < 0) {
          part2remain++;
          a = 0;
          break;
        }
        if (get1bit())
          val -= a;
      }

      for(i=0;i<4;i++) {
        if(!(i & 1)) {
          if(!mc) {
            mc = *m++;
            cb = *m++;
            v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
          }
          mc--;
        }
        if ( (a & (0x8>>i)) ) {
          max = cb;
          part2remain--;
          if(part2remain <= 0) {
            part2remain++;
            break;
          }
          if(get1bit()) {
            *xrpnt++ = *xr0pnt + v;
            *xr0pnt++ -= v;
          }
          else {
            *xrpnt++ = *xr0pnt - v;
            *xr0pnt++ += v;
          }
        }
        else
          *xrpnt++ = *xr0pnt++;
      }
    }
    for(i=(&xr[1][SBLIMIT][0]-xrpnt)>>1;i;i--) {
      *xrpnt++ = *xr0pnt++;
      *xrpnt++ = *xr0pnt++;
    }

    gr_info->maxbandl = max+1;
    gr_info->maxb = longLimit[g_DownSampleRate][sfreq][gr_info->maxbandl];
  }

  while ( part2remain > 16 ) {
    getbits(16); /* Dismiss stuffing Bits */
    part2remain -= 16;
  }
  if(part2remain > 0 )
    getbits(part2remain);
  else if(part2remain < 0) {
//    fprintf(stderr,"mpg123_ms: Can't rewind stream by %d bits!\n",-part2remain);
    return 1; /* -> error */
  }
  return 0;
}


