/*
 * Mpeg Layer-1,2,3 audio decoder
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 * version for slower machines .. decodes only every fourth sample
 * dunno why it sounds THIS annoying (maybe we should adapt the window?)
 * absolutely not optimized for this operation
 */
#include <math.h>

#include "mpg123.h"
#include "dct64.h"

/*
#define WRITE_SAMPLE(samples,sum,clip) \
  if( (sum) > 32767.0) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < -32768.0) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = sum; }
*/
// samples -- short *
// sum -- real
// clip -- int
#ifdef REAL_IS_FIXED /* ezra: changed so didn't use floats */
#define WRITE_SAMPLE(samples,sum,clip) \
  if( sum.i>(32767<<FRACBITS)) { *(samples) = 0x7fff;} \
  else if (sum.i < (-32768<<FRACBITS)) { *(samples) = -0x8000; } \
  else { *(samples) = (short)(sum.i>>FRACBITS); }
#elif defined(REAL_IS_INT)
#define WRITE_SAMPLE(samples,sum,clip) \
  if( sum>(32767<<FRACBITS)) { *(samples) = 0x7fff;} \
  else if (sum < (-32768<<FRACBITS)) { *(samples) = -0x8000; } \
  else { *(samples) = (short)(sum>>FRACBITS); }
#else
#define WRITE_SAMPLE(samples,sum,clip) \
  if( ((float)sum) > 32767.0) { *(samples) = 0x7fff; (clip)++; } \
  else if( ((float)sum) < -32768.0) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = (short)((float)sum); }
#endif	// REAL_IS_FIXED

int synth_4to1_8bit(real *bandPtr,int channel,unsigned char *samples)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp + channel;
  int i,ret;

  samples += channel;
  ret = synth_4to1(bandPtr,channel,(unsigned char *) samples_tmp);

  for(i=0;i<8;i++) {
    *samples = conv16to8[*tmp1>>4];
    samples += 2;
    tmp1 += 2;
  }

  return ret;
}

int synth_4to1_8bit_mono(real *bandPtr,unsigned char *samples)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;

  ret = synth_4to1(bandPtr,0,(unsigned char *) samples_tmp);

  for(i=0;i<8;i++) {
    *samples++ = conv16to8[*tmp1>>4];
    tmp1 += 2;
  }

  return ret;
}


int synth_4to1_8bit_mono2stereo(real *bandPtr,unsigned char *samples)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;

  ret = synth_4to1(bandPtr,0,(unsigned char *) samples_tmp);

  for(i=0;i<8;i++) {
    *samples++ = conv16to8[*tmp1>>4];
    *samples++ = conv16to8[*tmp1>>4];
    tmp1 += 2;
  }

  return ret;
}

int synth_4to1_mono(real *bandPtr,unsigned char *samples)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;

  ret = synth_4to1(bandPtr,0,(unsigned char *) samples_tmp);

  for(i=0;i<8;i++) {
    *( (short *)samples) = *tmp1;
    samples += 2;
    tmp1 += 2;
  }

  return ret;
}

int synth_4to1_mono2stereo(real *bandPtr,unsigned char *samples)
{
  int i,ret = synth_4to1(bandPtr,0,samples);
  for(i=0;i<8;i++) {
    ((short *)samples)[1] = ((short *)samples)[0];
    samples+=4;
  }
  return ret;
}

#if defined(MIPS_4121) && defined(OPT_MACC)
//#pragma optimize("", off)  // turn all optimizations off, otherwise
						   // clmips will mess up the registers used by
						   // the 4121 macc instruction
static real mips_4121_helper1(real *window,real *b0)
{
	real res; //42
/*
	  sum  = MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
*/
	__asm("lw %t3,0(%0);"		//load pointer to first array in %t3
		"lw %t4,0(%1);"		//load pointer to second array in %t4
		"mtlo zero;"		//clear accumulate register
		"mthi zero;",&window, &b0);

	__asm("lw %t5,0x0(%t3);"	//load value from %t3 at offset 0 (index 0)
		"lw %t6,0x0(%t4);"	//load value from %t4 at offset 0
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x4(%t3);"	//load value from %t3 at offset 4 (index 1)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x4(%t4);"	//load value from %t4 at offset 4
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x8(%t3);"	//load value from %t3 at offset 8 (index 2)
		"lw %t6,0x8(%t4);"	//load value from %t4 at offset 8
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;" //macc %t5, %t6

		"lw %t5,0xC(%t3);"  //load value from %t3 at offset 12 (index 3)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0xC(%t4);"	//load value from %t4 at offset 12 
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x10(%t3);"	//load value from %t3 at offset 0 (index 4)
		"lw %t6,0x10(%t4);"	//load value from %t4 at offset 0
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x14(%t3);"	//load value from %t3 at offset 4 (index 5)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x14(%t4);"	//load value from %t4 at offset 4
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x18(%t3);"	//load value from %t3 at offset 8 (index 6)
		"lw %t6,0x18(%t4);"	//load value from %t4 at offset 8
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;" //macc %t5, %t6

		"lw %t5,0x1C(%t3);"  //load value from %t3 at offset 12 (index 7)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x1C(%t4);"	//load value from %t4 at offset 12 
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x20(%t3);" //load value from %t3 at offset 0 (index 8)
		"lw %t6,0x20(%t4);"	//load value from %t4 at offset 0
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x24(%t3);"	//load value from %t3 at offset 4 (index 9)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x24(%t4);"	//load value from %t4 at offset 4
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x28(%t3);"	//load value from %t3 at offset 8 (index 10)
		"lw %t6,0x28(%t4);"	//load value from %t4 at offset 8
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;" //macc %t5, %t6

		"lw %t5,0x2C(%t3);"  //load value from %t3 at offset 12 (index 11)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x2C(%t4);"	//load value from %t4 at offset 12 
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x30(%t3);"	//load value from %t3 at offset 0 (index 12)
		"lw %t6,0x30(%t4);"	//load value from %t4 at offset 0
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x34(%t3);"	//load value from %t3 at offset 4 (index 13)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x34(%t4);"	//load value from %t4 at offset 4
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"lw %t5,0x38(%t3);"	//load value from %t3 at offset 8 (index 14)
		"lw %t6,0x38(%t4);"	//load value from %t4 at offset 8
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;" //macc %t5, %t6

		"lw %t5,0x3C(%t3);"  //load value from %t3 at offset 12 (index 15)
		"sub %t5,zero,%t5;" //negate %t5
		"lw %t6,0x3C(%t4);"	//load value from %t4 at offset 12 
		"madd16 %t5,%t6;"
		//".word 0x01AE0028;"	//macc %t5, %t6

		"mflo %t1;"
		"srl %t1,%t1,15;"  //FRACBITS
		"mfhi %t2;"
		"sll %t2,%t2,32-15;"  //FRACBITS
		"or %t2,%t2,%t1;"
		"sw %t2,0(%0)",&res);		

	return res;
}

static real mips_4121_helper2(real *window,real *b0)
{
	real res;
/*
	sum  = MULT_REAL(window[0x0], b0[0x0]);
    sum += MULT_REAL(window[0x2], b0[0x2]);
    sum += MULT_REAL(window[0x4], b0[0x4]);
    sum += MULT_REAL(window[0x6], b0[0x6]);
    sum += MULT_REAL(window[0x8], b0[0x8]);
	sum += MULT_REAL(window[0xA], b0[0xA]);
	sum += MULT_REAL(window[0xC], b0[0xC]);
	sum += MULT_REAL(window[0xE], b0[0xE]);
*/
	__asm(	"lw %t3,0(%0);"		//load pointer to first array in %t3
			"lw %t4,0(%1);"		//load pointer to second array in %t4
			"mtlo zero;"		//clear accumulate register
			"mthi zero;"

			"lw %t5,0x0(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x0(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x8(%t3);"	//load value from %t3 at offset 8 (index 2)
			"lw %t6,0x8(%t4);"	//load value from %t4 at offset 8
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x10(%t3);"	//load value from %t3 at offset 16 (index 4)
			"lw %t6,0x10(%t4);"	//load value from %t4 at offset 16
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x18(%t3);"	//load value from %t3 at offset 24 (index 6)
			"lw %t6,0x18(%t4);"	//load value from %t4 at offset 24
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x20(%t3);"	//load value from %t3 at offset 32 (index 8)
			"lw %t6,0x20(%t4);"	//load value from %t4 at offset 32
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x28(%t3);"	//load value from %t3 at offset 40 (index 10)
			"lw %t6,0x28(%t4);"	//load value from %t4 at offset 40
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x30(%t3);"	//load value from %t3 at offset 48 (index 12)
			"lw %t6,0x30(%t4);"	//load value from %t4 at offset 48
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x38(%t3);"	//load value from %t3 at offset 56 (index 14)
			"lw %t6,0x38(%t4);"	//load value from %t4 at offset 56
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"mflo %t1;"
			"srl %t1,%t1,15;"  //FRACBITS
			"mfhi %t2;"
			"sll %t2,%t2,32-15;"  //FRACBITS
			"or %t2,%t2,%t1;"
			"sw %t2,0(%2);" 
			,&window,&b0,&res);
	return res;
}

static real mips_4121_helper3(real *window,real *b0)
{
	real res;

/*
	sum = MULT_REAL(-*(--window), *b0++);
    sum -= MULT_REAL(*(--window), *b0++);
    sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
	sum -= MULT_REAL(*(--window), *b0++);
*/
	__asm(	"lw %t3,0(%0);"		//load pointer to first array in %t3
			"lw %t4,0(%1);"		//load pointer to second array in %t4
			"mtlo zero;"		//clear accumulate register
			"mthi zero;"

			"lw %t5,0x3C(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x0(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x38(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x4(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x34(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x8(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x30(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0xC(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x2C(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x10(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x28(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x14(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x24(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x18(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x20(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x1C(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x1C(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x20(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x18(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x24(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x14(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x28(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x10(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x2C(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0xC(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x30(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x8(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x34(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0x4(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x38(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"lw %t5,0(%t3);"	//load value from %t3 at offset 0 (index 0)
			"lw %t6,0x3C(%t4);"	//load value from %t4 at offset 0
			"madd16 %t5,%t6;"
			//".word 0x01AE0028;"	//macc %t5, %t6

			"mflo %t1;"
			"srl %t1,%t1,15;"  //FRACBITS
			"mfhi %t2;"
			"sll %t2,%t2,32-15;"  //FRACBITS
			"or %t2,%t2,%t1;"
			"sub %t2,zero,%t2;"
			"sw %t2,0(%2);" 
			,&window,&b0,&res);
	return res;
}
//#pragma optimize("",on)
#endif // MIPS_4121

extern real buffs[2][2][0x110];
#if defined(_SH3_) || defined(_SH4_)
extern "C"  { void __asm(const char *, ...); }
#endif

int synth_4to1(real *bandPtr,int channel,unsigned char *out)
{

  static const int step = 2;
  static int bo = 1;
  short *samples = (short *) out;

  real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    dct64_4to1(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    dct64_4to1(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }

  {
    register int j;
    real *window = decwin + 16 - bo1;

    for (j=4;j;j--,b0+=0x30,window+=0x70)
    {
      real sum;
	  //MultiplyAndAccumulate

#if (defined(REAL_IS_FIXED) || defined(REAL_IS_INT)) && defined(OPT_MACC)
#if defined(_MIPS_) && defined(MIPS_4121)
	  sum = mips_4121_helper1(window,b0);

	  window += 16;
	  b0 += 16;
#elif defined(_MIPS_) && defined(MIPS_4111)
	  LONGLONG _sum, tmp;
	  _sum = 0;

	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;
	  mips_macc(*window,*b0,&tmp);
	  _sum += tmp;
	  window++;
	  b0++;
  	  mips_macc((*window),*b0,&tmp);
	  _sum -= tmp;
	  window++;
	  b0++;

	  mips_macc_end(_sum,&sum);
#elif defined(_SH3_) || defined(_SH4_)

    real *tempstore = b0;

	__asm("mov.l   @R4,R2", &window);
	__asm("mov.l   @R4,R3", &b0);

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	
	__asm("mac.l   @R2+,@R3+");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("mac.l   @R2+,@R3+");
	

	// Store the new values of window and b0.
	__asm("mov.l   R2,@R4", &window);
	__asm("mov.l   R3,@R4", &b0);
	
	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &sum);


	// Negate the numbers that were negated in the above loop.

	__asm("mov.l   @R4,R3", &tempstore);

	__asm("add     #4, R3");
	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");
	__asm("add     #8, R3");

	__asm("mov.l   @R3, R1");
	__asm("neg	   R1, R1");
	__asm("mov.l   R1, @R3");

/*
	window += 16;
	b0 += 16;
*/
#else
#error	Currently, we support OPT_MACC for the following
#error	processors:
#error		NEC VR4121 - MIPS_4121
#error		NEC VR4111 - MIPS_4111
#error		Hitachi SH3/SH4
#endif	// SH3_ASSEM

#else //OPT_MACC not defined

	  sum  = MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
      sum += MULT_REAL(*window++, *b0++);
      sum -= MULT_REAL(*window++, *b0++);
#endif //_MIPS_

      WRITE_SAMPLE(samples,sum,clip); samples += step;
    }

    {
      real sum;
	  //MultiplyAndAccumulate

#if (defined(REAL_IS_FIXED) || defined(REAL_IS_INT)) && defined(OPT_MACC)
#if defined(_MIPS_) && defined(MIPS_4121)
	 sum = mips_4121_helper2(window,b0);
#elif defined(_MIPS_) && defined(MIPS_4111)
	  LONGLONG _sum, tmp;
	  _sum = 0;

	  mips_macc(window[0x0],b0[0x0],&tmp);
	  _sum += tmp;
	  mips_macc(window[0x2],b0[0x2],&tmp);
	  _sum += tmp;
	  mips_macc(window[0x4],b0[0x4],&tmp);
	  _sum += tmp;
	  mips_macc(window[0x6],b0[0x6],&tmp);
	  _sum += tmp;
	  mips_macc(window[0x8],b0[0x8],&tmp);
	  _sum += tmp;
	  mips_macc(window[0xA],b0[0xA],&tmp);
	  _sum += tmp;
	  mips_macc(window[0xC],b0[0xC],&tmp);
	  _sum += tmp;
	  mips_macc(window[0xE],b0[0xE],&tmp);
	  _sum += tmp;

	  mips_macc_end(_sum,&sum);
#elif defined(_SH3_) || defined(_SH4_)

	__asm("mov.l   @R4,R2", &window);
	__asm("mov.l   @R4,R3", &b0);

	__asm("clrmac");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");
	__asm("add     #4, R2");
	__asm("add     #4, R3");

	__asm("mac.l   @R2+,@R3+");

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4", &sum);
#else
#error	Currently, we support OPT_MACC for the following
#error	processors:
#error		NEC VR4121 - MIPS_4121
#error		NEC VR4111 - MIPS_4111
#error		Hitachi SH3/SH4
#endif	// SH3_ASSEM

#else //OPT_MACC not defined

	  sum  = MULT_REAL(window[0x0], b0[0x0]);
      sum += MULT_REAL(window[0x2], b0[0x2]);
      sum += MULT_REAL(window[0x4], b0[0x4]);
      sum += MULT_REAL(window[0x6], b0[0x6]);
      sum += MULT_REAL(window[0x8], b0[0x8]);
      sum += MULT_REAL(window[0xA], b0[0xA]);
      sum += MULT_REAL(window[0xC], b0[0xC]);
      sum += MULT_REAL(window[0xE], b0[0xE]);
#endif // _MIPS_


      WRITE_SAMPLE(samples,sum,clip); samples += step;
      b0-=0x40,window-=0x80;
    }
    window += bo1<<1;

    for (j=3;j;j--,b0-=0x50,window-=0x70)
    {
      real sum;
	  //MultiplyAndAccumulate

#if (defined(REAL_IS_FIXED) || defined(REAL_IS_INT)) && defined(OPT_MACC)
#if defined(_MIPS_) && defined(MIPS_4121)
	  window -= 16;
	  sum = mips_4121_helper3(window,b0);
	  b0 += 16;
#elif defined(_MIPS_) && defined(MIPS_4111)
	  LONGLONG _sum, tmp;
	  _sum = 0;

	  mips_macc(-*(--window),*b0++,&tmp);
	  _sum += tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;
	  mips_macc(*(--window),*b0++,&tmp);
	  _sum -= tmp;

  	  mips_macc_end(_sum,&sum);
#elif defined(_SH3_) || defined(_SH4_)

	__asm("mov.l   @R4,R2", &window);
	__asm("mov.l   @R4,R3", &b0);

	__asm("clrmac");

	__asm("add     #-4, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	__asm("add     #-8, R2");
	__asm("mac.l   @R2+,@R3+");

	// Store the new value of b0.
	__asm("mov.l   R3,@R4", &b0);

	__asm("sts MACL,R3");
	__asm("sts MACH,R2");

	__asm("mov #-15,R0");
	__asm("shld R0,R3");
	__asm("mov #D'17,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("neg R3, R3");
	__asm("mov.l R3,@R4", &sum);

	window -= 16;
//	b0 += 16;

#else
#error	Currently, we support OPT_MACC for the following
#error	processors:
#error		NEC VR4121 - MIPS_4121
#error		NEC VR4111 - MIPS_4111
#error		Hitachi SH3/SH4
#endif	// SH3_ASSEM

#else //OPT_MACC not defined

	  sum = MULT_REAL(-*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
      sum -= MULT_REAL(*(--window), *b0++);
#endif // _MIPS_

      WRITE_SAMPLE(samples,sum,clip); samples += step;
    }
  }

  return clip;
}


