/*****************************************************************************/
/*        Copyright (C) 2012  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef MMHTBL_H_
#define MMHTBL_H_

#include <stdio.h>
#include "mmvstr.h"
#include "mmdata.h"

typedef int (hashFunc)(void *);
typedef int (eqFunc)(void *, void *);
typedef void (letFunc)(void **, void **, void *, void*);
typedef void (freeFunc)(void **, void **);
typedef void (dumpFunc)(void *, void *);

struct linked_struct {
  void *object;
  void *key;
  int next;
};

typedef struct linked_struct linked;

struct hashtable_struct {
  vstring name;
  int bucket_count;
  int *buckets;
  int entries;
  int gets;
  int founds;
  int biggest_bucket;
  int biggest_bucket_size;
  void *no_obj;
  hashFunc *hashFunc;
  eqFunc *eqFunc;
  letFunc *letFunc;
  freeFunc *freeFunc;
  dumpFunc *dumpFunc;
};

typedef struct hashtable_struct hashtable;

hashtable htcreate(vstring name, int bucket_count, void *no_obj,
		   hashFunc *hashF, eqFunc *eqF, letFunc *letF,
		   freeFunc *freeF, dumpFunc *dumpF);
void htdelete(hashtable *hashtable);
flag htput(hashtable *hashtable, void *key, void *object);
void *htget(hashtable *hashtable, void *key);
flag htremove(hashtable *hashtable, void *key);
void htstats(hashtable *hashtable);
void htdump(hashtable *hashtable);

#endif /* MMHTBL_H_ */
