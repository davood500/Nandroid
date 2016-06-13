#include <nandroid/scheme/ns_gc.h>
#include <string.h>
#include <stdio.h>

#define WITH_HEADER(size) ((size) + sizeof(int))
#define WITHOUT_HEADER(size) ((size) - sizeof(int))
#define FLAGS(chunk) (*((unsigned int*)(chunk)))
#define CHUNK_SIZE(chunk) (FLAGS((chunk)) & (~3))
#define MARK_CHUNK(chunk, c) ((FLAGS(chunk)) = CHUNK_SIZE(chunk)|c)
#define PTR_INDEX(ofs) ((ofs) / sizeof(void*))
#define BITS(chunk) (WITH_HEADER(chunk))
#define BITS_AT(chunk, idx) ((((void**)(BITS(chunk)) + (idx) * sizeof(void*))))
#define MEM_TAG(ptr) (!(((unsigned int)(ptr)) & 1))
#define POINTS_MINOR(ptr) (((ns_byte*)(ptr)) >= &(gc)->minor_heap[0] && ((ns_byte*)(ptr)) < &(gc)->minor_heap[GC_MINHEAP_SIZE])
#define POINTS_MAJOR(ptr) (((ns_byte*)(ptr)) >= &(gc)->major_heap[0] && ((ns_byte*)(ptr)) < &(gc)->major_heap[GC_MAJHEAP_SIZE])
#define REF_PTR_MIN(ptr) (MEM_TAG((ptr)) && POINTS_MINOR((ptr)))
#define REF_PTR_MAJ(ptr) (MEM_TAG((ptr)) && POINTS_MAJOR((ptr)))
#define MINOR_CHUNK(ptr) (((((ns_byte*)(ptr) - &(gc)->minor_heap[0]) / GC_MINCHUNK_SIZE) * GC_MINCHUNK_SIZE) + &(gc)->minor_heap[0])
#define MARKED(chunk) (FLAGS(chunk) & 1)
#define CHUNK_FLAGS(chunk) (FLAGS((chunk)) & (3))
#define ALIGN(ptr) (((ptr) + 3) & ~3)
#define CHUNK_AT(idx) (&(gc)->minor_heap[(idx) * GC_MINCHUNK_SIZE])
#define CHUNK_OFFSET(chunk) ((((chunk)) - &(gc)->minor_heap[0]) / GC_MINCHUNK_SIZE)

typedef enum{
  GC_FREE = 0,
  GC_WHITE,
  GC_BLACK,
  GC_GRAY
} ns_gc_color;

void
ns_gc_init(ns_gc* gc){
  gc->ref_count = 0;
  gc->free_chunk = 0;
  FLAGS(&gc->major_heap[0]) = GC_MAJHEAP_SIZE;
  memset(gc->backpatch, 0, sizeof(gc->backpatch));
}

static void
set_ref_block(ns_gc* gc, int index, void* start, void* end){
  gc->refs[index][0] = start;
  gc->refs[index][1] = end;
}

static ns_byte*
find_major_chunk(ns_gc* gc, ns_byte* ptr){
  ns_byte* curr;
  for(curr = &gc->major_heap[0]; !(ptr >= curr && (ptr < (curr + CHUNK_SIZE(curr)))) && curr < &gc->major_heap[GC_MAJHEAP_SIZE]; curr += CHUNK_SIZE(curr));
  if(curr < &gc->major_heap[GC_MAJHEAP_SIZE]) return curr;
  return 0;
}

static void
mark_major_chunk(ns_gc* gc, ns_byte* ptr){
  if(CHUNK_FLAGS(ptr) != GC_BLACK){
    MARK_CHUNK(ptr, CHUNK_FLAGS(ptr) | GC_BLACK);
    for(int i = 0; i < PTR_INDEX(CHUNK_SIZE(ptr)); i++){
      if(REF_PTR_MAJ(*BITS_AT(ptr, i))){
        mark_major_chunk(gc, find_major_chunk(gc, *BITS_AT(ptr, i)));
      }
    }
  }
}

static void* major_alloc(ns_gc* gc, size_t size);

static void*
minor_alloc(ns_gc* gc, size_t size){
  if(size == 0) return NULL;
  size = WITH_HEADER(ALIGN(size));
  if(size > GC_MINCHUNK_SIZE) {
    return major_alloc(gc, WITHOUT_HEADER(size));
  }
  if(gc->free_chunk >= GC_MINCHUNKS){
    ns_gc_minor(gc);
    return minor_alloc(gc, WITHOUT_HEADER(size));
  }
  FLAGS(CHUNK_AT(gc->free_chunk)) = ((unsigned int) size);
  return BITS(CHUNK_AT(gc->free_chunk++));
}

static void*
major_alloc(ns_gc* gc, size_t size){
  if(size == 0) return NULL;
  size = WITH_HEADER(ALIGN(size));

  ns_byte* curr;
  for(curr = &gc->major_heap[0]; (CHUNK_FLAGS(curr) != GC_FREE || size > CHUNK_SIZE(curr)) && curr < &gc->major_heap[GC_MAJHEAP_SIZE]; curr += CHUNK_SIZE(curr));

  if(curr >= &gc->major_heap[GC_MAJHEAP_SIZE]) return NULL;

  ns_byte* free_chunk = curr;
  unsigned int prev_size = CHUNK_SIZE(free_chunk);
  FLAGS(free_chunk) = prev_size;
  MARK_CHUNK(free_chunk, GC_WHITE);
  ns_byte* next = (curr + size);
  FLAGS(next) = ((unsigned int) (prev_size - size));
  return ((void*) WITH_HEADER(free_chunk));
}

static void
darken_chunk(ns_gc* gc, ns_byte* ptr){
  ns_byte* curr = find_major_chunk(gc, ptr);
  if(curr != 0 && CHUNK_FLAGS(curr) == GC_WHITE) MARK_CHUNK(curr, GC_GRAY);
}

static void
darken_roots(ns_gc* gc){
  for(int i = 0; i < gc->ref_count; i++){
    for(void** ref = gc->refs[i][0]; ref < gc->refs[i][1]; ref++){
      if(ref != 0) darken_chunk(gc, *ref);
    }
  }
}

static void
darken_major(ns_gc* gc){
  ns_byte* curr;
  for(curr = &gc->major_heap[0]; curr != NULL && curr < &gc->major_heap[GC_MAJHEAP_SIZE]; curr += CHUNK_SIZE(curr)){
    switch(CHUNK_FLAGS(curr)){
      case GC_GRAY: mark_major_chunk(gc, curr); break;
      default: break;
    }
  }
}

static void
mark_chunk(ns_gc* gc, ns_byte* ptr){
  MARK_CHUNK(ptr, 1);
  for(int i = 0; i < PTR_INDEX(CHUNK_SIZE(ptr)); i++){
    if(REF_PTR_MIN(*BITS_AT(ptr, i))){
      ns_byte* p = *BITS_AT(ptr, i);
      ns_byte* ref = MINOR_CHUNK(p);
      if(!MARKED(ref)) mark_chunk(gc, ref);
    }
  }
}

static void
mark_minor(ns_gc* gc){
  memset(gc->backpatch, 0, sizeof(gc->backpatch));
  for(int i = 0; i < gc->ref_count; i++){
    for(void** ref = gc->refs[i][0]; ref < gc->refs[i][1]; ref++){
      if(ref != NULL){
        if(REF_PTR_MIN(*ref)) mark_chunk(gc, MINOR_CHUNK(*ref));
      }
    }
  }
}

static void
backpatch_chunk(ns_gc* gc, ns_byte* ptr){
  for(int i = 0; i < PTR_INDEX(CHUNK_SIZE(ptr)); i++){
    if(REF_PTR_MIN(*BITS_AT(ptr, i))){
      ns_byte* p = *BITS_AT(ptr, i);
      ns_byte* ref = MINOR_CHUNK(p);
      if(MARKED(ref)){
        ns_byte* new_ptr = ((ns_byte*) gc->backpatch[CHUNK_OFFSET(ref)]);
        *BITS_AT(ptr, i) += (new_ptr - ref);
      }
    }
  }
}

static void
backpatch_refs(ns_gc* gc){
  for(int i = 0; i < gc->ref_count; i++){
    for(void** ref = gc->refs[i][0]; ref < gc->refs[i][1]; ref++){
      if(ref != NULL){
        if(POINTS_MINOR(*ref)){
          ns_byte* chunk = MINOR_CHUNK(*ref);
          if(MARKED(chunk)){
            ns_byte* new_ptr = ((ns_byte*) gc->backpatch[CHUNK_OFFSET(chunk)]);
            if(new_ptr != NULL) *ref += (new_ptr - chunk);
          }
        }
      }
    }
  }
}

static void
copy_minor_heap(ns_gc* gc){
  for(int i = 0; i < gc->free_chunk; i++){
    ns_byte* chunk = CHUNK_AT(i);
    void* ptr = WITHOUT_HEADER(major_alloc(gc, WITH_HEADER(CHUNK_SIZE(chunk))));
    gc->backpatch[i] = ptr;
  }

  backpatch_refs(gc);

  for(int i = 0; i < gc->ref_count; i++){
    backpatch_chunk(gc, CHUNK_AT(i));
  }

  for(int i = 0; i < gc->ref_count; i++){
    ns_byte* chunk = CHUNK_AT(i);
    if(MARKED(chunk)){
      ns_byte* new_chunk = ((ns_byte*) gc->backpatch[i]);
      memcpy(new_chunk, chunk, CHUNK_SIZE(chunk));
    }
  }
}

void
ns_gc_major(ns_gc* gc){
  darken_roots(gc);
  darken_major(gc);
  for(ns_byte* curr = &gc->major_heap[0]; curr < &gc->major_heap[GC_MAJHEAP_SIZE]; curr += CHUNK_SIZE(curr)){
    switch(CHUNK_FLAGS(curr)){
      case GC_WHITE: MARK_CHUNK(curr, GC_FREE); break;
      default: break;
    }
  }
}

void
ns_gc_minor(ns_gc* gc){
  mark_minor(gc);
  copy_minor_heap(gc);
  gc->free_chunk = 0;
}

void*
ns_gc_alloc(ns_gc* gc, size_t size){
  void* ptr = minor_alloc(gc, size);
  if(ptr == NULL) return major_alloc(gc, size);
  return ptr;
}

#define CHECK_CHUNK(begin, end)({ \
    if(begin == end){ \
        return; \
    } \
    if(begin > end){ \
        void** tmp = begin; \
        begin = end; \
        end = tmp; \
    } \
})

static void
add_ref(ns_gc* gc, void** begin, void** end){
  CHECK_CHUNK(begin, end);
  for(int i = 0; i < gc->ref_count; i++){
    if(begin >= gc->refs[i][0] && end <= gc->refs[i][1]) return;
  }

  for(int i = 0; i < gc->ref_count; i++){
    if(gc->refs[i][0] == NULL){
      set_ref_block(gc, i, begin, end);
      return;
    }
  }

  set_ref_block(gc, gc->ref_count++, begin, end);
}

static void
remove_ref(ns_gc* gc, void* begin, void* end){
  CHECK_CHUNK(begin, end);
  int i;
  for(i = 0; i < gc->ref_count - 1; i++){
    if(((void**) begin) >= gc->refs[i][0] && ((void**) end) <= gc->refs[i][1]){
      gc->refs[i][0] = NULL;
      return;
    }
  }

  if(((void**) begin) >= gc->refs[i][0] && ((void**) end) <= gc->refs[i][1]){
    gc->ref_count--;
  }
}

void
ns_gc_add_ref(ns_gc* gc, void* ref){
  add_ref(gc, ref, ref + sizeof(void*));
}

void
ns_gc_del_ref(ns_gc* gc, void* ref){
  remove_ref(gc, ref, ref + sizeof(void*));
}

#ifdef NS_DEBUG
void
ns_gc_print_minor(ns_gc* gc){
  printf("*** List of minor heap allocated chunks\n");
  printf("*** %d Allocated Chunks\n", gc->free_chunk);
  for(int i = 0; i < gc->free_chunk; i++){
    ns_byte* chunk = CHUNK_AT(i);
    printf("  Chunk: %.4d  Size: %.3d  Marked: %c\n", ((int) CHUNK_OFFSET(chunk)), CHUNK_SIZE(chunk), (MARKED(chunk) ? 'y' : 'n'));
  }
  printf("*** End of minor chunks list\n");
}

void
ns_gc_print_major(ns_gc* gc){
  printf("*** List of major heap allocated chunks\n");

  for(ns_byte* curr = &gc->major_heap[0]; curr < &gc->major_heap[GC_MAJHEAP_SIZE]; curr += CHUNK_SIZE(curr)){
    char* flags;
    switch(CHUNK_FLAGS(curr)){
      case GC_FREE: flags = "free"; break;
      case GC_WHITE: flags = "white"; break;
      case GC_GRAY: flags = "gray"; break;
      case GC_BLACK: flags = "black"; break;
      default: flags = "unknown"; break;
    }

    printf("  Size: %.3d  Marked: %s\n", CHUNK_SIZE(curr), flags);
  }

  printf("*** End of major chunks list\n");
}

void
ns_gc_print_refs(ns_gc* gc){
  printf("*** List of stored references\n");
  printf("*** %d Stored References\n", gc->ref_count);
  for(int i = 0; i < gc->ref_count; i++){
    if(gc->refs[i][0] == NULL) continue;
    for(void** ref = gc->refs[i][0]; ref < gc->refs[i][1]; ref++){
      if(ref != NULL && gc->refs[i][0] != NULL){
        char* points_to = "??";
        void* heap = 0;
        if(POINTS_MINOR(ref)){
          points_to = "Minor";
          heap = gc->minor_heap;
        } else if(POINTS_MAJOR(ref)){
          points_to = "Major";
          heap = gc->major_heap;
        }

        printf("  Reference pointing to: %.8x (%s)\n", ((unsigned int) (*ref - heap)), points_to);
      }
    }
  }
  printf("*** End of stored references\n");
}

#endif