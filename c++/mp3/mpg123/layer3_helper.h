#ifndef LAYER3_HELPER_H_
#define LAYER3_HELPER_H_

#include <stdlib.h>

#include "mpg123.h"
#include "huffman.h"

#include "getbits.h"

#include "layer3.h"

#include "options.h"
//#include "AutoProfile.h"

#ifdef HEAPIFY
extern real *ispow;
#else
extern real ispow[8207];
#endif	// HEAPIFY
extern real aa_ca[8];
extern real aa_cs[8];
extern real COS1[12][6];
extern real win[4][36];
extern real win1[4][36];
#ifdef HEAPIFY
extern real *gainpow2;
#else
extern real gainpow2[256+118+4];
#endif	// HEAPIFY
extern real COS9[9];
extern real COS6_1;
extern real COS6_2;
extern real tfcos36[9];
extern real tfcos12[3];
#ifdef NEW_DCT9
extern real cos9[3]
extern real cos18[3];
#endif

extern struct bandInfoStruct bandInfo[9];

extern int mapbuf0[9][152];
extern int mapbuf1[9][156];
extern int mapbuf2[9][44];
extern int *map[9][3];
extern int *mapend[9][3];

extern unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
extern unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

extern real tan1_1[16];
extern real tan2_1[16];
extern real tan1_2[16];
extern real tan2_2[16];
extern real pow1_1[2][16];
extern real pow2_1[2][16];
extern real pow1_2[2][16];
extern real pow2_2[2][16];


extern void III_get_side_info_1(struct III_sideinfo *si,int stereo, int ms_stereo,long sfreq,int single);
extern void III_get_side_info_2(struct III_sideinfo *si,int stereo, int ms_stereo,long sfreq,int single);
extern int III_get_scale_factors_1(int *scf,struct gr_info_s *gr_info);
extern int III_get_scale_factors_2(int *scf,struct gr_info_s *gr_info,int i_stereo);
extern int III_dequantize_sample(real xr[SBLIMIT][SSLIMIT],int *scf, struct gr_info_s *gr_info,int sfreq,int part2bits);
extern int III_dequantize_sample_ms(real xr[2][SBLIMIT][SSLIMIT],int *scf, struct gr_info_s *gr_info,int sfreq,int part2bits);

#endif	// LAYER3_HELPER_H_
