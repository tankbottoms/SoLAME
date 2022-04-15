/*
 * real.h 
 */

#ifndef REAL_H_
#define REAL_H_

//The following two defines are now in the projects listed below
// MIPS_4121 - Win32 (WCE MIPS) 4121 Release
// MIPS_4111 - Win32 (WCE MIPS) Release
//,MIPS_4121,OPT_MACC,OPT_MULTREAL,
//Set the defines below in the global preprocessor definitions for
// your appropriate project

//#define MIPS_4121		//Compiling for NEC VR4121 (E-100)
//#define MIPS_4111
//#define OPT_MACC
//#define OPT_MULTREAL
//#define REAL_IS_FLOAT
//#define REAL_IS_INT
//#define REAL_IS_FIXED
//#define REAL_IS_LONG_DOUBLE
//#define REAL_IS_DOUBLE

#include <wtypes.h>

#ifdef REAL_IS_FLOAT
#  define real float

#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double

#elif defined(REAL_IS_FIXED)
#define real fixed
#include "fixed.h"

#elif defined(REAL_IS_INT)
#define real int

#else
#  define real double
#endif


#ifdef REAL_IS_INT

#define FRACBITS 15
#define FRACF ((float)(1<<FRACBITS))


#if UNDER_CE

#if defined(_MIPS_)
extern "C" { 
void __asm(char*, ...); 
}; 
#pragma intrinsic (__asm)

#elif defined(_SH3_) || defined(_SH4_)
//extern "C" { void __asm(const char *, ...); };
#endif

#endif	// UNDER_CE

inline int mult_real(int a, int b)
{
	int result;

#if defined(UNDER_CE) && defined(OPT_MULTREAL)  /* ezra: added inline assembly */
	#if defined(_MIPS_)	 
#if 1
		__asm("mult %0,%1",a,b);
		__asm("mflo %t1");
		__asm("srl %t1,%t1,15");  //FRACBITS
		__asm("mfhi %t2");
		__asm("sll %t2,%t2,32-15"); //FRACBITS
		__asm("or %t2,%t2,%t1");
		__asm("sw %t2,0(%0)",&result); 
#else	// Matt's optimizations using doublewords	
		__asm("mult %0,%1",a,b);
		__asm("mflo %t1");
		__asm("mfhi %t0");
		__asm("dsrl %t0,%t0,15");  //FRACBITS
		__asm("sw %t1,0(%0)",&result); 
#endif
	#elif defined(_SH3_)
		__asm("mov.l   @R4,R2",&b);
		__asm(
			  "mov     #H'FFFFFFF1,R0\n"  //FFFFFFF1 = -15
			  "mov.l   @R4,R3\n"
			  "dmuls.l R3,R2\n"
			  "sts     MACL,R3\n"
			  "sts     MACH,R2\n"
			  "shld    R0,R3\n"
			  "mov     #D'17,R0\n"
			  "shad    R0,R2\n"
			  "or      R2,R3\n"
			  "mov.l   R3,@R5\n",
			  &a, &result);
	#elif defined(_SH4_)
		__asm("mov.l   @R4,R2\n"
			  "mov.l   @R5,R3\n",&a,&b);
		__asm("dmuls.l R3,R2");
		__asm("sts     MACL,R3");
		__asm("sts     MACH,R0");
		__asm("mov     #-15,R1");  //FFFFFFF4 = -15			  
		__asm("shld    R1,R3");
		__asm("mov     #17,R1");
		__asm("shld    R1,R0");
		__asm("or      R3,R0");
		__asm("mov.l   R0,@R4",&result);
	#endif
#else
	 result = (int)((((LONGLONG)a)*((LONGLONG)b))>>FRACBITS);
#endif  //UNDER_CE
	return result;
}

inline int multeq_real(int& a, int b)
{

#if defined(UNDER_CE)  && defined(OPT_MULTREAL)/* ezra: added inline assembly */
	#if defined(_MIPS_)	  
#if 1
		__asm("mult %0,%1",a,b);
		__asm("mflo %t1");
		__asm("srl %t1,%t1,15");  //FRACBITS
		__asm("mfhi %t2");
		__asm("sll %t2,%t2,32-15");  //FRACBITS
		__asm("or %t2,%t2,%t1");
		__asm("sw %t2,0(%0)",&a); 
#else	// Matt's optimizations using doublewords	
		__asm("mult %0,%1",a,b);
		__asm("mflo %t1");
		__asm("mfhi %t0");
		__asm("dsrl %t0,%t0,15"); //FRACBITS
		__asm("sw %t1,0(%0)",&a); 
#endif
	#elif defined(_SH3_) || defined(_SH4_)
		__asm("mov.l   @R4,R2",&b);
		__asm(
			  "mov     #H'FFFFFFF1,R0\n"  //FFFFFFF4 = -15
			  "mov.l   @R4,R3\n"
			  "dmuls.l R3,R2\n"
			  "sts     MACL,R3\n"
			  "sts     MACH,R2\n"
			  "shld    R0,R3\n"
			  "mov     #D'17,R0\n"
			  "shad    R0,R2\n"
			  "or      R2,R3\n"
			  "mov.l   R3,@R4\n",
			  &a);
	#endif
#else
	 a = (int)((((LONGLONG)a)*((LONGLONG)b))>>FRACBITS);
#endif  //UNDER_CE

	return a;
}

#if defined(_MIPS_)
inline void mips_macc(int a, int b,LONGLONG* sum)
{
	__asm("mult %0,%1",a,b);
	__asm("mflo %t1");
	__asm("mfhi %t0");
	__asm("sd %t0,(%0)",sum);
}

inline void mips_macc_end(LONGLONG sum,int* res)
{
#if 1
	__asm("ld %t0,0(%0)",&sum);
	__asm("srl %t1,%t1,15");  //FRACBITS
	__asm("sll %t0,%t0,32-15");  //FRACBITS
	__asm("or %t0,%t0,%t1");
	__asm("sw %t0,0(%0)",res); 
#else
	__asm("ld %t0,0(%0)",&sum);
	__asm("dsrl %t0,%t0,15");
	__asm("sw %t1,0(%0)",res);
#endif
}
#endif

#define REAL_INT(a) ((a) << FRACBITS)
#define REAL_FLOAT(a) ((int)((a)*FRACF))
#define REAL_ZERO	0

#define MULT_REAL(a, b) mult_real(a, b)
#define MULTEQ_REAL(a, b) multeq_real(a, b)


#elif !defined (REAL_IS_FIXED)

#define REAL_INT(a) a
#define REAL_FLOAT(a) a
#define REAL_ZERO	0

#define MULT_REAL(a, b) a * b
#define MULTEQ_REAL(a, b) a *= b

#endif


#endif	// REAL_H_
