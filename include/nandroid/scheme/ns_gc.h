#ifndef NANDROID_NS_GC_H
#define NANDROID_NS_GC_H

#include <stddef.h>
#include "ns_common.h"

#define GC_MAX_REFS 65536
#define GC_MINCHUNK_SIZE 256
#define GC_MINHEAP_SIZE (1024 * 64)
#define GC_MAJHEAP_SIZE (1024 * 1024 * 32)
#define GC_MINCHUNKS (GC_MINHEAP_SIZE / GC_MINCHUNK_SIZE)

typedef struct{
  ns_byte minor_heap[GC_MINHEAP_SIZE];
  ns_byte major_heap[GC_MAJHEAP_SIZE];
  int free_chunk;
  int ref_count;
  void* backpatch[GC_MINCHUNKS];
  void** refs[GC_MAX_REFS][2];
} ns_gc;

void ns_gc_init(ns_gc* gc);
void ns_gc_add_ref(ns_gc* gc, void* ref);
void ns_gc_del_ref(ns_gc* gc, void* ref);
void ns_gc_major(ns_gc* gc);
void ns_gc_minor(ns_gc* gc);
void* ns_gc_alloc(ns_gc* gc, size_t size);

#ifdef NS_DEBUG
void ns_gc_print_minor(ns_gc* gc);
void ns_gc_print_major(ns_gc* gc);
void ns_gc_print_refs(ns_gc* gc);
#endif

#endif