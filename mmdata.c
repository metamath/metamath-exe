/*****************************************************************************/
/*        Copyright (C) 2005  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*
mmdata.c
*/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmcmdl.h" /* Needed for logFileName */

#include <limits.h>
#include <setjmp.h>
/*E*/long db=0,db0=0,db1=0,db2=0,db3=0,db4=0,db5=0,db6=0,db7=0,db8=0,db9=0;
flag listMode = 0; /* 0 = metamath, 1 = list utility */
flag toolsMode = 0; /* In metamath: 0 = metamath, 1 = text tools utility */

/* Global variables related to current statement */
int currentScope = 0;
long beginScopeStatementNum = 0;

long MAX_STATEMENTS = 1;
long MAX_MATHTOKENS = 1;
long MAX_INCLUDECALLS = 2; /* Must be at least 2 (the single-file case) !!!
                         (A dummy extra top entry is used by parseKeywords().) */
struct statement_struct *statement;
long *labelKey;
struct mathToken_struct *mathToken;
long *mathKey;
long statements = 0, labels = 0, mathTokens = 0;
long maxMathTokenLength = 0;

struct includeCall_struct *includeCall;
long includeCalls = 0;

char *sourcePtr;
long sourceLen;

/* 18-Jan-05 nm The structs below, and several other places, were changed
   from hard-coded byte lengths to 'sizeof's by Waldek Hebisch
   (hebisch at math dot uni dot wroc dot pl) so this will work on the
   AMD64. */

/* Null numString */
struct nullNmbrStruct nmbrNull = {-1, sizeof(long), sizeof(long), -1};

/* Null ptrString */
struct nullPntrStruct pntrNull = {-1, sizeof(long), sizeof(long), NULL};

nmbrString *nmbrTempAlloc(long size);
        /* nmbrString memory allocation/deallocation */
void nmbrCpy(nmbrString *sout, nmbrString *sin);
void nmbrNCpy(nmbrString *s, nmbrString *t, long n);

pntrString *pntrTempAlloc(long size);
        /* pntrString memory allocation/deallocation */
void pntrCpy(pntrString *sout, pntrString *sin);
void pntrNCpy(pntrString *s, pntrString *t, long n);


/* Memory pools are used to reduce the number of malloc and alloc calls that
   allocate arrays (strings or nmbr/pntrStrings typically).   The "free" pool
   contains previously allocated arrays that are no longer used but that we
   have not freed yet.  A call to allocate a new array fetches one from here
   first.   The "used"
   pool contains arrays that are partially used; each array has some free space
   at the end that can be deallocated if memory gets low.   Any array that is
   totally used (no free space) is not in any pool. */
/* Each pool array has 3 "hidden" long elements before it, used by these
   procedures.
     Element -1:  actual size (bytes) of array, excluding the 3 "hidden"
       long elements.
     Element -2:  allocated size.  If all elements are used, allocated = actual.
     Element -3:  location of array in memUsedPool.  If -1, it means that
       actual = allocated and storage in memUsedPool is therefore not nec.
   The pointer to an array always points to element 0 (recast to right size).
*/

#define MEM_POOL_GROW 1000 /* Amount that a pool grows when it overflows. */
/*??? Let user set this from menu. */
long poolAbsoluteMax = /*2000000*/1000000; /* Pools will be purged when this is reached */
long poolTotalFree = 0; /* Total amount of free space allocated in pool */
/*E*/long i1,j1,k1;
void **memUsedPool = NULL;
long memUsedPoolSize = 0; /* Current # of partially filled arrays in use */
long memUsedPoolMax = 0; /* Maximum # of entries in 'in use' table (grows
                               as nec.) */
void **memFreePool = NULL;
long memFreePoolSize = 0; /* Current # of available, allocated arrays */
long memFreePoolMax = 0; /* Maximum # of entries in 'free' table (grows
                               as nec.) */

/* poolFixedMalloc should be called when the allocated array will rarely be
   changed; a malloc or realloc with no unused array bytes will be done. */
void *poolFixedMalloc(long size /* bytes */)
{
  void *ptr;
  void *ptr2;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("a0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  if (!memFreePoolSize) { /* The pool is empty; we must allocate memory */
    ptr = malloc( 3 * sizeof(long) + size);
    if (!ptr) outOfMemory(
        cat("#25 (poolFixedMalloc ", str(size), ")", NULL));

    ptr = (long *)ptr + 3;
    ((long *)ptr)[-1] = size; /* Actual size */
    ((long *)ptr)[-2] = size; /* Allocated size */
    ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
    return (ptr);
  } else {
    memFreePoolSize--;
    ptr = memFreePool[memFreePoolSize];
    poolTotalFree = poolTotalFree - ((long *)ptr)[-2];
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("a: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    if (size <= ((long *)ptr)[-2]) { /* We have enough space already */
      ptr2 = realloc( (long *)ptr - 3, 3 * sizeof(long) + size);
      /* Reallocation cannot fail, since we are shrinking space */
      if (!ptr2) bug(1382);
      ptr = ptr2;
    } else { /* The pool's last entry is too small; free and allocate new */
      free((long *)ptr - 3);
      ptr = malloc( 3 * sizeof(long) + size);
    }
    if (!ptr) {
      /* Try freeing space */
      print2("Memory is low.  Deallocating storage pool...\n");
      memFreePoolPurge(0);
      ptr = malloc( 3 * sizeof(long) + size);
      if (!ptr) outOfMemory(
          cat("#26 (poolMalloc ", str(size), ")", NULL));
                                            /* Nothing more can be done */
    }
    ptr = (long *)ptr + 3;
    ((long *)ptr)[-1] = size; /* Actual size */
    ((long *)ptr)[-2] = size; /* Allocated size */
    ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
    return (ptr);
  }
}



/* poolMalloc tries first to use an array in the memFreePool before actually
   malloc'ing */
void *poolMalloc(long size /* bytes */)
{
  void *ptr;
  long memUsedPoolTmpMax;
  void *memUsedPoolTmpPtr;

  /* Check to see if the pool total exceeds max. */
  if (poolTotalFree > poolAbsoluteMax) {
    memFreePoolPurge(1);
  }

/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("b0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  if (!memFreePoolSize) { /* The pool is empty; we must allocate memory */
    ptr = malloc( 3 * sizeof(long) + size);
    if (!ptr) {
      outOfMemory(cat("#27 (poolMalloc ", str(size), ")", NULL));
    }
    ptr = (long *)ptr + 3;
    ((long *)ptr)[-1] = size; /* Actual size */
    ((long *)ptr)[-2] = size; /* Allocated size */
    ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
    return (ptr);
  } else {
    memFreePoolSize--;
    ptr = memFreePool[memFreePoolSize];
    poolTotalFree = poolTotalFree - ((long *)ptr)[-2];
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("b: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    if (size <= ((long *)ptr)[-2]) { /* We have enough space already */
      ((long *)ptr)[-1] = size; /* Actual size */
      ((long *)ptr)[-3] = -1; /* Not in storage pool yet */
    } else { /* We must reallocate */
      free((long *)ptr - 3);
      ptr = malloc( 3 * sizeof(long) + size);
      if (!ptr) {
        /* Try freeing space */
        print2("Memory is low.  Deallocating storage pool...\n");
        memFreePoolPurge(0);
        ptr = malloc( 3 * sizeof(long) + size);
        if (!ptr) outOfMemory(
            cat("#28 (poolMalloc ", str(size), ")", NULL));
                                              /* Nothing more can be done */
      }
      ptr = (long *)ptr + 3;
      ((long *)ptr)[-1] = size; /* Actual size */
      ((long *)ptr)[-2] = size; /* Allocated size */
      ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("bb: pool %ld stat %ld\n",poolTotalFree,i1+j1);
      return (ptr);
    }
  }
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("bc: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) return (ptr);
  /* Allocated and actual sizes are different, so add this array to used pool */
  poolTotalFree = poolTotalFree + ((long *)ptr)[-2] - ((long *)ptr)[-1];
  if (memUsedPoolSize >= memUsedPoolMax) { /* Increase size of used pool */
    memUsedPoolTmpMax = memUsedPoolMax + MEM_POOL_GROW;
/*E*/if(db9)print2("Growing used pool to %ld\n",memUsedPoolTmpMax);
    if (!memUsedPoolMax) {
      /* The program has just started; initialize */
      memUsedPoolTmpPtr = malloc(memUsedPoolTmpMax
          * sizeof(void *));
      if (!memUsedPoolTmpPtr) bug(1303); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memUsedPoolTmpPtr = realloc(memUsedPool,
          memUsedPoolTmpMax * sizeof(void *));
    }
    if (!memUsedPoolTmpPtr) {
      outOfMemory(cat("#29 (poolMalloc ", str(memUsedPoolTmpMax), ")", NULL));
    } else {
      /* Reallocation successful */
      memUsedPool = memUsedPoolTmpPtr;
      memUsedPoolMax = memUsedPoolTmpMax;
    }
  }
  memUsedPool[memUsedPoolSize] = ptr;
  ((long *)ptr)[-3] = memUsedPoolSize;
  memUsedPoolSize++;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("c: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return (ptr);
}

/* poolFree puts freed up space in memFreePool. */
void poolFree(void *ptr)
{
  void *ptr1;
  long usedLoc;
  long memFreePoolTmpMax;
  void *memFreePoolTmpPtr;

/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("c0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  /* First, see if the array is in memUsedPool; if so, remove it. */
  usedLoc = ((long *)ptr)[-3];
  if (usedLoc >= 0) { /* It is */
    poolTotalFree = poolTotalFree - ((long *)ptr)[-2] + ((long *)ptr)[-1];
    memUsedPoolSize--;
    memUsedPool[usedLoc] = memUsedPool[memUsedPoolSize];
    ptr1 = memUsedPool[usedLoc];
    ((long *)ptr1)[-3] = usedLoc;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("d: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  }

  /* Next, add the array to the memFreePool */
  /* First, allocate more memFreePool pointer space if needed */
  if (memFreePoolSize >= memFreePoolMax) { /* Increase size of free pool */
    memFreePoolTmpMax = memFreePoolMax + MEM_POOL_GROW;
/*E*/if(db9)print2("Growing free pool to %ld\n",memFreePoolTmpMax);
    if (!memFreePoolMax) {
      /* The program has just started; initialize */
      memFreePoolTmpPtr = malloc(memFreePoolTmpMax
          * sizeof(void *));
      if (!memFreePoolTmpPtr) bug(1304); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memFreePoolTmpPtr = realloc(memFreePool,
          memFreePoolTmpMax * sizeof(void *));
    }
    if (!memFreePoolTmpPtr) {
/*E*/if(db9)print2("Realloc failed\n");
      outOfMemory(cat("#30 (poolFree ", str(memFreePoolTmpMax), ")", NULL));
    } else {
      /* Reallocation successful */
      memFreePool = memFreePoolTmpPtr;
      memFreePoolMax = memFreePoolTmpMax;
    }
  }
  /* Add the free array to the free pool */
  memFreePool[memFreePoolSize] = ptr;
  memFreePoolSize++;
  poolTotalFree = poolTotalFree + ((long *)ptr)[-2];
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("e: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return;
}


/* addToUsedPool adds a (partially used) array to the memUsedPool */
void addToUsedPool(void *ptr)
{
  long memUsedPoolTmpMax;
  void *memUsedPoolTmpPtr;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("d0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) bug(1305); /* No need to add it
                                 when it's not partially used */
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) return;
  /* Allocated and actual sizes are different, so add this array to used pool */
  if (memUsedPoolSize >= memUsedPoolMax) { /* Increase size of used pool */
    memUsedPoolTmpMax = memUsedPoolMax + MEM_POOL_GROW;
/*E*/if(db9)print2("1Growing used pool to %ld\n",memUsedPoolTmpMax);
    if (!memUsedPoolMax) {
      /* The program has just started; initialize */
      memUsedPoolTmpPtr = malloc(memUsedPoolTmpMax
          * sizeof(void *));
      if (!memUsedPoolTmpPtr) bug(1362); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memUsedPoolTmpPtr = realloc(memUsedPool, memUsedPoolTmpMax
          * sizeof(void *));
    }
    if (!memUsedPoolTmpPtr) {
      outOfMemory("#31 (addToUsedPool)");
    } else {
      /* Reallocation successful */
      memUsedPool = memUsedPoolTmpPtr;
      memUsedPoolMax = memUsedPoolTmpMax;
    }
  }
  memUsedPool[memUsedPoolSize] = ptr;
  ((long *)ptr)[-3] = memUsedPoolSize;
  memUsedPoolSize++;
  poolTotalFree = poolTotalFree + ((long *)ptr)[-2] - ((long *)ptr)[-1];
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("f: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return;
}

/* Free all arrays in the free pool. */
void memFreePoolPurge(flag untilOK)
{
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("e0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  while (memFreePoolSize) {
    memFreePoolSize--;
    /* Free an array */
    poolTotalFree = poolTotalFree -
        ((long *)(memFreePool[memFreePoolSize]))[-2];
    free((long *)(memFreePool[memFreePoolSize]) - 3);
    if (untilOK) {
      /* If pool size is OK, return. */
      if (poolTotalFree <= poolAbsoluteMax) return;
    }
  }
  /* memFreePoolSize = 0 now. */
  if (memFreePoolMax != MEM_POOL_GROW) {
    /* Reduce size of pool pointer array to minimum growth increment. */
    if (memFreePool) free(memFreePool); /* Only when starting program */
    memFreePool = malloc(MEM_POOL_GROW
        * sizeof(void *)); /* Allocate starting increment */
    memFreePoolMax = MEM_POOL_GROW;
  }
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("g: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return;
}


/* Get statistics for SHOW MEMORY command */
void getPoolStats(long *freeAlloc, long *usedAlloc, long *usedActual)
{
  long i;
  *freeAlloc = 0;
  *usedAlloc = 0;
  *usedActual = 0;
  for (i = 0; i < memFreePoolSize; i++) {
    *freeAlloc = *freeAlloc + /*12 +*/ ((long *)(memFreePool[i]))[-2];
  }
  for (i = 0; i < memUsedPoolSize; i++) {
    *usedActual = *usedActual + 12 + ((long *)(memUsedPool[i]))[-1];
    *usedAlloc = *usedAlloc + ((long *)(memUsedPool[i]))[-2] -
        ((long *)(memUsedPool[i]))[-1];
  }
/*E*/ if (!db9)print2("poolTotalFree %ld  alloc %ld\n", poolTotalFree, *freeAlloc +
/*E*/   *usedAlloc);
}



void initBigArrays(void)
{

/*??? This should all become obsolete. */
  statement = malloc(MAX_STATEMENTS * sizeof(struct statement_struct));
/*E*//*db=db+MAX_STATEMENTS * sizeof(struct statement_struct);*/
  if (!statement) {
    print2("*** FATAL ***  Could not allocate statement space\n");
    bug(1363);
    }
  mathToken = malloc(MAX_MATHTOKENS * sizeof(struct mathToken_struct));
/*E*//*db=db+MAX_MATHTOKENS * sizeof(struct mathToken_struct);*/
  if (!mathToken) {
    print2("*** FATAL ***  Could not allocate mathToken space\n");
    bug(1364);
    }
  includeCall = malloc(MAX_INCLUDECALLS * sizeof(struct includeCall_struct));
/*E*//*db=db+MAX_INCLUDECALLS * sizeof(struct includeCall_struct);*/
  if (!includeCall) {
    print2("*** FATAL ***  Could not allocate includeCall space\n");
    bug(1365);
    }
}

/* Find the number of free memory bytes */
long getFreeSpace(long max)
{
  long i , j, k;
  char *s;
  i = 0;
  j = max + 2;
  k = j;
  while (i < j - 2) {
    k = (i + j) / 2;
    s = malloc(k);
    if (s) {
      free(s);
      i = k;
    } else {
      j = k;
    }
  }
  return (i);
}

/* Fatal memory allocation error */
void outOfMemory(vstring msg)
{
  vstring tmpStr = "";
  print2("*** FATAL ERROR:  Out of memory.\n");
  print2("Internal identifier (for technical support):  %s\n",msg);
  print2(
"To solve this problem, remove some unnecessary statements or file\n");
  print2(
"inclusions to reduce the size of your input source.\n");
  print2(
"Monitor memory periodically with SHOW MEMORY.\n");
#ifdef THINK_C
  print2(
"You may also increase the \"Application Memory Size\" under \"Get Info\"\n");
  print2(
"under \"File\" in the Finder after clicking once on the Metamath\n");
  print2("application icon.\n");
#endif
  print2("\n");
  print2("Press <return> to exit Metamath.\n");
  tmpStr = cmdInput1("");
  /* Close the log to make sure error log is saved */
  if (logFileOpenFlag) {
    fclose(logFilePtr);
    logFileOpenFlag = 0;
  }

  exit(0);
}


/* Bug check */
void bug(int bugNum)
{
  vstring tmpStr = "";

  /* 10/10/02 */
  long saveOutputToString = outputToString;
  outputToString = 0; /* Make sure we print to screen and not to string */

  print2("*** A PROGRAM BUG WAS DETECTED.\n");
  print2("Bug identifier (for technical support):  %ld\n",(long)bugNum);
  print2(
"To get technical support, please send Norm Megill (%salum.mit.edu) a\n",
      "nm@");
  print2(
"command file that reproduces this bug, along with the source files that\n");
  print2(
"were used.  See HELP SUBMIT for help on command files.\n");
  print2("\n");
  let(&tmpStr, "?");
  while (strcmp(tmpStr, "cont") && strcmp(tmpStr, "")) {
    let(&tmpStr, "");
    print2(
"Press RETURN to abort program, or type 'cont' to continue at your own risk\n");
    tmpStr = cmdInput1("?");
    /******* 8-Nov-03 This loop caused an infinite loop in a cron job when bug
     detection was triggered.  Now, with this loop broken, if the user
     types anything other than 'cont' the program will abort. *******/
    break; /* Added 8-Nov-03 */
  }
  if (!strcmp(tmpStr, "cont")) {
    print2(
    "Warning!!!  A bug was detected, but you are continuing anyway.\n");
    print2(
    "The program may be corrupted, so you are proceeding at your own risk.\n");
    print2("\n");
    let(&tmpStr, "");

    /* 10/10/02 */
    outputToString = saveOutputToString; /* Restore for continuation */

    return;
  }
  let(&tmpStr, "");
#ifdef THINK_C
  cmdInput1("Program has crashed.  Press <return> to leave.");
#endif

  /* Close the log to make sure error log is saved */
  if (logFileOpenFlag) {
    fclose(logFilePtr);
    logFileOpenFlag = 0;
  }

  exit(1); /* Use 1 instead of 0 to flag abnormal termination to scripts */
}


#define M_MAX_ALLOC_STACK 100


/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have '*' wildcard characters.*/
flag matches(vstring testString, vstring pattern, char wildCard) {

  long i, ppos, pctr, tpos;
  /* Get to first wild card character */
  ppos = 0;
  while (pattern[ppos] == testString[ppos] && pattern[ppos] != 0) ppos++;
  if (pattern[ppos] == 0) {
    if (testString[ppos] != 0) return (0); /* No wildcards; mismatched */
    else return (1); /* No wildcards; matched */
  }
  if (pattern[ppos] != wildCard) return (0); /* Mismatched */
  tpos = ppos;

  /* Scan remainder of pattern */
  pctr = 0;
  i = 0;
  while (1) {
    if (pattern[ppos + 1 + i] == wildCard) { /* Next wildcard found */
      tpos = tpos + pctr + i;
      ppos = ppos + 1 + i;
      i = 0;
      pctr = 0;
      continue;
    }
    if (pattern[ppos + 1 + i] != testString[tpos + pctr + i]) {
      if (testString[tpos + pctr + i] == 0) return (0);
      pctr++;
      i = 0;
      continue;
    }
    if (pattern[ppos + 1 + i] == 0) return(1); /* Matched */
    i++;
  }
  return (0); /* Dummy return - never used */
}




/*******************************************************************/
/*********** Number string functions *******************************/
/*******************************************************************/

int nmbrTempAllocStackTop = 0;     /* Top of stack for nmbrTempAlloc functon */
int nmbrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when nmbrLet() is called (normally 0, except in
                                    special nested vstring functions) */
nmbrString *nmbrTempAllocStack[M_MAX_ALLOC_STACK];


nmbrString *nmbrTempAlloc(long size)
                                /* nmbrString memory allocation/deallocation */
{
  /* When "size" is >0, "size" instances of nmbrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to nmbrStartTempAllocStack. */
  int i;
  if (size) {
    if (nmbrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
      /*??? Fix to allocate more */
      outOfMemory("#105 (nmbrString stack array)");
    }
    if (!(nmbrTempAllocStack[nmbrTempAllocStackTop++]=poolMalloc(size
        *sizeof(nmbrString))))
      /* outOfMemory("#106 (nmbrString stack)"); */ /*???Unnec. w/ poolMalloc*/
/*E*/db2=db2+(size)*sizeof(nmbrString);
    return (nmbrTempAllocStack[nmbrTempAllocStackTop-1]);
  } else {
    for (i=nmbrStartTempAllocStack; i < nmbrTempAllocStackTop; i++) {
/*E*/db2=db2-(nmbrLen(nmbrTempAllocStack[i])+1)*sizeof(nmbrString);
      poolFree(nmbrTempAllocStack[i]);
    }
    nmbrTempAllocStackTop=nmbrStartTempAllocStack;
    return (0);
  }
}


/* Make string have temporary allocation to be released by next nmbrLet() */
/* Warning:  after nmbrMakeTempAlloc() is called, the nmbrString may NOT be
   assigned again with nmbrLet() */
void nmbrMakeTempAlloc(nmbrString *s)
{
    if (nmbrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
      printf(
      "*** FATAL ERROR ***  Temporary nmbrString stack overflow in nmbrMakeTempAlloc()\n");
      bug(1368);
    }
    if (s[0] != -1) { /* End of string */
      /* Do it only if nmbrString is not empty */
      nmbrTempAllocStack[nmbrTempAllocStackTop++] = s;
    }
/*E*/db2=db2+(nmbrLen(s)+1)*sizeof(nmbrString);
/*E*/db3=db3-(nmbrLen(s)+1)*sizeof(nmbrString);
}


void nmbrLet(nmbrString **target,nmbrString *source)
/* nmbrString assignment */
/* This function must ALWAYS be called to make assignment to */
/* a nmbrString in order for the memory cleanup routines, etc. */
/* to work properly.  If a nmbrString has never been assigned before, */
/* it is the user's responsibility to initialize it to NULL_NMBRSTRING (the */
/* null string). */
{
  long targetLength,sourceLength;
  long targetAllocLen;
  long poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  sourceLength=nmbrLen(source);  /* Save its actual length */
  targetLength=nmbrLen(*target);  /* Save its actual length */
  targetAllocLen=nmbrAllocLen(*target); /* Save target's allocated length */
/*E*/if (targetLength) {
/*E*/  /* printf("Deleting %s\n",cvtMToVString(*target,0)); */
/*E*/  db3 = db3 - (targetLength+1)*sizeof(nmbrString);
/*E*/}
/*E*/if (sourceLength) {
/*E*/  /* printf("Adding %s\n",cvtMToVString(source,0)); */
/*E*/  db3 = db3 + (sourceLength+1)*sizeof(nmbrString);
/*E*/}
  if (targetAllocLen) {
    if (sourceLength) { /* source and target are both nonzero length */

      if (targetAllocLen >= sourceLength) { /* Old string has room for new one */
        nmbrCpy(*target,source); /* Re-use the old space to save CPU time */

        /* Memory pool handling */
        /* Assign actual size of target string */
        poolDiff = ((long *)(*target))[-1] - ((long *)source)[-1];
        ((long *)(*target))[-1] = ((long *)source)[-1];
        /* If actual size of target string is less than allocated size, we
           may have to add it to the used pool */
        if (((long *)(*target))[-1] != ((long *)(*target))[-2]) {
          if (((long *)(*target))[-1] > ((long *)(*target))[-2]) bug(1325);
          if (((long *)(*target))[-3] == -1) {
            /* It's not already in the used pool, so add it */
            addToUsedPool(*target);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0aa: pool %ld stat %ld\n",poolTotalFree,i1+j1);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0ab: pool %ld stat %ld\n",poolTotalFree,i1+j1);
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }


/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0a: pool %ld stat %ld\n",poolTotalFree,i1+j1);
      } else {
        /* Free old string space and allocate new space */
        poolFree(*target);  /* Free old space */
        /* *target=poolMalloc((sourceLength + 1) * sizeof(nmbrString)); */
        *target=poolMalloc((sourceLength + 1) * sizeof(nmbrString) * 2);
                        /* Allocate new space --
                            We are replacing a smaller string with a larger one;
                            assume it is growing, and allocate twice as much as
                            needed. */
        /*if (!*target) outOfMemory("#107 (nmbrString)");*/ /*???Unnec. w/ poolMalloc*/
        nmbrCpy(*target,source);

        /* Memory pool handling */
        /* Assign actual size of target string */
        poolDiff = ((long *)(*target))[-1] - ((long *)source)[-1];
        ((long *)(*target))[-1] = ((long *)source)[-1];
        /* If actual size of target string is less than allocated size, we
           may have to add it to the used pool */
        /* (The 1st 'if' is redundant with target doubling above) */
        if (((long *)(*target))[-1] != ((long *)(*target))[-2]) {
          if (((long *)(*target))[-1] > ((long *)(*target))[-2]) bug(1326);
          if (((long *)(*target))[-3] == -1) {
            /* It's not already in the used pool, so add it */
            addToUsedPool(*target);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0b: pool %ld stat %ld\n",poolTotalFree,i1+j1);

      }

    } else {    /* source is 0 length, target is not */
      poolFree(*target);
      *target= NULL_NMBRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0c: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    }
  } else {
    if (sourceLength) { /* target is 0 length, source is not */
      *target=poolMalloc((sourceLength + 1) * sizeof(nmbrString));
                        /* Allocate new space */
      /* if (!*target) outOfMemory("#108 (nmbrString)"); */ /*???Unnec. w/ poolMalloc*/
      nmbrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    } else {    /* source and target are both 0 length */
      /* *target= NULL_NMBRSTRING; */ /* Redundant */
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  nmbrTempAlloc(0); /* Free up temporary strings used in expression computation*/

}



nmbrString *nmbrCat(nmbrString *string1,...) /* String concatenation */
#define M_MAX_CAT_ARGS 30
{
  va_list ap;   /* Declare list incrementer */
  nmbrString *arg[M_MAX_CAT_ARGS];        /* Array to store arguments */
  long argLength[M_MAX_CAT_ARGS];       /* Array to store argument lengths */
  int numArgs=1;        /* Define "last argument" */
  int i;
  long j;
  nmbrString *ptr;
  arg[0]=string1;       /* First argument */

  va_start(ap,string1); /* Begin the session */
  while ((arg[numArgs++]=va_arg(ap,nmbrString *)))
        /* User-provided argument list must terminate with NULL */
    if (numArgs>=M_MAX_CAT_ARGS-1) {
      printf("*** FATAL ERROR ***  Too many cat() arguments\n");
      bug(1369);
    }
  va_end(ap);           /* End var args session */

  numArgs--;    /* The last argument (0) is not a string */

  /* Find out the total string length needed */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    argLength[i]=nmbrLen(arg[i]);
    j=j+argLength[i];
  }
  /* Allocate the memory for it */
  ptr=nmbrTempAlloc(j+1);
  /* Move the strings into the newly allocated area */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    nmbrCpy(ptr+j,arg[i]);
    j=j+argLength[i];
  }
  return (ptr);

}



/* Find out the length of a nmbrString */
long nmbrLen(nmbrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return (((unsigned long)(((long *)s)[-1] - sizeof(nmbrString)))
              / sizeof(nmbrString));
}


/* Find out the allocated length of a nmbrString */
long nmbrAllocLen(nmbrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return (((unsigned long)(((long *)s)[-2] - sizeof(nmbrString)))
              / sizeof(nmbrString));
}

/* Set the actual size field in a nmbrString allocated with poolFixedMalloc() */
/* Use this if "zapping" a nmbrString element with -1 to reduce its length. */
/* Note that the nmbrString will not be moved to the "used pool", even if
   zapping its length results in free space; thus the free space will never
   get recovered unless done by the caller or poolFree is called.  (This is
   done on purpose so the caller can know what free space is left.) */
/* ???Note that nmbrZapLen's not moving string to used pool wastes potential
   space when called by the routines in this module.  Effect should be minor. */
void nmbrZapLen(nmbrString *s, long len) {
  if (((long *)s)[-3] != -1) {
    /* It's already in the used pool, so adjust free space tally */
    poolTotalFree = poolTotalFree + ((long *)s)[-1]
        - (len + 1) * sizeof(nmbrString);
  }
  ((long *)s)[-1] = (len + 1) * sizeof(nmbrString);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("l: pool %ld stat %ld\n",poolTotalFree,i1+j1);
}


/* Copy a string to another (pre-allocated) string */
/* Dangerous for general purpose use */
void nmbrCpy(nmbrString *s,nmbrString *t)
{
  long i;
  i = 0;
  while (t[i] != -1) { /* End of string -- nmbrRight depends on it!! */
    s[i] = t[i];
    i++;
  }
  s[i] = t[i]; /* End of string */
}


/* Copy a string to another (pre-allocated) string */
/* Like strncpy, only the 1st n characters are copied. */
/* Dangerous for general purpose use */
void nmbrNCpy(nmbrString *s,nmbrString *t,long n)
{
  long i;
  i = 0;
  while (t[i] != -1) { /* End of string -- nmbrSeg, nmbrMid depend on it!! */
    if (i >= n) break;
    s[i] = t[i];
    i++;
  }
  s[i] = t[i]; /* End of string */
}


/* Compare two strings */
/* Unlike strcmp, this returns a 1 if the strings are equal
   and 0 otherwise. */
/* Only the token is compared.  The whiteSpace string is
   ignored. */
int nmbrEq(nmbrString *s,nmbrString *t)
{
  long i;
  if (nmbrLen(s) != nmbrLen(t)) return 0; /* Speedup */
  for (i = 0; s[i] == t[i]; i++)
    if (s[i] == -1) /* End of string */
      return 1;
  return 0;
}


/* Extract sin from character position start to stop into sout */
nmbrString *nmbrSeg(nmbrString *sin,long start,long stop)
{
  nmbrString *sout;
  long len;
  if (start<1) start=1;
  if (stop<1) stop = 0;
  len=stop-start+1;
  if (len<0) len = 0;
  sout=nmbrTempAlloc(len+1);
  nmbrNCpy(sout,sin+start-1,len);
  sout[len] = *NULL_NMBRSTRING;
  return (sout);
}

/* Extract sin from character position start for length len */
nmbrString *nmbrMid(nmbrString *sin,long start,long len)
{
  nmbrString *sout;
  if (start<1) start=1;
  if (len<0) len = 0;
  sout=nmbrTempAlloc(len+1);
  nmbrNCpy(sout,sin+start-1,len);
  sout[len] = *NULL_NMBRSTRING;
  return (sout);
}

/* Extract leftmost n characters */
nmbrString *nmbrLeft(nmbrString *sin,long n)
{
  nmbrString *sout;
  if (n < 0) n = 0;
  sout=nmbrTempAlloc(n + 1);
  nmbrNCpy(sout, sin, n);
  sout[n] = *NULL_NMBRSTRING;
  return (sout);
}

/* Extract after character n */
nmbrString *nmbrRight(nmbrString *sin,long n)
{
  /*??? We could just return &sin[n-1], but this is safer for debugging. */
  nmbrString *sout;
  long i;
  if (n < 1) n = 1;
  i = nmbrLen(sin);
  if (n > i) return (NULL_NMBRSTRING);
  sout = nmbrTempAlloc(i - n + 2);
  nmbrCpy(sout, &sin[n - 1]);
  return (sout);
}


/* Allocate and return an "empty" string n "characters" long */
nmbrString *nmbrSpace(long n)
{
  nmbrString *sout;
  long j = 0;
  if (n < 0) bug(1327);
  sout = nmbrTempAlloc(n + 1);
  while (j < n) {
    /* Initialize all fields */
    sout[j] = 0;
    j++;
  }
  sout[j] = *NULL_NMBRSTRING; /* End of string */
  return (sout);
}

/* Search for string2 in string1 starting at start_position */
long nmbrInstr(long start_position,nmbrString *string1,
  nmbrString *string2)
{
   long ls1, ls2, i, j;
   if (start_position < 1) start_position = 1;
   ls1 = nmbrLen(string1);
   ls2 = nmbrLen(string2);
   for (i = start_position - 1; i <= ls1 - ls2; i++) {
     for (j = 0; j < ls2; j++) {
       if (string1[i+j] != string2[j])
         break;
     }
     if (j == ls2) return (i+1);
   }
   return (0);

}

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse nmbrInstr) */
long nmbrRevInstr(long start_position,nmbrString *string1,
    nmbrString *string2)
{
   long ls1, ls2;
   nmbrString *tmp = NULL_NMBRSTRING;
   ls1 = nmbrLen(string1);
   ls2 = nmbrLen(string2);
   if (start_position > ls1 - ls2 + 1) start_position = ls1 - ls2 + 2;
   if (start_position<1) return 0;
   while (!nmbrEq(string2, nmbrMid(string1, start_position, ls2))) {
     start_position--;
     nmbrLet(&tmp, NULL_NMBRSTRING);
              /* Clear nmbrString buffer to prevent overflow caused by "mid" */
     if (start_position < 1) return 0;
   }
   return (start_position);
}


/* Converts nmbrString to a vstring with one space between tokens */
vstring nmbrCvtMToVString(nmbrString *s)
{
  long i, j, outputLen, mstrLen;
  vstring tmpStr = "";
  vstring ptr;
  vstring ptr2;

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack; /* For let() stack cleanup */
  startTempAllocStack = tempAllocStackTop;

  mstrLen = nmbrLen(s);
  /* Precalculate output length */
  outputLen = -1;
  for (i = 0; i < mstrLen; i++) {
    outputLen = outputLen + strlen(mathToken[s[i]].tokenName) + 1;
  }
  let(&tmpStr, space(outputLen)); /* Preallocate output string */
  /* Assign output string */
  ptr = tmpStr;
  for (i = 0; i < mstrLen; i++) {
    ptr2 = mathToken[s[i]].tokenName;
    j = strlen(ptr2);
    memcpy(ptr, ptr2, j);
    ptr = ptr + j + 1;
  }

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deallocation */
  return (tmpStr);
}


/* Converts proof to a vstring with one space between tokens */
vstring nmbrCvtRToVString(nmbrString *proof)
{
  long i, j, plen, maxLabelLen, maxLocalLen, step, stmt;
  vstring proofStr = "";
  vstring tmpStr = "";
  vstring ptr;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  long nextLocLabNum = 1; /* Next number to be used for a local label */
  void *voidPtr; /* bsearch result */

  long saveTempAllocStack;
  long nmbrSaveTempAllocStack;
  saveTempAllocStack = startTempAllocStack; /* For let() stack cleanup */
  startTempAllocStack = tempAllocStackTop;
  nmbrSaveTempAllocStack = nmbrStartTempAllocStack; /*For nmbrLet() stack cleanup*/
  nmbrStartTempAllocStack = nmbrTempAllocStackTop;

  plen = nmbrLen(proof);

  /* Find longest local label name */
  maxLocalLen = 0;
  i = plen;
  while (i) {
    i = i / 10;
    maxLocalLen++;
  }

  /* Collect local labels */
  /* Also, find longest statement label name */
  maxLabelLen = 0;
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      if (!nmbrElementIn(1, localLabels, stmt)) {
        nmbrLet(&localLabels, nmbrAddElement(localLabels, stmt));
      }
    } else {
      if (stmt > 0) {
        if (strlen(statement[stmt].labelName) > maxLabelLen) {
          maxLabelLen = strlen(statement[stmt].labelName);
        }
      }
    }
  }

 /* localLabelNames[] holds an integer which, when converted to string,
    is the local label name. */
  nmbrLet(&localLabelNames, nmbrSpace(plen));

  /* Build the ASCII string */
  /* Preallocate the string for speed (the "2" accounts for a space and a
     colon). */
  let(&proofStr, space(plen * (2 + maxLabelLen + maxLocalLen)));
  ptr = proofStr;
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        stmt = -1000 - stmt;
        /* stmt is now the step number a local label refers to */
        let(&tmpStr, cat(str(localLabelNames[stmt]), " ", NULL));
      } else {
        let(&tmpStr, cat(chr(-stmt), " ", NULL));
      }
    } else {
      let(&tmpStr,"");
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        /* First, get a name for the local label, using the next integer that
           does not match any integer used for a statement label. */
        let(&tmpStr,str(nextLocLabNum));
        while (1) {
          voidPtr = (void *)bsearch(tmpStr,
              allLabelKeyBase, numAllLabelKeys,
              sizeof(long), labelSrchCmp);
          if (!voidPtr) break; /* It does not conflict */
          nextLocLabNum++; /* Try the next one */
          let(&tmpStr,str(nextLocLabNum));
        }
        localLabelNames[step] = nextLocLabNum;
        let(&tmpStr, cat(tmpStr, ":", NULL));
        nextLocLabNum++; /* Prepare for next local label */
      }
      let(&tmpStr, cat(tmpStr, statement[stmt].labelName, " ", NULL));
    }
    j = strlen(tmpStr);
    memcpy(ptr, tmpStr, j);
    ptr = ptr + j;
  } /* Next step */

  if (ptr - proofStr) {
    /* Deallocate large pool and trim trailing space */
    let(&proofStr, left(proofStr, ptr - proofStr - 1));
  } else {
    let(&proofStr, "");
  }
  let(&tmpStr, "");
  nmbrLet(&localLabels, NULL_NMBRSTRING);
  nmbrLet(&localLabelNames, NULL_NMBRSTRING);

  startTempAllocStack = saveTempAllocStack;
  nmbrStartTempAllocStack = nmbrSaveTempAllocStack;
  if (proofStr[0]) makeTempAlloc(proofStr); /* Flag it for deallocation */
  return (proofStr);
}


nmbrString *nmbrGetProofStepNumbs(nmbrString *reason)
{
  /* This function returns a nmbrString of length of reason with
     step numbers assigned to tokens which are steps, and 0 otherwise.
     The returned string is allocated; THE CALLER MUST DEALLOCATE IT. */
  nmbrString *stepNumbs = NULL_NMBRSTRING;
  long rlen, start, end, i, step;

  rlen = nmbrLen(reason);
  nmbrLet(&stepNumbs,nmbrSpace(rlen)); /* All stepNumbs[] are initialized
                                        to 0 by nmbrSpace() */
  if (!rlen) return (stepNumbs);
  if (reason[1] == -(long)'=') {
    /* The proof is in "internal" format, with "proveStatement = (...)" added */
    start = 2; /* 2, not 3, so empty proof '?' will be seen */
    if (rlen == 3) {
      end = rlen; /* Empty proof case */
    } else {
      end = rlen - 1; /* Trim off trailing ')' */
    }
  } else {
    start = 1;
    end = rlen;
  }
  step = 0;
  for (i = start; i < end; i++) {
    if (i == 0) {
      /* i = 0 must be handled separately to prevent a reference to
         a field outside of the nmbrString */
      step++;
      stepNumbs[0] = step;
      continue;
    }
    if (reason[i] < 0 && reason[i] != -(long)'?') continue;
    if (reason[i - 1] == -(long)'('
        || reason[i - 1] == -(long)'{'
        || reason[i - 1] == -(long)'=') {
      step++;
      stepNumbs[i] = step;
    }
  }
  return (stepNumbs);
}


/* Converts any nmbrString to an ASCII string of numbers
   -- used for debugging only. */
vstring nmbrCvtAnyToVString(nmbrString *s)
{
  long i;
  vstring tmpStr = "";

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack; /* For let() stack cleanup */
  startTempAllocStack = tempAllocStackTop;

  for (i = 1; i <= nmbrLen(s); i++) {
    let(&tmpStr,cat(tmpStr," ",str(s[i-1]),NULL));
  }

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deallocation */
  return (tmpStr);
}


/* Extract variables from a math token string */
nmbrString *nmbrExtractVars(nmbrString *m)
{
  long i,j,len;
  nmbrString *v;
  len = nmbrLen(m);
  v=nmbrTempAlloc(len+1); /* Pre-allocate maximum possible space */
  v[0] = *NULL_NMBRSTRING;
  j = 0; /* Length of output string */
  for (i = 0; i < len; i++) {
    /*if (m[i] < 0 || m[i] >= mathTokens) {*/
    /* Changed >= to > because tokenNum=mathTokens is used by mmveri.c for
       dummy token */
    if (m[i] < 0 || m[i] > mathTokens) bug(1328);
    if (mathToken[m[i]].tokenType == (char)var__) {
      if (!nmbrElementIn(1,v,m[i])) { /* Don't duplicate variable */
        v[j] = m[i];
        j++;
        v[j] = *NULL_NMBRSTRING; /* Add temp. end-of-string for getElementOf() */
      }
    }
  }
  nmbrZapLen(v, j); /* Zap mem pool fields */
/*E*/db2=db2-(len-nmbrLen(v))*sizeof(nmbrString);
  return v;
}


/* Determine if an element (after start) is in a nmbrString; return position
   if it is.  Like nmbrInstr(), but faster.  Warning:  start must NOT
   be greater than length, otherwise results are unpredictable!!  This
   is not checked in order to speed up search. */
long nmbrElementIn(long start, nmbrString *g, long element)
{
  long i = start - 1;
  while (g[i] != -1) { /* End of string */
    if (g[i] == element) return(i + 1);
    i++;
  }
  return(0);
}


/* Add a single number to end of a nmbrString - faster than nmbrCat */
nmbrString *nmbrAddElement(nmbrString *g, long element)
{
  long len;
  nmbrString *v;
  len = nmbrLen(g);
  v = nmbrTempAlloc(len + 2); /* Allow for end of string */
  nmbrCpy(v, g);
  v[len] = element;
  v[len + 1] = *NULL_NMBRSTRING; /* End of string */
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("bbg2: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return(v);
}


/* Get the set union of two math token strings (presumably
   variable lists) */
nmbrString *nmbrUnion(nmbrString *m1, nmbrString *m2)
{
  long i,j,len1,len2;
  nmbrString *v;
  len1 = nmbrLen(m1);
  len2 = nmbrLen(m2);
  v=nmbrTempAlloc(len1+len2+1); /* Pre-allocate maximum possible space */
  nmbrCpy(v,m1);
  nmbrZapLen(v, len1);
  j = 0;
  for (i = 0; i < len2; i++) {
    if (!nmbrElementIn(1, v, m2[i])) {
      nmbrZapLen(v, len1 + j + 1);
      v[len1 + j] = m2[i];
      j++;
      v[len1 + j] = *NULL_NMBRSTRING;
    }
  }
  v[len1 + j] = *NULL_NMBRSTRING;
  nmbrZapLen(v, len1 + j);
/*E*/db2=db2-(len1+len2-nmbrLen(v))*sizeof(nmbrString);
  return(v);
}


/* Get the set intersection of two math token strings (presumably
   variable lists) */
nmbrString *nmbrIntersection(nmbrString *m1,nmbrString *m2)
{
  long i,j,len2;
  nmbrString *v;
  len2 = nmbrLen(m2);
  v=nmbrTempAlloc(len2+1); /* Pre-allocate maximum possible space */
  j = 0;
  for (i = 0; i < len2; i++) {
    if (nmbrElementIn(1,m1,m2[i])) {
      v[j] = m2[i];
      j++;
    }
  }
  /* Add end-of-string */
  v[j] = *NULL_NMBRSTRING;
  nmbrZapLen(v, j);
/*E*/db2=db2-(len2-nmbrLen(v))*sizeof(nmbrString);
  return v;
}


/* Get the set difference m1-m2 of two math token strings (presumably
   variable lists) */
nmbrString *nmbrSetMinus(nmbrString *m1,nmbrString *m2)
{
  long i,j,len1;
  nmbrString *v;
  len1 = nmbrLen(m1);
  v=nmbrTempAlloc(len1+1); /* Pre-allocate maximum possible space */
  j = 0;
  for (i = 0; i < len1; i++) {
    if (!nmbrElementIn(1,m2,m1[i])) {
      v[j] = m1[i];
      j++;
    }
  }
  /* Add end-of-string */
  v[j] = *NULL_NMBRSTRING;
  nmbrZapLen(v, j);
/*E*/db2=db2-(len1-nmbrLen(v))*sizeof(nmbrString);
  return v;
}


/* This is a utility function that returns the length of a subproof that
   ends at step */
long nmbrGetSubProofLen(nmbrString *proof, long step)
{
  long stmt, hyps, pos, i;
  char type;

  if (step < 0) bug(1329);
  stmt = proof[step];
  if (stmt < 0) return (1); /* Unknown or label ref */
  type = statement[stmt].type;
  if (type == f__ || type == e__) return (1); /* Hypothesis */
  hyps = statement[stmt].numReqHyp;
  pos = step - 1;
  for (i = 0; i < hyps; i++) {
    pos = pos - nmbrGetSubProofLen(proof, pos);
  }
  return (step - pos);
}




/* This function returns a "squished" proof, putting in local label references
   to previous subproofs. */
nmbrString *nmbrSquishProof(nmbrString *proof)
{
  nmbrString *newProof = NULL_NMBRSTRING;
  nmbrString *dummyProof = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  long step, dummyStep, subProofLen, matchStep, plen;
  flag foundFlag;

  nmbrLet(&newProof,proof); /* In case of temp. alloc. of proof */
  plen = nmbrLen(newProof);
  dummyStep = 0;
  nmbrLet(&dummyProof, newProof); /* Parallel proof with test subproof replaced
                                 with a reference to itself, for matching. */
  for (step = 0; step < plen; step++) {
    subProofLen = nmbrGetSubProofLen(dummyProof, dummyStep);
    if (subProofLen <= 1) {
      dummyStep++;
      continue;
    }
    nmbrLet(&subProof, nmbrSeg(dummyProof, dummyStep - subProofLen + 2,
        dummyStep + 1));
    matchStep = step + 1;
    foundFlag = 0;
    while (1) {
      matchStep = nmbrInstr(matchStep + 1, newProof, subProof);
      if (!matchStep) break; /* No more occurrences */
      foundFlag = 1;
      /* Replace the found subproof with a reference to this subproof */
      nmbrLet(&newProof,
            nmbrCat(nmbrAddElement(nmbrLeft(newProof, matchStep - 1),
            -1000 - step), nmbrRight(newProof, matchStep + subProofLen), NULL));
      matchStep = matchStep - subProofLen + 1;
    }
    if (foundFlag) {
      plen = nmbrLen(newProof); /* Update the new proof length */
      /* Replace this subproof with a reference to itself, for later matching */
      /* and add on rest of real proof. */
      dummyStep = dummyStep + 1 - subProofLen;
      nmbrLet(&dummyProof,
          nmbrCat(nmbrAddElement(nmbrLeft(dummyProof, dummyStep),
          -1000 - step), nmbrRight(newProof, step + 2), NULL));
    }
    dummyStep++;
  } /* Next step */
  nmbrLet(&subProof, NULL_NMBRSTRING);
  nmbrLet(&dummyProof, NULL_NMBRSTRING);
  nmbrMakeTempAlloc(newProof); /* Flag it for deallocation */
  return (newProof);
}


/* This function unsquishes a "squished" proof, replacing local label references
   to previous subproofs by the subproofs themselves. */
nmbrString *nmbrUnsquishProof(nmbrString *proof)
{
  nmbrString *newProof = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  long step, plen, subProofLen, stmt;

  nmbrLet(&newProof, proof);
  plen = nmbrLen(newProof);
  for (step = plen - 1; step >= 0; step--) {
    stmt = newProof[step];
    if (stmt > -1000) continue;
    /* It's a local label reference */
    stmt = -1000 - stmt;
    subProofLen = nmbrGetSubProofLen(newProof, stmt);
    nmbrLet(&newProof, nmbrCat(nmbrLeft(newProof, step),
        nmbrSeg(newProof, stmt - subProofLen + 2, stmt + 1),
        nmbrRight(newProof, step + 2), NULL));
    step = step + subProofLen - 1;
  }
  nmbrLet(&subProof, NULL_NMBRSTRING);
  nmbrMakeTempAlloc(newProof); /* Flag it for deallocation */
  return (newProof);
}


/* This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
nmbrString *nmbrGetIndentation(nmbrString *proof,
  long startingLevel)
{
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString *indentationLevel = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  nmbrString *nmbrTmp = NULL_NMBRSTRING;

  plen = nmbrLen(proof);
  stmt = proof[plen - 1];
  nmbrLet(&indentationLevel, nmbrSpace(plen));
  indentationLevel[plen - 1] = startingLevel;
  if (stmt < 0) { /* A local label reference or unknown */
    if (plen != 1) bug(1330);
    nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
    return (indentationLevel);
  }
  type = statement[stmt].type;
  if (type == f__ || type == e__) { /* A hypothesis */
    if (plen != 1) bug(1331);
    nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
    return (indentationLevel);
  }
  /* An assertion */
  if (type != a__ && type != p__) bug(1332);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubProofLen(proof, pos);
    nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
    nmbrLet(&nmbrTmp, nmbrGetIndentation(subProof, startingLevel + 1));
    for (j = 0; j < splen; j++) {
      indentationLevel[j + pos - splen + 1] = nmbrTmp[j];
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (333);

  nmbrLet(&subProof,NULL_NMBRSTRING); /* Deallocate */
  nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Deallocate */
  nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
  return (indentationLevel);
}


/* This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
nmbrString *nmbrGetEssential(nmbrString *proof)
{
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  nmbrString *nmbrTmp = NULL_NMBRSTRING;
  nmbrString *nmbrTmpPtr2;

  plen = nmbrLen(proof);
  stmt = proof[plen - 1];
  nmbrLet(&essentialFlags, nmbrSpace(plen));
  essentialFlags[plen - 1] = 1;
  if (stmt < 0) { /* A local label reference or unknown */
    if (plen != 1) bug(1334);
    /* The only time it should get here is if the original proof has only one
       step, which would be an unknown step */
    if (stmt != -(long)'?' && stmt > -1000) bug(1335);
    nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
    return (essentialFlags);
  }
  type = statement[stmt].type;
  if (type == f__ || type == e__) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1336);
    nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
    return (essentialFlags);
  }
  /* An assertion */
  if (type != a__ && type != p__) bug(1337);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  nmbrTmpPtr2 = statement[stmt].reqHypList;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubProofLen(proof, pos);
    if (statement[nmbrTmpPtr2[hyps - i - 1]].type == e__) {
      nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
      nmbrLet(&nmbrTmp, nmbrGetEssential(subProof));
      for (j = 0; j < splen; j++) {
        essentialFlags[j + pos - splen + 1] = nmbrTmp[j];
      }
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (338);

  nmbrLet(&subProof,NULL_NMBRSTRING); /* Deallocate */
  nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Deallocate */
  nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
  return (essentialFlags);
}


/* This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.
   statemNum is the statement being proved. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
nmbrString *nmbrGetTargetHyp(nmbrString *proof, long statemNum)
{
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString *targetHyp = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  nmbrString *nmbrTmp = NULL_NMBRSTRING;

  plen = nmbrLen(proof);
  stmt = proof[plen - 1];
  nmbrLet(&targetHyp, nmbrSpace(plen));
  if (statemNum) { /* First (rather than recursive) call */
    targetHyp[plen - 1] = statemNum; /* Statement being proved */
  }
  if (stmt < 0) { /* A local label reference or unknown */
    if (plen != 1) bug(1339);
    /* The only time it should get here is if the original proof has only one
       step, which would be an unknown step */
    if (stmt != -(long)'?') bug(1340);
    nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
    return (targetHyp);
  }
  type = statement[stmt].type;
  if (type == f__ || type == e__) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1341);
    nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
    return (targetHyp);
  }
  /* An assertion */
  if (type != a__ && type != p__) bug(1342);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubProofLen(proof, pos);
    if (splen > 1) {
      nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
      nmbrLet(&nmbrTmp, nmbrGetTargetHyp(subProof,
          statement[stmt].reqHypList[hyps - i - 1]));
      for (j = 0; j < splen; j++) {
        targetHyp[j + pos - splen + 1] = nmbrTmp[j];
      }
    } else {
      /* A one-step subproof; don't bother with recursive call */
      targetHyp[pos] = statement[stmt].reqHypList[hyps - i - 1];
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (343);

  nmbrLet(&subProof,NULL_NMBRSTRING); /* Deallocate */
  nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Deallocate */
  nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
  return (targetHyp);
}


/* Converts a proof string to a compressed-proof-format ASCII string.
   Normally, the proof string would be compacted with nmbrSquishProof first,
   although it's not a requirement. */
/* The statement number is needed because required hypotheses are
   implicit in the compressed proof. */
vstring compressProof(nmbrString *proof, long statemNum)
{
  vstring output = "";
  long outputLen;
  long outputAllocated;
  nmbrString *saveProof = NULL_NMBRSTRING;
  nmbrString *labelList = NULL_NMBRSTRING;
  nmbrString *hypList = NULL_NMBRSTRING;
  nmbrString *assertionList = NULL_NMBRSTRING;
  nmbrString *localList = NULL_NMBRSTRING;
  nmbrString *localLabelFlags = NULL_NMBRSTRING;
  long hypLabels, assertionLabels, localLabels;
  long plen, step, stmt, labelLen, lab, numchrs;
  /* long thresh, newnumchrs, newlab; */ /* 15-Oct-05 nm No longer used */
  long i, j;
  /* flag breakFlag; */ /* 15-Oct-05 nm No longer used */
  /* char c; */ /* 15-Oct-05 nm No longer used */
  long lettersLen, digitsLen;
  static char *digits = "0123456789";
  static char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char labelChar = ':';

  /* Compression standard with all cap letters */
  /* (For 500-700 step proofs, we only lose about 18% of file size --
      but the compressed proof is more pleasing to the eye) */
  letters = "ABCDEFGHIJKLMNOPQRST"; /* LSB is base 20 */
  digits = "UVWXY"; /* MSB's are base 5 */
  labelChar = 'Z'; /* Was colon */

  lettersLen = strlen(letters);
  digitsLen = strlen(digits);

  nmbrLet(&saveProof, proof); /* In case of temp. alloc. of proof */

  if (statement[statemNum].type != (char)p__) bug(1344);
  plen = nmbrLen(saveProof);

  /* Create the initial label list of required hypotheses */
  nmbrLet(&labelList, statement[statemNum].reqHypList);

  /* Add the other statement labels to the list */

  /* Warning:  The exact union algorithm is crucial here; the required
     hypotheses MUST remain at the beginning of the list. */
  nmbrLet(&labelList, nmbrUnion(labelList, saveProof));

  /* Break the list into hypotheses, assertions, and local labels */
  labelLen = nmbrLen(labelList);
  nmbrLet(&hypList, nmbrSpace(labelLen));
  nmbrLet(&assertionList, nmbrSpace(labelLen));
  nmbrLet(&localLabelFlags, nmbrSpace(plen)); /* Warning: nmbrSpace() must
                                                 produce a string of 0's */
  hypLabels = 0;
  assertionLabels = 0;
  localLabels = 0;
  for (lab = 0; lab < labelLen; lab++) {
    stmt = labelList[lab];
    if (stmt < 0) {
      if (stmt <= -1000) {
        if (-1000 - stmt >= plen) bug (345);
        localLabelFlags[-1000 - stmt] = 1;
        localLabels++;
      } else {
        if (stmt != -(long)'?') bug(1346);
      }
    } else {
      if (statement[stmt].type != (char)a__ &&
          statement[stmt].type != (char)p__) {
        hypList[hypLabels] = stmt;
        hypLabels++;
      } else {
        assertionList[assertionLabels] = stmt;
        assertionLabels++;
      }
    }
  } /* Next lab */
  nmbrLet(&hypList, nmbrLeft(hypList, hypLabels));
  nmbrLet(&assertionList, nmbrLeft(assertionList, assertionLabels));

  /* Get list of local labels, sorted in order of declaration */
  nmbrLet(&localList, nmbrSpace(localLabels));
  lab = 0;
  for (step = 0; step < plen; step++) {
    if (localLabelFlags[step]) {
      localList[lab] = -1000 - step;
      lab++;
    }
  }
  if (lab != localLabels) bug(1347);

  /* Combine all label lists */
  nmbrLet(&labelList, nmbrCat(hypList, assertionList, localList, NULL));

  /* Create the compressed proof */
  outputLen = 0;
#define COMPR_INC 1000
  let(&output, space(COMPR_INC));
  outputAllocated = COMPR_INC;

  plen = nmbrLen(saveProof);
  for (step = 0; step < plen; step++) {

    stmt = saveProof[step];

    if (stmt == -(long)'?') {
      /* Unknown step */
      if (outputLen + 1 > outputAllocated) {
        /* Increase allocation of the output string */
        let(&output, cat(output, space(outputLen + 1 - outputAllocated +
            COMPR_INC), NULL));
        outputAllocated = outputLen + 1 + COMPR_INC; /* = strlen(output) */
        /* CPU-intensive bug check; enable only if required: */
        /* if (outputAllocated != strlen(output)) bug(1348); */
        if (output[outputAllocated - 1] == 0 ||
            output[outputAllocated] != 0) bug(1348); /* 13-Oct-05 nm */
      }
      output[outputLen] = '?';
      outputLen++;
      continue;
    }

    lab = nmbrElementIn(1, labelList, stmt);
    if (!lab) bug(1349);
    lab--; /* labelList array starts at 0, not 1 */

    /* Determine the # of chars in the compressed label */
    /* 15-Oct-05 nm - Obsolete (skips from YT to UVA, missing UUA) */
    /*
    numchrs = 1;
    if (lab > lettersLen - 1) {
      / * It requires a numeric prefix * /
      i = lab / lettersLen;
      while(i) {
        numchrs++;
        if (i > digitsLen) {
          i = i / digitsLen;
        } else {
          i = 0; / * MSB is sort of 'mod digitsLen+1' since
                                a blank is the MSB in the case of one
                                fewer characters in the label * /
        }
      }
    }
    */

    /* 15-Oct-05 nm - A corrected algorithm was provided by Marnix Klooster. */
    /* For encoding we'd get (starting with n, counting from 1):
        * start with the empty string
        * prepend (n-1) mod 20 + 1 as character using 1->'A' .. 20->'T'
        * n := (n-1) div 20
        * while n > 0:
           * prepend (n-1) mod 5 + 1 as character using 1->'U' .. 5->'Y'
           * n := (n-1) div 5 */
    if (lab < 0) bug(1373);
    numchrs = 1;
    i = lab / lettersLen;
    while (1) {
      if (!i) break;
      numchrs++;
      i = (i - 1) / digitsLen;
    }

    /* Add the compressed label to the proof */
    if (outputLen + numchrs > outputAllocated) {
      /* Increase allocation of the output string */
      let(&output, cat(output, space(outputLen + numchrs - outputAllocated +
          COMPR_INC), NULL));
      outputAllocated = outputLen + numchrs + COMPR_INC; /* = strlen(output) */
      /* CPU-intensive bug check; enable only if required: */
      /* if (outputAllocated != strlen(output)) bug(1350); */
      if (output[outputAllocated - 1] == 0 ||
          output[outputAllocated] != 0) bug(1350); /* 13-Oct-05 nm */
    }
    outputLen = outputLen + numchrs;

    /* 15-Oct-05 nm - Obsolete (skips from YT to UVA, missing UUA) */
    /*
    j = lab;
    for (i = 0; i < numchrs; i++) { / * Create from LSB to MSB * /
      if (!i) {
        c = letters[j % lettersLen];
        j = j / lettersLen;
      } else {
        if (j > digitsLen) {
          c = digits[j % digitsLen];
          j = j / digitsLen;
        } else {
          c = digits[j - 1]; / * MSB is sort of 'mod digitsLen+1' since
                                a blank is the MSB in the case of one
                                fewer characters in the label * /
        }
      }
      output[outputLen - i - 1] = c;
    } / * Next i * /
    */

    /* 15-Oct-05 nm - A corrected algorithm was provided by Marnix Klooster. */
    /* For encoding we'd get (starting with n, counting from 1):
        * start with the empty string
        * prepend (n-1) mod 20 + 1 as character using 1->'A' .. 20->'T'
        * n := (n-1) div 20
        * while n > 0:
           * prepend (n-1) mod 5 + 1 as character using 1->'U' .. 5->'Y'
           * n := (n-1) div 5 */
    j = lab + 1; /* lab starts at 0, not 1 */
    i = 1;
    output[outputLen - i] = letters[(j - 1) % lettersLen];
    j = (j - 1) / lettersLen;
    while (1) {
      if (!j) break;
      i++;
      output[outputLen - i] = digits[(j - 1) % digitsLen];
      j = (j - 1) / digitsLen;
    }
    if (i != numchrs) bug(1374);


    /***** Local labels ******/
    /* See if a local label is declared in this step */
    if (!localLabelFlags[step]) continue;
    if (outputLen + 1 > outputAllocated) {
      /* Increase allocation of the output string */
      let(&output, cat(output, space(outputLen + 1 - outputAllocated +
          COMPR_INC), NULL));
      outputAllocated = outputLen + 1 + COMPR_INC; /* = strlen(output) */
      /* CPU-intensive bug check; enable only if required: */
      /* if (outputAllocated != strlen(output)) bug(1352); */
      if (output[outputAllocated - 1] == 0 ||
          output[outputAllocated] != 0) bug(1352); /* 13-Oct-05 nm */
    }
    output[outputLen] = labelChar;
    outputLen++;

  } /* Next step */

  /* Create the final compressed proof */
  let(&output, cat("( ", nmbrCvtRToVString(nmbrCat(
      /* Trim off leading implicit required hypotheses */
      nmbrRight(hypList, statement[statemNum].numReqHyp + 1),
      assertionList, NULL)),
      " ) ", left(output, outputLen), NULL));

  nmbrLet(&saveProof, NULL_NMBRSTRING);
  nmbrLet(&labelList, NULL_NMBRSTRING);
  nmbrLet(&hypList, NULL_NMBRSTRING);
  nmbrLet(&assertionList, NULL_NMBRSTRING);
  nmbrLet(&localList, NULL_NMBRSTRING);
  nmbrLet(&localLabelFlags, NULL_NMBRSTRING);
  makeTempAlloc(output); /* Flag it for deallocation */
  return(output);
}





/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

int pntrTempAllocStackTop = 0;     /* Top of stack for pntrTempAlloc functon */
int pntrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when pntrLet() is called (normally 0, except in
                                    special nested vstring functions) */
pntrString *pntrTempAllocStack[M_MAX_ALLOC_STACK];


pntrString *pntrTempAlloc(long size)
                                /* pntrString memory allocation/deallocation */
{
  /* When "size" is >0, "size" instances of pntrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to pntrStartTempAllocStack. */
  int i;
  if (size) {
    if (pntrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1))
      /*??? Fix to allocate more */
      outOfMemory("#109 (pntrString stack array)");
    if (!(pntrTempAllocStack[pntrTempAllocStackTop++]=poolMalloc(size
        *sizeof(pntrString))))
      /* outOfMemory("#110 (pntrString stack)"); */ /*???Unnec. w/ poolMalloc*/
/*E*/db2=db2+(size)*sizeof(pntrString);
    return (pntrTempAllocStack[pntrTempAllocStackTop-1]);
  } else {
    for (i=pntrStartTempAllocStack; i < pntrTempAllocStackTop; i++) {
/*E*/db2=db2-(pntrLen(pntrTempAllocStack[i])+1)*sizeof(pntrString);
      poolFree(pntrTempAllocStack[i]);
    }
    pntrTempAllocStackTop=pntrStartTempAllocStack;
    return (0);
  }
}


/* Make string have temporary allocation to be released by next pntrLet() */
/* Warning:  after pntrMakeTempAlloc() is called, the pntrString may NOT be
   assigned again with pntrLet() */
void pntrMakeTempAlloc(pntrString *s)
{
    if (pntrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
      printf(
      "*** FATAL ERROR ***  Temporary pntrString stack overflow in pntrMakeTempAlloc()\n");
      bug(1370);
    }
    if (s[0] != NULL) { /* Don't do it if pntrString is empty */
      pntrTempAllocStack[pntrTempAllocStackTop++] = s;
    }
/*E*/db2=db2+(pntrLen(s)+1)*sizeof(pntrString);
/*E*/db3=db3-(pntrLen(s)+1)*sizeof(pntrString);
}


void pntrLet(pntrString **target,pntrString *source)
/* pntrString assignment */
/* This function must ALWAYS be called to make assignment to */
/* a pntrString in order for the memory cleanup routines, etc. */
/* to work properly.  If a pntrString has never been assigned before, */
/* it is the user's responsibility to initialize it to NULL_PNTRSTRING (the */
/* null string). */
{
  long targetLength,sourceLength;
  long targetAllocLen;
  long poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  sourceLength=pntrLen(source);  /* Save its actual length */
  targetLength=pntrLen(*target);  /* Save its actual length */
  targetAllocLen=pntrAllocLen(*target); /* Save target's allocated length */
/*E*/if (targetLength) {
/*E*/  /* printf("Deleting %s\n",cvtMToVString(*target,0)); */
/*E*/  db3 = db3 - (targetLength+1)*sizeof(pntrString);
/*E*/}
/*E*/if (sourceLength) {
/*E*/  /* printf("Adding %s\n",cvtMToVString(source,0)); */
/*E*/  db3 = db3 + (sourceLength+1)*sizeof(pntrString);
/*E*/}
  if (targetAllocLen) {
    if (sourceLength) { /* source and target are both nonzero length */

      if (targetAllocLen >= sourceLength) { /* Old string has room for new one */
        pntrCpy(*target,source); /* Re-use the old space to save CPU time */

        /* Memory pool handling */
        /* Assign actual size of target string */
        poolDiff = ((long *)(*target))[-1] - ((long *)source)[-1];
        ((long *)(*target))[-1] = ((long *)source)[-1];
        /* If actual size of target string is less than allocated size, we
           may have to add it to the used pool */
        if (((long *)(*target))[-1] != ((long *)(*target))[-2]) {
          if (((long *)(*target))[-1] > ((long *)(*target))[-2]) bug(1359);
          if (((long *)(*target))[-3] == -1) {
            /* It's not already in the used pool, so add it */
            addToUsedPool(*target);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0aa: pool %ld stat %ld\n",poolTotalFree,i1+j1);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0ab: pool %ld stat %ld\n",poolTotalFree,i1+j1);
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }


/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0a: pool %ld stat %ld\n",poolTotalFree,i1+j1);
      } else {
        /* Free old string space and allocate new space */
        poolFree(*target);  /* Free old space */
        /* *target=poolMalloc((sourceLength + 1) * sizeof(pntrString)); */
        *target=poolMalloc((sourceLength + 1) * sizeof(pntrString) * 2);
                        /* Allocate new space --
                            We are replacing a smaller string with a larger one;
                            assume it is growing, and allocate twice as much as
                            needed. */
        /*if (!*target) outOfMemory("#111 (pntrString)");*/ /*???Unnec. w/ poolMalloc*/
        pntrCpy(*target,source);

        /* Memory pool handling */
        /* Assign actual size of target string */
        poolDiff = ((long *)(*target))[-1] - ((long *)source)[-1];
        ((long *)(*target))[-1] = ((long *)source)[-1];
        /* If actual size of target string is less than allocated size, we
           may have to add it to the used pool */
        /* (The 1st 'if' is redundant with target doubling above) */
        if (((long *)(*target))[-1] != ((long *)(*target))[-2]) {
          if (((long *)(*target))[-1] > ((long *)(*target))[-2]) bug(1360);
          if (((long *)(*target))[-3] == -1) {
            /* It's not already in the used pool, so add it */
            addToUsedPool(*target);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0b: pool %ld stat %ld\n",poolTotalFree,i1+j1);

      }

    } else {    /* source is 0 length, target is not */
      poolFree(*target);
      *target= NULL_PNTRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0c: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    }
  } else {
    if (sourceLength) { /* target is 0 length, source is not */
      *target=poolMalloc((sourceLength + 1) * sizeof(pntrString));
                        /* Allocate new space */
      /* if (!*target) outOfMemory("#112 (pntrString)"); */ /*???Unnec. w/ poolMalloc*/
      pntrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    } else {    /* source and target are both 0 length */
      /* *target= NULL_PNTRSTRING; */ /* Redundant */
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  pntrTempAlloc(0); /* Free up temporary strings used in expression computation*/

}



pntrString *pntrCat(pntrString *string1,...) /* String concatenation */
{
  va_list ap;   /* Declare list incrementer */
  pntrString *arg[M_MAX_CAT_ARGS];        /* Array to store arguments */
  long argLength[M_MAX_CAT_ARGS];       /* Array to store argument lengths */
  int numArgs=1;        /* Define "last argument" */
  int i;
  long j;
  pntrString *ptr;
  arg[0]=string1;       /* First argument */

  va_start(ap,string1); /* Begin the session */
  while ((arg[numArgs++]=va_arg(ap,pntrString *)))
        /* User-provided argument list must terminate with NULL */
    if (numArgs>=M_MAX_CAT_ARGS-1) {
      printf("*** FATAL ERROR ***  Too many cat() arguments\n");
      bug(1371);
    }
  va_end(ap);           /* End var args session */

  numArgs--;    /* The last argument (0) is not a string */

  /* Find out the total string length needed */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    argLength[i]=pntrLen(arg[i]);
    j=j+argLength[i];
  }
  /* Allocate the memory for it */
  ptr=pntrTempAlloc(j+1);
  /* Move the strings into the newly allocated area */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    pntrCpy(ptr+j,arg[i]);
    j=j+argLength[i];
  }
  return (ptr);

}



/* Find out the length of a pntrString */
long pntrLen(pntrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return ((((long *)s)[-1] - sizeof(pntrString)) / sizeof(pntrString));
}


/* Find out the allocated length of a pntrString */
long pntrAllocLen(pntrString *s)
{
  return ((((long *)s)[-2] - sizeof(pntrString)) / sizeof(pntrString));
}

/* Set the actual size field in a pntrString allocated with poolFixedMalloc() */
/* Use this if "zapping" a pntrString element with -1 to reduce its length. */
/* Note that the pntrString will not be moved to the "used pool", even if
   zapping its length results in free space; thus the free space will never
   get recovered unless done by the caller or poolFree is called.  (This is
   done on purpose so the caller can know what free space is left.) */
/* ???Note that pntrZapLen's not moving string to used pool wastes potential
   space when called by the routines in this module.  Effect should be minor. */
void pntrZapLen(pntrString *s, long len) {
  if (((long *)s)[-3] != -1) {
    /* It's already in the used pool, so adjust free space tally */
    poolTotalFree = poolTotalFree + ((long *)s)[-1]
        - (len + 1) * sizeof(pntrString);
  }
  ((long *)s)[-1] = (len + 1) * sizeof(pntrString);
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("l: pool %ld stat %ld\n",poolTotalFree,i1+j1);
}


/* Copy a string to another (pre-allocated) string */
/* Dangerous for general purpose use */
void pntrCpy(pntrString *s, pntrString *t)
{
  long i;
  i = 0;
  while (t[i] != NULL) { /* End of string -- pntrRight depends on it!! */
    s[i] = t[i];
    i++;
  }
  s[i] = t[i]; /* End of string */
}


/* Copy a string to another (pre-allocated) string */
/* Like strncpy, only the 1st n characters are copied. */
/* Dangerous for general purpose use */
void pntrNCpy(pntrString *s,pntrString *t,long n)
{
  long i;
  i = 0;
  while (t[i] != NULL) { /* End of string -- pntrSeg, pntrMid depend on it!! */
    if (i >= n) break;
    s[i] = t[i];
    i++;
  }
  s[i] = t[i]; /* End of string */
}


/* Compare two strings */
/* Unlike strcmp, this returns a 1 if the strings are equal
   and 0 otherwise. */
/* Only the pointers are compared.  If pointers are different,
   0 will be returned, even if the things pointed to have same contents. */
int pntrEq(pntrString *s,pntrString *t)
{
  long i;
  for (i = 0; s[i] == t[i]; i++)
    if (s[i] == NULL) /* End of string */
      return 1;
  return 0;
}


/* Extract sin from character position start to stop into sout */
pntrString *pntrSeg(pntrString *sin,long start,long stop)
{
  pntrString *sout;
  long len;
  if (start<1) start=1;
  if (stop<1) stop = 0;
  len=stop-start+1;
  if (len<0) len = 0;
  sout=pntrTempAlloc(len+1);
  pntrNCpy(sout,sin+start-1,len);
  sout[len] = *NULL_PNTRSTRING;
  return (sout);
}

/* Extract sin from character position start for length len */
pntrString *pntrMid(pntrString *sin,long start,long len)
{
  pntrString *sout;
  if (start<1) start=1;
  if (len<0) len = 0;
  sout=pntrTempAlloc(len+1);
  pntrNCpy(sout,sin+start-1,len);
  sout[len] = *NULL_PNTRSTRING;
  return (sout);
}

/* Extract leftmost n characters */
pntrString *pntrLeft(pntrString *sin,long n)
{
  pntrString *sout;
  if (n < 0) n = 0;
  sout=pntrTempAlloc(n+1);
  pntrNCpy(sout,sin,n);
  sout[n] = *NULL_PNTRSTRING;
  return (sout);
}

/* Extract after character n */
pntrString *pntrRight(pntrString *sin,long n)
{
  /*??? We could just return &sin[n-1], but this is safer for debugging. */
  pntrString *sout;
  long i;
  if (n < 1) n = 1;
  i = pntrLen(sin);
  if (n > i) return (NULL_PNTRSTRING);
  sout = pntrTempAlloc(i - n + 2);
  pntrCpy(sout, &sin[n-1]);
  return (sout);
}


/* Allocate and return an "empty" string n "characters" long */
pntrString *pntrSpace(long n)
{
  pntrString *sout;
  long j = 0;
  if (n<0) bug(1360);
  sout=pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = "";
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return (sout);
}

/* Allocate and return an "empty" string n "characters" long
   initialized to nmbrStrings instead of vStrings */
pntrString *pntrNSpace(long n)
{
  pntrString *sout;
  long j = 0;
  if (n<0) bug(1361);
  sout=pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = NULL_NMBRSTRING;
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return (sout);
}

/* Allocate and return an "empty" string n "characters" long
   initialized to pntrStrings instead of vStrings */
pntrString *pntrPSpace(long n)
{
  pntrString *sout;
  long j = 0;
  if (n<0) bug(1372);
  sout=pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = NULL_PNTRSTRING;
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return (sout);
}

/* Search for string2 in string1 starting at start_position */
long pntrInstr(long start_position,pntrString *string1,
  pntrString *string2)
{
   long ls1,ls2,i,j;
   if (start_position<1) start_position=1;
   ls1=pntrLen(string1);
   ls2=pntrLen(string2);
   for (i=start_position - 1; i <= ls1 - ls2; i++) {
     for (j = 0; j<ls2; j++) {
       if (string1[i+j] != string2[j])
         break;
     }
     if (j == ls2) return (i+1);
   }
   return (0);

}

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse pntrInstr) */
long pntrRevInstr(long start_position,pntrString *string1,
    pntrString *string2)
{
   long ls1,ls2;
   pntrString *tmp = NULL_PNTRSTRING;
   ls1=pntrLen(string1);
   ls2=pntrLen(string2);
   if (start_position>ls1-ls2+1) start_position=ls1-ls2+2;
   if (start_position<1) return 0;
   while (!pntrEq(string2,pntrMid(string1,start_position,ls2))) {
     start_position--;
     pntrLet(&tmp,NULL_PNTRSTRING);
                /* Clear pntrString buffer to prevent overflow caused by "mid" */
     if (start_position < 1) return 0;
   }
   return (start_position);
}


/* Add a single null string element to a pntrString - faster than pntrCat */
pntrString *pntrAddElement(pntrString *g)
{
  long len;
  pntrString *v;
  len = pntrLen(g);
  v=pntrTempAlloc(len+2);
  pntrCpy(v,g);
  v[len] = "";
  v[len + 1] = *NULL_PNTRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1,&k1); if(db9)print2("bbg3: pool %ld stat %ld\n",poolTotalFree,i1+j1);
  return(v);
}


/* Add a single null pntrString element to a pntrString - faster than pntrCat */
pntrString *pntrAddGElement(pntrString *g)
{
  long len;
  pntrString *v;
  len = pntrLen(g);
  v=pntrTempAlloc(len+2);
  pntrCpy(v,g);
  v[len] = NULL_PNTRSTRING;
  v[len + 1] = *NULL_PNTRSTRING;
  return(v);
}
