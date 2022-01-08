/*****************************************************************************/
/*        Copyright (C) 2017  Thierry Arnoux                                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* This module manages hash tables. */

#include <stdlib.h>
#include "mmhtbl.h"
#include "mminou.h"

/* Static buffer for the linked lists */
#define NB_LINKEDITEMS 50000L
#define NO_LINKEDITEM -1
linked *linkedItems;
int free_linkedItem;
flag htinit_done = 0;

void htinit() {
  if(htinit_done) return;
  free_linkedItem = 0;
  size_t linkedItemsSize = NB_LINKEDITEMS * sizeof(linked);
  linkedItems = malloc(linkedItemsSize);
  if (linkedItems == NULL) outOfMemory("#511 (linked items)");
  for(int i=0; i<NB_LINKEDITEMS-1 ; i++) linkedItems[i].next = i+1;
  linkedItems[NB_LINKEDITEMS-1].next = NO_LINKEDITEM;
  htinit_done = 1;
}

/* Creates a new hashtable.
   Returns the hashtable created. */
hashtable htcreate(vstring name, int bucket_count, void *no_obj,
		  hashFunc *hashF, eqFunc *eqF, letFunc *letF,
		  freeFunc *freeF, dumpFunc *dumpF) {
  htinit();

  /* Allocate space for the bucket pointers */
  int *buckets = malloc(bucket_count * sizeof(int));
  if (!buckets) outOfMemory("#512 (hashtable buckets)");

  /* Each bucket initially contains an empty list */
  for(int i=0; i<bucket_count ; i++) buckets[i] = NO_LINKEDITEM;

  /* Create and fill the structure */
  hashtable ht;
  ht.name = name;
  ht.bucket_count = bucket_count;
  ht.buckets = buckets;
  ht.no_obj = no_obj;
  ht.hashFunc = hashF;
  ht.eqFunc = eqF;
  ht.letFunc = letF;
  ht.freeFunc = freeF;
  ht.dumpFunc = dumpF;
  ht.entries = 0;
  ht.gets = 0;
  ht.founds = 0;
  ht.biggest_bucket_size = 0;
  return ht;
}

/* Deletes a hashtable.
   Cleans up the hashtable, returning linked items and freeing objects and keys. */
void htdelete(hashtable *hashtable) {
  /* for each bucket */
  for(int bucket = 0;bucket<hashtable->bucket_count;bucket++) {
    /* Go through the linked list and free each key/object */
    int *pli = &hashtable->buckets[bucket];
    while(*pli != NO_LINKEDITEM) {
      hashtable->freeFunc(&linkedItems[*pli].key, &linkedItems[*pli].object);
      pli = &linkedItems[*pli].next;
    }
    /* Then put the whole linked list on the free list */
    if(hashtable->buckets[bucket] != NO_LINKEDITEM) {
      int old_free = free_linkedItem;
      free_linkedItem = hashtable->buckets[bucket];
      *pli = old_free;
    }
  }
}

/* Puts a new object in the hashtable.
   Returns 1 if successful, 0 otherwise. */
flag htput(hashtable *hashtable, void *key, void *object) {
  /* First we need a free linked item */
  int linkedItem = free_linkedItem;
  if(linkedItem == NO_LINKEDITEM) return 0;
  free_linkedItem = linkedItems[linkedItem].next;

  /* Store object and key */
  hashtable->letFunc(&linkedItems[linkedItem].key,
		     &linkedItems[linkedItem].object, key, object);
  hashtable->entries++;

  /* Compute the bucket */
  unsigned int bucket = hashtable->hashFunc(key) % hashtable->bucket_count;

  /* Insert at the beginning of the list */
  linkedItems[linkedItem].next = hashtable->buckets[bucket];
  hashtable->buckets[bucket] = linkedItem;
  return 1;
}

/* Gets an object from the hashtable, based on its key.
   Returns the object pointer if successful, NULL otherwise. */
void *htget(hashtable *hashtable, void *key) {
  hashtable->gets++;
  /* Compute the bucket */
  unsigned int bucket = hashtable->hashFunc(key) % hashtable->bucket_count;
  int bucket_size = 0;

  /* Look for the key */
  int li = hashtable->buckets[bucket];
  while(li != NO_LINKEDITEM) {
    if(hashtable->eqFunc(linkedItems[li].key, key)) {
      /* Found it, return the associated object */
      hashtable->founds++;
      return linkedItems[li].object;
    }
    li = linkedItems[li].next;
    bucket_size++;
  }
  /* For stats only */
  if(bucket_size > hashtable->biggest_bucket_size) {
    hashtable->biggest_bucket_size = bucket_size;
    hashtable->biggest_bucket = bucket;
  }

  /* Object was not found */
  return hashtable->no_obj;
}

flag htremove(hashtable *hashtable, void *key) {
  /* Compute the bucket */
  unsigned int bucket = hashtable->hashFunc(key) % hashtable->bucket_count;

  /* Look for the key */
  int *pli = &hashtable->buckets[bucket];
  while(*pli != NO_LINKEDITEM) {
    if(!hashtable->eqFunc(linkedItems[*pli].key, key)) {
      /* Found it, free the object and remove it from the chain */
      hashtable->freeFunc(&linkedItems[*pli].key, &linkedItems[*pli].object);
      int old_free = free_linkedItem;
      free_linkedItem = *pli;
      *pli = linkedItems[*pli].next;
      linkedItems[*pli].next = old_free;
      hashtable->entries--;
      return 1;
    }
    pli = &linkedItems[*pli].next;
  };

  /* Object was not found */
  return 0;
}

/*  Returns the hashtable load factor in percent */
int htlf(hashtable *hashtable) {
  return 100 * hashtable->entries / hashtable->bucket_count;
}

/* Dumps stats about the hash table */
void htstats(hashtable *hashtable) {
  int empty_buckets = 0;
  for(int bucket=0;bucket<hashtable->bucket_count;bucket++) empty_buckets+=hashtable->buckets[bucket] == NO_LINKEDITEM;
  printf("Hashtable %s: %d entries, load factor=%d%%, %d used buckets, %d%% misses, biggest bucket=%d\n", hashtable->name, hashtable->entries,
	 htlf(hashtable), (hashtable->bucket_count - empty_buckets), (100-(100*hashtable->founds/hashtable->gets)),
	 hashtable->biggest_bucket_size);
}

/* Dumpts the whole table */
void htdump(hashtable *hashtable) {
  print2("Hashtable %s:\n", hashtable->name);
  //for(int bucket=0;bucket<hashtable->bucket_count;bucket++) {
  int bucket = hashtable->biggest_bucket;
  {
    int li = hashtable->buckets[bucket];
    while(li != NO_LINKEDITEM) {
      hashtable->dumpFunc(linkedItems[li].key, linkedItems[li].object);
      li = linkedItems[li].next;
    }
  }
}
