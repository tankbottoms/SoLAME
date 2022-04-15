#ifndef _FIXED_H__
#define _FIXED_H__

#define FRACBITS 12 /* ezra: changed from 8 (if this is chaned it needs to be changed in two places in *=) */
#define FRACF ((float)(1<<FRACBITS))

#ifdef UNDER_CE

#if defined(_MIPS_)
extern "C" { 
void __asm(char*, ...); 
}; 
#pragma intrinsic (__asm)

#elif defined(_SH3_) || defined(_SH4_)
//extern "C" { void __asm(const char *, ...); };
#endif

#endif

#ifdef REAL_IS_FIXED

#if 1

#define MULT_REAL(a, b) ((a) * (b))
#define MULTEQ_REAL(a, b) ((a) *= (b))

#define REAL_INT(a) fixed(a)
//#define REAL_FLOAT(a) fixed((int)((a)*FRACF), 0)
#define REAL_FLOAT(a) fixed(a)
#define REAL_ZERO fixed(0, 0)

class fixed
{public:
  fixed(void) {}
//private:
  fixed(int _i, int dummy) {i=_i;}
public:
  fixed(float f) {i=(int)(f*FRACF);}
  fixed(const fixed &o) {i=o.i;}  /* ezra: added this, don't know if it makes any difference */
  fixed& operator=(const float f)
  {
	  i = (int)(f*FRACF);
	  return *this;
  }

  operator float() {return i*(1.f/FRACF);}
  operator int() {return i>>FRACBITS;}
  friend fixed operator +(const fixed &o1,const fixed &o2)
    {return fixed(o1)+=o2;
    }
  friend fixed operator -(const fixed &o1,const fixed &o2)
    {return fixed(o1)-=o2;
    }
  friend fixed operator *(const fixed &o1,const fixed &o2)
    {
	  return fixed(o1)*=o2;
    }

  fixed& operator +=(const fixed &o)
    {i+=o.i;
     return *this;
    }

  fixed& operator *=(const fixed &o)
    {
#if defined(UNDER_CE)  /* ezra: added inline assembly */
	#if defined(_MIPS_)	  
		__asm("mult %0,%1",i,o.i);
		__asm("mflo %t1");
		__asm("srl %t1,%t1,12");
		__asm("mfhi %t2");
		__asm("sll %t2,%t2,32-12");
		__asm("or %t2,%t2,%t1");
		__asm("sw %t2,0(%0)",&i); 
	#elif defined(_SH3_) || defined(_SH4_)
	__asm("mov.l   @R4,R2",&o.i);
	__asm(
		  "mov     #H'FFFFFFF1,R0\n"  //FFFFFFF4 = -15
		  "mov.l   @R4,R3\n"
		  "dmuls.l R3,R2\n"
		  "sts     MACL,R3\n"
		  "sts     MACH,R2\n"
//		  "mov     #H'FFFFFFF4,R0\n"  //FFFFFFF4 = -12
		  "shld    R0,R3\n"
		  "mov     #D'17,R0\n"
		  "shad    R0,R2\n"
		  "or      R2,R3\n"
		  "mov.l   R3,@R4\n",
		  &i);
	#endif
#else
	 i = (int)((((LONGLONG)i)*((LONGLONG)o.i))>>FRACBITS);
#endif  //UNDER_CE
     return *this;
    }

  fixed& operator -=(const fixed &o)
    {i-=o.i;
     return *this;
    }
    
  friend fixed operator -(const fixed &o)
    {return fixed(-o.i, (int)0);
    }
 public:
  int i;

};


#else

#define REAL_INT(a) fixed((a) << 12, 0)
#define REAL_FLOAT(a) fixed((int)((a)*FRACF) << 12, 0)

#define MULT_REAL(a, b) fixed(((a).i * (b).i) >> 12, 0)
#define MULTEQ_REAL(a, b) (a.i = ((a).i * (b).i) >> 12)

#define REAL_ZERO fixed(0, 0)


class fixed
{public:
  fixed(void) {}
//private:
  fixed(int _i, int dummy) {i=_i;}
public:
//  fixed(float f) {i=(int)(f*FRACF);}
  //Copy constructor causes unexpected heap error in layer3 due to too much inline expansion
  //fixed(const fixed &o) { i=o.i; }	/* ezra: added this, don't know if it makes any difference */
/*
  fixed& operator=(const float f)
  {
	  i = (int)(f*FRACF);
	  return *this;
  }
*/

//  operator float() {return i*(IFRACF);}
//  operator int() {return i>>FRACBITS;}
  friend fixed operator +(const fixed &o1,const fixed &o2)
    {return fixed(o1)+=o2;
    }
  friend fixed operator -(const fixed &o1,const fixed &o2)
    {return fixed(o1)-=o2;
    }
/*
  friend fixed operator *(const fixed &o1,const fixed &o2)
    {
	  return fixed(o1)*=o2;
    }
*/

  fixed& operator +=(const fixed &o)
    {i+=o.i;
     return *this;
    }
/*
  fixed& operator *=(const fixed &o)
    {
#if defined(UNDER_CE) && defined(_SH3_)

	//Matt - by combining asm statements containing 
	//       similar C expressions, you can reduce the number
	//       of times a variable must be moved.  e.g. the
	//       asm below is 200 ms faster than the seemingly
	//       same code commented further below. (the value
	//       this-> is only referenced once instead of twice.
#if 1
	__asm("mov.l   @R4,R2",&o.i);
	__asm(
		  "mov     #H'FFFFFFF4,R0\n"  //FFFFFFF4 = -12
		  "mov.l   @R4,R3\n"
		  "dmuls.l R3,R2\n"
		  "sts     MACL,R3\n"
		  "sts     MACH,R2\n"
//		  "mov     #H'FFFFFFF4,R0\n"  //FFFFFFF4 = -12
		  "shld    R0,R3\n"
		  "mov     #D'20,R0\n"
		  "shad    R0,R2\n"
		  "or      R2,R3\n"
		  "mov.l   R3,@R4\n",
		  &i);
#else
	__asm("mov.l @R4,R3",&i);
	__asm("mov.l @R4,R2",&o.i);
	//__asm("clrmac");
	__asm("dmuls.l R3,R2");
	__asm("sts MACL,R3");
	__asm("sts MACH,R2");
	__asm("mov #H'FFFFFFF4,R0");
	__asm("shld R0,R3");
	__asm("mov #D'20,R0");
	__asm("shad R0,R2");
	__asm("or R2,R3");
	__asm("mov.l R3,@R4",&i);
#endif

#else

	 i = (int)((((LONGLONG)i)*((LONGLONG)o.i))>>FRACBITS);

#endif
	 
     return *this;
    }
*/

  fixed& operator -=(const fixed &o)
    {i-=o.i;
     return *this;
    }
    
  friend fixed operator -(const fixed &o)
    {return fixed(-o.i, (int)0);
    }
 public:
  int i;

};




#endif

#endif	// REAL_IS_FIXED


#endif	// _FIXED_H__