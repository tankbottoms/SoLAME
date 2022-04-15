#define WIN32_LEAN_AND_MEAN

/*
	by Ezra Driebach, Mark Phillips

	#define PROFILE 
	#define DEBUG_BAG
	#include "autoprofile.h"

	Used at the beginning of functions:
	-->FunctionProfiler fp("function1");	
	
	To print out profile data:
	#ifdef PROFILE 
	 sProfiler.dumpProfileData();
	#endif 
*/

#ifndef AUTOPROFILE_H_
#define AUTOPROFILE_H_

#include "options.h"

#ifdef PROFILE

//#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
//#include <assert.h>

#define TIMESHIFT 0
#define MAXNMCHILDREN 8
#define MAXNMNODES 60

#ifdef DEBUG_FILE
#elif defined(DEBUG_BAG)
//#	include "apcdebug.h"
#endif  

typedef struct _node
{WCHAR* id;
 unsigned int totalTime,lastReportedTime;
 struct _node *child[MAXNMCHILDREN];
 struct _node *parent;
 int nmChildren;
} ProfileNode;

class FunctionProfiler;
class Profiler
{
	friend FunctionProfiler;
public:
	Profiler(WCHAR*);
	~Profiler(void);
	
	unsigned int sumChildren(ProfileNode *node);
	void printTree(	ProfileNode *tree,int level,unsigned int parentTotal,
					unsigned int parentDifference);
	void dumpProfileData(void);
	unsigned int getTimer(void);
	void print(WCHAR* fmt);

private:
	ProfileNode nodes[MAXNMNODES];
	int nmNodes;
	int level;
	unsigned int lastTime;
	
#ifdef DEBUG_FILE
	HANDLE hProfile;
#endif
	UINT uiTimer;

	ProfileNode *currentNode;
};

extern Profiler sProfiler;

class FunctionProfiler
{
public:
	FunctionProfiler(WCHAR* wcsFunctionName);
	~FunctionProfiler(void);
};

#else

/*
class FunctionProfiler 
{public:
 FunctionProfiler(WCHAR*){}; 
 void dumpProfileData(void){};
};
*/

#endif	// AUTOPROFILE_H_
	
#endif	// PROFILE
