#include "layer3_helper.h"

#include "options.h"
#include "AutoProfile.h"

/* 
 * III_stereo: calculate real channel values for Joint-I-Stereo-mode
 */
void III_i_stereo(real xr_buf[2][SBLIMIT][SSLIMIT],int *scalefac,
   struct gr_info_s *gr_info,int sfreq,int ms_stereo,int lsf)
{
#ifdef PROFILE
	FunctionProfiler fp(L"III_i_stereo");
#endif

      real (*xr)[SBLIMIT*SSLIMIT] = (real (*)[SBLIMIT*SSLIMIT] ) xr_buf;
      struct bandInfoStruct *bi = &bandInfo[sfreq];
      real *tab1,*tab2;

      if(lsf) {
        int p = gr_info->scalefac_compress & 0x1;
	    if(ms_stereo) {
          tab1 = pow1_2[p]; tab2 = pow2_2[p];
        }
        else {
          tab1 = pow1_1[p]; tab2 = pow2_1[p];
        }
      }
      else {
        if(ms_stereo) {
          tab1 = tan1_2; tab2 = tan2_2;
        }
        else {
          tab1 = tan1_1; tab2 = tan2_1;
        }
      }

      if (gr_info->block_type == 2)
      {
         int lwin,do_l = 0;
         if( gr_info->mixed_block_flag )
           do_l = 1;

         for (lwin=0;lwin<3;lwin++) /* process each window */
         {
             /* get first band with zero values */
           int is_p,sb,idx,sfb = gr_info->maxband[lwin];  /* sfb is minimal 3 for mixed mode */
           if(sfb > 3)
             do_l = 0;

           for(;sfb<12;sfb++)
           {
             is_p = scalefac[sfb*3+lwin-gr_info->mixed_block_flag]; /* scale: 0-15 */ 
             if(is_p != 7) {
               real t1,t2;
               sb = bi->shortDiff[sfb];
               idx = bi->shortIdx[sfb] + lwin;
               t1 = tab1[is_p]; t2 = tab2[is_p];
               for (; sb > 0; sb--,idx+=3)
               {
                 real v = xr[0][idx];
                 xr[0][idx] = MULT_REAL(v, t1);
                 xr[1][idx] = MULT_REAL(v, t2);
               }
             }
           }

#if 1
/* in the original: copy 10 to 11 , here: copy 11 to 12 
maybe still wrong??? (copy 12 to 13?) */
           is_p = scalefac[11*3+lwin-gr_info->mixed_block_flag]; /* scale: 0-15 */
           sb = bi->shortDiff[12];
           idx = bi->shortIdx[12] + lwin;
#else
           is_p = scalefac[10*3+lwin-gr_info->mixed_block_flag]; /* scale: 0-15 */
           sb = bi->shortDiff[11];
           idx = bi->shortIdx[11] + lwin;
#endif
           if(is_p != 7)
           {
             real t1,t2;
             t1 = tab1[is_p]; t2 = tab2[is_p];
             for ( ; sb > 0; sb--,idx+=3 )
             {  
               real v = xr[0][idx];
               xr[0][idx] = MULT_REAL(v, t1);
               xr[1][idx] = MULT_REAL(v, t2);
             }
           }
         } /* end for(lwin; .. ; . ) */

         if (do_l)
         {
/* also check l-part, if ALL bands in the three windows are 'empty'
 * and mode = mixed_mode 
 */
           int sfb = gr_info->maxbandl;
           int idx = bi->longIdx[sfb];

           for ( ; sfb<8; sfb++ )
           {
             int sb = bi->longDiff[sfb];
             int is_p = scalefac[sfb]; /* scale: 0-15 */
             if(is_p != 7) {
               real t1,t2;
               t1 = tab1[is_p]; t2 = tab2[is_p];
               for ( ; sb > 0; sb--,idx++)
               {
                 real v = xr[0][idx];
                 xr[0][idx] = MULT_REAL(v, t1);
                 xr[1][idx] = MULT_REAL(v, t2);
               }
             }
             else 
               idx += sb;
           }
         }     
      } 
      else /* ((gr_info->block_type != 2)) */
      {
        int sfb = gr_info->maxbandl;
        int is_p,idx = bi->longIdx[sfb];
        for ( ; sfb<21; sfb++)
        {
          int sb = bi->longDiff[sfb];
          is_p = scalefac[sfb]; /* scale: 0-15 */
          if(is_p != 7) {
            real t1,t2;
            t1 = tab1[is_p]; t2 = tab2[is_p];
            for ( ; sb > 0; sb--,idx++)
            {
               real v = xr[0][idx];
               xr[0][idx] = MULT_REAL(v, t1);
               xr[1][idx] = MULT_REAL(v, t2);
            }
          }
          else
            idx += sb;
        }

        is_p = scalefac[20]; /* copy l-band 20 to l-band 21 */
        if(is_p != 7)
        {
          int sb;
          real t1 = tab1[is_p],t2 = tab2[is_p]; 

          for ( sb = bi->longDiff[21]; sb > 0; sb--,idx++ )
          {
            real v = xr[0][idx];
            xr[0][idx] = MULT_REAL(v, t1);
            xr[1][idx] = MULT_REAL(v, t2);
          }
        }
      } /* ... */
}

void III_antialias(real xr[SBLIMIT][SSLIMIT],struct gr_info_s *gr_info)
{
#ifdef PROFILE
	FunctionProfiler fp(L"III_antialias");
#endif

    int sblim;

   if(gr_info->block_type == 2)
   {
      if(!gr_info->mixed_block_flag) 
        return;
      sblim = 1; 
   }
   else {
     sblim = gr_info->maxb-1;
   }

   /* 31 alias-reduction operations between each pair of sub-bands */
   /* with 8 butterflies between each pair                         */

   {
     int sb;
     real *xr1=(real *) xr[1];

     for(sb=sblim;sb;sb--,xr1+=10)
     {
       int ss;
       real *cs=aa_cs,*ca=aa_ca;
       real *xr2 = xr1;

       for(ss=7;ss>=0;ss--)
       {       /* upper and lower butterfly inputs */
         register real bu = *--xr2,bd = *xr1;
         *xr2   = (MULT_REAL(bu, (*cs))   ) - (MULT_REAL(bd, (*ca))   );
         *xr1++ = (MULT_REAL(bd, (*cs++)) ) + (MULT_REAL(bu, (*ca++)) );
       }
     }
  }
}

/* 
// This is an optimized DCT from Jeff Tsay's maplay 1.2+ package.
// Saved one multiplication by doing the 'twiddle factor' stuff
// together with the window mul. (MH)
//
// This uses Byeong Gi Lee's Fast Cosine Transform algorithm, but the
// 9 point IDCT needs to be reduced further. Unfortunately, I don't
// know how to do that, because 9 is not an even number. - Jeff.
//
//////////////////////////////////////////////////////////////////
//
// 9 Point Inverse Discrete Cosine Transform
//
// This piece of code is Copyright 1997 Mikko Tommila and is freely usable
// by anybody. The algorithm itself is of course in the public domain.
//
// Again derived heuristically from the 9-point WFTA.
//
// The algorithm is optimized (?) for speed, not for small rounding errors or
// good readability.
//
// 36 additions, 11 multiplications
//
// Again this is very likely sub-optimal.
//
// The code is optimized to use a minimum number of temporary variables,
// so it should compile quite well even on 8-register Intel x86 processors.
// This makes the code quite obfuscated and very difficult to understand.
//
// References:
// [1] S. Winograd: "On Computing the Discrete Fourier Transform",
//     Mathematics of Computation, Volume 32, Number 141, January 1978,
//     Pages 175-199
*/

/*------------------------------------------------------------------*/
/*                                                                  */
/*    Function: Calculation of the inverse MDCT                     */
/*                                                                  */
/*------------------------------------------------------------------*/

#define DCTMACRO0(v) { \
    out2[9+(v)] = MULT_REAL((tmp = sum0 + sum1), w[27+(v)]); \
    out2[8-(v)] = MULT_REAL(tmp, w[26-(v)]);  } \
    sum0 -= sum1; \
    ts[SBLIMIT*(8-(v))] = out1[8-(v)] + MULT_REAL(sum0, w[8-(v)]); \
    ts[SBLIMIT*(9+(v))] = out1[9+(v)] + MULT_REAL(sum0, w[9+(v)]); 
#define DCTMACRO1(v) { \
    sum0 = tmp1a + tmp2a; \
	sum1 = MULT_REAL((tmp1b + tmp2b), tfcos36[(v)]); \
	DCTMACRO0(v); }
#define DCTMACRO2(v) { \
    sum0 = tmp2a - tmp1a; \
    sum1 = MULT_REAL((tmp2b - tmp1b), tfcos36[(v)]); \
	DCTMACRO0(v); }



//Matt - I've cut out alot of #ifdef's and moved MACROs and variables
//       around...its offered a performance gain of 200 ms roughly
void dct36(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf)
{
#ifdef PROFILE
	FunctionProfiler fp(L"dct36");
#endif

 
    register real *in = inbuf;

    in[17]+=in[16]; in[16]+=in[15]; in[15]+=in[14];
    in[14]+=in[13]; in[13]+=in[12]; in[12]+=in[11];
    in[11]+=in[10]; in[10]+=in[9];  in[9] +=in[8];
    in[8] +=in[7];  in[7] +=in[6];  in[6] +=in[5];
    in[5] +=in[4];  in[4] +=in[3];  in[3] +=in[2];
    in[2] +=in[1];  in[1] +=in[0];

    in[17]+=in[15]; in[15]+=in[13]; in[13]+=in[11]; in[11]+=in[9];
    in[9] +=in[7];  in[7] +=in[5];  in[5] +=in[3];  in[3] +=in[1];

  {


    /*register*/ const real *c = COS9;
    /*register*/ real *out2 = o2;
	/*register*/ real *w = wintab;
	/*register*/ real *out1 = o1;
	/*register*/ real *ts = tsbuf;

    static real ta33,ta66,tb33,tb66;

    ta33 = MULT_REAL(in[2*3+0], c[3]);
    ta66 = MULT_REAL(in[2*6+0], c[6]);
    tb33 = MULT_REAL(in[2*3+1], c[3]);
    tb66 = MULT_REAL(in[2*6+1], c[6]);

	static real tmp1a, tmp2a, tmp1b, tmp2b;
	static real tmp;
	static real sum0, sum1;
    { 
//#if !(defined(_SH3_) && (defined(REAL_IS_FIXED) || defined(REAL_IS_INT)))
#if (defined(REAL_IS_FIXED) || defined(REAL_IS_INT))

      tmp1a =             MULT_REAL(in[2*1+0], c[1]) + ta33 + MULT_REAL(in[2*5+0], c[5]) + MULT_REAL(in[2*7+0], c[7]);
      tmp1b =             MULT_REAL(in[2*1+1], c[1]) + tb33 + MULT_REAL(in[2*5+1], c[5]) + MULT_REAL(in[2*7+1], c[7]);
      tmp2a = in[2*0+0] + MULT_REAL(in[2*2+0], c[2]) + MULT_REAL(in[2*4+0], c[4]) + ta66 + MULT_REAL(in[2*8+0], c[8]);
      tmp2b = in[2*0+1] + MULT_REAL(in[2*2+1], c[2]) + MULT_REAL(in[2*4+1], c[4]) + tb66 + MULT_REAL(in[2*8+1], c[8]);

#else

    //  tmp1a =             in[2*1+0] * c[1] + ta33 + in[2*5+0] * c[5] + in[2*7+0] * c[7];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #8, R2");
	__asm("add     #4, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #12, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp1a);

	tmp1a += ta33;


    //  tmp1b =             in[2*1+1] * c[1] + tb33 + in[2*5+1] * c[5] + in[2*7+1] * c[7];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #12, R2");
	__asm("add     #4, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #12, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp1b);

	tmp1b += tb33;


    //  tmp2a = in[2*0+0] + in[2*2+0] * c[2] + in[2*4+0] * c[4] + ta66 + in[2*8+0] * c[8];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #16, R2");
	__asm("add     #8, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #12, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp2a);

	tmp2a += in[2*0+0] + ta66;

	//  tmp2b = in[2*0+1] + in[2*2+1] * c[2] + in[2*4+1] * c[4] + tb66 + in[2*8+1] * c[8];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #20, R2");
	__asm("add     #8, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #12, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp2b);

	tmp2b += in[2*0+1] + tb66;

#endif


      DCTMACRO1(0);
      DCTMACRO2(8);
    }

    {
      tmp1a = MULT_REAL(( in[2*1+0] - in[2*5+0] - in[2*7+0] ), c[3]);
      tmp1b = MULT_REAL(( in[2*1+1] - in[2*5+1] - in[2*7+1] ), c[3]);
      tmp2a = MULT_REAL(( in[2*2+0] - in[2*4+0] - in[2*8+0] ), c[6]) - in[2*6+0] + in[2*0+0];
      tmp2b = MULT_REAL(( in[2*2+1] - in[2*4+1] - in[2*8+1] ), c[6]) - in[2*6+1] + in[2*0+1];

      DCTMACRO1(1);
      DCTMACRO2(7);
    }

    {

//#if !(defined(_SH3_) && (defined(REAL_IS_FIXED) || defined(REAL_IS_INT)))
#if (defined(REAL_IS_FIXED) || defined(REAL_IS_INT))

      tmp1a =             MULT_REAL(in[2*1+0], c[5]) - ta33 - MULT_REAL(in[2*5+0], c[7]) + MULT_REAL(in[2*7+0], c[1]);
      tmp1b =             MULT_REAL(in[2*1+1], c[5]) - tb33 - MULT_REAL(in[2*5+1], c[7]) + MULT_REAL(in[2*7+1], c[1]);
      tmp2a = in[2*0+0] - MULT_REAL(in[2*2+0], c[8]) - MULT_REAL(in[2*4+0], c[2]) + ta66 + MULT_REAL(in[2*8+0], c[4]);
      tmp2b = in[2*0+1] - MULT_REAL(in[2*2+1], c[8]) - MULT_REAL(in[2*4+1], c[2]) + tb66 + MULT_REAL(in[2*8+1], c[4]);

#else

	// tmp1a =             in[2*1+0] * c[5] - ta33 - in[2*5+0] * c[7] + in[2*7+0] * c[1];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #8, R2");
	__asm("add     #20, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #4, R3");

	// Negate c[7]
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #-28, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp1a);

	  tmp1a -= ta33;

	// tmp1b =             in[2*1+1] * c[5] - tb33 - in[2*5+1] * c[7] + in[2*7+1] * c[1];
	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #12, R2");
	__asm("add     #20, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");

	// Negate c[7]
	__asm("add     #-4, R3");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #-24, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp1b);

	tmp1b -= tb33;


/*
	// tmp2a = in[2*0+0] - in[2*2+0] * c[8] - in[2*4+0] * c[2] + ta66 + in[2*8+0] * c[4];

	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #8, R2");
	__asm("add     #32, R3");

	// Negate c[8]
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");
	__asm("add     #-28, R3");

	// Negate c[2]
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-12,R0");
	__asm("shld R0,R3");
	__asm("mov #D'20,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp2a);

	tmp2a += in[2*0+0] + ta66;


	// tmp2b = in[2*0+1] - in[2*2+1] * c[8] - in[2*4+1] * c[2] + tb66 + in[2*8+1] * c[4];

	__asm("mov.l   @R4,R2", &in);
	__asm("mov.l   @R4,R3", &c);
	__asm("add     #12, R2");
	__asm("add     #32, R3");

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #12, R2");

	// Negate c[8]
	__asm("add     #-4, R3");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #-24, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #28, R2");

	// Negate c[2]
	__asm("add     #-4, R3");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-12,R0");
	__asm("shld R0,R3");
	__asm("mov #D'20,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &tmp2b);

	tmp2b += in[2*0+1] + tb66;
*/
      tmp2a = in[2*0+0] - MULT_REAL(in[2*2+0], c[8]) - MULT_REAL(in[2*4+0], c[2]) + ta66 + MULT_REAL(in[2*8+0], c[4]);
      tmp2b = in[2*0+1] - MULT_REAL(in[2*2+1], c[8]) - MULT_REAL(in[2*4+1], c[2]) + tb66 + MULT_REAL(in[2*8+1], c[4]);

#endif

      DCTMACRO1(2);
      DCTMACRO2(6);
    }

    {
      tmp1a =             MULT_REAL(in[2*1+0], c[7]) - ta33 + MULT_REAL(in[2*5+0], c[1]) - MULT_REAL(in[2*7+0], c[5]);
      tmp1b =             MULT_REAL(in[2*1+1], c[7]) - tb33 + MULT_REAL(in[2*5+1], c[1]) - MULT_REAL(in[2*7+1], c[5]);
      tmp2a = in[2*0+0] - MULT_REAL(in[2*2+0], c[4]) + MULT_REAL(in[2*4+0], c[8]) + ta66 - MULT_REAL(in[2*8+0], c[2]);
      tmp2b = in[2*0+1] - MULT_REAL(in[2*2+1], c[4]) + MULT_REAL(in[2*4+1], c[8]) + tb66 - MULT_REAL(in[2*8+1], c[2]);

      DCTMACRO1(3);
      DCTMACRO2(5);
    }

	{

		sum0 =  in[2*0+0] - in[2*2+0] + in[2*4+0] - in[2*6+0] + in[2*8+0];
    	sum1 = MULT_REAL((in[2*0+1] - in[2*2+1] + in[2*4+1] - in[2*6+1] + in[2*8+1] ), tfcos36[4]);

		//DCTMACRO0(4);
	    out2[9+(4)] = MULT_REAL((tmp = sum0 + sum1), w[27+(4)]); 
		out2[8-(4)] = MULT_REAL(tmp, w[26-(4)]);  
		sum0 -= sum1; 

		ts[SBLIMIT*(8-(4))] = out1[8-(4)] + MULT_REAL(sum0, w[8-(4)]); 

		ts[SBLIMIT*(9+(4))] = out1[9+(4)] + MULT_REAL(sum0, w[9+(4)]); 

#pragma inline_depth(0)		
	}
  }
 
}


