#ifndef __DEBUG_
#define __DEBUG_

// Debug.h
//===================================================================
/*
	Implementation 
*/
//===================================================================
#ifdef DEBUG

/*
#include <varargs.h>

static char szerrormsg[1024];
static void Fatal(char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);	
	vsprintf(szerrormsg, fmt, ap);
	OutputDebugStringA("Fatal Error: ");
	OutputDebugStringA(szerrormsg);
}

static void Warning(char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	OutputDebugStringA("Warning: ");
	vsprintf(szerrormsg, fmt, ap);
	OutputDebugStringA(szerrormsg);	
}

static void Message(char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt); 
	vsprintf(szerrormsg, fmt, ap);
	OutputDebugStringA(szerrormsg);	
}
*/
static void Fatal(char *fmt, ...)
{
}

static void Warning(char *fmt, ...)
{
}

static void Message(char *fmt, ...)
{
}

#endif

#endif
