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

#include "options.h"

//#ifdef PROFILE
#if defined(PROFILE) && defined(UNDER_CE)

#include "AutoProfile.h"

#ifdef DEBUG_FILE
#elif defined(DEBUG_BAG)
//	#include "apcdebug.h"
#endif  

#ifndef UNDER_CE
#include <stdio.h>
#endif

Profiler::Profiler(WCHAR* wcsProfilerName)
{int i;
 level=0;

 for (i=0;i<MAXNMNODES;i++)
    {nodes[i].nmChildren=0;
     nodes[i].totalTime=0;
     nodes[i].parent=NULL;
     nodes[i].lastReportedTime=0;
    }

 nmNodes=1;
 currentNode=nodes;
 currentNode->id=L"root";
#ifdef DEBUG_FILE
 hProfile = CreateFileW(wcsProfilerName, GENERIC_WRITE, FILE_SHARE_WRITE, 
						NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#elif defined (DEBUG_BAG)
 DEBUGMSG(ZONE_VERBOSE,(L"\nDebuging session: %s.", wcsProfilerName));	
#elif defined (PROFILE_NODEBUG)
 NKDbgPrintfW(L"\nDebuging session: %s.\n", wcsProfilerName);
#endif
 uiTimer = SetTimer(NULL, NULL, 60000, NULL);
 lastTime=getTimer();
}

Profiler::~Profiler(void)
{
#ifdef DEBUG_FILE
	CloseHandle(hProfile);
#endif
	KillTimer(NULL, uiTimer);
}

unsigned int Profiler::sumChildren(ProfileNode *node)
{int i;
 unsigned int tot;
 tot=node->totalTime;
 for (i=0;i<node->nmChildren;i++)
    tot+=sumChildren(node->child[i]);
 return tot;
}

void Profiler::printTree(ProfileNode *tree,int level,
		      unsigned int parentTotal,unsigned int parentDifference)
{int i;
 WCHAR buff[80];
 unsigned int difPercent,totPercent;
 unsigned int sum;
 for (i=0;i<level;i++)
    print(L"   ");
 sum=sumChildren(tree)>>10;
 if (parentTotal<1)
    totPercent=0;
 else
    totPercent=(1000*sum)/parentTotal;

 if (parentDifference<1)
    difPercent=0;
 else
    difPercent=(1000*(sum-tree->lastReportedTime))/parentDifference;

// swprintf(buff,L"%.1f (%.1f): %s\n",difPercent*.1f,totPercent*.1f,tree->id);
 swprintf(buff,L"%.1f (%.1f) (%d): %s\n",difPercent*.1f,totPercent*.1f,sum,tree->id);
 print(buff);
/*
 for (i=0;i<tree->nmChildren;i++)
    printTree(tree->child[i],level+1,sum,sum-tree->lastReportedTime);
*/
 for (i=0;i<tree->nmChildren;i++)
	 printTree(tree->child[i],level+1,level == 0 ? sum : parentTotal,sum-tree->lastReportedTime);

 tree->lastReportedTime=sum;
}

void Profiler::dumpProfileData(void)
{unsigned int sum;
 print(L"\n\n");
 sum=sumChildren(nodes)>>10;
 printTree(nodes,0,sum,sum-nodes->lastReportedTime);
}

void Profiler::print(WCHAR* szMsg)
{
#ifdef DEBUG_FILE
 DWORD dwBytesWritten;		
 if (hProfile)
 {
	 WriteFile(hProfile, (void*) szMsg, sizeof(WCHAR) * strlen(szMsg), &dwBytesWritten, NULL);
 }		
#elif defined (DEBUG_BAG) 
	DEBUGMSG(ZONE_VERBOSE,(szMsg));
#elif defined (PROFILE_NODEBUG)
	NKDbgPrintfW(szMsg);
#endif
}

unsigned int Profiler::getTimer(void)
{LARGE_INTEGER lrgint;
 QueryPerformanceCounter(&lrgint);
 return static_cast<unsigned int>(lrgint.QuadPart);	
}

Profiler sProfiler(L"profiling.txt");


FunctionProfiler::FunctionProfiler(WCHAR* id) 
{unsigned int time;
 int i;
 for (i=0;i<sProfiler.currentNode->nmChildren;i++)
    if (sProfiler.currentNode->child[i]->id==id)
       break;

 time=sProfiler.getTimer();
 sProfiler.currentNode->totalTime+=(time-sProfiler.lastTime)>>TIMESHIFT;
 sProfiler.lastTime=time;

 if (i==sProfiler.currentNode->nmChildren)
    {DEBUGCHK(sProfiler.nmNodes<MAXNMNODES);
     DEBUGCHK(sProfiler.currentNode->nmChildren<MAXNMCHILDREN);
     sProfiler.currentNode->child[sProfiler.currentNode->nmChildren++]=sProfiler.nodes+sProfiler.nmNodes;
     sProfiler.nodes[sProfiler.nmNodes].parent=sProfiler.currentNode;
     sProfiler.nodes[sProfiler.nmNodes].id=id;
     sProfiler.currentNode=sProfiler.nodes+sProfiler.nmNodes; 
     sProfiler.nmNodes++;
    }
 else
    sProfiler.currentNode=sProfiler.currentNode->child[i];

}

FunctionProfiler::~FunctionProfiler(void)
{
 unsigned int time=sProfiler.getTimer();
 sProfiler.currentNode->totalTime+=(time-sProfiler.lastTime)>>TIMESHIFT;
 sProfiler.lastTime=time;
 sProfiler.currentNode=sProfiler.currentNode->parent; 
 DEBUGCHK(sProfiler.currentNode);
}

#endif	// PROFILE
