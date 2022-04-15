#include "AutoProfile.h"
#include "debug.h"

//#define OPT_GETBITS 

#if defined(UNDER_CE) && defined(OPT_GETBITS) && (defined(_SH3_) || defined(_SH4_))
extern "C" { void __asm(const char *, ...); };
#endif

int bitindex = 0;
extern unsigned char *wordpointer;

/*
        Func          Func+Child           Hit
        Time   %         Time      %      Count  Function
---------------------------------------------------------
       3.516   0.1        3.516   0.1     5048 getbits(int) (common.obj)       
*/
unsigned int getbits(int number_of_bits)
{

  if(!number_of_bits)
    return 0;

#if defined(UNDER_CE) && defined(OPT_GETBITS) && (defined(_SH3_) || defined(_SH4_))
	unsigned int rval;

	static int bitmask = 16777215;

	__asm("mov.b  @R4,R0",wordpointer);
	__asm("shll8  R0");
	__asm("mov.b  @R4,R1",(wordpointer+1));
	__asm("extu.b R1,R1");
	__asm("or     R1,R0");
	__asm("shll8  R0");
	__asm("mov.b  @R4,R1",(wordpointer+2));
	__asm("extu.b R1,R1");
	__asm("or     R1,R0");
	__asm("mov.l  @R4,R3",&bitmask);
	__asm("mov.l  @R4,R2\n"
		"shld   R2,R0\n"
		"and	R3,R0\n"
		"mov.l  @R5,R3\n"
		"add    R3,R2\n"
		"mov    #-24,R1\n"
		"add    R3,R1\n"
		"shld   R1,R0\n"
		"mov.l  R0,@R6\n"
		"mov    R2,R0\n"
		"mov    #-3,R1\n"
		"shld   R1,R2\n"
		"mov.l  @R7,R3\n"
		"add    R2,R3\n"
		"mov.l  R3,@R7\n"
		"and    #7,R0\n"
		"mov.l  R0,@R4\n",
		&bitindex,			// R4
		&number_of_bits,	// R5
		&rval,				// R6
		&wordpointer);		// R7
	return rval;

#else

  unsigned int rval;
  {
    rval = wordpointer[0];
    rval <<= 8;
    rval |= wordpointer[1];
    rval <<= 8;
    rval |= wordpointer[2];
    rval <<= bitindex;
    rval &= 0xffffff;

    bitindex += number_of_bits;

    rval >>= (24-number_of_bits);

    wordpointer += (bitindex>>3);
    bitindex &= 7;
  }
	return rval;
#endif

  
}

/*
        Func          Func+Child           Hit
        Time   %         Time      %      Count  Function
---------------------------------------------------------
	1.091   0.0        1.091   0.0     1704 getbits_fast(int) (common.obj)              
*/
unsigned int getbits_fast(int number_of_bits)
{
  unsigned long rval;

#if defined(UNDER_CE) && defined(OPT_GETBITS) && (defined(_SH3_) || defined(_SH4_))

	__asm("mov.l  @R4,R3",&number_of_bits);
	__asm("mov.b  @R4,R0",wordpointer);
	__asm("shll8  R0");
	__asm("mov.b  @R4,R1",(wordpointer+1));
	__asm("extu.b R1,R1");
	__asm("or     R1,R0");
	__asm("mov.l  @R4,R2\n"
			"shld   R2,R0\n"
			"extu.w R0,R0\n"
			"add    R3,R2\n"
			"mov    #-16,R1\n"
			"add    R3,R1\n"
			"shld   R1,R0\n"
			"mov.w  R0,@R5\n"
			"mov    R2,R0\n"
			"mov    #-3,R1\n"
			"shld   R1,R2\n"
			"mov.l  @R6,R3\n"
			"add    R2,R3\n"
			"mov.l  R3,@R6\n"
			"and    #7,R0\n"
			"mov.l  R0,@R4\n",
			&bitindex,			// R4
			&rval,				// R5
			&wordpointer);		// R6

#else

	rval = wordpointer[0];
    rval <<= 8;	
    rval |= wordpointer[1];
    rval <<= bitindex;
    rval &= 0xffff;
    bitindex += number_of_bits;

    rval >>= (16-number_of_bits);

    wordpointer += (bitindex>>3);
    bitindex &= 7;

#endif

  return rval;
}

/*
        Func          Func+Child           Hit
        Time   %         Time      %      Count  Function
---------------------------------------------------------
      91.699   3.6       91.699   3.6   156776 get1bit(void) (common.obj)

*/
unsigned int get1bit(void)
{
  unsigned char rval;
#if defined(UNDER_CE) && defined(OPT_GETBITS) && (defined(_SH3_) || defined(_SH4_))

  __asm("mov.l  @R4,R2\n" //rval = (*wordpointer << bitindex);
	    "mov.b  @R5,R3\n"
		"shld   R2,R3\n"
		"extu.b R3,R3\n"
		"mov    #-7,R0\n" //rval >>= 7;
		"shad   R0,R3\n"
		"mov.b  R3,@R7\n"
		"add    #1,R2\n"  //++bitindex;
		"mov    #-3,R0\n" //wordpointer += bitindex >> 3;
		"mov    R2,R3\n"
		"shad   R0,R3\n"
		"mov.l  @R6,R1\n"
		"add    R3,R1\n"
		"mov.l  R1,@R6\n"
		"mov    #7,R0\n"  //bitindex &= 7;
		"and    R0,R2\n"
		"mov.l  R2,@R4\n",
		&bitindex,         //R4
		wordpointer,       //R5
		&wordpointer,      //R6
		&rval);            //R7
  return rval;
#else

  rval = *wordpointer << bitindex;

  bitindex++;
  wordpointer += (bitindex>>3);
  bitindex &= 7;
  return rval>>7;

#endif
}
