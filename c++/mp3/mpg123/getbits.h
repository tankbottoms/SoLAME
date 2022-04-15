
#ifndef GETBITS_H_
#define GETBITS_H_

unsigned int getbits(int number_of_bits);
unsigned int getbits_fast(int number_of_bits);
unsigned int get1bit(void);

//extern int bitindex;
extern unsigned char *wordpointer;
/*
inline int getbitoffset(void)
{
	return (-bitindex)&0x7;
}

inline unsigned char getbyte(void)
{
	return *wordpointer++;
}

inline void backbits(int number_of_bits)
{
	bitindex -= number_of_bits;
	wordpointer += bitindex >> 3;
	bitindex &= 0x7;
}
*/
#endif	// GETBITS_H_
