/*
 * Discrete Cosine Tansform (DCT) for subband synthesis
 *
 * -funroll-loops (for gcc) will remove the loops for better performance
 * using loops in the source-code enhances readabillity
 */

/*
 * TODO: write an optimized version for the down-sampling modes
 *       (in these modes the bands 16-31 (2:1) or 8-31 (4:1) are zero 
 */

#include "dct64.h"
#include "mpg123.h"

#include "options.h"
#include "AutoProfile.h"

static real bufs[64] = {
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO
};

void reinit_dct()
{
	memset(bufs, 0, 64*sizeof(real));
}

void dct64(real *out0,real *out1,real *samples)
{
#ifdef PROFILE
	FunctionProfiler fp(L"dct64");
#endif

//  static real bufs[64];

 {
  register int i,j;
  register real *b1,*b2,*bs,*costab;

  b1 = samples;
  bs = bufs;
  costab = pnts[0]+16;
  b2 = b1 + 32;

  for(i=15;i>=0;i--)
    *bs++ = (*b1++ + *--b2); 
  for(i=15;i>=0;i--)
    *bs++ = MULT_REAL((*--b2 - *b1++), *--costab);

  b1 = bufs;
  costab = pnts[1]+8;
  b2 = b1 + 16;

  {
    for(i=7;i>=0;i--)
      *bs++ = (*b1++ + *--b2); 
    for(i=7;i>=0;i--)
      *bs++ = MULT_REAL((*--b2 - *b1++), *--costab);
    b2 += 32;
    costab += 8;
    for(i=7;i>=0;i--)
      *bs++ = (*b1++ + *--b2); 
    for(i=7;i>=0;i--)
      *bs++ = MULT_REAL((*b1++ - *--b2), *--costab);
    b2 += 32;
  }

  bs = bufs;
  costab = pnts[2];
  b2 = b1 + 8;

  for(j=2;j;j--)
  {
    for(i=3;i>=0;i--)
      *bs++ = (*b1++ + *--b2); 
    for(i=3;i>=0;i--)
      *bs++ = MULT_REAL((*--b2 - *b1++), costab[i]);
    b2 += 16;
    for(i=3;i>=0;i--)
      *bs++ = (*b1++ + *--b2); 
    for(i=3;i>=0;i--)
      *bs++ = MULT_REAL((*b1++ - *--b2), costab[i]);
    b2 += 16;
  }

  b1 = bufs;
  costab = pnts[3];
  b2 = b1 + 4;

  for(j=4;j;j--)
  {
    *bs++ = (*b1++ + *--b2); 
    *bs++ = (*b1++ + *--b2);
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[1]); 
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[0]);
    b2 += 8;
    *bs++ = (*b1++ + *--b2); 
    *bs++ = (*b1++ + *--b2);
    *bs++ = MULT_REAL((*b1++ - *--b2), costab[1]);
    *bs++ = MULT_REAL((*b1++ - *--b2), costab[0]);
    b2 += 8;
  }
  bs = bufs;
  costab = pnts[4];

  for(j=8;j;j--)
  {
    real v0,v1;
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v0 - v1), (*costab));
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v1 - v0), (*costab));
  }

 }


 {
  register real *b1;
  register int i;

  for(b1=bufs,i=8;i;i--,b1+=4)
    b1[2] += b1[3];

  for(b1=bufs,i=4;i;i--,b1+=8)
  {
    b1[4] += b1[6];
    b1[6] += b1[5];
    b1[5] += b1[7];
  }

  for(b1=bufs,i=2;i;i--,b1+=16)
  {
    b1[8]  += b1[12];
    b1[12] += b1[10];
    b1[10] += b1[14];
    b1[14] += b1[9];
    b1[9]  += b1[13];
    b1[13] += b1[11];
    b1[11] += b1[15];
  }
 }


  out0[0x10*16] = bufs[0];
  out0[0x10*15] = bufs[16+0]  + bufs[16+8];
  out0[0x10*14] = bufs[8];
  out0[0x10*13] = bufs[16+8]  + bufs[16+4];
  out0[0x10*12] = bufs[4];
  out0[0x10*11] = bufs[16+4]  + bufs[16+12];
  out0[0x10*10] = bufs[12];
  out0[0x10* 9] = bufs[16+12] + bufs[16+2];
  out0[0x10* 8] = bufs[2];
  out0[0x10* 7] = bufs[16+2]  + bufs[16+10];
  out0[0x10* 6] = bufs[10];
  out0[0x10* 5] = bufs[16+10] + bufs[16+6];
  out0[0x10* 4] = bufs[6];
  out0[0x10* 3] = bufs[16+6]  + bufs[16+14];
  out0[0x10* 2] = bufs[14];
  out0[0x10* 1] = bufs[16+14] + bufs[16+1];
  out0[0x10* 0] = bufs[1];

  out1[0x10* 0] = bufs[1];
  out1[0x10* 1] = bufs[16+1]  + bufs[16+9];
  out1[0x10* 2] = bufs[9];
  out1[0x10* 3] = bufs[16+9]  + bufs[16+5];
  out1[0x10* 4] = bufs[5];
  out1[0x10* 5] = bufs[16+5]  + bufs[16+13];
  out1[0x10* 6] = bufs[13];
  out1[0x10* 7] = bufs[16+13] + bufs[16+3];
  out1[0x10* 8] = bufs[3];
  out1[0x10* 9] = bufs[16+3]  + bufs[16+11];
  out1[0x10*10] = bufs[11];
  out1[0x10*11] = bufs[16+11] + bufs[16+7];
  out1[0x10*12] = bufs[7];
  out1[0x10*13] = bufs[16+7]  + bufs[16+15];
  out1[0x10*14] = bufs[15];
  out1[0x10*15] = bufs[16+15];

}



void dct64_2to1(real *out0,real *out1,real *samples)
{
#ifdef PROFILE
	FunctionProfiler fp(L"dct64_2to1");
#endif

/*
	static real bufs[64] = {
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO
	};
*/

 {
  register int i,j;
  register real *b1,*b2,*bs,*costab;

  memcpy(bufs, samples, 16 * sizeof(real));
  bs = &(bufs[32]);

  b1 = bufs;
  costab = pnts[1]+8;
  b2 = b1 + 16;

  {
    for(i=7;i>=0;i--)
      *bs++ = (*b1++ + *--b2); 
    for(i=7;i>=0;i--)
      *bs++ = MULT_REAL((*--b2 - *b1++), *--costab);
    b2 += 32;
    costab += 8;
	b1 += 16;
    b2 += 32;
  }

  bs = bufs;
  costab = pnts[2];
  b2 = b1 + 8;

  for(i=3;i>=0;i--)
    *bs++ = (*b1++ + *--b2); 
  for(i=3;i>=0;i--)
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[i]);
  b2 += 16;
  for(i=3;i>=0;i--)
    *bs++ = (*b1++ + *--b2); 
  for(i=3;i>=0;i--)
    *bs++ = MULT_REAL((*b1++ - *--b2), costab[i]);
  b2 += 16;

  bs += 16;

  b1 = bufs;
  costab = pnts[3];
  b2 = b1 + 4;

  for(j=2;j;j--)
  {
    *bs++ = (*b1++ + *--b2); 
    *bs++ = (*b1++ + *--b2);
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[1]);
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[0]);
    b2 += 8;
    *bs++ = (*b1++ + *--b2); 
    *bs++ = (*b1++ + *--b2);
    *bs++ = MULT_REAL((*b1++ - *--b2), costab[1]);
    *bs++ = MULT_REAL((*b1++ - *--b2), costab[0]);
    b2 += 8;
  }
  bs = bufs;
  costab = pnts[4];
  b1 += 16;

  for(j=4;j;j--)
  {
    real v0,v1;
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v0 - v1), (*costab));
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v1 - v0), (*costab));
  }

 }

  bufs[2] += bufs[3];
  bufs[6] += bufs[7];
  bufs[10] += bufs[11];
  bufs[14] += bufs[15];

  bufs[4] += bufs[6];
  bufs[6] += bufs[5];
  bufs[5] += bufs[7];
  bufs[12] += bufs[14];
  bufs[14] += bufs[13];
  bufs[13] += bufs[15];

  bufs[8]  += bufs[12];
  bufs[12] += bufs[10];
  bufs[10] += bufs[14];
  bufs[14] += bufs[9];
  bufs[9]  += bufs[13];
  bufs[13] += bufs[11];
  bufs[11] += bufs[15];

  out0[0x10*16] = bufs[0];
  out0[0x10*14] = bufs[8];
  out0[0x10*12] = bufs[4];
  out0[0x10*10] = bufs[12];
  out0[0x10* 8] = bufs[2];
  out0[0x10* 6] = bufs[10];
  out0[0x10* 4] = bufs[6];
  out0[0x10* 2] = bufs[14];
  out0[0x10* 0] = bufs[1];

  out1[0x10* 0] = bufs[1];
  out1[0x10* 2] = bufs[9];
  out1[0x10* 4] = bufs[5];
  out1[0x10* 6] = bufs[13];
  out1[0x10* 8] = bufs[3];
  out1[0x10*10] = bufs[11];
  out1[0x10*12] = bufs[7];
  out1[0x10*14] = bufs[15];

}



void dct64_4to1(real *out0,real *out1,real *samples)
{
#ifdef PROFILE
	FunctionProfiler fp(L"dct64_4to1");
#endif

/*
	static real bufs[64] = {
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO,
		REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO, REAL_ZERO
	};
*/

 {
  register int i,j;
  register real *b1,*b2,*bs,*costab;

  memcpy(bufs, samples, 8 * sizeof(real));
  memcpy(&(bufs[32]), samples, 8 * sizeof(real));

  b1 = bufs + 32;
  bs = bufs;
  costab = pnts[2];
  b2 = b1 + 8;

  for(i=3;i>=0;i--)
    *bs++ = (*b1++ + *--b2); 
  for(i=3;i>=0;i--)
    *bs++ = MULT_REAL((*--b2 - *b1++), costab[i]);
  bs += 24;

  b1 = bufs;
  costab = pnts[3];
  b2 = b1 + 4;

  *bs++ = (*b1++ + *--b2); 
  *bs++ = (*b1++ + *--b2);
  *bs++ = MULT_REAL((*--b2 - *b1++), costab[1]);
  *bs++ = MULT_REAL((*--b2 - *b1++), costab[0]);
  b2 += 8;
  *bs++ = (*b1++ + *--b2); 
  *bs++ = (*b1++ + *--b2);
  *bs++ = MULT_REAL((*b1++ - *--b2), costab[1]);
  *bs++ = MULT_REAL((*b1++ - *--b2), costab[0]);

  b1 += 24;
  
  bs = bufs;
  costab = pnts[4];

  for(j=2;j;j--)
  {
    real v0,v1;
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v0 - v1), (*costab));
    v0=*b1++; v1 = *b1++;
    *bs++ = (v0 + v1);
    *bs++ = MULT_REAL((v1 - v0), (*costab));
  }

 }



  bufs[2] += bufs[3];
  bufs[6] += bufs[7];

  bufs[4] += bufs[6];
  bufs[6] += bufs[5];
  bufs[5] += bufs[7];

  out0[0x10*16] = bufs[0];
  out0[0x10*12] = bufs[4];
  out0[0x10* 8] = bufs[2];
  out0[0x10* 4] = bufs[6];
  out0[0x10* 0] = bufs[1];


  out1[0x10* 0] = bufs[1];
  out1[0x10* 4] = bufs[5];
  out1[0x10* 8] = bufs[3];
  out1[0x10*12] = bufs[7];


}

