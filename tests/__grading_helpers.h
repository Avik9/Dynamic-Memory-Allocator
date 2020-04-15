#ifndef SFMM_TEST_H
#define SFMM_TEST_H

#include <criterion/criterion.h>
#include "sfmm.h"
#include <signal.h>
#include <stdio.h>
#include <errno.h>

// ====================================================================================================
// updated macros

#define MAX_SIZE                  (16 * PAGE_SZ)

#define ALIGN_SHIFT               6U
#define ALIGN(v)                  ((((v) + (1U << ALIGN_SHIFT) - 1U) >> ALIGN_SHIFT) << ALIGN_SHIFT)
#define MIN_BLOCK_SIZE            ((size_t) (ALIGN(sizeof(sf_block))))

#define max(x, y)                 (((x) > (y)) ? (x) : (y))

// Modified for S2020. Allocated blocks do not have footer overhead
#define ADJUSTED_BLOCK_SIZE(sz)   (max(ALIGN(sz + sizeof(sf_header) /*+ sizeof(sf_footer)*/), MIN_BLOCK_SIZE))

#define ALIGNMENT_MASK            ((1U << ALIGN_SHIFT) - 1U)

#define ALIGNED_SIZE(size)        (((size) & ALIGNMENT_MASK) == 0)
#define ALIGNED_BLOCK(bp)         ((((uint64_t) (bp->body.payload)) & (ALIGNMENT_MASK)) == 0)

#define BLOCK_SIZE(bp)            ((bp)->header & BLOCK_SIZE_MASK)

#define PAYLOAD_TO_BLOCK(pp)                                                                         \
    ((sf_block *) ((char *) (pp) - (char *) &(((sf_block *) 0x0)->body.payload)))

#define NEXT_BLOCK(bp)            ((sf_block *) ((char *) (bp) + BLOCK_SIZE(bp)))
#define FOOTER(bp)                (NEXT_BLOCK(bp)->prev_footer)

#define PREV_FOOTER(bp)           (bp->prev_footer)
#define PREV_BLOCK(bp)            ((sf_block *) ((char *) (bp) - (PREV_FOOTER(bp) & BLOCK_SIZE_MASK)))

/*
 * The prologue consists of an allocated block of minimum size starting
 * at whatever offset from the beginning of the heap is necessary to align
 * the payload area.
 */
#define PROLOGUE                  ((sf_block *)((char *)sf_mem_start() + (1 << ALIGN_SHIFT) \
                                                 - sizeof(sf_header) - sizeof(sf_footer)))           

/*
 * The epilogue consists of an allocated header at the end of the heap.
 */
#define EPILOGUE                  ((sf_block *)(sf_mem_end() \
                                                  - sizeof(sf_header) - sizeof(sf_footer)))
#define BLOCK_IS_EPILOGUE(bp)     ((char *) (bp) == (char *)EPILOGUE)

#define WILDERNESS                ((sf_block *) sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next)

/* Macros to maintain allocation status bits. */
#define ALLOCATED(bp)             (((bp)->header & THIS_BLOCK_ALLOCATED) != 0)
#define PREV_ALLOCATED(bp)        (((bp)->header & PREV_BLOCK_ALLOCATED) != 0)

#define VALID_BLOCK_SIZE(bp)                                                        \
   (BLOCK_SIZE(bp) >= MIN_BLOCK_SIZE &&                                             \
    BLOCK_SIZE(bp) <= 100 * PAGE_SZ &&                                              \
    ALIGNED_SIZE(BLOCK_SIZE(bp)))

#define VALID_FOOTER(bp)          (ALLOCATED(bp) || FOOTER(bp) == (bp)->header)

#define VALID_NEXT_BLOCK_PREV_ALLOC(bp)                                             \
    (PREV_ALLOCATED(NEXT_BLOCK(bp)) == ALLOCATED(bp))

#define LOOP_LIMIT                1000

// ====================================================================================================


/* The following structs are defined for compatibility with last semester's 
 * grading_helpers.c. They are renamed from sf -> _sf in case there is a clash
 * with student's code.
 */
typedef struct _sf_prologue {
    void* padding1;
    void* padding2;
    void* padding3;
    void* padding4;
    void* padding5;
    void* padding6;
    void* padding7;
    
    sf_header header;
    void* unused1;
    void* unused2;
    void* unused3;
    void* unused4;
    void* unused5;
    void* unused6;
    sf_footer footer;
} _sf_prologue;

typedef struct _sf_epilogue {
    sf_header header;
} _sf_epilogue;

void _assert_free_list_is_empty(void);

void _assert_prologue_is_valid(void);

void _assert_epilogue_is_valid(void);

void _assert_block_is_valid(sf_block * hp);

void _assert_heap_is_valid(void);

void _assert_block_info(sf_block * hp, int alloc, size_t b_size);

void _assert_nonnull_payload_pointer(void * pp);

void _assert_null_payload_pointer(void * pp);

void _assert_free_block_count(size_t size, int count);

void _assert_errno_eq(int n);

#endif
