/*****************************************************************************/
/*        Copyright (C) 2018  NORMAN MEGILL  nm at alum.mit.edu              */
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
#include "mmpfas.h" /* Needed for proveStatement */

#include <limits.h>
#include <setjmp.h>
/*E*/long db=0,db0=0,db2=0,db3=0,db4=0,db5=0,db6=0,db7=0,db8=0,db9=0;
flag listMode = 0; /* 0 = metamath, 1 = list utility */
flag toolsMode = 0; /* In metamath: 0 = metamath, 1 = text tools utility */


/* 4-May-2015 nm */
/* For use by getMarkupFlag() */
vstring proofDiscouragedMarkup = "";
vstring usageDiscouragedMarkup = "";
flag globalDiscouragement = 1; /* SET DISCOURAGEMENT ON */

/* 14-May-2017 nm */
vstring contributorName = "";

/* Global variables related to current statement */
int currentScope = 0;
long beginScopeStatementNum = 0;

long MAX_STATEMENTS = 1;
long MAX_MATHTOKENS = 1;
long MAX_INCLUDECALLS = 2; /* Must be at least 2 (the single-file case) !!!
                         (A dummy extra top entry is used by parseKeywords().) */
struct statement_struct *statement = NULL;
long *labelKey = NULL; /* 4-May-2017 Ari Ferrera - added "= NULL" */
struct mathToken_struct *mathToken;
long *mathKey = NULL;
long statements = 0, labels = 0, mathTokens = 0;
long maxMathTokenLength = 0;

struct includeCall_struct *includeCall = NULL; /* 4-May-2017 Ari Ferrera
                                                            - added "= NULL" */
long includeCalls = -1;  /* For eraseSouce() in mmcmds.c */

char *sourcePtr = NULL; /* 4-May-2017 Ari Ferrera - added "= NULL" */
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

vstring qsortKey; /* Used by qsortStringCmp; pointer only, do not deallocate */


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
/*E*/long i1,j1_,k1; /* 11-Sep-2009 nm Fix "built-in function 'j1'" warning */
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
/*E*/ /* 11-Jul-2014 nm Don't call print2() if db9 is set, since it will */
/*E*/ /* recursively call the pool stuff causing a crash.  I changed */
/*E*/ /* 41 cases of print2() to printf() below to resolve this. */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("a0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  if (!memFreePoolSize) { /* The pool is empty; we must allocate memory */
    ptr = malloc( 3 * sizeof(long) + (size_t)size);
    if (!ptr) outOfMemory(
        cat("#25 (poolFixedMalloc ", str((double)size), ")", NULL));

    ptr = (long *)ptr + 3;
    ((long *)ptr)[-1] = size; /* Actual size */
    ((long *)ptr)[-2] = size; /* Allocated size */
    ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
    return (ptr);
  } else {
    memFreePoolSize--;
    ptr = memFreePool[memFreePoolSize];
    poolTotalFree = poolTotalFree - ((long *)ptr)[-2];
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("a: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    if (size <= ((long *)ptr)[-2]) { /* We have enough space already */
      ptr2 = realloc( (long *)ptr - 3, 3 * sizeof(long) + (size_t)size);
      /* Reallocation cannot fail, since we are shrinking space */
      if (!ptr2) bug(1382);
      ptr = ptr2;
    } else { /* The pool's last entry is too small; free and allocate new */
      free((long *)ptr - 3);
      ptr = malloc( 3 * sizeof(long) + (size_t)size);
    }
    if (!ptr) {
      /* Try freeing space */
      print2("Memory is low.  Deallocating storage pool...\n");
      memFreePoolPurge(0);
      ptr = malloc( 3 * sizeof(long) + (size_t)size);
      if (!ptr) outOfMemory(
          cat("#26 (poolMalloc ", str((double)size), ")", NULL));
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

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("b0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  if (!memFreePoolSize) { /* The pool is empty; we must allocate memory */
    ptr = malloc( 3 * sizeof(long) + (size_t)size);
    if (!ptr) {
      outOfMemory(cat("#27 (poolMalloc ", str((double)size), ")", NULL));
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("b: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    if (size <= ((long *)ptr)[-2]) { /* We have enough space already */
      ((long *)ptr)[-1] = size; /* Actual size */
      ((long *)ptr)[-3] = -1; /* Not in storage pool yet */
    } else { /* We must reallocate */
      free((long *)ptr - 3);
      ptr = malloc( 3 * sizeof(long) + (size_t)size);
      if (!ptr) {
        /* Try freeing space */
        print2("Memory is low.  Deallocating storage pool...\n");
        memFreePoolPurge(0);
        ptr = malloc( 3 * sizeof(long) + (size_t)size);
        if (!ptr) outOfMemory(
            cat("#28 (poolMalloc ", str((double)size), ")", NULL));
                                              /* Nothing more can be done */
      }
      ptr = (long *)ptr + 3;
      ((long *)ptr)[-1] = size; /* Actual size */
      ((long *)ptr)[-2] = size; /* Allocated size */
      ((long *)ptr)[-3] = -1;  /* Location in memUsedPool (-1 = none) */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bb: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
      return (ptr);
    }
  }
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bc: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) return (ptr);
  /* Allocated and actual sizes are different, so add this array to used pool */
  poolTotalFree = poolTotalFree + ((long *)ptr)[-2] - ((long *)ptr)[-1];
  if (memUsedPoolSize >= memUsedPoolMax) { /* Increase size of used pool */
    memUsedPoolTmpMax = memUsedPoolMax + MEM_POOL_GROW;
/*E*/if(db9)printf("Growing used pool to %ld\n",memUsedPoolTmpMax);
    if (!memUsedPoolMax) {
      /* The program has just started; initialize */
      memUsedPoolTmpPtr = malloc((size_t)memUsedPoolTmpMax
          * sizeof(void *));
      if (!memUsedPoolTmpPtr) bug(1303); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memUsedPoolTmpPtr = realloc(memUsedPool,
          (size_t)memUsedPoolTmpMax * sizeof(void *));
    }
    if (!memUsedPoolTmpPtr) {
      outOfMemory(cat("#29 (poolMalloc ", str((double)memUsedPoolTmpMax), ")", NULL));
    } else {
      /* Reallocation successful */
      memUsedPool = memUsedPoolTmpPtr;
      memUsedPoolMax = memUsedPoolTmpMax;
    }
  }
  memUsedPool[memUsedPoolSize] = ptr;
  ((long *)ptr)[-3] = memUsedPoolSize;
  memUsedPoolSize++;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("c: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return (ptr);
}

/* poolFree puts freed up space in memFreePool. */
void poolFree(void *ptr)
{
  void *ptr1;
  long usedLoc;
  long memFreePoolTmpMax;
  void *memFreePoolTmpPtr;

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("c0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  /* First, see if the array is in memUsedPool; if so, remove it. */
  usedLoc = ((long *)ptr)[-3];
  if (usedLoc >= 0) { /* It is */
    poolTotalFree = poolTotalFree - ((long *)ptr)[-2] + ((long *)ptr)[-1];
    memUsedPoolSize--;

    /* 11-Jul-2014 WL old code deleted */
    /*
    memUsedPool[usedLoc] = memUsedPool[memUsedPoolSize];
    ptr1 = memUsedPool[usedLoc];
    ((long @)ptr1)[-3] = usedLoc;
/@E@/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    */
    /* 11-Jul-2014 WL new code */
    if (usedLoc < memUsedPoolSize) {
      memUsedPool[usedLoc] = memUsedPool[memUsedPoolSize];
      ptr1 = memUsedPool[usedLoc];
      ((long *)ptr1)[-3] = usedLoc;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
    /* end of 11-Jul-2014 WL new code */
  }

  /* Next, add the array to the memFreePool */
  /* First, allocate more memFreePool pointer space if needed */
  if (memFreePoolSize >= memFreePoolMax) { /* Increase size of free pool */
    memFreePoolTmpMax = memFreePoolMax + MEM_POOL_GROW;
/*E*/if(db9)printf("Growing free pool to %ld\n",memFreePoolTmpMax);
    if (!memFreePoolMax) {
      /* The program has just started; initialize */
      memFreePoolTmpPtr = malloc((size_t)memFreePoolTmpMax
          * sizeof(void *));
      if (!memFreePoolTmpPtr) bug(1304); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memFreePoolTmpPtr = realloc(memFreePool,
          (size_t)memFreePoolTmpMax * sizeof(void *));
    }
    if (!memFreePoolTmpPtr) {
/*E*/if(db9)printf("Realloc failed\n");
      outOfMemory(cat("#30 (poolFree ", str((double)memFreePoolTmpMax), ")", NULL));
    } else {
      /* Reallocation successful */
      memFreePool = memFreePoolTmpPtr;
      memFreePoolMax = memFreePoolTmpMax;
    }
  }
  /* Add the free array to the free pool */
  memFreePool[memFreePoolSize] = ptr;
  /* In theory, [-3] should never get referenced for an entry in the
     memFreePool. However, here we make it a definite (illegal) value in
     case it is referenced by code with a bug. */
  ((long *)ptr)[-3] = -2;  /* 11-Jul-2014 WL */
  memFreePoolSize++;
  poolTotalFree = poolTotalFree + ((long *)ptr)[-2];
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("e: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return;
}


/* addToUsedPool adds a (partially used) array to the memUsedPool */
void addToUsedPool(void *ptr)
{
  long memUsedPoolTmpMax;
  void *memUsedPoolTmpPtr;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("d0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) bug(1305); /* No need to add it
                                 when it's not partially used */
  if (((long *)ptr)[-1] == ((long *)ptr)[-2]) return;
  /* Allocated and actual sizes are different, so add this array to used pool */
  if (memUsedPoolSize >= memUsedPoolMax) { /* Increase size of used pool */
    memUsedPoolTmpMax = memUsedPoolMax + MEM_POOL_GROW;
/*E*/if(db9)printf("1Growing used pool to %ld\n",memUsedPoolTmpMax);
    if (!memUsedPoolMax) {
      /* The program has just started; initialize */
      memUsedPoolTmpPtr = malloc((size_t)memUsedPoolTmpMax
          * sizeof(void *));
      if (!memUsedPoolTmpPtr) bug(1362); /* Shouldn't have allocation problems
                                                    when program first starts */
    } else {
      /* Normal reallocation */
      memUsedPoolTmpPtr = realloc(memUsedPool, (size_t)memUsedPoolTmpMax
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("f: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return;
}

/* Free all arrays in the free pool. */
void memFreePoolPurge(flag untilOK)
{
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("e0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("g: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
  statement = malloc((size_t)MAX_STATEMENTS * sizeof(struct statement_struct));
/*E*//*db=db+MAX_STATEMENTS * sizeof(struct statement_struct);*/
  if (!statement) {
    print2("*** FATAL ***  Could not allocate statement space\n");
    bug(1363);
    }
  mathToken = malloc((size_t)MAX_MATHTOKENS * sizeof(struct mathToken_struct));
/*E*//*db=db+MAX_MATHTOKENS * sizeof(struct mathToken_struct);*/
  if (!mathToken) {
    print2("*** FATAL ***  Could not allocate mathToken space\n");
    bug(1364);
    }
  includeCall = malloc((size_t)MAX_INCLUDECALLS * sizeof(struct includeCall_struct));
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
  while (i < j - 2) {
    k = (i + j) / 2;
    s = malloc((size_t)k);
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
  /* let(&tmpStr, ""); */
  let(&tmpStr, left(tmpStr, 0)); /* Prevent "not used" compiler warning */
  /* Close the log to make sure error log is saved */
  if (logFileOpenFlag) {
    fclose(logFilePtr);
    logFileOpenFlag = 0;
  }

  exit(1);
}


/* 17-Nov-2015 nm Added abort, skip, ignore options */
/* Bug check */
void bug(int bugNum)
{
  vstring tmpStr = "";
  flag oldMode;
  long wrongAnswerCount = 0;
  static flag mode = 0; /* 1 = run to next bug, 2 = continue and ignore bugs */

  /* 10/10/02 */
  flag saveOutputToString = outputToString;
  outputToString = 0; /* Make sure we print to screen and not to string */

  if (mode == 2) {
    /* If user chose to ignore bugs, print brief info and return */
    print2("?BUG CHECK:  *** DETECTED BUG %ld, IGNORING IT...\n", (long)bugNum);
    return;
  }

  print2("?BUG CHECK:  *** DETECTED BUG %ld\n", (long)bugNum);
  if (mode == 0) { /* Print detailed info for first bug */
    print2("\n");
    print2(
  "To get technical support, please send Norm Megill (%salum.mit.edu) the\n",
        "nm@");
    print2(
  "detailed command sequence or a command file that reproduces this bug,\n");
    print2(
  "along with the source file that was used.  See HELP LOG for help on\n");
    print2(
  "recording a session.  See HELP SUBMIT for help on command files.  Search\n");
    print2(
  "for \"bug(%ld)\" in the m*.c source code to find its origin.\n", bugNum);
    print2("\n");
  }

  let(&tmpStr, "?");
  while (strcmp(tmpStr, "A") && strcmp(tmpStr, "a")
      && strcmp(tmpStr, "S") && strcmp(tmpStr, "s")
      && strcmp(tmpStr, "I") && strcmp(tmpStr, "i")
      /* The above is actually useless because of break below, but we'll leave
         it in case we want to re-ask after wrong answers in the future */
      ) {
    if (wrongAnswerCount > 6) {
      print2(
"Too many wrong answers; program will be aborted to exit scripting loops.\n");
      break; /* Added 8-Nov-03 */
    }
    if (wrongAnswerCount > 0) {
      let(&tmpStr, "");
      tmpStr = cmdInput1("Please answer I, S, or A:  ");
    } else {
      print2(
 "Press S <return> to step to next bug, I <return> to ignore further bugs,\n");
      let(&tmpStr, "");
      tmpStr = cmdInput1("or A <return> to abort program:  ");
    }
    /******* 8-Nov-03 This loop caused an infinite loop in a cron job when bug
      detection was triggered.  Now, when the loop breaks above,
      the program will abort. *******/
    wrongAnswerCount++;
  }
  oldMode = mode;
  mode = 0;
  if (!strcmp(tmpStr, "S") || !strcmp(tmpStr, "s")) mode = 1; /* Skip to next bug */
  if (!strcmp(tmpStr, "I") || !strcmp(tmpStr, "i")) mode = 2; /* Ignore bugs */
  if (oldMode == 0 && mode > 0) {
    /* Print dire warning after the first bug only */
    print2("\n");
    print2(
    "Warning!!!  A bug was detected, but you are continuing anyway.\n");
    print2(
    "The program may be corrupted, so you are proceeding at your own risk.\n");
    print2("\n");
    let(&tmpStr, "");
  }
  if (mode > 0) {
    /* 10/10/02 */
    outputToString = saveOutputToString; /* Restore for continuation */
    return;
  }
  let(&tmpStr, "");
#ifdef THINK_C
  cmdInput1("Program has crashed.  Press <return> to leave.");
#endif

  print2("\n");
  /* Close the log to make sure error log is saved */
  if (logFileOpenFlag) {
    print2("The log file \"%s\" was closed %s %s.\n",logFileName,
        date(),time_());
    fclose(logFilePtr);
    logFileOpenFlag = 0;
  }
  print2("The program was aborted.\n");
  exit(1); /* Use 1 instead of 0 to flag abnormal termination to scripts */
}


#define M_MAX_ALLOC_STACK 100

/* 26-Apr-2008 nm Added */
/* This function returns a 1 if any entry in a comma-separated list
   matches using the matches() function. */
flag matchesList(vstring testString, vstring pattern, char wildCard,
    char oneCharWildCard) {
  long entries, i;
  flag matchVal = 0;
  vstring entryPattern = "";

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack; /* For let() stack cleanup */
  startTempAllocStack = tempAllocStackTop;

  entries = numEntries(pattern);
  for (i = 1; i <= entries; i++) {
    let(&entryPattern, entry(i, pattern));
    matchVal = matches(testString, entryPattern, wildCard, oneCharWildCard);
    if (matchVal) break;
  }

  let(&entryPattern, ""); /* Deallocate */ /* 3-Jul-2011 nm Added to fix
                                              memory leak */
  startTempAllocStack = saveTempAllocStack;
  return (matchVal);
}


/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have wildcard characters.
   wildCard matches 0 or more characters; oneCharWildCard matches any
   single character. */
/* 30-Jan-06 nm Added single-character-match wildcard argument */
/* 19-Apr-2015 so, nm - Added "=" to match statement being proved */
/* 19-Apr-2015 so, nm - Added "%" to match changed proofs */
/* 8-Mar-2016 nm Added "#12345" to match internal statement number */
flag matches(vstring testString, vstring pattern, char wildCard,
    char oneCharWildCard) {
  long i, ppos, pctr, tpos, s1, s2, s3;
  vstring tmpStr = "";

  /* 21-Nov-2014 Stefan O'Rear - added label ranges - see HELP SEARCH */
  if (wildCard == '*') {
    /* Checking for wildCard = * means this is only for label listing */
    i = instr(1, pattern, "~");
    if (i != 0) {
      s1 = lookupLabel(left(pattern, i - 1));
      s2 = lookupLabel(testString);
      s3 = lookupLabel(right(pattern, i + 1));
      let(&tmpStr, ""); /* Clean up temporary allocations of left and right */
      return ((s1 >= 0 && s2 >= 0 && s3 >= 0 && s1 <= s2 && s2 <= s3)
          ? 1 : 0);
    }

    /* 8-Mar-2016 nm Added "#12345" to match internal statement number */
    if (pattern[0] == '#') {
      s1 = (long)val(right(pattern, 2));
      if (!strcmp(statement[s1].labelName, testString)) {
        return 1;
      } else {
        return 0;
      }
    }

    /* 19-Apr-2015 so, nm - Added "=" to match statement being proved */
    if (!strcmp(pattern,"=")) {
      s1 = lookupLabel(testString);
      return (PFASmode && proveStatement == s1);
    }

    /* 19-Apr-2015 so, nm - Added "%" to match changed proofs */
    if (!strcmp(pattern,"%")) {
      s1 = lookupLabel(testString);  /* Returns -1 if not found or (not
                                        $a and not $p) */
      if (s1 > 0) { /* It's a $a or $p statement */
        /* (If it's not $p, we don't want to peek at proofSectionPtr[-1]
           to prevent bad pointer. */
        if (statement[s1].type == (char)p_) { /* $p so it has a proof */
          /*
          /@ ASCII 1 is flag that proof is not from original source file @/
          if (statement[s1].proofSectionPtr[-1] == 1) {
          */
          /* 3-May-2017 nm */
          /* The proof is not from the original source file */
          if (statement[s1].proofSectionChanged == 1) {
            return 1;
          }
        }
      }
      return 0;
      /*
      return nmbrElementIn(1, changedStmtNmbr, s1);
      */
    }
  }

  /* Get to first wild card character */
  ppos = 0;
  /*if (wildCard!='*') printf("'%s' vs. '%s'\n", pattern, testString);*/
  while ((pattern[ppos] == testString[ppos] ||
          (pattern[ppos] == oneCharWildCard && testString[ppos] != 0))
      && pattern[ppos] != 0) ppos++;
  if (pattern[ppos] == 0) {
    if (testString[ppos] != 0) {
      return (0); /* No wildcards; mismatched */
    } else {
      return (1); /* No wildcards; matched */
    }
  }
  if (pattern[ppos] != wildCard) {
    return (0); /* Mismatched */
  }
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
    if (pattern[ppos + 1 + i] != testString[tpos + pctr + i]
          && (pattern[ppos + 1 + i] != oneCharWildCard
              || testString[tpos + pctr + i] == 0)) {
      if (testString[tpos + pctr + i] == 0) {
        return (0);
      }
      pctr++;
      i = 0;
      continue;
    }
    if (pattern[ppos + 1 + i] == 0) {
      return(1); /* Matched */
    }
    i++;
  }
  bug(1375);
  return (0); /* Dummy return - never used */
}




/*******************************************************************/
/*********** Number string functions *******************************/
/*******************************************************************/

long nmbrTempAllocStackTop = 0;     /* Top of stack for nmbrTempAlloc functon */
long nmbrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when nmbrLet() is called (normally 0, except in
                                    special nested vstring functions) */
nmbrString *nmbrTempAllocStack[M_MAX_ALLOC_STACK];


nmbrString *nmbrTempAlloc(long size)
                                /* nmbrString memory allocation/deallocation */
{
  /* When "size" is >0, "size" instances of nmbrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to nmbrStartTempAllocStack. */
  /* int i; */  /* 11-Jul-2014 WL old code deleted */
  if (size) {
    if (nmbrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
      /*??? Fix to allocate more */
      outOfMemory("#105 (nmbrString stack array)");
    }
    if (!(nmbrTempAllocStack[nmbrTempAllocStackTop++]=poolMalloc(size
        *(long)(sizeof(nmbrString)))))
      /* outOfMemory("#106 (nmbrString stack)"); */ /*???Unnec. w/ poolMalloc*/
/*E*/db2=db2+size*(long)(sizeof(nmbrString));
    return (nmbrTempAllocStack[nmbrTempAllocStackTop-1]);
  } else {
    /* 11-Jul-2014 WL old code deleted */
    /*
    for (i=nmbrStartTempAllocStack; i < nmbrTempAllocStackTop; i++) {
/@E@/db2=db2-(nmbrLen(nmbrTempAllocStack[i])+1)*(long)(sizeof(nmbrString));
      poolFree(nmbrTempAllocStack[i]);
    }
    */
    /* 11-Jul-2014 WL new code */
    while(nmbrTempAllocStackTop != nmbrStartTempAllocStack) {
/*E*/db2=db2-(nmbrLen(nmbrTempAllocStack[nmbrTempAllocStackTop-1])+1)
/*E*/                                              *(long)(sizeof(nmbrString));
      poolFree(nmbrTempAllocStack[--nmbrTempAllocStackTop]);
    }
    /* end of 11-Jul-2014 WL new code */
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
#if __STDC__
      fflush(stdout);
#endif
      bug(1368);
    }
    if (s[0] != -1) { /* End of string */
      /* Do it only if nmbrString is not empty */
      nmbrTempAllocStack[nmbrTempAllocStackTop++] = s;
    }
/*E*/db2=db2+(nmbrLen(s)+1)*(long)(sizeof(nmbrString));
/*E*/db3=db3-(nmbrLen(s)+1)*(long)(sizeof(nmbrString));
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  sourceLength=nmbrLen(source);  /* Save its actual length */
  targetLength=nmbrLen(*target);  /* Save its actual length */
  targetAllocLen=nmbrAllocLen(*target); /* Save target's allocated length */
/*E*/if (targetLength) {
/*E*/  /* printf("Deleting %s\n",cvtMToVString(*target,0)); */
/*E*/  db3 = db3 - (targetLength+1)*(long)(sizeof(nmbrString));
/*E*/}
/*E*/if (sourceLength) {
/*E*/  /* printf("Adding %s\n",cvtMToVString(source,0)); */
/*E*/  db3 = db3 + (sourceLength+1)*(long)(sizeof(nmbrString));
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0aa: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0ab: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }


/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0a: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
      } else {
        /* Free old string space and allocate new space */
        poolFree(*target);  /* Free old space */
        /* *target=poolMalloc((sourceLength + 1) * sizeof(nmbrString)); */
        *target=poolMalloc((sourceLength + 1) * (long)(sizeof(nmbrString)) * 2);
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0b: pool %ld stat %ld\n",poolTotalFree,i1+j1_);

      }

    } else {    /* source is 0 length, target is not */
      poolFree(*target);
      *target= NULL_NMBRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0c: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  } else {
    if (sourceLength) { /* target is 0 length, source is not */
      *target=poolMalloc((sourceLength + 1) * (long)(sizeof(nmbrString)));
                        /* Allocate new space */
      /* if (!*target) outOfMemory("#108 (nmbrString)"); */ /*???Unnec. w/ poolMalloc*/
      nmbrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    } else {    /* source and target are both 0 length */
      /* *target= NULL_NMBRSTRING; */ /* Redundant */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
#if __STDC__
      fflush(stdout);
#endif
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
  return (((long)(((long *)s)[-1] - (long)(sizeof(nmbrString))))
              / (long)(sizeof(nmbrString)));
}


/* Find out the allocated length of a nmbrString */
long nmbrAllocLen(nmbrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return (((long)(((long *)s)[-2] - (long)(sizeof(nmbrString))))
              / (long)(sizeof(nmbrString)));
}

/* Set the actual size field in a nmbrString allocated with poolFixedMalloc() */
/* Use this if "zapping" a nmbrString element with -1 to reduce its length. */
/* Note that the nmbrString will not be moved to the "used pool", even if
   zapping its length results in free space; thus the free space will never
   get recovered unless done by the caller or poolFree is called.  (This is
   done on purpose so the caller can know what free space is left.) */
/* ???Note that nmbrZapLen's not moving string to used pool wastes potential
   space when called by the routines in this module.  Effect should be minor. */
void nmbrZapLen(nmbrString *s, long length) {
  if (((long *)s)[-3] != -1) {
    /* It's already in the used pool, so adjust free space tally */
    poolTotalFree = poolTotalFree + ((long *)s)[-1]
        - (length + 1) * (long)(sizeof(nmbrString));
  }
  ((long *)s)[-1] = (length + 1) * (long)(sizeof(nmbrString));
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("l: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
nmbrString *nmbrSeg(nmbrString *sin, long start, long stop)
{
  nmbrString *sout;
  long length;
  if (start < 1) start = 1;
  if (stop < 1) stop = 0;
  length=stop - start + 1;
  if (length < 0) length = 0;
  sout = nmbrTempAlloc(length + 1);
  nmbrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_NMBRSTRING;
  return (sout);
}

/* Extract sin from character position start for length len */
nmbrString *nmbrMid(nmbrString *sin, long start, long length)
{
  nmbrString *sout;
  if (start < 1) start = 1;
  if (length < 0) length = 0;
  sout = nmbrTempAlloc(length + 1);
  nmbrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_NMBRSTRING;
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
/* Warning:  This has 'let' inside of it and is not safe for use inside
   of 'let' statements.  (To make it safe, it must be rewritten to expand
   the 'mid' and remove the 'let'.) */
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
    outputLen = outputLen + (long)strlen(mathToken[s[i]].tokenName) + 1;
  }
  let(&tmpStr, space(outputLen)); /* Preallocate output string */
  /* Assign output string */
  ptr = tmpStr;
  for (i = 0; i < mstrLen; i++) {
    ptr2 = mathToken[s[i]].tokenName;
    j = (long)strlen(ptr2);
    memcpy(ptr, ptr2, (size_t)j);
    ptr = ptr + j + 1;
  }

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deallocation */
  return (tmpStr);
}


/* Converts proof to a vstring with one space between tokens */
/* 11-Sep-2016 nm Allow it to tolerate garbage entries for debugging */
vstring nmbrCvtRToVString(nmbrString *proof,
    /* 25-Jan-2016 */
    flag explicitTargets, /* 1 = "target=source" for /EXPLICIT proof format */
    long statemNum) /* used only if explicitTargets=1 */
{
  long i, j, plen, maxLabelLen, maxLocalLen, step, stmt;
  long maxTargetLabelLen; /* 25-Jan-2016 nm */
  vstring proofStr = "";
  vstring tmpStr = "";
  vstring ptr;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  long nextLocLabNum = 1; /* Next number to be used for a local label */
  void *voidPtr; /* bsearch result */
  /* 26-Jan-2016 nm */
  nmbrString *targetHyps = NULL_NMBRSTRING; /* Targets for /EXPLICIT format */

  long saveTempAllocStack;
  long nmbrSaveTempAllocStack;
  saveTempAllocStack = startTempAllocStack; /* For let() stack cleanup */
  startTempAllocStack = tempAllocStackTop;
  nmbrSaveTempAllocStack = nmbrStartTempAllocStack;
                                           /* For nmbrLet() stack cleanup*/
  nmbrStartTempAllocStack = nmbrTempAllocStackTop;

  plen = nmbrLen(proof);

  /* 25-Jan-2016 nm */
  if (explicitTargets == 1) {
    /* Get the list of targets for /EXPLICIT format */
    if (statemNum <= 0) bug(1388);
    nmbrLet(&targetHyps, nmbrGetTargetHyp(proof, statemNum));
  }

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
  maxTargetLabelLen = 0; /* 25-Jan-2016 nm */
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      if (!nmbrElementIn(1, localLabels, stmt)) {
        nmbrLet(&localLabels, nmbrAddElement(localLabels, stmt));
      }
    } else {

      /* 11-Sep-2016 nm */
      if (stmt < 1 || stmt > statements) {
        maxLabelLen = 100; /* For safety */
        maxTargetLabelLen = 100; /* For safety */
        continue; /* Ignore bad entry */
      }

      if (stmt > 0) {
        if ((signed)(strlen(statement[stmt].labelName)) > maxLabelLen) {
          maxLabelLen = (long)strlen(statement[stmt].labelName);
        }
      }
    }

    /* 25-Jan-2016 nm */
    if (explicitTargets == 1) {
      /* Also consider longest target label name */
      stmt = targetHyps[step];
      if (stmt <= 0) bug(1390);
      if ((signed)(strlen(statement[stmt].labelName)) > maxTargetLabelLen) {
        maxTargetLabelLen = (long)strlen(statement[stmt].labelName);
      }
    }

  } /* next step */

  /* localLabelNames[] holds an integer which, when converted to string,
    is the local label name. */
  nmbrLet(&localLabelNames, nmbrSpace(plen));

  /* Build the ASCII string */
  /* Preallocate the string for speed (the "2" accounts for a space and a
     colon). */
  let(&proofStr, space(plen * (2 + maxLabelLen
      + ((explicitTargets == 1) ? maxTargetLabelLen + 1 : 0)  /* 25-Jan-2016 */
                                          /* The "1" accounts for equal sign */
      + maxLocalLen)));
  ptr = proofStr;
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        stmt = -1000 - stmt;
            /* Change stmt to the step number a local label refers to */
        let(&tmpStr, cat(

            /* 25-Jan-2016 nm */
            ((explicitTargets == 1) ? statement[targetHyps[step]].labelName : ""),
            ((explicitTargets == 1) ? "=" : ""),

            str((double)(localLabelNames[stmt])), " ", NULL));

      /* 11-Sep-2016 nm */
      } else if (stmt != -(long)'?') {
        let(&tmpStr, cat("??", str((double)stmt), " ", NULL)); /* For safety */

      } else {
        if (stmt != -(long)'?') bug(1391); /* Must be an unknown step */
        let(&tmpStr, cat(

            /* 25-Jan-2016 nm */
            ((explicitTargets == 1) ? statement[targetHyps[step]].labelName : ""),
            ((explicitTargets == 1) ? "=" : ""),

            chr(-stmt), " ", NULL));
      }

    /* 11-Sep-2016 nm */
    } else if (stmt < 1 || stmt > statements) {
      let(&tmpStr, cat("??", str((double)stmt), " ", NULL)); /* For safety */

    } else {
      let(&tmpStr,"");
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        /* First, get a name for the local label, using the next integer that
           does not match any integer used for a statement label. */
        let(&tmpStr, str((double)nextLocLabNum));
        while (1) {
          voidPtr = (void *)bsearch(tmpStr,
              allLabelKeyBase, (size_t)numAllLabelKeys,
              sizeof(long), labelSrchCmp);
          if (!voidPtr) break; /* It does not conflict */
          nextLocLabNum++; /* Try the next one */
          let(&tmpStr, str((double)nextLocLabNum));
        }
        localLabelNames[step] = nextLocLabNum;
        let(&tmpStr, cat(tmpStr, ":", NULL));
        nextLocLabNum++; /* Prepare for next local label */
      }
      let(&tmpStr, cat(tmpStr,

          /* 25-Jan-2016 nm */
          ((explicitTargets == 1) ? statement[targetHyps[step]].labelName : ""),
          ((explicitTargets == 1) ? "=" : ""),

          statement[stmt].labelName, " ", NULL));
    }
    j = (long)strlen(tmpStr);
    memcpy(ptr, tmpStr, (size_t)j);
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
    let(&tmpStr,cat(tmpStr," ", str((double)(s[i-1])),NULL));
  }

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deallocation */
  return (tmpStr);
}


/* Extract variables from a math token string */
nmbrString *nmbrExtractVars(nmbrString *m)
{
  long i, j, length;
  nmbrString *v;
  length = nmbrLen(m);
  v=nmbrTempAlloc(length + 1); /* Pre-allocate maximum possible space */
  v[0] = *NULL_NMBRSTRING;
  j = 0; /* Length of output string */
  for (i = 0; i < length; i++) {
    /*if (m[i] < 0 || m[i] >= mathTokens) {*/
    /* Changed >= to > because tokenNum=mathTokens is used by mmveri.c for
       dummy token */
    if (m[i] < 0 || m[i] > mathTokens) bug(1328);
    if (mathToken[m[i]].tokenType == (char)var_) {
      if (!nmbrElementIn(1, v, m[i])) { /* Don't duplicate variable */
        v[j] = m[i];
        j++;
        v[j] = *NULL_NMBRSTRING; /* Add temp. end-of-string for getElementOf() */
      }
    }
  }
  nmbrZapLen(v, j); /* Zap mem pool fields */
/*E*/db2=db2-(length-nmbrLen(v))*(long)(sizeof(nmbrString));
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
  long length;
  nmbrString *v;
  length = nmbrLen(g);
  v = nmbrTempAlloc(length + 2); /* Allow for end of string */
  nmbrCpy(v, g);
  v[length] = element;
  v[length + 1] = *NULL_NMBRSTRING; /* End of string */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bbg2: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
/*E*/db2=db2-(len1+len2-nmbrLen(v))*(long)(sizeof(nmbrString));
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
/*E*/db2=db2-(len2-nmbrLen(v))*(long)(sizeof(nmbrString));
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
/*E*/db2=db2-(len1-nmbrLen(v))*(long)(sizeof(nmbrString));
  return v;
}


/* This is a utility function that returns the length of a subproof that
   ends at step */
/* 22-Aug-2012 nm - this doesn't seem to be used outside of mmdata.c -
   should we replace it with subproofLen() in mmpfas.c? */
long nmbrGetSubproofLen(nmbrString *proof, long step)
{
  long stmt, hyps, pos, i;
  char type;

  if (step < 0) bug(1329);
  stmt = proof[step];
  if (stmt < 0) return (1); /* Unknown or label ref */
  type = statement[stmt].type;
  if (type == f_ || type == e_) return (1); /* Hypothesis */
  hyps = statement[stmt].numReqHyp;
  pos = step - 1;
  for (i = 0; i < hyps; i++) {
    pos = pos - nmbrGetSubproofLen(proof, pos);
  }
  return (step - pos);
}




/* This function returns a packed or "squished" proof, putting in local label
   references to previous subproofs. */
nmbrString *nmbrSquishProof(nmbrString *proof)
{
  nmbrString *newProof = NULL_NMBRSTRING;
  nmbrString *dummyProof = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  long step, dummyStep, subPrfLen, matchStep, plen;
  flag foundFlag;

  nmbrLet(&newProof,proof); /* In case of temp. alloc. of proof */
  plen = nmbrLen(newProof);
  dummyStep = 0;
  nmbrLet(&dummyProof, newProof); /* Parallel proof with test subproof replaced
                                 with a reference to itself, for matching. */
  for (step = 0; step < plen; step++) {
    subPrfLen = nmbrGetSubproofLen(dummyProof, dummyStep);
    if (subPrfLen <= 1) {
      dummyStep++;
      continue;
    }
    nmbrLet(&subProof, nmbrSeg(dummyProof, dummyStep - subPrfLen + 2,
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
            -1000 - step), nmbrRight(newProof, matchStep + subPrfLen), NULL));
      matchStep = matchStep - subPrfLen + 1;
    }
    if (foundFlag) {
      plen = nmbrLen(newProof); /* Update the new proof length */
      /* Replace this subproof with a reference to itself, for later matching */
      /* and add on rest of real proof. */
      dummyStep = dummyStep + 1 - subPrfLen;
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


/* This function unpacks a "squished" proof, replacing local label references
   to previous subproofs by the subproofs themselves. */
nmbrString *nmbrUnsquishProof(nmbrString *proof)
{
  nmbrString *newProof = NULL_NMBRSTRING;
  nmbrString *subProof = NULL_NMBRSTRING;
  long step, plen, subPrfLen, stmt;

  nmbrLet(&newProof, proof);
  plen = nmbrLen(newProof);
  for (step = plen - 1; step >= 0; step--) {
    stmt = newProof[step];
    if (stmt > -1000) continue;
    /* It's a local label reference */
    stmt = -1000 - stmt;
    subPrfLen = nmbrGetSubproofLen(newProof, stmt);
    nmbrLet(&newProof, nmbrCat(nmbrLeft(newProof, step),
        nmbrSeg(newProof, stmt - subPrfLen + 2, stmt + 1),
        nmbrRight(newProof, step + 2), NULL));
    step = step + subPrfLen - 1;
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
  if (type == f_ || type == e_) { /* A hypothesis */
    if (plen != 1) bug(1331);
    nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
    return (indentationLevel);
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1332);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubproofLen(proof, pos);
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
} /* nmbrGetIndentation */


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
  if (type == f_ || type == e_) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1336);
    nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
    return (essentialFlags);
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1337);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  nmbrTmpPtr2 = statement[stmt].reqHypList;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubproofLen(proof, pos);
    if (statement[nmbrTmpPtr2[hyps - i - 1]].type == e_) {
      nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
      nmbrLet(&nmbrTmp, nmbrGetEssential(subProof));
      for (j = 0; j < splen; j++) {
        essentialFlags[j + pos - splen + 1] = nmbrTmp[j];
      }
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (1338);

  nmbrLet(&subProof,NULL_NMBRSTRING); /* Deallocate */
  nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Deallocate */
  nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
  return (essentialFlags);
} /* nmbrGetEssential */


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
  if (type == f_ || type == e_) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1341);
    nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
    return (targetHyp);
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1342);
  hyps = statement[stmt].numReqHyp;
  pos = plen - 2;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubproofLen(proof, pos);
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
} /* nmbrGetTargetHyp */


/* Converts a proof string to a compressed-proof-format ASCII string.
   Normally, the proof string would be packed with nmbrSquishProof first,
   although it's not a requirement (in which case the compressed proof will
   be much longer of course). */
/* The statement number is needed because required hypotheses are
   implicit in the compressed proof. */
/* The returned ASCII string isn't surrounded by spaces e.g. it
   could be "( a1i a1d ) ACBCADEF". */
vstring compressProof(nmbrString *proof, long statemNum,
    flag oldCompressionAlgorithm)
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
  long i, j, k;
  /* flag breakFlag; */ /* 15-Oct-05 nm No longer used */
  /* char c; */ /* 15-Oct-05 nm No longer used */
  long lettersLen, digitsLen;
  static char *digits = "0123456789";
  static char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char labelChar = ':';

  /* 27-Dec-2013 nm Variables for new algorithm */
  nmbrString *explList = NULL_NMBRSTRING;
  long explLabels;
  nmbrString *explRefCount = NULL_NMBRSTRING;
  nmbrString *labelRefCount = NULL_NMBRSTRING;
  long maxExplRefCount;
  nmbrString *explComprLen = NULL_NMBRSTRING;
  long explSortPosition;
  long maxExplComprLen;
  vstring explUsedFlag = "";
  nmbrString *explLabelLen = NULL_NMBRSTRING;
  nmbrString *newExplList = NULL_NMBRSTRING;
  long newExplPosition;
  long indentation;
  long explOffset;
  long explUnassignedCount;
  nmbrString *explWorth = NULL_NMBRSTRING;
  long explWidth;
  vstring explIncluded = "";


  /* Compression standard with all cap letters */
  /* (For 500-700 step proofs, we only lose about 18% of file size --
      but the compressed proof is more pleasing to the eye) */
  letters = "ABCDEFGHIJKLMNOPQRST"; /* LSB is base 20 */
  digits = "UVWXY"; /* MSB's are base 5 */
  labelChar = 'Z'; /* Was colon */

  lettersLen = (long)strlen(letters);
  digitsLen = (long)strlen(digits);

  nmbrLet(&saveProof, proof); /* In case of temp. alloc. of proof */

  if (statement[statemNum].type != (char)p_) bug(1344);
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
        if (-1000 - stmt >= plen) bug(345);
        localLabelFlags[-1000 - stmt] = 1;
        localLabels++;
      } else {
        if (stmt != -(long)'?') bug(1346);
      }
    } else {
      if (statement[stmt].type != (char)a_ &&
          statement[stmt].type != (char)p_) {
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

  /* To obtain the old algorithm, we simply skip the new label re-ordering */
  if (oldCompressionAlgorithm) goto OLD_ALGORITHM;


  /* 27-Dec-2013 nm */
  /************ New algorithm to sort labels according to usage ***********/

  /* This algorithm, based on an idea proposed by Mario Carneiro, sorts
     the explicit labels so that the most-used labels occur first, optimizing
     the use of 1-character compressed label lengths, then 2-character
     lengths, and so on.  Also, an attempt is made to fit the label list into
     the exact maximum current screen width, using the 0/1-knapsack
     algorithm, so that fewer lines will result due to wasted space at the
     end of each line with labels.  */

  /* Get the list of explicit labels */
  nmbrLet(&explList, nmbrCat(
      /* Trim off leading implicit required hypotheses */
      nmbrRight(hypList, statement[statemNum].numReqHyp + 1),
      /* Add in the list of assertion ($a, $p) references */
      assertionList, NULL));
  explLabels = nmbrLen(explList);

  /* Initialize reference counts for the explicit labels */
  nmbrLet(&explRefCount, nmbrSpace(explLabels));

  /* Count the number of references to labels in the original proof */
  /* We allocate up to statemNum, since any earlier statement could appear */
  nmbrLet(&labelRefCount, nmbrSpace(statemNum));  /* Warning: nmbrSpace() must
                                                 produce a string of 0's */
  for (step = 0; step < plen; step++) { /* Scan the proof */
    if (saveProof[step] > 0) { /* Ignore local labels and '?' */
      if (saveProof[step] < statemNum) {
        labelRefCount[saveProof[step]]++;
      } else {
        bug(1380); /* Corrupted proof should have been caught earlier */
      }
    }
  }
  maxExplRefCount = 0;  /* Largest number of reference counts found */
  /* Populate the explict label list with the counts */
  for (i = 0; i < explLabels; i++) {
    explRefCount[i] = labelRefCount[explList[i]]; /* Save the ref count */
    if (explRefCount[i] <= 0) bug(1381);
    if (explRefCount[i] > maxExplRefCount) {
      maxExplRefCount = explRefCount[i]; /* Update largest count */
    }
  }
  /* We're done with giant labelRefCount array; deallocate */
  nmbrLet(&labelRefCount, NULL_NMBRSTRING);

  /* Assign compressed label lengths starting from most used to least
     used label */
  /* Initialize compressed label lengths for the explicit labels */
  nmbrLet(&explComprLen, nmbrSpace(explLabels));
  explSortPosition = 0;
  maxExplComprLen = 0;
  /* The "sorting" below has n^2 behavior; improve if is it a problem */
  /* explSortPosition is where the label would occur if reverse-sorted
     by reference count, for the purpose of computing the compressed
     label length.  No actual sorting is done, since later we're
     only interested in groups with the same compressed label length. */
  for (i = maxExplRefCount; i >= 1; i--) {
    for (j = 0; j < explLabels; j++) {
      if (explRefCount[j] == i) {
        /* Find length, numchrs, of compressed label */
        /* If there are no req hyps, 0 = 1st label in explict list */
        lab = statement[statemNum].numReqHyp + explSortPosition;

        /* The following 7 lines are from the compressed label length
           determination algorithm below */
        numchrs = 1;
        k = lab / lettersLen;
        while (1) {
          if (!k) break;
          numchrs++;
          k = (k - 1) / digitsLen;
        }

        explComprLen[j] = numchrs; /* Assign the compressed label length */
        if (numchrs > maxExplComprLen) {
          maxExplComprLen = numchrs; /* Update maximum length */
        }
        explSortPosition++;
      }
    }
  }

  let(&explUsedFlag, string(explLabels, 'n')); /* Mark with 'y' when placed in
                                            output label list (newExplList) */
  nmbrLet(&explLabelLen, nmbrSpace(explLabels));
  /* Populate list of label lengths for knapsack01() "size" */
  for (i = 0; i < explLabels; i++) {
    stmt = explList[i];
    explLabelLen[i] = (long)(strlen(statement[stmt].labelName)) + 1;
                                     /* +1 accounts for space between labels */
  }

  /* Re-distribute labels in order of compressed label length, fitted to
     line by knapsack01() algorithm */

  nmbrLet(&newExplList, nmbrSpace(explLabels)); /* List in final order */
  nmbrLet(&explWorth, nmbrSpace(explLabels));  /* "Value" for knapsack01() */
  let(&explIncluded, string(explLabels, '?')); /* Returned by knapsack01() */
  newExplPosition = 0; /* Counter for position in output label list */

  indentation =  2 + getSourceIndentation(statemNum); /* Proof indentation */
  explOffset = 2; /* add 2 for "( " opening parenthesis of compressed proof */

  /* Fill up the output with labels in groups of increasing compressed label
     size */
  for (i = 1; i <= maxExplComprLen; i++) {
    explUnassignedCount = 0; /* Unassigned at current compressed label size */
    /* Initialize worths for knapsack01() */
    for (j = 0; j < explLabels; j++) {
      if (explComprLen[j] == i) {
        if (explUsedFlag[j] == 'y') bug(1382);
        explWorth[j] = explLabelLen[j]; /* Make worth=size so that label
            length does not affect whether the label is chosen by knapsack01(),
            so the only influence is whether it fits */
        explUnassignedCount++;
      } else { /* Not the current compressed label size */
        explWorth[j] = -1; /* Negative worth will make knapsack avoid it */
      }
    }
    while (explUnassignedCount > 0) {
      /* Find the the amount of space available on the remainder of the line */
      /* The +1 accounts for space after last label, which wrapping will trim */
      /* Note that the actual line wrapping will happen with printLongLine
         far in the future.  Here we will just put the labels in the order
         that will cause it to wrap at the desired place. */
      explWidth = screenWidth - indentation - explOffset + 1;

      /* Fill in the label list output line with labels that fit best */
      /* The knapsack01() call below is always given the entire set of
         explicit labels, with -1 worth assigned to the ones to be avoided.
         It would be more efficient to give it a smaller list with -1s
         removed, if run time becomes a problem. */
      j = knapsack01(explLabels /*#items*/,
          explLabelLen /*array of sizes*/,
          explWorth /*array of worths*/,
          explWidth /*maxSize*/,
          explIncluded /*itemIncluded return values*/);
      /*if (j == 0) bug(1383);*/ /* j=0 is legal when it can't fit any labels
         on the rest of the line (such as if the line only has 1 space left
         i.e. explWidth=1) */
      if (j < 0) bug(1383);

      /* Accumulate the labels selected by knapsack01() into the output list,
         in the same order as they appeared in the original explicit label
         list */
      explUnassignedCount = 0;
      /* Scan expIncluded y/n string returned by knapsack01() */
      for (j = 0; j < explLabels; j++) {
        if (explIncluded[j] == 'y') { /* was chosen by knapsack01() */
          if (explComprLen[j] != i) bug(1384); /* Other compressed length
             shouldn't occur because -1 worth should have been rejected by
             knapsack01() */
          newExplList[newExplPosition] = explList[j];
          newExplPosition++;
          explUsedFlag[j] = 'y';
          if (explWorth[j] == -1) bug(1385); /* knapsack01() should
              have rejected all previously assigned labels */
          explWorth[j] = -1; /* Negative worth will avoid it next loop iter */
          explOffset = explOffset + explLabelLen[j];
        } else {
          if (explComprLen[j] == i && explUsedFlag[j] == 'n') {
            explUnassignedCount++; /* There are still more to be assigned
                                      at this compressed label length */
            if (explWorth[j] != explLabelLen[j]) bug(1386); /* Sanity check */
          }
        }
      }
      if (explUnassignedCount > 0) {
        /* If there are labels still at this level (of compressed
           label length), so start a new line for next knapsack01() call */
        explOffset = 0;
      }
    }
  }
  if (newExplPosition != explLabels) bug(1387); /* Labels should be exhausted */

  /* The hypList and assertionList below are artificially assigned
     for use by the continuation of the old algorithm that follows */

  /* "hypList" is truncated to have only the required hypotheses with no
     optional ones */
  nmbrLet(&hypList, nmbrLeft(hypList, statement[statemNum].numReqHyp));
  /* "assertionList" will have both the optional hypotheses and the assertions,
     reordered */
  nmbrLet(&assertionList, newExplList);

  /********************** End of new algorithm ****************************/


 OLD_ALGORITHM:
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
        outputAllocated = outputLen + 1 + COMPR_INC; /* = (long)strlen(output) */
        /* CPU-intensive bug check; enable only if required: */
        /* if (outputAllocated != (long)strlen(output)) bug(1348); */
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
      outputAllocated = outputLen + numchrs + COMPR_INC; /* = (long)strlen(output) */
      /* CPU-intensive bug check; enable only if required: */
      /* if (outputAllocated != (long)strlen(output)) bug(1350); */
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
      outputAllocated = outputLen + 1 + COMPR_INC; /* = (long)strlen(output) */
      /* CPU-intensive bug check due to strlen; enable only if required: */
      /* if (outputAllocated != (long)strlen(output)) bug(1352); */
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
      assertionList, NULL),
                /* 25-Jan-2016 nm */
                0, /*explicitTargets*/
                0 /*statemNum used only if explicitTargets*/),
      " ) ", left(output, outputLen), NULL));

  nmbrLet(&saveProof, NULL_NMBRSTRING);
  nmbrLet(&labelList, NULL_NMBRSTRING);
  nmbrLet(&hypList, NULL_NMBRSTRING);
  nmbrLet(&assertionList, NULL_NMBRSTRING);
  nmbrLet(&localList, NULL_NMBRSTRING);
  nmbrLet(&localLabelFlags, NULL_NMBRSTRING);

  /* Deallocate arrays for new algorithm */  /* 27-Dec-2013 nm */

  nmbrLet(&explList, NULL_NMBRSTRING);
  nmbrLet(&explRefCount, NULL_NMBRSTRING);
  nmbrLet(&labelRefCount, NULL_NMBRSTRING);
  nmbrLet(&explComprLen, NULL_NMBRSTRING);
  let(&explUsedFlag, "");
  nmbrLet(&explLabelLen, NULL_NMBRSTRING);
  nmbrLet(&newExplList, NULL_NMBRSTRING);
  nmbrLet(&explWorth, NULL_NMBRSTRING);
  let(&explIncluded, "");

  makeTempAlloc(output); /* Flag it for deallocation */
  return(output);
} /* compressProof */


/* Added 11-Sep-2016 nm */
/* Compress the input proof, create the ASCII compressed proof,
   and return its size in bytes. */
/* TODO: call this in MINIMIZE_WITH in metamath.c */
long compressedProofSize(nmbrString *proof, long statemNum) {
  vstring tmpStr = "";
  nmbrString *tmpNmbr = NULL_NMBRSTRING;
  long bytes;
  nmbrLet(&tmpNmbr, nmbrSquishProof(proof));
  let(&tmpStr, compressProof(tmpNmbr,
          statemNum, /* statement being proved */
          0 /* don't use old algorithm (this will become obsolete) */
          ));
  bytes = (long)strlen(tmpStr);
  /* Deallocate memory */
  let(&tmpStr, "");
  nmbrLet(&tmpNmbr, NULL_NMBRSTRING);
  return bytes;
} /* compressedProofSize */



/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

long pntrTempAllocStackTop = 0;     /* Top of stack for pntrTempAlloc functon */
long pntrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when pntrLet() is called (normally 0, except in
                                    special nested vstring functions) */
pntrString *pntrTempAllocStack[M_MAX_ALLOC_STACK];


pntrString *pntrTempAlloc(long size)
                                /* pntrString memory allocation/deallocation */
{
  /* When "size" is >0, "size" instances of pntrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to pntrStartTempAllocStack. */
  /* int i; */   /* 11-Jul-2014 WL old code deleted */
  if (size) {
    if (pntrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1))
      /*??? Fix to allocate more */
      outOfMemory("#109 (pntrString stack array)");
    if (!(pntrTempAllocStack[pntrTempAllocStackTop++]=poolMalloc(size
        *(long)(sizeof(pntrString)))))
      /* outOfMemory("#110 (pntrString stack)"); */ /*???Unnec. w/ poolMalloc*/
/*E*/db2=db2+(size)*(long)(sizeof(pntrString));
    return (pntrTempAllocStack[pntrTempAllocStackTop-1]);
  } else {
    /* 11-Jul-2014 WL old code deleted */
    /*
    for (i=pntrStartTempAllocStack; i < pntrTempAllocStackTop; i++) {
/@E@/db2=db2-(pntrLen(pntrTempAllocStack[i])+1)*(long)(sizeof(pntrString));
      poolFree(pntrTempAllocStack[i]);
    }
    */
    /* 11-Jul-2014 WL new code */
    while(pntrTempAllocStackTop != pntrStartTempAllocStack) {
/*E*/db2=db2-(pntrLen(pntrTempAllocStack[pntrTempAllocStackTop-1])+1)
/*E*/                                              *(long)(sizeof(pntrString));
      poolFree(pntrTempAllocStack[--pntrTempAllocStackTop]);
    }
    /* end of 11-Jul-2014 WL new code */
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
#if __STDC__
      fflush(stdout);
#endif
      bug(1370);
    }
    if (s[0] != NULL) { /* Don't do it if pntrString is empty */
      pntrTempAllocStack[pntrTempAllocStackTop++] = s;
    }
/*E*/db2=db2+(pntrLen(s)+1)*(long)(sizeof(pntrString));
/*E*/db3=db3-(pntrLen(s)+1)*(long)(sizeof(pntrString));
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  sourceLength=pntrLen(source);  /* Save its actual length */
  targetLength=pntrLen(*target);  /* Save its actual length */
  targetAllocLen=pntrAllocLen(*target); /* Save target's allocated length */
/*E*/if (targetLength) {
/*E*/  /* printf("Deleting %s\n",cvtMToVString(*target,0)); */
/*E*/  db3 = db3 - (targetLength+1)*(long)(sizeof(pntrString));
/*E*/}
/*E*/if (sourceLength) {
/*E*/  /* printf("Adding %s\n",cvtMToVString(source,0)); */
/*E*/  db3 = db3 + (sourceLength+1)*(long)(sizeof(pntrString));
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0aa: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
          } else {
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0ab: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
          }
        } else {
          if (((long *)(*target))[-3] != -1) {
            /* It's in the pool (but all allocated space coincidentally used) */
            /* Adjust free space independently */
            poolTotalFree = poolTotalFree + poolDiff;
          }
        }


/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0a: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
      } else {
        /* Free old string space and allocate new space */
        poolFree(*target);  /* Free old space */
        /* *target=poolMalloc((sourceLength + 1) * sizeof(pntrString)); */
        *target=poolMalloc((sourceLength + 1) * (long)(sizeof(pntrString)) * 2);
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
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0b: pool %ld stat %ld\n",poolTotalFree,i1+j1_);

      }

    } else {    /* source is 0 length, target is not */
      poolFree(*target);
      *target= NULL_PNTRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0c: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  } else {
    if (sourceLength) { /* target is 0 length, source is not */
      *target=poolMalloc((sourceLength + 1) * (long)(sizeof(pntrString)));
                        /* Allocate new space */
      /* if (!*target) outOfMemory("#112 (pntrString)"); */ /*???Unnec. w/ poolMalloc*/
      pntrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    } else {    /* source and target are both 0 length */
      /* *target= NULL_PNTRSTRING; */ /* Redundant */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
#if __STDC__
      fflush(stdout);
#endif
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
  return ((((long *)s)[-1] - (long)(sizeof(pntrString)))
      / (long)(sizeof(pntrString)));
}


/* Find out the allocated length of a pntrString */
long pntrAllocLen(pntrString *s)
{
  return ((((long *)s)[-2] - (long)(sizeof(pntrString)))
    / (long)(sizeof(pntrString)));
}

/* Set the actual size field in a pntrString allocated with poolFixedMalloc() */
/* Use this if "zapping" a pntrString element with -1 to reduce its length. */
/* Note that the pntrString will not be moved to the "used pool", even if
   zapping its length results in free space; thus the free space will never
   get recovered unless done by the caller or poolFree is called.  (This is
   done on purpose so the caller can know what free space is left.) */
/* ???Note that pntrZapLen's not moving string to used pool wastes potential
   space when called by the routines in this module.  Effect should be minor. */
void pntrZapLen(pntrString *s, long length) {
  if (((long *)s)[-3] != -1) {
    /* It's already in the used pool, so adjust free space tally */
    poolTotalFree = poolTotalFree + ((long *)s)[-1]
        - (length + 1) * (long)(sizeof(pntrString));
  }
  ((long *)s)[-1] = (length + 1) * (long)(sizeof(pntrString));
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("l: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
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
pntrString *pntrSeg(pntrString *sin, long start, long stop)
{
  pntrString *sout;
  long length;
  if (start < 1 ) start = 1;
  if (stop < 1 ) stop = 0;
  length = stop - start + 1;
  if (length < 0) length = 0;
  sout = pntrTempAlloc(length + 1);
  pntrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_PNTRSTRING;
  return (sout);
}

/* Extract sin from character position start for length len */
pntrString *pntrMid(pntrString *sin, long start, long length)
{
  pntrString *sout;
  if (start < 1) start = 1;
  if (length < 0) length = 0;
  sout = pntrTempAlloc(length + 1);
  pntrNCpy(sout, sin + start-1, length);
  sout[length] = *NULL_PNTRSTRING;
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
/* Each entry in the allocated array points to an empty vString. */
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
  long length;
  pntrString *v;
  length = pntrLen(g);
  v = pntrTempAlloc(length + 2);
  pntrCpy(v, g);
  v[length] = "";
  v[length + 1] = *NULL_PNTRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bbg3: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return(v);
}


/* Add a single null pntrString element to a pntrString -faster than pntrCat */
pntrString *pntrAddGElement(pntrString *g)
{
  long length;
  pntrString *v;
  length = pntrLen(g);
  v = pntrTempAlloc(length + 2);
  pntrCpy(v, g);
  v[length] = NULL_PNTRSTRING;
  v[length + 1] = *NULL_PNTRSTRING;
  return(v);
}


/*******************************************************************/
/*********** Miscellaneous utility functions ***********************/
/*******************************************************************/


/* 0/1 knapsack algorithm */
/* Returns the maximum worth (value) for items that can fit into maxSize */
/* itemIncluded[] will be populated with 'y'/'n' if item included/excluded */
long knapsack01(long items, /* # of items available to populate knapsack */
    long *size, /* size of item 0,...,items-1 */
    long *worth, /* worth (value) of item 0,...,items-1 */
    long maxSize, /* size of knapsack (largest total size that will fit) */
    char *itemIncluded /* output: 'y'/'n' if item 0..items-1 incl/excluded */)
    {
  long witem, wsize, a, b;

  /* Maximum worth that can be attained for given #items and size */
  long **maxWorth;  /* 2d matrix */
  maxWorth = alloc2DMatrix((size_t)items + 1, (size_t)maxSize + 1);

  /* This may run faster for applications that have hard-coded limits
#define KS_MAX_ITEMS 100
#define KS_MAX_SIZE 200
  static long maxWorth[KS_MAX_ITEMS + 1][KS_MAX_SIZE + 1];
  if (items > KS_MAX_ITEMS) {
    printf("matrix item overflow\n"); exit(1);
  }
  if (maxSize > KS_MAX_SIZE) {
    printf("matrix size overflow\n"); exit(1);
  }
  */

  /* Populate the maximum worth matrix */
  for (wsize = 0; wsize <= maxSize; wsize++) {
    maxWorth[0][wsize] = 0;
  }
  for (witem = 1; witem <= items; witem++) {
    for (wsize = 0; wsize <= maxSize; wsize++) {
      if (wsize >= size[witem - 1]) {
        /* Item witem can be part of the solution */
        a = maxWorth[witem - 1][wsize];
        b = maxWorth[witem - 1][wsize - size[witem - 1]] + worth[witem - 1];
        /* Choose the case with greater value */
        maxWorth[witem][wsize] = (a > b) ? a : b;  /* max(a,b) */
      } else {
        /* Item witem can't be part of the solution, otherwise total size
           would exceed the intermediate size wsize. */
        maxWorth[witem][wsize] = maxWorth[witem - 1][wsize];
      }
    }
  }

  /* Find the included items */
  wsize = maxSize;
  for (witem = items; witem > 0; witem--) {
    itemIncluded[witem - 1] = 'n'; /* Initialize as excluded */
    if (wsize > 0) {
      if (maxWorth[witem][wsize] != maxWorth[witem - 1][wsize]) {
        itemIncluded[witem - 1] = 'y'; /* Include the item */
        wsize = wsize - size[witem - 1];
      }
    }
  }

  a = maxWorth[items][maxSize]; /* Final maximum worth */
  free2DMatrix(maxWorth, (size_t)items + 1 /*, maxSize + 1*/);
  return a;
} /* knapsack01 */


/* Allocate a 2-dimensional long integer matrix */
/* Warning:  only entries 0,...,xsize-1 and 0,...,ysize-1 are allocated;
   don't use entry xsize or ysize! */
long **alloc2DMatrix(size_t xsize, size_t ysize)
{
  long **matrix;
  long i;
  matrix = malloc(xsize * sizeof(long *));
  if (matrix == NULL) {
    fprintf(stderr,"?FATAL ERROR 1376 Out of memory\n");
    exit(1);
  }
  for (i = 0; i < (long)xsize; i++) {
    matrix[i] = malloc(ysize * sizeof(long));
    if (matrix[i] == NULL) {
      fprintf(stderr,"?FATAL ERROR 1377 Out of memory\n");
      exit(1);
    }
  }
  return matrix;
} /* alloc2DMatrix */


/* Free a 2-dimensional long integer matrix */
/* Note: the ysize argument isn't used, but is commented out as
   a reminder so the caller doesn't confuse x and y */
void free2DMatrix(long **matrix, size_t xsize /*, size_t ysize*/)
{
  long i;
  for (i = (long)xsize - 1; i >= 0; i--) {
    if (matrix[i] == NULL) bug(1378);
    free(matrix[i]);
  }
  if (matrix == NULL) bug(1379);
  free(matrix);
  return;
} /* free2DMatrix */


/* Returns the amount of indentation of a statement label.  Used to
   determine how much to indent a saved proof. */
long getSourceIndentation(long statemNum) {
  char *fbPtr; /* Source buffer pointer */
  char *startLabel;
  long indentation = 0;

  fbPtr = statement[statemNum].mathSectionPtr;
  if (fbPtr[0] == 0) return 0;
  startLabel = statement[statemNum].labelSectionPtr;
  if (startLabel[0] == 0) return 0;
  while (1) { /* Go back to first line feed prior to the label */
    if (fbPtr <= startLabel) break;
    if (fbPtr[0] == '\n') break;
    if (fbPtr[0] == ' ') {
      indentation++; /* Space increments indentation */
    } else {
      indentation = 0; /* Non-space (i.e. a label character) resets back to 0 */
    }
    fbPtr--;
  }
  return indentation;
} /* getSourceIndentation */


/* Returns the last embedded comment (if any) in the label section of
   a statement.  This is used to provide the user with information in the SHOW
   STATEMENT command.  The caller must deallocate the result. */
vstring getDescription(long statemNum) {
  vstring description = "";
  long p1, p2;

  let(&description, space(statement[statemNum].labelSectionLen));
  memcpy(description, statement[statemNum].labelSectionPtr,
      (size_t)(statement[statemNum].labelSectionLen));
  p1 = rinstr(description, "$(");
  p2 = rinstr(description, "$)");
  if (p1 == 0 || p2 == 0 || p2 < p1) {
    let(&description, "");
    return description;
  }
  let(&description, edit(seg(description, p1 + 2, p2 - 1),
      8 + 128 /* discard leading and trailing blanks */));
  return description;

  /* 3-May-2017 nm Old code may have been somewhat faster, but it doesn't
     work when statement[statemNum].labelSectionChanged */
  /************* deleted *******
  char @fbPtr; /@ Source buffer pointer @/
  vstring description = "";
  char @startDescription;
  char @endDescription;
  char @startLabel;

  fbPtr = statement[statemNum].mathSectionPtr;
  if (!fbPtr[0]) return (description);
  startLabel = statement[statemNum].labelSectionPtr;
  if (!startLabel[0]) return (description);
  endDescription = NULL;
  while (1) { /@ Get end of embedded comment @/
    if (fbPtr <= startLabel) break;
    if (fbPtr[0] == '$' && fbPtr[1] == ')') {
      endDescription = fbPtr;
      break;
    }
    fbPtr--;
  }
  if (!endDescription) return (description); /@ No embedded comment @/
  while (1) { /@ Get start of embedded comment @/
    if (fbPtr < startLabel) bug(216);
    if (fbPtr[0] == '$' && fbPtr[1] == '(') {
      startDescription = fbPtr + 2;
      break;
    }
    fbPtr--;
  }
  let(&description, space(endDescription - startDescription));
  memcpy(description, startDescription,
      (size_t)(endDescription - startDescription));
  if (description[endDescription - startDescription - 1] == '\n') {
    /@ Trim trailing new line @/
    let(&description, left(description, endDescription - startDescription - 1));
  }
  /@ Discard leading and trailing blanks @/
  let(&description, edit(description, 8 + 128));
  return (description);
  *********** end of 3-May-2017 deletion *****/

} /* getDescription */



/* Returns 0 or 1 to indicate absence or presence of an indicator in
   the comment of the statement. */
/* mode = 1 = PROOF_DISCOURAGED means get any proof modification discouraged
                indicator
   mode = 2 = USAGE_DISCOURAGED means get any new usage discouraged indicator
   mode = 0 = RESET  means to reset everything (statemeNum is ignored) */
/* TODO: add a mode to reset a single statement if in the future we add
   the ability to change the markup within the program. */
flag getMarkupFlag(long statemNum, flag mode) {
  /* For speedup, the algorithm searches a statement's comment for markup
     matches only the first time, then saves the result for subsequent calls
     for that statement. */
  static char init = 0;
  static vstring commentSearchedFlags = ""; /* Y if comment was searched */
  static vstring proofFlags = "";  /* Y if proof discouragement, else N */
  static vstring usageFlags = "";  /* Y if usage discouragement, else N */
  vstring str1 = "";
  /* These are global in mmdata.h
#define PROOF_DISCOURAGED_MARKUP "(Proof modification is discouraged.)"
#define USAGE_DISCOURAGED_MARKUP "(New usage is discouraged.)"
  extern vstring proofDiscouragedMarkup;
  extern vstring usageDiscouragedMarkup;
  */

  if (mode == RESET) { /* Deallocate */ /* Should be called by ERASE command */
    let(&commentSearchedFlags, "");
    let(&proofFlags, "");
    let(&usageFlags, "");
    init = 0;
    return 0;
  }

  if (init == 0) {
    init = 1;
    /* The global variables proofDiscouragedMarkup and usageDiscouragedMarkup
       are initialized to "" like all vstrings to allow them to be reassigned
       by a possible future SET command.  So the first time this is called
       we need to assign them to the default markup strings. */
    if (proofDiscouragedMarkup[0] == 0) {
      let(&proofDiscouragedMarkup, PROOF_DISCOURAGED_MARKUP);
    }
    if (usageDiscouragedMarkup[0] == 0) {
      let(&usageDiscouragedMarkup, USAGE_DISCOURAGED_MARKUP);
    }
    /* Initialize flag strings */
    let(&commentSearchedFlags, string(statements + 1, 'N'));
    let(&proofFlags, space(statements + 1));
    let(&usageFlags, space(statements + 1));
  }

  if (statemNum < 1 || statemNum > statements) bug(1392);

  if (commentSearchedFlags[statemNum] == 'N') {
    if (statement[statemNum].type == f_
        || statement[statemNum].type == e_ /* 24-May-2016 nm */ ) {
      /* Any comment before a $f, $e statement is assumed irrelevant */
      proofFlags[statemNum] = 'N';
      usageFlags[statemNum] = 'N';
    } else {
      if (statement[statemNum].type != a_ && statement[statemNum].type != p_) {
        bug(1393);
      }
      str1 = getDescription(statemNum);  /* str1 must be deallocated here */
      /* Strip linefeeds and reduce spaces */
      let(&str1, edit(str1, 4 + 8 + 16 + 128));
      if (instr(1, str1, proofDiscouragedMarkup)) {
        proofFlags[statemNum] = 'Y';
      } else {
        proofFlags[statemNum] = 'N';
      }
      if (instr(1, str1, usageDiscouragedMarkup)) {
        usageFlags[statemNum] = 'Y';
      } else {
        usageFlags[statemNum] = 'N';
      }
      let(&str1, ""); /* Deallocate */
    }
    commentSearchedFlags[statemNum] = 'Y';
  }

  if (mode == PROOF_DISCOURAGED) return (proofFlags[statemNum] == 'Y') ? 1 : 0;
  if (mode == USAGE_DISCOURAGED) return (usageFlags[statemNum] == 'Y') ? 1 : 0;
  bug(1394);
  return 0;
} /* getMarkupFlag */


/* 7-Nov-2015 nm */

/* Extract contributor or date from statement description per the
   following mode argument:

       CONTRIBUTOR 1
       CONTRIB_DATE 2
       REVISER 3
       REVISE_DATE 4
       SHORTENER 5
       SHORTEN_DATE 6
       MOST_RECENT_DATE 7

   When an item above is missing, the empty string is returned for that item.
   The following utility modes are available:

       GC_ERROR_CHECK_SILENT 8
       GC_ERROR_CHECK_PRINT 9
       GC_RESET 0
       GC_RESET_STMT 10

   For GC_ERROR_CHECK_SILENT and GC_ERROR_CHECK_PRINT, a "F" is returned if
   error-checking fails, otherwise "P" is returned.  GC_ERROR_CHECK_PRINT also
   prints the errors found.

   GC_RESET clears the cache and returns the empty string.  It is normally
   used by the ERASE command.  The stmtNum argument should be 0.  The
   empty string is returned.

   GC_RESET_STMT re-initializes the cache for the specified statement only.
   It should be called whenever the labelSection is changed e.g. by
   SAVE PROOF.  The empty string is returned.
*/
/* 3-May-2017 nm Changed to return a single result each call, to allow
   expandability in the future */
/* The caller must deallocate the returned string. */
vstring getContrib(long stmtNum, char mode) {

/******** deleted 3-May-2017
flag getContrib(long stmtNum,
    vstring @contributor, vstring @contribDate,
    vstring @reviser, vstring @reviseDate,
    vstring @shortener, vstring @shortenDate,
    vstring @mostRecentDate, /@ The most recent of all 3 dates @/
    flag printErrorsFlag,
    flag mode /@ 0 == RESET = reset, 1 = normal @/ /@ 2-May-2017 nm @/) {
*******/
  /* 2-May-2017 nm */
  /* For speedup, the algorithm searches a statement's comment for markup
     matches only the first time, then saves the result for subsequent calls
     for that statement. */
  static char init = 0;

  vstring contributor = "";
  vstring contribDate = "";
  vstring reviser = "";
  vstring reviseDate = "";
  vstring shortener = "";
  vstring shortenDate = "";
  vstring mostRecentDate = "";   /* The most recent of all 3 dates */

  static vstring commentSearchedFlags = ""; /* Y if comment was searched */
  static pntrString *contributorList = NULL_PNTRSTRING;
  static pntrString *contribDateList = NULL_PNTRSTRING;
  static pntrString *reviserList = NULL_PNTRSTRING;
  static pntrString *reviseDateList = NULL_PNTRSTRING;
  static pntrString *shortenerList = NULL_PNTRSTRING;
  static pntrString *shortenDateList = NULL_PNTRSTRING;
  static pntrString *mostRecentDateList = NULL_PNTRSTRING;

  long cStart = 0, cMid = 0, cEnd = 0;
  long rStart = 0, rMid = 0, rEnd = 0;
  long sStart = 0, sMid = 0, sEnd = 0;
  long firstR = 0, firstS = 0;
  vstring description = "";
  vstring tmpDate0 = "";
  vstring tmpDate1 = "";
  vstring tmpDate2 = "";
  long stmt, p, dd, mmm, yyyy;
  flag errorCheckFlag = 0;
  flag err = 0;
  vstring returnStr = ""; /* Return value */
#define CONTRIB_MATCH " (Contributed by "
#define REVISE_MATCH " (Revised by "
#define SHORTEN_MATCH " (Proof shortened by "
#define END_MATCH ".) "

  /* 3-May-2017 nm */
  if (mode == GC_ERROR_CHECK_SILENT || mode == GC_ERROR_CHECK_PRINT) {
    errorCheckFlag = 1;
  }

  /* 2-May-2017 nm */
  if (mode == GC_RESET) {
    /* This is normally called by the ERASE command only */
    if (init != 0) {
      if ((long)strlen(commentSearchedFlags) != statements + 1) {
        bug(1395);
      }
      if (stmtNum != 0) {
        bug(1400);
      }
      for (stmt = 1; stmt <= statements; stmt++) {
        if (commentSearchedFlags[stmt] == 'Y') {
          /* Deallocate cached strings */
          let((vstring *)(&(contributorList[stmt])), "");
          let((vstring *)(&(contribDateList[stmt])), "");
          let((vstring *)(&(reviserList[stmt])), "");
          let((vstring *)(&(reviseDateList[stmt])), "");
          let((vstring *)(&(shortenerList[stmt])), "");
          let((vstring *)(&(shortenDateList[stmt])), "");
          let((vstring *)(&(mostRecentDateList[stmt])), "");
        }
      }
      /* Deallocate the lists of pointers to cached strings */
      pntrLet(&contributorList, NULL_PNTRSTRING);
      pntrLet(&contribDateList, NULL_PNTRSTRING);
      pntrLet(&reviserList, NULL_PNTRSTRING);
      pntrLet(&reviseDateList, NULL_PNTRSTRING);
      pntrLet(&shortenerList, NULL_PNTRSTRING);
      pntrLet(&shortenDateList, NULL_PNTRSTRING);
      pntrLet(&mostRecentDateList, NULL_PNTRSTRING);
      let(&commentSearchedFlags, "");
      init = 0;
    } /* if (init != 0) */
    return "";
  }

  /* 3-May-2017 nm */
  if (mode == GC_RESET_STMT) {
    /* This should be called whenever the labelSection is changed e.g. by
       SAVE PROOF. */
    if (init != 0) {
      if ((long)strlen(commentSearchedFlags) != statements + 1) {
        bug(1398);
      }
      if (stmtNum < 1 || stmtNum > statements + 1) {
        bug(1399);
      }
      if (commentSearchedFlags[stmtNum] == 'Y') {
        /* Deallocate cached strings */
        let((vstring *)(&(contributorList[stmtNum])), "");
        let((vstring *)(&(contribDateList[stmtNum])), "");
        let((vstring *)(&(reviserList[stmtNum])), "");
        let((vstring *)(&(reviseDateList[stmtNum])), "");
        let((vstring *)(&(shortenerList[stmtNum])), "");
        let((vstring *)(&(shortenDateList[stmtNum])), "");
        let((vstring *)(&(mostRecentDateList[stmtNum])), "");
        commentSearchedFlags[stmtNum] = 'N';
      }
    } /* if (init != 0) */
    return "";
  }

  /* We now check only $a and $p statements - should we do others? */
  if (statement[stmtNum].type != a_ && statement[stmtNum].type != p_) {
    goto RETURN_POINT;
  }

  if (init == 0) {
    init = 1;
    /* Initialize flag string */
    let(&commentSearchedFlags, string(statements + 1, 'N'));
    /* Initialize pointers to "" (null vstring) */
    pntrLet(&contributorList, pntrSpace(statements + 1));
    pntrLet(&contribDateList, pntrSpace(statements + 1));
    pntrLet(&reviserList, pntrSpace(statements + 1));
    pntrLet(&reviseDateList, pntrSpace(statements + 1));
    pntrLet(&shortenerList, pntrSpace(statements + 1));
    pntrLet(&shortenDateList, pntrSpace(statements + 1));
    pntrLet(&mostRecentDateList, pntrSpace(statements + 1));
  }

  if (stmtNum < 1 || stmtNum > statements) bug(1396);

  if (commentSearchedFlags[stmtNum] == 'N' /* Not in cache */
      || errorCheckFlag == 1 /* Needed to get sStart, rStart, cStart */) {
    /* It wasn't cached, so we extract from the statement's comment */

    let(&description, "");
    description = getDescription(stmtNum);
    let(&description, edit(description,
        4/*ctrl*/ + 8/*leading*/ + 16/*reduce*/ + 128/*trailing*/));
    let(&description, cat(" ", description, " ", NULL)); /* Add for matching */

    cStart = instr(1, description, CONTRIB_MATCH);
    if (cStart != 0) {
      cStart = cStart + (long)strlen(CONTRIB_MATCH); /* Start of contributor */
      cEnd = instr(cStart, description, END_MATCH); /* End of date */
      cMid = cEnd; /* After end of contributor and before start of date */
      if (cMid != 0) {
        while (description[cMid - 1] != ' ') {
          cMid--;
          if (cMid == 0) break;
        }
      }
      /* We assign contributorList entry instead of contributor,
         contribDateList entry instead of contribDate, etc. in case the
         same string variable is used for several arguments for convenience
         (e.g. to avoid having to declare 7 string variables if only one date
         is needed) */
      let((vstring *)(&(contributorList[stmtNum])),
          seg(description, cStart, cMid - 2));
      let((vstring *)(&(contribDateList[stmtNum])),
          seg(description, cMid + 1, cEnd - 1));
    } else {
      /* The contributorList etc. are already initialized to the empty
         string, so we don't have to assign them here. */
      /*
      let((vstring *)(&(contributorList[stmtNum])), "");
      let((vstring *)(&(contribDateList[stmtNum])), "");
      */
    }

    rStart = 0;
    do {  /* Get the last revision entry */
      p = instr(rStart + 1, description, REVISE_MATCH);
      if (p != 0) {
        rStart = p;
        if (firstR == 0) firstR = p + (long)strlen(REVISE_MATCH);
                               /* Add the strlen so to later compare to rStart */
      }
    } while (p != 0);
    if (rStart != 0) {
      rStart = rStart + (long)strlen(REVISE_MATCH); /* Start of reviser */
      rEnd = instr(rStart, description, END_MATCH); /* End of date */
      rMid = rEnd; /* After end of reviser and before start of date */
      if (rMid != 0) {
        while (description[rMid - 1] != ' ') {
          rMid--;
          if (rMid == 0) break;
        }
      }
      let((vstring *)(&(reviserList[stmtNum])),
          seg(description, rStart, rMid - 2));
      let((vstring *)(&(reviseDateList[stmtNum])),
          seg(description, rMid + 1, rEnd - 1));
    } else {
      /* redundant; already done by init
      let((vstring *)(&(reviserList[stmtNum])), "");
      let((vstring *)(&(reviseDateList[stmtNum])), "");
      */
    }

    sStart = 0;
    do {  /* Get the last shorten entry */
      p = instr(sStart + 1, description, SHORTEN_MATCH);
      if (p != 0) {
        sStart = p;
        if (firstS == 0) firstS = p + (long)strlen(SHORTEN_MATCH);
                               /* Add the strlen so to later compare to rStart */
      }
    } while (p != 0);
    if (sStart != 0) {
      sStart = sStart + (long)strlen(SHORTEN_MATCH); /* Start of shortener */
      sEnd = instr(sStart, description, END_MATCH); /* End of date */
      sMid = sEnd; /* After end of shortener and before start of date */
      if (sMid != 0) {
        while (description[sMid - 1] != ' ') {
          sMid--;
          if (sMid == 0) break;
        }
      }
      let((vstring *)(&(shortenerList[stmtNum])),
          seg(description, sStart, sMid - 2));
      let((vstring *)(&(shortenDateList[stmtNum])),
         seg(description, sMid + 1, sEnd - 1));
    } else {
      /* redundant; already done by init
      let((vstring *)(&(shortenerList[stmtNum])), "");
      let((vstring *)(&(shortenDateList[stmtNum])), "");
      */
    }


    /* 13-Dec-2016 nm Get the most recent date */
    let((vstring *)(&(mostRecentDateList[stmtNum])),
        (vstring)(contribDateList[stmtNum]));
    /* Note that compareDate() treats empty string as earliest date */
    if (compareDates((vstring)(mostRecentDateList[stmtNum]),
        (vstring)(reviseDateList[stmtNum])) == -1) {
      let((vstring *)(&(mostRecentDateList[stmtNum])),
          (vstring)(reviseDateList[stmtNum]));
    }
    if (compareDates((vstring)(mostRecentDateList[stmtNum]),
        (vstring)(shortenDateList[stmtNum])) == -1) {
      let((vstring *)(&(mostRecentDateList[stmtNum])),
          (vstring)(shortenDateList[stmtNum]));
    }

    /* 2-May-2017 nm Tag the cache entry as updated */
    commentSearchedFlags[stmtNum] = 'Y';
  } /* commentSearchedFlags[stmtNum] == 'N' || errorCheckFlag == 1 */

  /* Assign the output strings from the cache */
  if (errorCheckFlag == 1) {
    let(&contributor, (vstring)(contributorList[stmtNum]));
    let(&contribDate, (vstring)(contribDateList[stmtNum]));
    let(&reviser, (vstring)(reviserList[stmtNum]));
    let(&reviseDate, (vstring)(reviseDateList[stmtNum]));
    let(&shortener, (vstring)(shortenerList[stmtNum]));
    let(&shortenDate, (vstring)(shortenDateList[stmtNum]));
    let(&mostRecentDate, (vstring)(mostRecentDateList[stmtNum]));
  } else {
    /* Assign only the requested field for faster speed */
    switch (mode) {
      case CONTRIBUTOR:
        let(&returnStr, (vstring)(contributorList[stmtNum])); break;
      case CONTRIB_DATE:
        let(&returnStr, (vstring)(contribDateList[stmtNum])); break;
      case REVISER:
        let(&returnStr, (vstring)(reviserList[stmtNum])); break;
      case REVISE_DATE:
        let(&returnStr, (vstring)(reviseDateList[stmtNum])); break;
      case SHORTENER:
        let(&returnStr, (vstring)(shortenerList[stmtNum])); break;
      case SHORTEN_DATE:
        let(&returnStr, (vstring)(shortenDateList[stmtNum])); break;
      case MOST_RECENT_DATE:
        let(&returnStr, (vstring)(mostRecentDateList[stmtNum])); break;
      default: bug(1397); /* Any future modes should be added here */
    } /* end switch (mode) */
  }

  /* Skip error checking for speedup if we're not printing errors */
  if (errorCheckFlag == 0) goto RETURN_POINT;

  /* For error checking, we don't require dates in syntax statements
     (**** Note that this is set.mm-specific! ****) */
  if (statement[stmtNum].type == a_   /* Don't check syntax statements */
      && strcmp(left(statement[stmtNum].labelName, 3), "df-")
      && strcmp(left(statement[stmtNum].labelName, 3), "ax-")) {
    goto RETURN_POINT;
  }

  if (cStart == 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  There is no \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (instr(cStart + 1, description, CONTRIB_MATCH) != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  There is more than one \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" ",
        "in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  /* 3-May-2017 nm */
  if (cStart != 0 && description[cMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is no comma between contributor and date",
        ", or period is missing,",   /* 5-Aug-2017 nm */
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (rStart != 0 && description[rMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is no comma between reviser and date",
        ", or period is missing,",   /* 5-Aug-2017 nm */
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (sStart != 0 && description[sMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is no comma between proof shortener and date",
        ", or period is missing,",   /* 5-Aug-2017 nm */
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, contributor, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is a comma in the contributor name \"",
        contributor,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, reviser, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is a comma in the reviser name \"",
        reviser,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, shortener, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning:  There is a comma in the proof shortener name \"",
        shortener,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }


  /*********  Turn off this warning unless we decide not to allow this
  if ((firstR != rStart) || (firstS != sStart)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /@ convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        @contributor, "/", @reviser, "/", @shortener, "] ",
        @/
        "?Warning: There are multiple \"",
        edit(REVISE_MATCH, 8+128) , "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128) ,
        "...)\" entries in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        "  The last one of each type was used.",
        NULL), "    ", " ");
  }
  *********/

  if ((firstR != 0 && firstR < cStart)
      || (firstS != 0 && firstS < cStart)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" is placed after \"",
        edit(REVISE_MATCH, 8+128) , "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128) ,
        "...)\" in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if ((cStart !=0 && (cMid == 0 || cEnd == 0 || cMid == cEnd
          || contributor[0] == 0 || contribDate[0] == 0))
      || (rStart !=0 && (rMid == 0 || rEnd == 0 || rMid == rEnd
          || reviser[0] == 0 || reviseDate[0] == 0))
      || (sStart !=0 && (sMid == 0 || sEnd == 0 || sMid == sEnd
          || shortener[0] == 0 || shortenDate[0] == 0))) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: There is a formatting error in a",
        " \"", edit(CONTRIB_MATCH, 8+128),  "...)\", \"",
        edit(REVISE_MATCH, 8+128) , "...)\", or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" entry in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (contribDate[0] != 0) {
    parseDate(contribDate, &dd, &mmm, &yyyy);
    buildDate(dd, mmm, yyyy, &tmpDate0);
    if (strcmp(contribDate, tmpDate0)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
          /* convenience prefix to assist massive revisions
          statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(CONTRIB_MATCH, 8+128),  "...)\" date \"", contribDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
          NULL), "    ", " ");
    }
  }

  if (reviseDate[0] != 0) {
    parseDate(reviseDate, &dd, &mmm, &yyyy);
    buildDate(dd, mmm, yyyy, &tmpDate0);
    if (strcmp(reviseDate, tmpDate0)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
          /* convenience prefix to assist massive revisions
          statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(REVISE_MATCH, 8+128) , "...)\" date \"", reviseDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
          NULL), "    ", " ");
    }
  }

  if (shortenDate[0] != 0) {
    parseDate(shortenDate, &dd, &mmm, &yyyy);
    buildDate(dd, mmm, yyyy, &tmpDate0);
    if (strcmp(shortenDate, tmpDate0)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
          /* convenience prefix to assist massive revisions
          statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(SHORTEN_MATCH, 8+128) , "...)\" date \"", shortenDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
          NULL), "    ", " ");
    }
  }

  if (contribDate[0] != 0 &&
     ((reviseDate[0] != 0
         && compareDates(contribDate, reviseDate) != -1)
     || (shortenDate[0] != 0
         && compareDates(contribDate, shortenDate) != -1))) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" date is not earlier than the \"",
        edit(REVISE_MATCH, 8+128), "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" date in the comment above statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (reviseDate[0] != 0 && shortenDate[0] != 0) {
    p = compareDates(reviseDate, shortenDate);
    if ((rStart < sStart && p == 1)
        || (rStart > sStart && p == -1)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
          "?Warning:  The \"", edit(REVISE_MATCH, 8+128), "...)\" and \"",
          edit(SHORTEN_MATCH, 8+128),
         "...)\" dates are in the wrong order in the comment above statement ",
          str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
          NULL), "    ", " ");
    }
  }

  #ifdef DATE_BELOW_PROOF /* 12-May-2017 nm */

  /* TODO ******** The rest of the checks should be deleted if we decide
     to drop the date after the proof */
  if (statement[stmtNum].type != p_) {
    goto RETURN_POINT;
  }
  getProofDate(stmtNum, &tmpDate1, &tmpDate2);
  if (tmpDate1[0] == 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  There is no date below the proof in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] == 0
      && (reviseDate[0] != 0 || shortenDate[0] != 0)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The comment has \"",
        edit(REVISE_MATCH, 8+128), "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" but there is only one date below the proof",
        " in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] != 0 && reviseDate[0] == 0 && shortenDate[0] == 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  There are two dates below the proof but no \"",
        edit(REVISE_MATCH, 8+128), "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" entry in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] != 0
      && (reviseDate[0] != 0 || shortenDate[0] != 0)
      && strcmp(tmpDate1, reviseDate)
      && strcmp(tmpDate1, shortenDate)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  Neither a \"",
        edit(REVISE_MATCH, 8+128), "...)\" date ",
        "nor a \"", edit(SHORTEN_MATCH, 8+128), "...)\" date ",
        "matches the date ", tmpDate1,
        " below the proof in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] != 0
      && reviseDate[0] != 0
      && compareDates(tmpDate1, reviseDate) == -1) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The \"",
        edit(REVISE_MATCH, 8+128), "...)\" date ", reviseDate,
        " is later than the date ", tmpDate1,
        " below the proof in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] != 0
      && shortenDate[0] != 0
      && compareDates(tmpDate1, shortenDate) == -1) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The \"",
        edit(SHORTEN_MATCH, 8+128), "...)\" date ", shortenDate,
        " is later than the date ", tmpDate1,
        " below the proof in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] != 0 && compareDates(tmpDate2, tmpDate1) != -1) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The first date below the proof, ", tmpDate1,
        ", is not newer than the second, ", tmpDate2,
        ", in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (tmpDate2[0] == 0) {
    let(&tmpDate0, tmpDate1);
  } else {
    let(&tmpDate0, tmpDate2);
  }
  if (contribDate[0] != 0
      && tmpDate0[0] != 0 && strcmp(contribDate, tmpDate0)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning:  The \"", edit(CONTRIB_MATCH, 8+128), "...)\" date ",
        contribDate,
        " doesn't match the date ", tmpDate0,
        " below the proof in statement ",
        str((double)stmtNum), ", label \"", statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  /***** End of section to delete if date after proof is dropped */
#endif /* #ifdef DATE_BELOW_PROOF */

  if (err == 1) {
    let(&returnStr, "F");  /* fail */
  } else {
    let(&returnStr, "P");  /* pass */
  }


 RETURN_POINT:

  let(&description, "");

  if (errorCheckFlag == 1) { /* Slight speedup */
    let(&contributor, "");
    let(&contribDate, "");
    let(&reviser, "");
    let(&reviseDate, "");
    let(&shortener, "");
    let(&shortenDate, "");
    let(&mostRecentDate, "");
    let(&tmpDate0, "");
    let(&tmpDate1, "");
    let(&tmpDate2, "");
  }

  return returnStr;
} /* getContrib */


/*#ifdef DATE_BELOW_PROOF*/ /* 12-May-2017 nm */
/* 14-May-2017 nm - re-enabled (temporarily?) for converting old .mm's */
/* 4-Nov-2015 nm */
/* Extract up to 2 dates after a statement's proof.  If no date is present,
   date1 will be blank.  If no 2nd date is present, date2 will be blank.
   THIS WILL BECOME OBSOLETE WHEN WE START TO USE DATES IN THE
   DESCRIPTION. */
void getProofDate(long stmtNum, vstring *date1, vstring *date2) {
  vstring textAfterProof = "";
  long p1, p2;
  let(&textAfterProof, space(statement[stmtNum + 1].labelSectionLen));
  memcpy(textAfterProof, statement[stmtNum + 1].labelSectionPtr,
      (size_t)(statement[stmtNum + 1].labelSectionLen));
  let(&textAfterProof, edit(textAfterProof, 2)); /* Discard spaces and tabs */
  p1 = instr(1, textAfterProof, "$([");
  p2 = instr(p1, textAfterProof, "]$)");
  if (p1 && p2) {
    let(&(*date1), seg(textAfterProof, p1 + 3, p2 - 1));  /* 1st date stamp */
    p1 = instr(p2, textAfterProof, "$([");
    p2 = instr(p1, textAfterProof, "]$)");
    if (p1 && p2) {
      let(&(*date2), seg(textAfterProof, p1 + 3, p2 - 1)); /* 2nd date stamp */
    } else {
      let(&(*date2), ""); /* No 2nd date stamp */
    }
  } else {
    let(&(*date1), ""); /* No 1st or 2nd date stamp */
    let(&(*date2), "");
  }
  let(&textAfterProof, ""); /* Deallocate */
  return;
} /* getProofDate */

/*#endif*/ /*#ifdef DATE_BELOW_PROOF*/ /* 12-May-2017 nm */


/* 4-Nov-2015 nm */
/* Get date, month, year fields from a dd-mmm-yyyy date string,
   where dd may be 1 or 2 digits, mmm is 1st 3 letters of month,
   and yyyy is 2 or 4 digits.  A 1 is returned if an error was detected. */
flag parseDate(vstring dateStr, long *dd, long *mmm, long *yyyy) {
  long j;
  flag err = 0;
  j = instr(1, dateStr, "-");
  *dd = (long)val(left(dateStr, j - 1)); /* Day */
#define MONTHS "JanFebMarAprMayJunJulAugSepOctNovDec"
  *mmm = ((instr(1, MONTHS, mid(dateStr, j + 1, 3)) - 1) / 3) + 1; /* 1 = Jan */
  j = instr(j + 1, dateStr, "-");
  *yyyy = (long)val(right(dateStr, j + 1));
  if (*yyyy < 100) { /* 2-digit year (obsolete) */
#define START_YEAR 93 /* Earliest 19xx year in set.mm database */
    if (*yyyy < START_YEAR) {
      *yyyy = *yyyy + 2000;
    } else {
      *yyyy = *yyyy + 1900;
    }
  }
  if (*dd < 1 || *dd > 31 || *mmm < 1 || *mmm > 12) err = 1; /* 13-Dec-2016 nm */
  return err;
} /* parseDate */


/* 4-Nov-2015 nm */
/* Build date from numeric fields.  mmm should be a number from 1 to 12.
   There is no error-checking. */
void buildDate(long dd, long mmm, long yyyy, vstring *dateStr) {
  let(&(*dateStr), cat(str((double)dd), "-", mid(MONTHS, mmm * 3 - 2, 3), "-",
      str((double)yyyy), NULL));
  return;
} /* buildDate */


/* 4-Nov-2015 nm */
/* Compare two dates in the form dd-mmm-yyyy.  -1 = date1 < date2,
   0 = date1 = date2,  1 = date1 > date2.  There is no error checking. */
flag compareDates(vstring date1, vstring date2) {
  long d1, m1, y1, d2, m2, y2, dd1, dd2;

  /* 13-Dec-2016 nm */
  /* If a date is the empty string, treat it as being _before_ any other
     date */
  if (date1[0] == 0 || date2[0] == 0) {
    if (date1[0] == 0 && date2[0] == 0) {
      return 0;
    } else if (date1[0] == 0) {
      return -1;
    } else {
      return 1;
    }
  }

  parseDate(date1, &d1, &m1, &y1);
  parseDate(date2, &d2, &m2, &y2);
  /* dd1, dd2 increase monotonically but aren't true days since 1-Jan-0000 */
  dd1 = d1 + m1 * 32 + y1 * 500;
  dd2 = d2 + m2 * 32 + y2 * 500;
  if (dd1 < dd2) {
    return -1;
  } else if (dd1 == dd2) {
    return 0;
  } else {
    return 1;
  }
} /* compareDates */




/* 17-Nov-2015 Moved out of metamath.c for better modularization */
/* Compare strings via pointers for qsort */
/* qsortKey is a global string key at which the sort starts; if empty,
   start at the beginning of each line. */
int qsortStringCmp(const void *p1, const void *p2)
{
  vstring tmp = "";
  long n1, n2;
  int r;
  /* Returns -1 if p1 < p2, 0 if equal, 1 if p1 > p2 */
  if (qsortKey[0] == 0) {
    /* No key, use full line */
    return strcmp(*(char * const *)p1, *(char * const *)p2);
  } else {
    n1 = instr(1, *(char * const *)p1, qsortKey);
    n2 = instr(1, *(char * const *)p2, qsortKey);
    r = strcmp(
        right(*(char * const *)p1, n1),
        right(*(char * const *)p2, n2));
    let(&tmp, ""); /* Deallocate temp string stack */
    return r;
  }
}

/* 4-May-2017 Ari Ferrera */
void freeData() {
  free(includeCall);
  free(statement);
  free(mathToken);
  free(memFreePool);
  free(memUsedPool);
}
