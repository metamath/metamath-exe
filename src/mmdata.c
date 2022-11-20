/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*
mmdata.c
*/

/*!
 * \file
 * Defines global data structures and manipulates arrays with functions similar
 * to BASIC string functions; memory management; converts between proof formats
*/
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmcmdl.h" /* Needed for g_logFileName */
#include "mmpfas.h" /* Needed for g_proveStatement */
#include "mmwtex.h" /* Needed for SMALL_DECORATION etc. */

/*E*/long db=0,db0=0,db2=0,db3=0,db4=0,db5=0,db6=0,db7=0,db8=0,db9=0;
flag g_listMode = 0; /* 0 = metamath, 1 = list utility */
flag g_toolsMode = 0; /* In metamath: 0 = metamath, 1 = text tools utility */


/* For use by getMarkupFlag() */
vstring_def(g_proofDiscouragedMarkup);
vstring_def(g_usageDiscouragedMarkup);
flag g_globalDiscouragement = 1; /* SET DISCOURAGEMENT ON */

vstring_def(g_contributorName);

/* Global variables related to current statement */
int g_currentScope = 0;

long g_MAX_STATEMENTS = 1;
long g_MAX_MATHTOKENS = 1;
long g_MAX_INCLUDECALLS = 2; /* Must be at least 2 (the single-file case) !!!
                         (A dummy extra top entry is used by parseKeywords().) */
struct statement_struct *g_Statement = NULL;
long *g_labelKey = NULL;
struct mathToken_struct *g_MathToken;
long *g_mathKey = NULL;
long g_statements = 0, labels = 0, g_mathTokens = 0;

struct includeCall_struct *g_IncludeCall = NULL;
long g_includeCalls = -1;  /* For eraseSource() in mmcmds.c */

char *g_sourcePtr = NULL;
long g_sourceLen;

/* Null nmbrString */
struct nullNmbrStruct g_NmbrNull = {-1, sizeof(long), sizeof(long), -1};

/* Null pntrString */
struct nullPntrStruct g_PntrNull = {-1, sizeof(long), sizeof(long), NULL};

temp_nmbrString *nmbrTempAlloc(long size);
        /* nmbrString memory allocation/deallocation */
void nmbrCpy(nmbrString *sout, const nmbrString *sin);
void nmbrNCpy(nmbrString *s, const nmbrString *t, long n);

temp_pntrString *pntrTempAlloc(long size);
        /* pntrString memory allocation/deallocation */
void pntrCpy(pntrString *sout, const pntrString *sin);
void pntrNCpy(pntrString *s, const pntrString *t, long n);

vstring g_qsortKey; /* Used by qsortStringCmp; pointer only, do not deallocate */

/*!
 * \page pgSuballocator Suballocator
 *
 * Metamath does not free memory by returning it to the operating system again.
 * To reduce frequent system de/allocation calls, it instead implements a
 * suballocator.  Each chunk of memory allocated from the system (we call them
 * \ref pgBlock "block" in this documentation) is equipped with a hidden header
 * containing administrative information private to the suballocator.
 *
 * During execution chunks of memory, either complete \ref pgBlock "blocks" or
 * \ref pgFragmentation "fragments" thereof, become free again.  The
 * suballocator adds them then to internal **pools** for reuse, one dedicated
 * to totally free blocks (\ref memFreePool), the other to fragmented ones
 * (\ref memUsedPool).  We call these pools **free block array** and
 * **used block array** in this documentation.  Fully occupied blocks are not
 * tracked by the suballocator.
 *
 * Although the suballocator tries to avoid returning memory to the system, it
 * can do so under extreme memory constraints, or when built-in limits are
 * surpassed.
 *
 * The suballocator was designed with stack-like memory usage in mind, where
 * data of the same type is pushed at, or popped off the end all the time.
 * Each \ref pgBlock block supports this kind of usage out of the box (see
 * \ref pgFragmentation).
 */

/*!
 * \page pgFragmentation Fragmented blocks
 *
 * Memory fragmentation is kept simple in Metamath.  If a \ref pgBlock "block"
 * contains both consumed and free space, all the free space is at the end.
 * This scheme supports the idea of stack-like memory usage, where free space
 * grows and shrinks behind a stack in a fixed size memory area, depending on
 * its usage.
 *
 * Other types of fragmentation is not directly supported by the
 * \ref pgSuballocator "suballocator".
 */

 /*! \page pgBlock Block of memory
 *
 * Each block used by the \ref pgSuballocator "suballocator" is formally´
 * treated as an array of pointer (void*).  It is divided into an
 * administrative header, followed by elements reserved for application data.
 * The header is assigned elements -3 to -1 in the formal array, so that
 * application data starts with element 0.  A **pointer to the block** always
 * refers to element 0, so the header appears somewhat hidden.  Its **size**
 * is given by the bytes reserved for application data, not including the
 * administrative header.
 *
 * The header elements are formally void*, but reinterpreted as long integer.
 * The values support a stack, where data is pushed at and popped off the end
 * during the course of execution.  The semantics of the header elements are:
 *
 * offset -1:\n
 *   is the current size of the stack (in bytes, not elements!),
 *   without header data. When interpreted as an offset into the stack, it
 *   references the first element past the top of the stack.  (See
 *   \ref pgFragmentation)
 *
 * offset -2:\n
 *   the allocated size of the array, in bytes, not counting the
 *   header.  When used as a stack, it marks the limit where the stack
 *   overflows.
 *
 * offset -3:\n
 *   If this block has free space at the end (is \ref pgFragmentation
 *   "fragmented"), then this value contains its index in the used blocks
 *   array, see \ref memUsedPool.  A value of -1 indicates it is either fully
 *   occupied or totally free.  It is not kept in the used blocks array then.
 *   If this block becomes full in the course of events, it is not
 *   automatically removed from \ref memUsedPool, though.
 */

/*! \page pgPool Pool
 * A pool is an array of pointers pointing to \ref pgBlock "blocks".  It may
 * only be partially filled, so it is usually accompanied by two variables
 * giving its current fill state and its capacity.
 *
 * In Metamath a pool has no gaps in between.
 *
 * The \ref pgSuballocator "suballocator" uses two pools:
 * - the **free block array** pointed to by \ref memFreePool;
 * - the **used block array** pointed to by \ref memUsedPool.
 */

/*! \page pgStack Temporary Allocated Memory
 * Very often a routine needs some memory, that must live only as long as the
 * routine is active.  Such memory is called **temporary**, or short **local**.
 * Once a routine finishes, on return to its caller, it deallocates all its
 * __local__ memory again.  Since routines frequently call subroutines, the
 * same may hold for nested code, and so on.  In fact, this concept is so
 * ubiquitous and frequent, that the processor, and all relevant program
 * languages provide simple mechanisms for de/allocation of such __local__
 * data.  Metamath is no exception to this.
 *
 * While the C compiler silently cares about __local__ variables, it must not
 * interfere with data managed by a \ref pgSuballocator "Suballocator". Instead
 * of tracking all __locally__ created memory individually for later
 * deallocation, a stack like \ref pgPool "pool" is used to automate this
 * handling.
 *
 * Stacks of temporary data only contain pointers to dynamically allocated
 * memory from the heap or the \ref pgSuballocator.  This stack functions like
 * an operand stack.  A final result depends on fragments, temporary results
 * and similar, all pushed onto this stack.  When the final operation is
 * executed, and its result is persisted in some variable, the dependency on
 * its temporary operands ceases.  Consequently, they should be freed again.
 * To automate this operation,  such a stack maintains a `start` index.  A
 * client saves this value and sets it to the current stack top, then starts
 * pushing dynamically allocated operands on the stack.  After the result is
 * persisted, all entries beginning with the element at index  `start` are
 * deallocated again, and the stack top is reset to the `start` value, while
 * the `start` value is reset to the saved value, to accommodate nesting of
 * this procedure.
 *
 * This scheme needs a few conditions to be met:
 * - No operand is used in more than one evaluation context;
 * - Operations are executed strictly sequential, or in a nested manner. No two
 *   operations interleave pushing operands.
 */

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
       actual = allocated and storage in memUsedPool is therefore not necessary.
   The pointer to an array always points to element 0 (recast to right size).
*/

/*!
 * \page doc-todo Improvements in documentation
 *
 * - Revisit the \ref pgBlock "block", \ref pgStack "stack" references to check
 *   the inserted wording.
 * - The formatting of __p__ tags seem insufficient.  Figure out whether and
 *   how doxygen allows assigning formats to a semantic tag.  Do not replace
 *   a tag with direct formattings like \p aParam vs `aParam`, as some editors
 *   recognize and highlight semantic tags.
 *     The parameters are included in <code>aParam</code> tags.  You can change
 *     the appearance by using your customized CSS file and let doxygen use it
 *     with HTML-EXTRA-STYLESHEET in your own Doxyfile.
 * - Regularly check the warning in \ref pntrString to see whether it still
 *   holds, or can be made more precise.
 */

/*!
 * \def MEM_POOL_GROW
 * Amount that \ref memUsedPool and \ref memFreePool grows when it overflows.
 */
#define MEM_POOL_GROW 1000
/*??? Let user set this from menu. */

/*!
 * \var long poolAbsoluteMax
 * The value is a memory amount in bytes.
 *
 * The \ref pgSuballocator scheme must not hold more memory than is short term
 * useful.  To the operating system all memory in \ref memFreePool appears as
 * allocated, although it is not really in use.  To prevent the system from
 * taking unnecessary action such as saving RAM to disk, a limit to the amount
 * of free memory managed by the suballocator can be set up.  This limit is
 * checked in frequent operations, and an automatic purge process is initiated
 * in \ref memFreePoolPurge should \ref poolTotalFree exceed this value.
 */
long poolAbsoluteMax = 1000000; /* Pools will be purged when this is reached */

/*!
 * \var long poolTotalFree
 * contains the number of free space available in bytes, in both pools
 * \ref memFreePool and \ref memUsedPool, never counting the hidden headers at
 * the beginning of each block, see \ref pgBlock.  Exceeding
 * \ref poolAbsoluteMax may trigger an automatic purge process by
 * \ref memFreePoolPurge.
 */
long poolTotalFree = 0; /* Total amount of free space allocated in pool */
/*E*/long i1,j1_,k1; /* 'j1' is a built-in function */

/*!
 * \var void** memUsedPool
 * \brief pointer to the pool of fragmented memory blocks
 *
 * If a \ref pgBlock "block" contains both consumed and free space, it is
 * \ref pgFragmentation "fragmented".  All fragmented blocks are kept in the
 * **used block array**, that memUsedPool points to.  See \ref pgSuballocator
 * "suballocator".  Since free space appears at the end of a \ref pgBlock
 * "block", this scheme supports in particular stack like memory, where data is
 * pushed at and popped off the end.
 *
 * The used blocks array does initially not exist.  This is indicated by a
 * null value.  Once this array is needed, space for it is allocated from the
 * system.
 *
 * The used block array may only be partially occupied, in which case elements
 * at the end of the array are unused.  Its current usage is given by
 * \ref memUsedPoolSize.  Its capacity is given by \ref memUsedPoolMax.
 *
 * \attention The pool may contain full \ref pgBlock "blocks".
 *
 * \invariant Each block in the used blocks array has its index noted in its
 * hidden header, for backward reference.
 *
 * \attention Despite the name of this variable, fully occupied blocks are never
 * kept in the used block array.
 */
void **memUsedPool = NULL;

/*!
 * \var long memUsedPoolSize
 * \attention this is the number of individual blocks, not the accumulated
 * (unused) bytes contained.
 *
 * The Metamath suballocator holds fragmented blocks in a used block array.
 * The number of occupied entries is kept in this variable.  Elements at the
 * end of the used block array may be unused.  The fill size is given by this
 * variable.  For further information see \ref memUsedPool.
 *
 * \invariant memUsedPoolSize <= \ref memUsedPoolMax.
 */
long memUsedPoolSize = 0; /* Current # of partially filled arrays in use */

/*!
 * \var long memUsedPoolMax
 * \attention this is the number of individual free blocks, not the accumulated
 * bytes contained.
 *
 * The Metamath suballocator holds fragmented blocks in the used block
 * array.  This array may only partially be occupied.  Its total capacity is
 * kept in this variable.  For further information see \ref memUsedPool.
 *
 * This variable may grow during a reallocation process.
 *
 * \invariant (memUsedPoolMax > 0) == (\ref memUsedPool != 0)
 */
long memUsedPoolMax = 0; /* Maximum # of entries in 'in use' table (grows
                               as necessary) */

/*!
 * \var void** memFreePool
 * \brief pointer to the pool of completely free memory blocks
 *
 * The \ref pgSuballocator "suballocator" is initially not equipped with a
 * **free block array**, pointed to by memFreePool, indicated by a null value.
 *
 * Once a \ref pgBlock "memory block" is returned to the \ref pgSuballocator
 * again, it allocates some space for the now needed array.
 *
 * The **free block array** contains only totally free \ref pgBlock "blocks".
 * This array may only be partially occupied, in which case the elements at the
 * end are the unused ones.  Its current fill size is given by
 * \ref memFreePoolSize.  Its capacity is given by \ref memFreePoolMax.
 *
 * Fragmented blocks are kept in a separate \ref memUsedPool.  The suballocator
 * never tracks fully used blocks.
 */
void **memFreePool = NULL;

/*!
 * \var long memFreePoolSize
 * \attention this is the number of individual free blocks, not the accumulated
 * bytes contained.
 *
 * The Metamath suballocator holds free blocks in a free block array.  The
 * number of occupied entries is kept in this variable.  Elements at the end of
 * the free block array may not be used.  The fill size is given by this
 * variable.  For further information see \ref memFreePool.
 *
 * \invariant memFreePoolSize <= \ref memFreePoolMax.
 */
long memFreePoolSize = 0; /* Current # of available, allocated arrays */

/*!
 * \var long memFreePoolMax
 * \attention this is the number of individual free blocks, not the accumulated
 * bytes contained.
 *
 * The Metamath suballocator holds free blocks in a **free block array**.  It
 * may only be partially occupied.  Its total capacity is kept in this variable.  For
 * further information see \ref memFreePool.
 *
 * This variable may grow during a reallocation process.
 *
 * \invariant (memFreePoolMax > 0) == (\ref memFreePool != 0)
 */
long memFreePoolMax = 0; /* Maximum # of entries in 'free' table (grows
                               as necessary) */

/* poolFixedMalloc should be called when the allocated array will rarely be
   changed; a malloc or realloc with no unused array bytes will be done. */
void *poolFixedMalloc(long size /* bytes */)
{
  void *ptr;
  void *ptr2;
/*E*/ /* Don't call print2() if db9 is set, since it will */
/*E*/ /* recursively call the pool stuff causing a crash. */
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

    if (usedLoc < memUsedPoolSize) {
      memUsedPool[usedLoc] = memUsedPool[memUsedPoolSize];
      ptr1 = memUsedPool[usedLoc];
      ((long *)ptr1)[-3] = usedLoc;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
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
  ((long *)ptr)[-3] = -2;
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

/*!
 * \fn void memFreePoolPurge(flag untilOK)
 * \brief returns memory held in \ref memFreePool
 * Starting with the last entry in \ref memFreePool, memory held in that pool
 * is returned to the system until all, or at least a sufficient amount is
 * freed again (see \p untilOK).
 * \param[in] untilOK
 *   - if 1 freeing \ref pgBlock "blocks" stops the moment \ref poolTotalFree
 *     gets within the range of \ref poolAbsoluteMax again.  Note that it is
 *     not guaranteed that the limit \ref poolAbsoluteMax is undercut because
 *     still too much free memory might be held in the \ref memUsedPool.
 *   - If 0, all \ref memFreePool entries are freed, and the pool itself is
 *     shrunk back to \ref MEM_POOL_GROW size.
 */
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
  g_Statement = malloc((size_t)g_MAX_STATEMENTS * sizeof(struct statement_struct));
/*E*//*db=db+g_MAX_STATEMENTS * sizeof(struct statement_struct);*/
  if (!g_Statement) {
    print2("*** FATAL ***  Could not allocate g_Statement space\n");
    bug(1363);
    }
  g_MathToken = malloc((size_t)g_MAX_MATHTOKENS * sizeof(struct mathToken_struct));
/*E*//*db=db+g_MAX_MATHTOKENS * sizeof(struct mathToken_struct);*/
  if (!g_MathToken) {
    print2("*** FATAL ***  Could not allocate g_MathToken space\n");
    bug(1364);
    }
  g_IncludeCall = malloc((size_t)g_MAX_INCLUDECALLS * sizeof(struct includeCall_struct));
/*E*//*db=db+g_MAX_INCLUDECALLS * sizeof(struct includeCall_struct);*/
  if (!g_IncludeCall) {
    print2("*** FATAL ***  Could not allocate g_IncludeCall space\n");
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
void outOfMemory(const char *msg) {
  vstring_def(tmpStr);
  print2("*** FATAL ERROR:  Out of memory.\n");
  print2("Internal identifier (for technical support):  %s\n", msg);
  print2("To solve this problem, remove some unnecessary statements or file\n");
  print2("inclusions to reduce the size of your input source.\n");
  print2("Monitor memory periodically with SHOW MEMORY.\n");
  print2("\n");
  print2("Press <return> to exit Metamath.\n");
  tmpStr = cmdInput1("");
  free_vstring(tmpStr);
  /* Close the log to make sure error log is saved */
  if (g_logFileOpenFlag) {
    fclose(g_logFilePtr);
    g_logFileOpenFlag = 0;
  }

  exit(1);
}


/* Bug check */
void bug(int bugNum)
{
  vstring_def(tmpStr);
  flag oldMode;
  long wrongAnswerCount = 0;
  static flag mode = 0; /* 1 = run to next bug, 2 = continue and ignore bugs */

  flag saveOutputToString = g_outputToString;
  g_outputToString = 0; /* Make sure we print to screen and not to string */

  if (mode == 2) {
    /* If user chose to ignore bugs, print brief info and return */
    print2("?BUG CHECK:  *** DETECTED BUG %ld, IGNORING IT...\n", (long)bugNum);
    return;
  }

  print2("?BUG CHECK:  *** DETECTED BUG %ld\n", (long)bugNum);
  if (mode == 0) { /* Print detailed info for first bug */
    print2("\n");
    print2("To get technical support, please open an issue \n");
    print2("(https://github.com/metamath/metamath-exe/issues) with the\n");
    print2("detailed command sequence or a command file that reproduces this bug,\n");
    print2("along with the source file that was used.  See HELP OPEN LOG for help on\n");
    print2("recording a session.  See HELP SUBMIT for help on command files.  Search\n");
    print2("for \"bug(%ld)\" in the m*.c source code to find its origin.\n", bugNum);
    print2("If earlier errors were reported, try fixing them first, because they\n");
    print2("may occasionally lead to false bug detection\n");
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
      print2("Too many wrong answers; program will be aborted to exit scripting loops.\n");
      break;
    }
    if (wrongAnswerCount > 0) {
      free_vstring(tmpStr);
      tmpStr = cmdInput1("Please answer I, S, or A:  ");
    } else {
      print2("Press S <return> to step to next bug, I <return> to ignore further bugs,\n");
      free_vstring(tmpStr);
      tmpStr = cmdInput1("or A <return> to abort program:  ");
    }
    wrongAnswerCount++;
  }
  oldMode = mode;
  mode = 0;
  if (!strcmp(tmpStr, "S") || !strcmp(tmpStr, "s")) mode = 1; /* Skip to next bug */
  if (!strcmp(tmpStr, "I") || !strcmp(tmpStr, "i")) mode = 2; /* Ignore bugs */
  if (oldMode == 0 && mode > 0) {
    /* Print dire warning after the first bug only */
    print2("\n");
    print2("Warning!!!  A bug was detected, but you are continuing anyway.\n");
    print2("The program may be corrupted, so you are proceeding at your own risk.\n");
    print2("\n");
    free_vstring(tmpStr);
  }
  if (mode > 0) {
    g_outputToString = saveOutputToString; /* Restore for continuation */
    return;
  }
  free_vstring(tmpStr);

  print2("\n");
  /* Close the log to make sure error log is saved */
  if (g_logFileOpenFlag) {
    print2("The log file \"%s\" was closed %s %s.\n", g_logFileName,
        date(), time_());
    fclose(g_logFilePtr);
    g_logFileOpenFlag = 0;
  }
  print2("The program was aborted.\n");
  exit(1); /* Use 1 instead of 0 to flag abnormal termination to scripts */
}

/*!
 * \def M_MAX_ALLOC_STACK
 *
 * The number of pointers in a \ref pgStack "stack" available for data reference.
 * Since a stack has a terminal null element, the usable count is one less than
 * the number given here.
 */
#define M_MAX_ALLOC_STACK 100

/* This function returns a 1 if any entry in a comma-separated list
   matches using the matches() function. */
flag matchesList(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard) {
  long entries, i;
  flag matchVal = 0;
  vstring_def(entryPattern);

  /* Done so we can use string functions like left() in call arguments */
  long saveTempAllocStack;
  saveTempAllocStack = g_startTempAllocStack; /* For let() stack cleanup */
  g_startTempAllocStack = g_tempAllocStackTop;

  entries = numEntries(pattern);
  for (i = 1; i <= entries; i++) {
    let(&entryPattern, entry(i, pattern)); /* If we didn't modify
          g_startTempAllocStack above, this let() would corrupt string
          functions in the matchesList() call arguments */
    matchVal = matches(testString, entryPattern, wildCard, oneCharWildCard);
    if (matchVal) break;
  }

  free_vstring(entryPattern); /* Deallocate */
  g_startTempAllocStack = saveTempAllocStack;
  return matchVal;
}


/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have wildcard characters.
   wildCard matches 0 or more characters; oneCharWildCard matches any
   single character. */
flag matches(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard) {
  long i, ppos, pctr, tpos, s1, s2, s3;
  vstring_def(tmpStr);

  if (wildCard == '*') {
    /* Checking for wildCard = * meaning this is only for labels, not
       math tokens */

    /* The following special chars are handled in this block:
       "~" Statement range
       "=" Most recent PROVE command statement
       "%" List of modified statements
       "#" Internal statement number
       "@" Web page statement number */

    i = instr(1, pattern, "~");
    if (i != 0) {
      if (i == 1) {
        s1 = 1; /* empty string before "~" */
      } else {
        s1 = lookupLabel(left(pattern, i - 1));
      }
      s2 = lookupLabel(testString);
      if (i == (long)strlen(pattern)) {
        s3 = g_statements; /* empty string after "~" */
      } else {
        s3 = lookupLabel(right(pattern, i + 1));
      }
      free_vstring(tmpStr); /* Clean up temporary allocations of left and right */
      return ((s1 >= 1 && s2 >= 1 && s3 >= 1 && s1 <= s2 && s2 <= s3)
          ? 1 : 0);
    }

    /* "#12345" matches internal statement number */
    if (pattern[0] == '#') {
      s1 = (long)val(right(pattern, 2));
      if (s1 < 1 || s1 > g_statements)
        return 0; /* # arg is out of range */
      if (!strcmp(g_Statement[s1].labelName, testString)) {
        return 1;
      } else {
        return 0;
      }
    }

    /* "@12345" matches web statement number */
    if (pattern[0] == '@') {
      s1 = lookupLabel(testString);
      if (s1 < 1) return 0;
      s2 = (long)val(right(pattern, 2));
      if (g_Statement[s1].pinkNumber == s2) {
        return 1;
      } else {
        return 0;
      }
    }

    /* "=" matches statement being proved */
    if (!strcmp(pattern,"=")) {
      s1 = lookupLabel(testString);
      /* We might as well use g_proveStatement outside of MM-PA, so =
         can be argument to PROVE command */
      return (g_proveStatement == s1);
    }

    /* "%" matches changed proofs */
    if (!strcmp(pattern,"%")) {
      s1 = lookupLabel(testString);  /* Returns -1 if not found or (not
                                        $a and not $p) */
      if (s1 > 0) { /* It's a $a or $p statement */
        /* (If it's not $p, we don't want to peek at proofSectionPtr[-1]
           to prevent bad pointer. */
        if (g_Statement[s1].type == (char)p_) { /* $p so it has a proof */
          /* The proof is not from the original source file */
          if (g_Statement[s1].proofSectionChanged == 1) {
            return 1;
          }
        }
      }
      return 0;
    } /* if (!strcmp(pattern,"%")) */
  } /* if (wildCard == '*') */

  /* Get to first wild card character */
  ppos = 0;
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

long g_nmbrTempAllocStackTop = 0;     /* Top of stack for nmbrTempAlloc function */
long g_nmbrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when nmbrLet() is called (normally 0, except in
                                    special nested vstring functions) */
temp_nmbrString *nmbrTempAllocStack[M_MAX_ALLOC_STACK];


temp_nmbrString *nmbrTempAlloc(long size)
                                /* nmbrString memory allocation/deallocation */
{
  /* When "size" is >0, "size" instances of nmbrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to g_nmbrStartTempAllocStack. */
  if (size) {
    if (g_nmbrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
      /*??? Fix to allocate more */
      outOfMemory("#105 (nmbrString stack array)");
    }
    if (!(nmbrTempAllocStack[g_nmbrTempAllocStackTop++]=poolMalloc(size
        *(long)(sizeof(nmbrString)))))
/*E*/db2=db2+size*(long)(sizeof(nmbrString));
    return (nmbrTempAllocStack[g_nmbrTempAllocStackTop-1]);
  } else {
    while(g_nmbrTempAllocStackTop != g_nmbrStartTempAllocStack) {
/*E*/db2=db2-(nmbrLen(nmbrTempAllocStack[g_nmbrTempAllocStackTop-1])+1)
/*E*/                                              *(long)(sizeof(nmbrString));
      poolFree(nmbrTempAllocStack[--g_nmbrTempAllocStackTop]);
    }
    g_nmbrTempAllocStackTop=g_nmbrStartTempAllocStack;
    return (0);
  }
}


/* Make string have temporary allocation to be released by next nmbrLet() */
/* Warning:  after nmbrMakeTempAlloc() is called, the nmbrString may NOT be
   assigned again with nmbrLet() */
temp_nmbrString *nmbrMakeTempAlloc(nmbrString *s)
{
  if (g_nmbrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
    printf("*** FATAL ERROR ***  Temporary nmbrString stack overflow in nmbrMakeTempAlloc()\n");
#if __STDC__
    fflush(stdout);
#endif
    bug(1368);
  }
  if (s[0] != -1) { /* End of string */
    /* Do it only if nmbrString is not empty */
    nmbrTempAllocStack[g_nmbrTempAllocStackTop++] = s;
  }
/*E*/db2=db2+(nmbrLen(s)+1)*(long)(sizeof(nmbrString));
/*E*/db3=db3-(nmbrLen(s)+1)*(long)(sizeof(nmbrString));
  return s;
}


/* nmbrString assignment */
/* This function must ALWAYS be called to make assignment to */
/* a nmbrString in order for the memory cleanup routines, etc. */
/* to work properly.  If a nmbrString has never been assigned before, */
/* it is the user's responsibility to initialize it to NULL_NMBRSTRING (the */
/* null string). */
void nmbrLet(nmbrString **target, const nmbrString *source) {
  long targetLength,sourceLength;
  long targetAllocLen;
  long poolDiff;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  sourceLength=nmbrLen(source);  /* Save its actual length */
  targetLength=nmbrLen(*target);  /* Save its actual length */
  targetAllocLen=nmbrAllocLen(*target); /* Save target's allocated length */
/*E*/if (targetLength) {
/*E*/  db3 = db3 - (targetLength+1)*(long)(sizeof(nmbrString));
/*E*/}
/*E*/if (sourceLength) {
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
      nmbrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    } else {    /* source and target are both 0 length */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  nmbrTempAlloc(0); /* Free up temporary strings used in expression computation*/

}



temp_nmbrString *nmbrCat(const nmbrString *string1,...) /* String concatenation */
#define M_MAX_CAT_ARGS 30
{
  va_list ap;   /* Declare list incrementer */
  const nmbrString *arg[M_MAX_CAT_ARGS];        /* Array to store arguments */
  long argLength[M_MAX_CAT_ARGS];       /* Array to store argument lengths */
  int numArgs = 1;        /* Define "last argument" */
  arg[0] = string1;       /* First argument */

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
  va_end(ap);           /* End varargs session */

  numArgs--;    /* The last argument (0) is not a string */

  /* Find out the total string length needed */
  long j = 0;
  for (int i = 0; i < numArgs; i++) {
    argLength[i]=nmbrLen(arg[i]);
    j += argLength[i];
  }
  /* Allocate the memory for it */
  temp_nmbrString *ptr = nmbrTempAlloc(j+1);
  /* Move the strings into the newly allocated area */
  j = 0;
  for (int i = 0; i < numArgs; i++) {
    nmbrCpy(ptr + j, arg[i]);
    j += argLength[i];
  }
  return ptr;

}



/* Find out the length of a nmbrString */
long nmbrLen(const nmbrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return (((long)(((const long *)s)[-1] - (long)(sizeof(nmbrString))))
              / (long)(sizeof(nmbrString)));
}


/* Find out the allocated length of a nmbrString */
long nmbrAllocLen(const nmbrString *s)
{
  /* Assume it's been allocated with poolMalloc. */
  return (((long)(((const long *)s)[-2] - (long)(sizeof(nmbrString))))
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
void nmbrCpy(nmbrString *s, const nmbrString *t) {
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
void nmbrNCpy(nmbrString *s, const nmbrString *t, long n) {
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
flag nmbrEq(const nmbrString *s, const nmbrString *t) {
  long i;
  if (nmbrLen(s) != nmbrLen(t)) return 0; /* Speedup */
  for (i = 0; s[i] == t[i]; i++)
    if (s[i] == -1) /* End of string */
      return 1;
  return 0;
}


/* Extract sin from character position start to stop into sout */
temp_nmbrString *nmbrSeg(const nmbrString *sin, long start, long stop) {
  long length;
  if (start < 1) start = 1;
  if (stop < 1) stop = 0;
  length=stop - start + 1;
  if (length < 0) length = 0;
  temp_nmbrString *sout = nmbrTempAlloc(length + 1);
  nmbrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_NMBRSTRING;
  return sout;
}

/* Extract sin from character position start for length len */
temp_nmbrString *nmbrMid(const nmbrString *sin, long start, long length) {
  if (start < 1) start = 1;
  if (length < 0) length = 0;
  temp_nmbrString *sout = nmbrTempAlloc(length + 1);
  nmbrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_NMBRSTRING;
  return sout;
}

/* Extract leftmost n characters */
temp_nmbrString *nmbrLeft(const nmbrString *sin, long n) {
  if (n < 0) n = 0;
  temp_nmbrString *sout = nmbrTempAlloc(n + 1);
  nmbrNCpy(sout, sin, n);
  sout[n] = *NULL_NMBRSTRING;
  return sout;
}

/* Extract after character n */
temp_nmbrString *nmbrRight(const nmbrString *sin, long n) {
  /*??? We could just return &sin[n-1], but this is safer for debugging. */
  if (n < 1) n = 1;
  long i = nmbrLen(sin);
  if (n > i) return (NULL_NMBRSTRING);
  temp_nmbrString *sout = nmbrTempAlloc(i - n + 2);
  nmbrCpy(sout, &sin[n - 1]);
  return sout;
}


/* Allocate and return an "empty" string n "characters" long */
temp_nmbrString *nmbrSpace(long n) {
  long j = 0;
  if (n < 0) bug(1327);
  temp_nmbrString *sout = nmbrTempAlloc(n + 1);
  while (j < n) {
    /* Initialize all fields */
    sout[j] = 0;
    j++;
  }
  sout[j] = *NULL_NMBRSTRING; /* End of string */
  return sout;
}

/* Search for string2 in string1 starting at start_position */
long nmbrInstr(long start_position, const nmbrString *string1,
  const nmbrString *string2)
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
   return 0;
}

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse nmbrInstr) */
/* Warning:  This has 'let' inside of it and is not safe for use inside
   of 'let' statements.  (To make it safe, it must be rewritten to expand
   the 'mid' and remove the 'let'.) */
long nmbrRevInstr(long start_position, const nmbrString *string1,
    const nmbrString *string2)
{
   long ls1, ls2;
   ls1 = nmbrLen(string1);
   ls2 = nmbrLen(string2);
   if (start_position > ls1 - ls2 + 1) start_position = ls1 - ls2 + 2;
   if (start_position<1) return 0;
   while (!nmbrEq(string2, nmbrMid(string1, start_position, ls2))) {
     start_position--;
     nmbrTempAlloc(0); /* Clear temporaries to prevent overflow caused by "mid" */
     if (start_position < 1) return 0;
   }
   return start_position;
}


/* Converts nmbrString to a vstring with one space between tokens */
temp_vstring nmbrCvtMToVString(const nmbrString *s) {
  long i, j, outputLen, mstrLen;
  vstring_def(tmpStr);
  vstring ptr;
  vstring ptr2;

  long saveTempAllocStack;
  saveTempAllocStack = g_startTempAllocStack; /* For let() stack cleanup */
  g_startTempAllocStack = g_tempAllocStackTop;

  mstrLen = nmbrLen(s);
  /* Precalculate output length */
  outputLen = -1;
  for (i = 0; i < mstrLen; i++) {
    outputLen = outputLen + (long)strlen(g_MathToken[s[i]].tokenName) + 1;
  }
  let(&tmpStr, space(outputLen)); /* Preallocate output string */
  /* Assign output string */
  ptr = tmpStr;
  for (i = 0; i < mstrLen; i++) {
    ptr2 = g_MathToken[s[i]].tokenName;
    j = (long)strlen(ptr2);
    memcpy(ptr, ptr2, (size_t)j);
    ptr = ptr + j + 1;
  }

  g_startTempAllocStack = saveTempAllocStack;
  return makeTempAlloc(tmpStr); /* Flag it for deallocation */
}


/* Converts proof to a vstring with one space between tokens */
temp_vstring nmbrCvtRToVString(const nmbrString *proof,
    flag explicitTargets, /* 1 = "target=source" for /EXPLICIT proof format */
    long statemNum) /* used only if explicitTargets=1 */
{
  long i, j, plen, maxLabelLen, maxLocalLen, step, stmt;
  long maxTargetLabelLen;
  vstring_def(proofStr);
  vstring_def(tmpStr);
  vstring ptr;
  nmbrString_def(localLabels);
  nmbrString_def(localLabelNames);
  long nextLocLabNum = 1; /* Next number to be used for a local label */
  void *voidPtr; /* bsearch result */
  nmbrString_def(targetHyps); /* Targets for /EXPLICIT format */

  long saveTempAllocStack;
  long nmbrSaveTempAllocStack;
  saveTempAllocStack = g_startTempAllocStack; /* For let() stack cleanup */
  g_startTempAllocStack = g_tempAllocStackTop;
  nmbrSaveTempAllocStack = g_nmbrStartTempAllocStack;
                                           /* For nmbrLet() stack cleanup*/
  g_nmbrStartTempAllocStack = g_nmbrTempAllocStackTop;

  plen = nmbrLen(proof);

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
  maxTargetLabelLen = 0;
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      if (!nmbrElementIn(1, localLabels, stmt)) {
        nmbrLet(&localLabels, nmbrAddElement(localLabels, stmt));
      }
    } else {

      if (stmt < 1 || stmt > g_statements) {
        maxLabelLen = 100; /* For safety */
        maxTargetLabelLen = 100; /* For safety */
        continue; /* Ignore bad entry */
      }

      if (stmt > 0) {
        if ((signed)(strlen(g_Statement[stmt].labelName)) > maxLabelLen) {
          maxLabelLen = (long)strlen(g_Statement[stmt].labelName);
        }
      }
    }

    if (explicitTargets == 1) {
      /* Also consider longest target label name */
      stmt = targetHyps[step];
      if (stmt <= 0) bug(1390);
      if ((signed)(strlen(g_Statement[stmt].labelName)) > maxTargetLabelLen) {
        maxTargetLabelLen = (long)strlen(g_Statement[stmt].labelName);
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
      + ((explicitTargets == 1) ? maxTargetLabelLen + 1 : 0)
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
            ((explicitTargets == 1) ? g_Statement[targetHyps[step]].labelName : ""),
            ((explicitTargets == 1) ? "=" : ""),
            str((double)(localLabelNames[stmt])), " ", NULL));

      } else if (stmt != -(long)'?') {
        let(&tmpStr, cat("??", str((double)stmt), " ", NULL)); /* For safety */

      } else {
        if (stmt != -(long)'?') bug(1391); /* Must be an unknown step */
        let(&tmpStr, cat(
            ((explicitTargets == 1) ? g_Statement[targetHyps[step]].labelName : ""),
            ((explicitTargets == 1) ? "=" : ""),
            chr(-stmt), " ", NULL));
      }

    } else if (stmt < 1 || stmt > g_statements) {
      let(&tmpStr, cat("??", str((double)stmt), " ", NULL)); /* For safety */

    } else {
      free_vstring(tmpStr);
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        /* First, get a name for the local label, using the next integer that
           does not match any integer used for a statement label. */
        let(&tmpStr, str((double)nextLocLabNum));
        while (1) {
          voidPtr = (void *)bsearch(tmpStr,
              g_allLabelKeyBase, (size_t)g_numAllLabelKeys,
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
          ((explicitTargets == 1) ? g_Statement[targetHyps[step]].labelName : ""),
          ((explicitTargets == 1) ? "=" : ""),
          g_Statement[stmt].labelName, " ", NULL));
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
  free_vstring(tmpStr);
  free_nmbrString(localLabels);
  free_nmbrString(localLabelNames);

  g_startTempAllocStack = saveTempAllocStack;
  g_nmbrStartTempAllocStack = nmbrSaveTempAllocStack;
  return makeTempAlloc(proofStr); /* Flag it for deallocation */
}


/* This function returns a nmbrString of length of reason with
   step numbers assigned to tokens which are steps, and 0 otherwise.
   The returned string is allocated; THE CALLER MUST DEALLOCATE IT. */
nmbrString *nmbrGetProofStepNumbs(const nmbrString *reason) {
  nmbrString_def(stepNumbs);
  long rlen, start, end, i, step;

  rlen = nmbrLen(reason);
  nmbrLet(&stepNumbs, nmbrSpace(rlen)); /* All stepNumbs[] are initialized
                                        to 0 by nmbrSpace() */
  if (!rlen) return (stepNumbs);
  if (reason[1] == -(long)'=') {
    /* The proof is in "internal" format, with "g_proveStatement = (...)" added */
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
  return stepNumbs;
}


/* Converts any nmbrString to an ASCII string of numbers
   -- used for debugging only. */
temp_vstring nmbrCvtAnyToVString(const nmbrString *s) {
  long i;
  vstring_def(tmpStr);

  long saveTempAllocStack;
  saveTempAllocStack = g_startTempAllocStack; /* For let() stack cleanup */
  g_startTempAllocStack = g_tempAllocStackTop;

  for (i = 1; i <= nmbrLen(s); i++) {
    let(&tmpStr,cat(tmpStr," ", str((double)(s[i-1])),NULL));
  }

  g_startTempAllocStack = saveTempAllocStack;
  return makeTempAlloc(tmpStr); /* Flag it for deallocation */
}


/* Extract variables from a math token string */
temp_nmbrString *nmbrExtractVars(const nmbrString *m) {
  long i, j, length;
  length = nmbrLen(m);
  temp_nmbrString *v = nmbrTempAlloc(length + 1); /* Pre-allocate maximum possible space */
  v[0] = *NULL_NMBRSTRING;
  j = 0; /* Length of output string */
  for (i = 0; i < length; i++) {
    /* Use > because tokenNum=g_mathTokens is used by mmveri.c for
       dummy token */
    if (m[i] < 0 || m[i] > g_mathTokens) bug(1328);
    if (g_MathToken[m[i]].tokenType == (char)var_) {
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
long nmbrElementIn(long start, const nmbrString *g, long element) {
  long i = start - 1;
  while (g[i] != -1) { /* End of string */
    if (g[i] == element) return i + 1;
    i++;
  }
  return 0;
}


/* Add a single number to end of a nmbrString - faster than nmbrCat */
temp_nmbrString *nmbrAddElement(const nmbrString *g, long element) {
  long length;
  length = nmbrLen(g);
  temp_nmbrString *v = nmbrTempAlloc(length + 2); /* Allow for end of string */
  nmbrCpy(v, g);
  v[length] = element;
  v[length + 1] = *NULL_NMBRSTRING; /* End of string */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bbg2: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return v;
}


/* Get the set union of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrUnion(const nmbrString *m1, const nmbrString *m2) {
  long i,j,len1,len2;
  len1 = nmbrLen(m1);
  len2 = nmbrLen(m2);
  temp_nmbrString *v = nmbrTempAlloc(len1+len2+1); /* Pre-allocate maximum possible space */
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
  return v;
}


/* Get the set intersection of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrIntersection(const nmbrString *m1, const nmbrString *m2)
{
  long i,j,len2;
  len2 = nmbrLen(m2);
  temp_nmbrString *v = nmbrTempAlloc(len2+1); /* Pre-allocate maximum possible space */
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
temp_nmbrString *nmbrSetMinus(const nmbrString *m1, const nmbrString *m2)
{
  long i,j,len1;
  len1 = nmbrLen(m1);
  temp_nmbrString *v = nmbrTempAlloc(len1+1); /* Pre-allocate maximum possible space */
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
long nmbrGetSubproofLen(const nmbrString *proof, long step)
{
  long stmt, hyps, pos, i;
  char type;

  if (step < 0) bug(1329);
  stmt = proof[step];
  if (stmt < 0) return (1); /* Unknown or label ref */
  type = g_Statement[stmt].type;
  if (type == f_ || type == e_) return (1); /* Hypothesis */
  hyps = g_Statement[stmt].numReqHyp;
  pos = step - 1;
  for (i = 0; i < hyps; i++) {
    pos = pos - nmbrGetSubproofLen(proof, pos);
  }
  return (step - pos);
}




/* This function returns a packed or "squished" proof, putting in local label
   references to previous subproofs. */
temp_nmbrString *nmbrSquishProof(const nmbrString *proof) {
  nmbrString_def(newProof);
  nmbrString_def(dummyProof);
  nmbrString_def(subProof);
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
  free_nmbrString(subProof);
  free_nmbrString(dummyProof);
  return nmbrMakeTempAlloc(newProof); /* Flag it for deallocation */
}


/* This function unpacks a "squished" proof, replacing local label references
   to previous subproofs by the subproofs themselves. */
temp_nmbrString *nmbrUnsquishProof(const nmbrString *proof) {
  nmbrString_def(newProof);
  nmbrString_def(subProof);
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
  free_nmbrString(subProof);
  return nmbrMakeTempAlloc(newProof); /* Flag it for deallocation */
}


/* This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
temp_nmbrString *nmbrGetIndentation(const nmbrString *proof, long startingLevel) {
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString_def(indentationLevel);
  nmbrString_def(subProof);
  nmbrString_def(nmbrTmp);

  plen = nmbrLen(proof);
  stmt = proof[plen - 1];
  nmbrLet(&indentationLevel, nmbrSpace(plen));
  indentationLevel[plen - 1] = startingLevel;
  if (stmt < 0) { /* A local label reference or unknown */
    if (plen != 1) bug(1330);
    return nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
  }
  type = g_Statement[stmt].type;
  if (type == f_ || type == e_) { /* A hypothesis */
    if (plen != 1) bug(1331);
    return nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1332);
  hyps = g_Statement[stmt].numReqHyp;
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
  if (pos != -1) bug(333);

  free_nmbrString(subProof); /* Deallocate */
  free_nmbrString(nmbrTmp); /* Deallocate */
  return nmbrMakeTempAlloc(indentationLevel); /* Flag it for deallocation */
} /* nmbrGetIndentation */


/* This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
nmbrString *nmbrGetEssential(const nmbrString *proof) {
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString_def(essentialFlags);
  nmbrString_def(subProof);
  nmbrString_def(nmbrTmp);
  nmbrString *nmbrTmpPtr2;

  plen = nmbrLen(proof);
  if (plen == 0) bug(1343);
  stmt = proof[plen - 1];
  nmbrLet(&essentialFlags, nmbrSpace(plen));
  essentialFlags[plen - 1] = 1;
  if (stmt < 0) { /* A local label reference or unknown */
    if (plen != 1) bug(1334);
    /* The only time it should get here is if the original proof has only one
       step, which would be an unknown step */
    if (stmt != -(long)'?' && stmt > -1000) bug(1335);
    return nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
  }
  type = g_Statement[stmt].type;
  if (type == f_ || type == e_) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1336);
    return nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1337);
  hyps = g_Statement[stmt].numReqHyp;
  pos = plen - 2;
  nmbrTmpPtr2 = g_Statement[stmt].reqHypList;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubproofLen(proof, pos);
    if (g_Statement[nmbrTmpPtr2[hyps - i - 1]].type == e_) {
      nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
      nmbrLet(&nmbrTmp, nmbrGetEssential(subProof));
      for (j = 0; j < splen; j++) {
        essentialFlags[j + pos - splen + 1] = nmbrTmp[j];
      }
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (1338);

  free_nmbrString(subProof); /* Deallocate */
  free_nmbrString(nmbrTmp); /* Deallocate */
  return nmbrMakeTempAlloc(essentialFlags); /* Flag it for deallocation */
} /* nmbrGetEssential */


/* This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.
   statemNum is the statement being proved. */
/* ???Optimization:  remove nmbrString calls and use static variables
   to communicate to recursive calls */
temp_nmbrString *nmbrGetTargetHyp(const nmbrString *proof, long statemNum) {
  long plen, stmt, pos, splen, hyps, i, j;
  char type;
  nmbrString_def(targetHyp);
  nmbrString_def(subProof);
  nmbrString_def(nmbrTmp);

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
    return nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
  }
  type = g_Statement[stmt].type;
  if (type == f_ || type == e_) { /* A hypothesis */
    /* The only time it should get here is if the original proof has only one
       step */
    if (plen != 1) bug(1341);
    return nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
  }
  /* An assertion */
  if (type != a_ && type != p_) bug(1342);
  hyps = g_Statement[stmt].numReqHyp;
  pos = plen - 2;
  for (i = 0; i < hyps; i++) {
    splen = nmbrGetSubproofLen(proof, pos);
    if (splen > 1) {
      nmbrLet(&subProof, nmbrSeg(proof, pos - splen + 2, pos + 1));
      nmbrLet(&nmbrTmp, nmbrGetTargetHyp(subProof,
          g_Statement[stmt].reqHypList[hyps - i - 1]));
      for (j = 0; j < splen; j++) {
        targetHyp[j + pos - splen + 1] = nmbrTmp[j];
      }
    } else {
      /* A one-step subproof; don't bother with recursive call */
      targetHyp[pos] = g_Statement[stmt].reqHypList[hyps - i - 1];
    }
    pos = pos - splen;
  }
  if (pos != -1) bug (343);

  free_nmbrString(subProof); /* Deallocate */
  free_nmbrString(nmbrTmp); /* Deallocate */
  return nmbrMakeTempAlloc(targetHyp); /* Flag it for deallocation */
} /* nmbrGetTargetHyp */


/* Converts a proof string to a compressed-proof-format ASCII string.
   Normally, the proof string would be packed with nmbrSquishProof first,
   although it's not a requirement (in which case the compressed proof will
   be much longer of course). */
/* The statement number is needed because required hypotheses are
   implicit in the compressed proof. */
/* The returned ASCII string isn't surrounded by spaces e.g. it
   could be "( a1i a1d ) ACBCADEF". */
temp_vstring compressProof(const nmbrString *proof, long statemNum,
    flag oldCompressionAlgorithm) {
  vstring_def(output);
  long outputLen;
  long outputAllocated;
  nmbrString_def(saveProof);
  nmbrString_def(labelList);
  nmbrString_def(hypList);
  nmbrString_def(assertionList);
  nmbrString_def(localList);
  nmbrString_def(localLabelFlags);
  long hypLabels, assertionLabels, localLabels;
  long plen, step, stmt, labelLen, lab, numchrs;
  long i, j, k;
  long lettersLen, digitsLen;
  static char *digits = "0123456789";
  static char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char labelChar = ':';

  nmbrString_def(explList);
  long explLabels;
  nmbrString_def(explRefCount);
  nmbrString_def(labelRefCount);
  long maxExplRefCount;
  nmbrString_def(explComprLen);
  long explSortPosition;
  long maxExplComprLen;
  vstring_def(explUsedFlag);
  nmbrString_def(explLabelLen);
  nmbrString_def(newExplList);
  long newExplPosition;
  long indentation;
  long explOffset;
  long explUnassignedCount;
  nmbrString_def(explWorth);
  long explWidth;
  vstring_def(explIncluded);


  /* Compression standard with all cap letters */
  /* (For 500-700 step proofs, we only lose about 18% of file size --
      but the compressed proof is more pleasing to the eye) */
  letters = "ABCDEFGHIJKLMNOPQRST"; /* LSB is base 20 */
  digits = "UVWXY"; /* MSB's are base 5 */
  labelChar = 'Z'; /* Was colon */

  lettersLen = (long)strlen(letters);
  digitsLen = (long)strlen(digits);

  nmbrLet(&saveProof, proof); /* In case of temp. alloc. of proof */

  if (g_Statement[statemNum].type != (char)p_) bug(1344);
  plen = nmbrLen(saveProof);

  /* Create the initial label list of required hypotheses */
  nmbrLet(&labelList, g_Statement[statemNum].reqHypList);

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
      if (g_Statement[stmt].type != (char)a_ &&
          g_Statement[stmt].type != (char)p_) {
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
      nmbrRight(hypList, g_Statement[statemNum].numReqHyp + 1),
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
  /* Populate the explicit label list with the counts */
  for (i = 0; i < explLabels; i++) {
    explRefCount[i] = labelRefCount[explList[i]]; /* Save the ref count */
    if (explRefCount[i] <= 0) bug(1381);
    if (explRefCount[i] > maxExplRefCount) {
      maxExplRefCount = explRefCount[i]; /* Update largest count */
    }
  }
  /* We're done with giant labelRefCount array; deallocate */
  free_nmbrString(labelRefCount);

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
        /* If there are no req hyps, 0 = 1st label in explicit list */
        lab = g_Statement[statemNum].numReqHyp + explSortPosition;

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
    explLabelLen[i] = (long)(strlen(g_Statement[stmt].labelName)) + 1;
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
      explWidth = g_screenWidth - indentation - explOffset + 1;

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
      /* j=0 is legal when it can't fit any labels
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
  nmbrLet(&hypList, nmbrLeft(hypList, g_Statement[statemNum].numReqHyp));
  /* "assertionList" will have both the optional hypotheses and the assertions,
     reordered */
  nmbrLet(&assertionList, newExplList);


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
            output[outputAllocated] != 0) bug(1348);
      }
      output[outputLen] = '?';
      outputLen++;
      continue;
    }

    lab = nmbrElementIn(1, labelList, stmt);
    if (!lab) bug(1349);
    lab--; /* labelList array starts at 0, not 1 */

    /* Determine the # of chars in the compressed label */
    /* A corrected algorithm was provided by Marnix Klooster. */
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
          output[outputAllocated] != 0) bug(1350);
    }
    outputLen = outputLen + numchrs;

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
          output[outputAllocated] != 0) bug(1352);
    }
    output[outputLen] = labelChar;
    outputLen++;

  } /* Next step */

  /* Create the final compressed proof */
  let(&output, cat("( ", nmbrCvtRToVString(nmbrCat(
      /* Trim off leading implicit required hypotheses */
      nmbrRight(hypList, g_Statement[statemNum].numReqHyp + 1),
      assertionList, NULL),
                0, /*explicitTargets*/
                0 /*statemNum used only if explicitTargets*/),
      " ) ", left(output, outputLen), NULL));

  free_nmbrString(saveProof);
  free_nmbrString(labelList);
  free_nmbrString(hypList);
  free_nmbrString(assertionList);
  free_nmbrString(localList);
  free_nmbrString(localLabelFlags);
  free_nmbrString(explList);
  free_nmbrString(explRefCount);
  free_nmbrString(labelRefCount);
  free_nmbrString(explComprLen);
  free_vstring(explUsedFlag);
  free_nmbrString(explLabelLen);
  free_nmbrString(newExplList);
  free_nmbrString(explWorth);
  free_vstring(explIncluded);

  return makeTempAlloc(output); /* Flag it for deallocation */
} /* compressProof */


/* Compress the input proof, create the ASCII compressed proof,
   and return its size in bytes. */
/* TODO: call this in MINIMIZE_WITH in metamath.c */
long compressedProofSize(const nmbrString *proof, long statemNum) {
  nmbrString_def(tmpNmbr);
  vstring_def(tmpStr);
  long bytes;
  nmbrLet(&tmpNmbr, nmbrSquishProof(proof));
  let(&tmpStr, compressProof(tmpNmbr,
          statemNum, /* statement being proved */
          0 /* don't use old algorithm (this will become obsolete) */
          ));
  bytes = (long)strlen(tmpStr);
  /* Deallocate memory */
  free_vstring(tmpStr);
  free_nmbrString(tmpNmbr);
  return bytes;
} /* compressedProofSize */



/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

long g_pntrTempAllocStackTop = 0;     /* Top of stack for pntrTempAlloc function */
long g_pntrStartTempAllocStack = 0;   /* Where to start freeing temporary allocation
                                    when pntrLet() is called (normally 0, except in
                                    special nested vstring functions) */

/*!
 * \var pntrString *pntrTempAllocStack[]
 * \brief a \ref pgStack "stack" of \ref temp_pntrString.
 *
 * Holds pointers to temporarily allocated data of type \ref temp_pntrString.  Such
 * a \ref pgStack "stack" is primarily designed to operate like one for
 * temporary allocated ad hoc operands, as described in \ref pgStack.  The
 * stack top index is \ref g_pntrTempAllocStackTop, always refering to the next
 * push position.
 * The \ref g_pntrStartTempAllocStack supports nested operations by indicating
 * where the operands for the upcoming operation start from.
 * \attention A \ref pntrString consists of an array of pointers.  These
 *   pointers may themself refer data that needs a clean up, when the last
 *   reference  to it disappears (such as deallocating memory for example).
 *   There is no automatic procedure handling such cases when pointers are
 *   popped off the stack to be freed.
 * \bug The element type should be temp_pntrString, because a NULL_PNTRSTRING
 *   must not be pushed on the stack.
 */
pntrString *pntrTempAllocStack[M_MAX_ALLOC_STACK];

/*!
 * \fn temp_pntrString *pntrTempAlloc(long size)
 * \par size > 0
 * allocates a \ref pgBlock capable of holding \p size \ref pntrString entries
 * and pushes it onto the \ref pntrTempAllocStack.
 * \par size == 0
 * pops off all entries from index \ref g_pntrStartTempAllocStack on from
 * \ref pntrTempAllocStack and adds them to the \ref memFreePool.
 * \param[in] size count of \ref pntrString entries.  This value must include
 *   a terminal NULL pointer if needed.
 * \return a pointer to the allocated \ref pgBlock, or NULL if deallocation
 *   requested
 * \pre
 *   \p size ==0: all entries in from \ref pntrTempAllocStack from
 *   \ref g_pntrStartTempAllocStack do not contain relevant data any more.
 * \post
 *   - \p size > 0: memory for \p size entries is reserved in the \ref pgBlock
 *     "block's" header, but the data is still random.
 *   - updates \ref db2
 *   - Exits on out-of-memory
 * \bug it is unfortunate that the same function is used for opposite
 *   operations like de-/allocation.
 */
temp_pntrString *pntrTempAlloc(long size) {
                                /* pntrString memory allocation/deallocation */
  /* When "size" is >0, "size" instances of pntrString are allocated. */
  /* When "size" is 0, all memory previously allocated with this */
  /* function is deallocated, down to g_pntrStartTempAllocStack. */
  if (size) {
    if (g_pntrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1))
      /*??? Fix to allocate more */
      outOfMemory("#109 (pntrString stack array)");
    if (!(pntrTempAllocStack[g_pntrTempAllocStackTop++]=poolMalloc(size
        *(long)(sizeof(pntrString)))))
/*E*/db2=db2+(size)*(long)(sizeof(pntrString));
    return (pntrTempAllocStack[g_pntrTempAllocStackTop-1]);
  } else {
    while(g_pntrTempAllocStackTop != g_pntrStartTempAllocStack) {
/*E*/db2=db2-(pntrLen(pntrTempAllocStack[g_pntrTempAllocStackTop-1])+1)
/*E*/                                              *(long)(sizeof(pntrString));
      poolFree(pntrTempAllocStack[--g_pntrTempAllocStackTop]);
    }
    g_pntrTempAllocStackTop=g_pntrStartTempAllocStack;
    return (0);
  }
}


temp_pntrString *pntrMakeTempAlloc(pntrString *s) {
  if (g_pntrTempAllocStackTop>=(M_MAX_ALLOC_STACK-1)) {
    printf(
    "*** FATAL ERROR ***  Temporary pntrString stack overflow in pntrMakeTempAlloc()\n");
#if __STDC__
    fflush(stdout);
#endif
    bug(1370);
  }
  if (s[0] != NULL) { /* Don't do it if pntrString is empty */
    pntrTempAllocStack[g_pntrTempAllocStackTop++] = s;
  }
/*E*/db2=db2+(pntrLen(s)+1)*(long)(sizeof(pntrString));
/*E*/db3=db3-(pntrLen(s)+1)*(long)(sizeof(pntrString));
  return s;
}


void pntrLet(pntrString **target, const pntrString *source) {
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
      pntrCpy(*target,source);
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0d: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    } else {    /* source and target are both 0 length */
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k0e: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
    }
  }

/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("k1: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  pntrTempAlloc(0); /* Free up temporary strings used in expression computation*/

}



/* String concatenation */
temp_pntrString *pntrCat(const pntrString *string1,...) {
  va_list ap;   /* Declare list incrementer */
  const pntrString *arg[M_MAX_CAT_ARGS];        /* Array to store arguments */
  long argLength[M_MAX_CAT_ARGS];       /* Array to store argument lengths */
  int numArgs=1;        /* Define "last argument" */
  int i;
  long j;
  arg[0] = string1;       /* First argument */

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
  va_end(ap);           /* End varargs session */

  numArgs--;    /* The last argument (0) is not a string */

  /* Find out the total string length needed */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    argLength[i]=pntrLen(arg[i]);
    j=j+argLength[i];
  }
  /* Allocate the memory for it */
  temp_pntrString *ptr = pntrTempAlloc(j+1);
  /* Move the strings into the newly allocated area */
  j = 0;
  for (i = 0; i < numArgs; i++) {
    pntrCpy(ptr + j, arg[i]);
    j=j+argLength[i];
  }
  return ptr;

}



/* Find out the length of a pntrString */
long pntrLen(const pntrString *s) {
  /* Assume it's been allocated with poolMalloc. */
  return ((((const long *)s)[-1] - (long)(sizeof(pntrString)))
      / (long)(sizeof(pntrString)));
}


/* Find out the allocated length of a pntrString */
long pntrAllocLen(const pntrString *s) {
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

/*!
 * \brief copies a null pointer terminated \ref pntrString to a destination
 * \ref pntrString.
 *
 * This function determines the length of the source \p t by scanning for a
 * terminal null pointer element.  The destination \p s must have enough space
 * for receiving this amount of pointers, including the terminal null pointer.
 * Then the source pointers are copied beginning with that at the
 * lowest address to the destination area \p t, including the terminal null
 * pointer.
 *
 * \attention make sure the destination area pointed to by \p s has enough
 * space for the copied pointers.
 *
 * \param [out] s (not null) pointer to the target array receiving the copied
 *   pointers.  This need not necessarily be the first element of the array.
 * \param [in] t (not null) pointer to the start of the source array terminated
 *   by a null pointer.
 *
 * \pre
 *   - \p t is terminated by the first null pointer element.
 *   - the target array \p s must have enough free space to hold the source array
 *     \p t including the terminal null pointer.
 *   - \p s and \p t can overlap if \p t points to a later or same element than
 *     \p s (move left semantics).
 * \invariant
 *   If \p s is contained in a \ref pgBlock "block", its administrative header
 *   is NOT updated.
 * \warning The thoughtless use of this function has the potential to create
 *   risks mentioned in the warning of \ref pntrString.
 */
void pntrCpy(pntrString *s, const pntrString *t) {
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
void pntrNCpy(pntrString *s, const pntrString *t, long n) {
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
flag pntrEq(const pntrString *s, const pntrString *t) {
  long i;
  for (i = 0; s[i] == t[i]; i++)
    if (s[i] == NULL) /* End of string */
      return 1;
  return 0;
}


/* Extract sin from character position start to stop into sout */
temp_pntrString *pntrSeg(const pntrString *sin, long start, long stop) {
  long length;
  if (start < 1) start = 1;
  if (stop < 1) stop = 0;
  length = stop - start + 1;
  if (length < 0) length = 0;
  temp_pntrString *sout = pntrTempAlloc(length + 1);
  pntrNCpy(sout, sin + start - 1, length);
  sout[length] = *NULL_PNTRSTRING;
  return sout;
}

/* Extract sin from character position start for length len */
temp_pntrString *pntrMid(const pntrString *sin, long start, long length) {
  if (start < 1) start = 1;
  if (length < 0) length = 0;
  temp_pntrString *sout = pntrTempAlloc(length + 1);
  pntrNCpy(sout, sin + start-1, length);
  sout[length] = *NULL_PNTRSTRING;
  return sout;
}

/* Extract leftmost n characters */
temp_pntrString *pntrLeft(const pntrString *sin, long n) {
  if (n < 0) n = 0;
  temp_pntrString *sout = pntrTempAlloc(n+1);
  pntrNCpy(sout,sin,n);
  sout[n] = *NULL_PNTRSTRING;
  return sout;
}

/* Extract after character n */
temp_pntrString *pntrRight(const pntrString *sin, long n) {
  /*??? We could just return &sin[n-1], but this is safer for debugging. */
  long i;
  if (n < 1) n = 1;
  i = pntrLen(sin);
  if (n > i) return (NULL_PNTRSTRING);
  temp_pntrString *sout = pntrTempAlloc(i - n + 2);
  pntrCpy(sout, &sin[n-1]);
  return sout;
}


/* Allocate and return an "empty" string n "characters" long */
/* Each entry in the allocated array points to an empty vString. */
temp_pntrString *pntrSpace(long n) {
  long j = 0;
  if (n<0) bug(1360);
  temp_pntrString *sout = pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = "";
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return sout;
}

/* Allocate and return an "empty" string n "characters" long
   initialized to nmbrStrings instead of vStrings */
temp_pntrString *pntrNSpace(long n) {
  long j = 0;
  if (n<0) bug(1361);
  temp_pntrString *sout = pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = NULL_NMBRSTRING;
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return sout;
}

/* Allocate and return an "empty" string n "characters" long
   initialized to pntrStrings instead of vStrings */
temp_pntrString *pntrPSpace(long n) {
  long j = 0;
  if (n<0) bug(1372);
  temp_pntrString *sout = pntrTempAlloc(n+1);
  while (j<n) {
    /* Initialize all fields */
    sout[j] = NULL_PNTRSTRING;
    j++;
  }
  sout[j] = *NULL_PNTRSTRING; /* Flags end of string */
  return sout;
}

/* Search for string2 in string1 starting at start_position */
long pntrInstr(long start_position, const pntrString *string1,
  const pntrString *string2)
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
long pntrRevInstr(long start_position, const pntrString *string1,
    const pntrString *string2)
{
   long ls1,ls2;
   ls1=pntrLen(string1);
   ls2=pntrLen(string2);
   if (start_position>ls1-ls2+1) start_position=ls1-ls2+2;
   if (start_position<1) return 0;
   while (!pntrEq(string2,pntrMid(string1,start_position,ls2))) {
     start_position--;
     pntrTempAlloc(0);
        /* Clear temporaries to prevent overflow caused by "mid" */
     if (start_position < 1) return 0;
   }
   return (start_position);
}


/* Add a single null string element to a pntrString - faster than pntrCat */
temp_pntrString *pntrAddElement(const pntrString *g)
{
  long length = pntrLen(g);
  temp_pntrString *v = pntrTempAlloc(length + 2);
  pntrCpy(v, g);
  v[length] = "";
  v[length + 1] = *NULL_PNTRSTRING;
/*E*/if(db9)getPoolStats(&i1,&j1_,&k1); if(db9)printf("bbg3: pool %ld stat %ld\n",poolTotalFree,i1+j1_);
  return v;
}


/* Add a single null pntrString element to a pntrString -faster than pntrCat */
temp_pntrString *pntrAddGElement(const pntrString *g)
{
  long length = pntrLen(g);
  temp_pntrString *v = pntrTempAlloc(length + 2);
  pntrCpy(v, g);
  v[length] = NULL_PNTRSTRING;
  v[length + 1] = *NULL_PNTRSTRING;
  return v;
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

  fbPtr = g_Statement[statemNum].mathSectionPtr;
  if (fbPtr[0] == 0) return 0;
  startLabel = g_Statement[statemNum].labelSectionPtr;
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
  vstring_def(description);
  long p1, p2;

  let(&description, space(g_Statement[statemNum].labelSectionLen));
  memcpy(description, g_Statement[statemNum].labelSectionPtr,
      (size_t)(g_Statement[statemNum].labelSectionLen));
  p1 = rinstr(description, "$(");
  p2 = rinstr(description, "$)");
  if (p1 == 0 || p2 == 0 || p2 < p1) {
    let(&description, "");
    return description;
  }
  let(&description, edit(seg(description, p1 + 2, p2 - 1),
      8 + 128 /* discard leading and trailing blanks */));
  return description;
} /* getDescription */


/* Returns the label section of a statement with all comments except the
   last removed.  Unlike getDescription, this function returns the comment
   surrounded by $( and $) as well as the leading indentation space
   and everything after this comment (such as the actual label).
   Since this is used for arbitrary (other than $a, $p) statements by the
   EXPAND command, we also suppress section headers if they are the last
   comment.  The caller must deallocate the result. */
vstring getDescriptionAndLabel(long stmt) {
  vstring_def(descriptionAndLabel);
  long p1, p2;
  flag dontUseComment = 0;

  let(&descriptionAndLabel, space(g_Statement[stmt].labelSectionLen));
  memcpy(descriptionAndLabel, g_Statement[stmt].labelSectionPtr,
      (size_t)(g_Statement[stmt].labelSectionLen));
  p1 = rinstr(descriptionAndLabel, "$(");
  p2 = rinstr(descriptionAndLabel, "$)");
  if (p1 == 0 || p2 == 0 || p2 < p1) {
    /* The statement has no comment; just return the label and
       surrounding spacing if any */
    return descriptionAndLabel;
  }
  /* Search backwards for non-space or beginning of string */
  p1--;
  while (p1 != 0) {
    if (descriptionAndLabel[p1 - 1] != ' '
          && descriptionAndLabel[p1 - 1] != '\n') break;
    p1--;
  }
  let(&descriptionAndLabel, right(descriptionAndLabel, p1 + 1));
  /* Ignore descriptionAndLabels that are section headers */
  /* TODO: make this more precise here and in mmwtex.c - use 79-char decorations? */
  if (instr(1, descriptionAndLabel, cat("\n", TINY_DECORATION, NULL)) != 0
      || instr(1, descriptionAndLabel, cat("\n", SMALL_DECORATION, NULL)) != 0
      || instr(1, descriptionAndLabel, cat("\n", BIG_DECORATION, NULL)) != 0
      || instr(1, descriptionAndLabel, cat("\n", HUGE_DECORATION, NULL)) != 0) {
    dontUseComment = 1;
  }
  /* Remove comments with file inclusion markup */
  if (instr(1, descriptionAndLabel, "$[") != 0) {
    dontUseComment = 1;
  }

  /* Remove comments with $j markup */
  if (instr(1, descriptionAndLabel, "$j") != 0) {
    dontUseComment = 1;
  }

  if (dontUseComment == 1) {
    /* Get everything that follows the comment */
    p2 = rinstr(descriptionAndLabel, "$)");
    if (p2 == 0) bug(1401); /* Should have exited earlier if no "$)" */
    let(&descriptionAndLabel, right(descriptionAndLabel, p2 + 2));
  }

  return descriptionAndLabel;
} /* getDescriptionAndLabel */


/* Returns 0 or 1 to indicate absence or presence of an indicator in
   the comment of the statement. */
/* mode = 1 = PROOF_DISCOURAGED means get any proof modification discouraged
                indicator
   mode = 2 = USAGE_DISCOURAGED means get any new usage discouraged indicator
   mode = 0 = RESET  means to reset everything (statemNum is ignored) */
/* TODO: add a mode to reset a single statement if in the future we add
   the ability to change the markup within the program. */
flag getMarkupFlag(long statemNum, flag mode) {
  /* For speedup, the algorithm searches a statement's comment for markup
     matches only the first time, then saves the result for subsequent calls
     for that statement. */
  static char init = 0;
  static vstring_def(commentSearchedFlags); /* Y if comment was searched */
  static vstring_def(proofFlags);  /* Y if proof discouragement, else N */
  static vstring_def(usageFlags);  /* Y if usage discouragement, else N */
  vstring_def(str1);

  if (mode == RESET) { /* Deallocate */ /* Should be called by ERASE command */
    free_vstring(commentSearchedFlags);
    free_vstring(proofFlags);
    free_vstring(usageFlags);
    init = 0;
    return 0;
  }

  if (init == 0) {
    init = 1;
    /* The global variables g_proofDiscouragedMarkup and g_usageDiscouragedMarkup
       are initialized to "" like all vstrings to allow them to be reassigned
       by a possible future SET command.  So the first time this is called
       we need to assign them to the default markup strings. */
    if (g_proofDiscouragedMarkup[0] == 0) {
      let(&g_proofDiscouragedMarkup, PROOF_DISCOURAGED_MARKUP);
    }
    if (g_usageDiscouragedMarkup[0] == 0) {
      let(&g_usageDiscouragedMarkup, USAGE_DISCOURAGED_MARKUP);
    }
    /* Initialize flag strings */
    let(&commentSearchedFlags, string(g_statements + 1, 'N'));
    let(&proofFlags, space(g_statements + 1));
    let(&usageFlags, space(g_statements + 1));
  }

  if (statemNum < 1 || statemNum > g_statements) bug(1392);

  if (commentSearchedFlags[statemNum] == 'N') {
    if (g_Statement[statemNum].type == f_ || g_Statement[statemNum].type == e_) {
      /* Any comment before a $f, $e statement is assumed irrelevant */
      proofFlags[statemNum] = 'N';
      usageFlags[statemNum] = 'N';
    } else {
      if (g_Statement[statemNum].type != a_ && g_Statement[statemNum].type != p_) {
        bug(1393);
      }
      str1 = getDescription(statemNum);  /* str1 must be deallocated here */
      /* Strip linefeeds and reduce spaces */
      let(&str1, edit(str1, 4 + 8 + 16 + 128));
      if (instr(1, str1, g_proofDiscouragedMarkup)) {
        proofFlags[statemNum] = 'Y';
      } else {
        proofFlags[statemNum] = 'N';
      }
      if (instr(1, str1, g_usageDiscouragedMarkup)) {
        usageFlags[statemNum] = 'Y';
      } else {
        usageFlags[statemNum] = 'N';
      }
      free_vstring(str1); /* Deallocate */
    }
    commentSearchedFlags[statemNum] = 'Y';
  }

  if (mode == PROOF_DISCOURAGED) return (proofFlags[statemNum] == 'Y') ? 1 : 0;
  if (mode == USAGE_DISCOURAGED) return (usageFlags[statemNum] == 'Y') ? 1 : 0;
  bug(1394);
  return 0;
} /* getMarkupFlag */


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
/* The caller must deallocate the returned string. */
vstring getContrib(long stmtNum, char mode) {
  /* For speedup, the algorithm searches a statement's comment for markup
     matches only the first time, then saves the result for subsequent calls
     for that statement. */
  static char init = 0;

  vstring_def(contributor);
  vstring_def(contribDate);
  vstring_def(reviser);
  vstring_def(reviseDate);
  vstring_def(shortener);
  vstring_def(shortenDate);
  vstring_def(mostRecentDate);   /* The most recent of all 3 dates */

  static vstring_def(commentSearchedFlags); /* Y if comment was searched */
  static pntrString_def(contributorList);
  static pntrString_def(contribDateList);
  static pntrString_def(reviserList);
  static pntrString_def(reviseDateList);
  static pntrString_def(shortenerList);
  static pntrString_def(shortenDateList);
  static pntrString_def(mostRecentDateList);

  long cStart = 0, cMid = 0, cEnd = 0;
  long rStart = 0, rMid = 0, rEnd = 0;
  long sStart = 0, sMid = 0, sEnd = 0;
  long firstR = 0, firstS = 0;
  vstring_def(description);
  vstring_def(tmpDate0);
  vstring_def(tmpDate1);
  vstring_def(tmpDate2);
  long stmt, p, dd, mmm, yyyy;
  flag errorCheckFlag = 0;
  flag err = 0;
  vstring_def(returnStr); /* Return value */
#define CONTRIB_MATCH " (Contributed by "
#define REVISE_MATCH " (Revised by "
#define SHORTEN_MATCH " (Proof shortened by "
#define END_MATCH ".) "

  if (mode == GC_ERROR_CHECK_SILENT || mode == GC_ERROR_CHECK_PRINT) {
    errorCheckFlag = 1;
  }

  if (mode == GC_RESET) {
    /* This is normally called by the ERASE command only */
    if (init != 0) {
      if ((long)strlen(commentSearchedFlags) != g_statements + 1) {
        bug(1395);
      }
      if (stmtNum != 0) {
        bug(1400);
      }
      for (stmt = 1; stmt <= g_statements; stmt++) {
        if (commentSearchedFlags[stmt] == 'Y') {
          /* Deallocate cached strings */
          free_vstring(*(vstring *)(&contributorList[stmt]));
          free_vstring(*(vstring *)(&contribDateList[stmt]));
          free_vstring(*(vstring *)(&reviserList[stmt]));
          free_vstring(*(vstring *)(&reviseDateList[stmt]));
          free_vstring(*(vstring *)(&shortenerList[stmt]));
          free_vstring(*(vstring *)(&shortenDateList[stmt]));
          free_vstring(*(vstring *)(&mostRecentDateList[stmt]));
        }
      }
      /* Deallocate the lists of pointers to cached strings */
      free_pntrString(contributorList);
      free_pntrString(contribDateList);
      free_pntrString(reviserList);
      free_pntrString(reviseDateList);
      free_pntrString(shortenerList);
      free_pntrString(shortenDateList);
      free_pntrString(mostRecentDateList);
      free_vstring(commentSearchedFlags);
      init = 0;
    } /* if (init != 0) */
    return "";
  }

  if (mode == GC_RESET_STMT) {
    /* This should be called whenever the labelSection is changed e.g. by
       SAVE PROOF. */
    if (init != 0) {
      if ((long)strlen(commentSearchedFlags) != g_statements + 1) {
        bug(1398);
      }
      if (stmtNum < 1 || stmtNum > g_statements + 1) {
        bug(1399);
      }
      if (commentSearchedFlags[stmtNum] == 'Y') {
        /* Deallocate cached strings */
        free_vstring(*(vstring *)(&contributorList[stmtNum]));
        free_vstring(*(vstring *)(&contribDateList[stmtNum]));
        free_vstring(*(vstring *)(&reviserList[stmtNum]));
        free_vstring(*(vstring *)(&reviseDateList[stmtNum]));
        free_vstring(*(vstring *)(&shortenerList[stmtNum]));
        free_vstring(*(vstring *)(&shortenDateList[stmtNum]));
        free_vstring(*(vstring *)(&mostRecentDateList[stmtNum]));
        commentSearchedFlags[stmtNum] = 'N';
      }
    } /* if (init != 0) */
    return "";
  }

  /* We now check only $a and $p statements - should we do others? */
  if (g_Statement[stmtNum].type != a_ && g_Statement[stmtNum].type != p_) {
    goto RETURN_POINT;
  }

  if (init == 0) {
    init = 1;
    /* Initialize flag string */
    let(&commentSearchedFlags, string(g_statements + 1, 'N'));
    /* Initialize pointers to "" (null vstring) */
    pntrLet(&contributorList, pntrSpace(g_statements + 1));
    pntrLet(&contribDateList, pntrSpace(g_statements + 1));
    pntrLet(&reviserList, pntrSpace(g_statements + 1));
    pntrLet(&reviseDateList, pntrSpace(g_statements + 1));
    pntrLet(&shortenerList, pntrSpace(g_statements + 1));
    pntrLet(&shortenDateList, pntrSpace(g_statements + 1));
    pntrLet(&mostRecentDateList, pntrSpace(g_statements + 1));
  }

  if (stmtNum < 1 || stmtNum > g_statements) bug(1396);

  if (commentSearchedFlags[stmtNum] == 'N' /* Not in cache */
      || errorCheckFlag == 1 /* Needed to get sStart, rStart, cStart */) {
    /* It wasn't cached, so we extract from the statement's comment */

    free_vstring(description);
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
    }


    /* Get the most recent date */
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

    /* Tag the cache entry as updated */
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
  if (g_Statement[stmtNum].type == a_   /* Don't check syntax statements */
      && strcmp(left(g_Statement[stmtNum].labelName, 3), "df-")
      && strcmp(left(g_Statement[stmtNum].labelName, 3), "ax-")) {
    goto RETURN_POINT;
  }

  if (cStart == 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: There is no \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (instr(cStart + 1, description, CONTRIB_MATCH) != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: There is more than one \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" ",
        "in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (cStart != 0 && description[cMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is no comma between contributor and date",
        ", or period is missing,",
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (rStart != 0 && description[rMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is no comma between reviser and date",
        ", or period is missing,",
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (sStart != 0 && description[sMid - 2] != ',') {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is no comma between proof shortener and date",
        ", or period is missing,",
        " in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, contributor, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is a comma in the contributor name \"",
        contributor,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, reviser, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is a comma in the reviser name \"",
        reviser,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }
  if (instr(1, shortener, ",") != 0) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        "?Warning: There is a comma in the proof shortener name \"",
        shortener,
        "\" in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }


  /*********  Turn off this warning unless we decide not to allow this
  if ((firstR != rStart) || (firstS != sStart)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /@ convenience prefix to assist massive revisions
        g_Statement[stmtNum].labelName, " [",
        @contributor, "/", @reviser, "/", @shortener, "] ",
        @/
        "?Warning: There are multiple \"",
        edit(REVISE_MATCH, 8+128) , "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128) ,
        "...)\" entries in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        "  The last one of each type was used.",
        NULL), "    ", " ");
  }
  *********/

  if ((firstR != 0 && firstR < cStart)
      || (firstS != 0 && firstS < cStart)) {
    err = 1;
    if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" is placed after \"",
        edit(REVISE_MATCH, 8+128) , "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128) ,
        "...)\" in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
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
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: There is a formatting error in a",
        " \"", edit(CONTRIB_MATCH, 8+128),  "...)\", \"",
        edit(REVISE_MATCH, 8+128) , "...)\", or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" entry in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (contribDate[0] != 0) {
    parseDate(contribDate, &dd, &mmm, &yyyy);
    buildDate(dd, mmm, yyyy, &tmpDate0);
    if (strcmp(contribDate, tmpDate0)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
          /* convenience prefix to assist massive revisions
          g_Statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(CONTRIB_MATCH, 8+128),  "...)\" date \"", contribDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
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
          g_Statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(REVISE_MATCH, 8+128) , "...)\" date \"", reviseDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
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
          g_Statement[stmtNum].labelName, " [",
          contributor, "/", reviser, "/", shortener, "] ",
          */
          "?Warning: There is a formatting error in the \"",
          edit(SHORTEN_MATCH, 8+128) , "...)\" date \"", shortenDate, "\""
          " in the comment above statement ",
          str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
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
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
        "?Warning: The \"", edit(CONTRIB_MATCH, 8+128),
        "...)\" date is not earlier than the \"",
        edit(REVISE_MATCH, 8+128), "...)\" or \"",
        edit(SHORTEN_MATCH, 8+128),
        "...)\" date in the comment above statement ",
        str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
        NULL), "    ", " ");
  }

  if (reviseDate[0] != 0 && shortenDate[0] != 0) {
    p = compareDates(reviseDate, shortenDate);
    if ((rStart < sStart && p == 1)
        || (rStart > sStart && p == -1)) {
      err = 1;
      if (mode == GC_ERROR_CHECK_PRINT) printLongLine(cat(
        /* convenience prefix to assist massive revisions
        g_Statement[stmtNum].labelName, " [",
        contributor, "/", reviser, "/", shortener, "] ",
        */
          "?Warning: The \"", edit(REVISE_MATCH, 8+128), "...)\" and \"",
          edit(SHORTEN_MATCH, 8+128),
         "...)\" dates are in the wrong order in the comment above statement ",
          str((double)stmtNum), ", label \"", g_Statement[stmtNum].labelName, "\".",
          NULL), "    ", " ");
    }
  }

  if (err == 1) {
    let(&returnStr, "F");  /* fail */
  } else {
    let(&returnStr, "P");  /* pass */
  }


 RETURN_POINT:

  free_vstring(description);

  if (errorCheckFlag == 1) { /* Slight speedup */
    free_vstring(contributor);
    free_vstring(contribDate);
    free_vstring(reviser);
    free_vstring(reviseDate);
    free_vstring(shortener);
    free_vstring(shortenDate);
    free_vstring(mostRecentDate);
    free_vstring(tmpDate0);
    free_vstring(tmpDate1);
    free_vstring(tmpDate2);
  }

  return returnStr;
} /* getContrib */


/* Extract up to 2 dates after a statement's proof.  If no date is present,
   date1 will be blank.  If no 2nd date is present, date2 will be blank.
   THIS WILL BECOME OBSOLETE WHEN WE START TO USE DATES IN THE
   DESCRIPTION. */
void getProofDate(long stmtNum, vstring *date1, vstring *date2) {
  vstring_def(textAfterProof);
  long p1, p2;
  let(&textAfterProof, space(g_Statement[stmtNum + 1].labelSectionLen));
  memcpy(textAfterProof, g_Statement[stmtNum + 1].labelSectionPtr,
      (size_t)(g_Statement[stmtNum + 1].labelSectionLen));
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
  free_vstring(textAfterProof); /* Deallocate */
  return;
} /* getProofDate */


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
  if (*dd < 1 || *dd > 31 || *mmm < 1 || *mmm > 12) err = 1;
  return err;
} /* parseDate */


/* Build date from numeric fields.  mmm should be a number from 1 to 12.
   There is no error-checking. */
void buildDate(long dd, long mmm, long yyyy, vstring *dateStr) {
  let(&(*dateStr), cat(str((double)dd), "-", mid(MONTHS, mmm * 3 - 2, 3), "-",
      str((double)yyyy), NULL));
  return;
} /* buildDate */


/* Compare two dates in the form dd-mmm-yyyy.  -1 = date1 < date2,
   0 = date1 = date2,  1 = date1 > date2.  There is no error checking. */
flag compareDates(vstring date1, vstring date2) {
  long d1, m1, y1, d2, m2, y2, dd1, dd2;

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




/* Compare strings via pointers for qsort */
/* g_qsortKey is a global string key at which the sort starts; if empty,
   start at the beginning of each line. */
int qsortStringCmp(const void *p1, const void *p2)
{
  vstring_def(tmp);
  long n1, n2;
  int r;
  /* Returns -1 if p1 < p2, 0 if equal, 1 if p1 > p2 */
  if (g_qsortKey[0] == 0) {
    /* No key, use full line */
    return strcmp(*(char * const *)p1, *(char * const *)p2);
  } else {
    n1 = instr(1, *(char * const *)p1, g_qsortKey);
    n2 = instr(1, *(char * const *)p2, g_qsortKey);
    r = strcmp(
        right(*(char * const *)p1, n1),
        right(*(char * const *)p2, n2));
    free_vstring(tmp); /* Deallocate temp string stack */
    return r;
  }
}

void freeData(void) {
  /* 15-Aug-2020 nm TODO: are some of these called twice? (in eraseSource) */
  free(g_IncludeCall);
  free(g_Statement);
  free(g_MathToken);
  free(memFreePool);
  free(memUsedPool);
}
