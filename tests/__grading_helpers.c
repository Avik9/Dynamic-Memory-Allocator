#include "__grading_helpers.h"
#include "debug.h"
#include "sfmm.h"

static bool free_list_is_empty()
{
    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
	if(sf_free_list_heads[i].body.links.next != &sf_free_list_heads[i] ||
	   sf_free_list_heads[i].body.links.prev != &sf_free_list_heads[i])
	    return false;
    }
    return true;
}

/*
 * This function determines the index of the freelist appropriate for a
 * specified block size.
 */
static int free_list_index(size_t size) {
    size_t s = MIN_BLOCK_SIZE;
    size_t ps = MIN_BLOCK_SIZE;
    size_t ns;
    int i = 0;
    while(i < NUM_FREE_LISTS-2) {
        if(size <= s)
            return i;
        i++;
	// Fibonacci sequence
	ns = s + ps;
	ps = s;
	s = ns;
    }
    // If we reach here, i == NUM_FREE_LISTS-2
    return i;
}

static bool block_is_in_free_list(sf_block * abp)
{
    int index = free_list_index(BLOCK_SIZE(abp));
    sf_block * bp = sf_free_list_heads[index].body.links.next;
    int limit = LOOP_LIMIT;
    while (bp != sf_free_list_heads + index && limit--)
    {
        if (bp == abp)
        {
            return true;
        }
        bp = bp->body.links.next;
    }
    return false;
}

void _assert_free_list_is_empty()
{
    cr_assert(free_list_is_empty(), "Free list is not empty");
}

/* 
 * Function checks if all blocks are unallocated in free_list.
 * This function does not check if the block belongs in the specified free_list.
 */
void _assert_free_list_is_valid()
{
    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
        sf_block * bp = sf_free_list_heads[i].body.links.next;
        int limit = LOOP_LIMIT;
        while (bp != &sf_free_list_heads[i] && limit--)
        {
            cr_assert(!(ALLOCATED(bp)),
                      "Block %p in free list is marked allocated", bp);
            bp = bp->body.links.next;
        }
        cr_assert(limit != 0, "Bad free list");
    }
}

/**
 * Checks if a block follows documentation constraints
 * 
 * @param bp pointer to the block to check
 */
void _assert_block_is_valid(sf_block * bp)
{
    // proper block alignment & size

    cr_assert(ALIGNED_BLOCK(bp),
              "Block %p is not properly aligned", bp);

    cr_assert(VALID_BLOCK_SIZE(bp),
              "Block size is invalid for %p. Got: %lu",
              bp, BLOCK_SIZE(bp));

    // prev alloc bit of next block == alloc bit of this block

    cr_assert(BLOCK_IS_EPILOGUE(bp) || VALID_NEXT_BLOCK_PREV_ALLOC(bp),
              "Prev allocated bit is not correctly set for %p. Should be: %d",
              NEXT_BLOCK(bp), ALLOCATED(bp));

    // no uncoalesced adjacent free blocks

    cr_assert(BLOCK_IS_EPILOGUE(bp) || ALLOCATED(bp) || ALLOCATED(NEXT_BLOCK(bp)),
              "Uncoalesced adjacent free blocks %p and %p",
              bp, NEXT_BLOCK(bp));

    // other issues to check

    cr_assert(VALID_FOOTER(bp),
              "block's footer does not match header for %p", bp);

    /* 
     * Added wilderness special case for S2020.
     * If wilderness does not exist, the macro would evaluate to a sentinel
     * and should never match the address of a given block.
     */
    if (bp == WILDERNESS) 
    {
        cr_assert(BLOCK_IS_EPILOGUE(NEXT_BLOCK(bp)), 
                 "Wilderness block is not the last free block!");
        cr_assert(!ALLOCATED(bp), "Wilderness block has alloc bit set!");
    }
    else if (ALLOCATED(bp))
    {
        cr_assert(!block_is_in_free_list(bp),
                  "Allocated block at %p is also in the free list", bp);
    }
    else
    {
        cr_assert(block_is_in_free_list(bp),
                  "Free block at %p is not contained in the free list", bp);
    }
}

void _assert_prologue_is_valid(void)
{
    sf_block * bp = (sf_block *) PROLOGUE;

    _assert_block_is_valid(bp);

    cr_assert_eq(BLOCK_SIZE(bp), MIN_BLOCK_SIZE,
                 "Prologue has incorrect size");

    cr_assert(ALLOCATED(bp),
              "Prologue is not marked allocated");

    cr_assert(VALID_FOOTER(bp),
              "Prologue's footer does not match header for %p", bp);

    // check that the first block has its prev_allocated bit set to 1
    cr_assert(VALID_NEXT_BLOCK_PREV_ALLOC(bp),
              "Prev allocated bit for first block is not set correctly");
}

void _assert_epilogue_is_valid(void)
{
    sf_block * ep = EPILOGUE;
    cr_assert(ALLOCATED(ep), "Epilogue is not marked allocated");
}

void _assert_heap_is_valid(void)
{
    char * heap_p = sf_mem_start(), * end_heap = sf_mem_end();

    // check if heap is empty then free list must be empty as well
    if (heap_p == end_heap)
    {
        cr_assert(free_list_is_empty(),
                  "The heap is empty, but the free list is not");
    }

    // assert every block in heap is valid
    heap_p += (sizeof(_sf_prologue) - sizeof(sf_footer));
    char * stop = end_heap - sizeof(_sf_epilogue) - sizeof(sf_footer);
    sf_block * bp;
    int limit = LOOP_LIMIT;
    while (heap_p < stop && limit--)
    {
        bp = (sf_block *) heap_p;
        _assert_block_is_valid(bp);
        heap_p += BLOCK_SIZE(bp);
    }

    // check if epilogue is correctly set
//    sf_epilogue * epilogue = EPILOGUE;
    cr_assert(heap_p == stop,
              "Heap blocks end before epilogue is reached");
//    cr_assert(ALLOCATED(epilogue),
//              "Epilogue is not marked allocated");

    // check the main free list
    _assert_free_list_is_valid();
}

/** 
 * Asserts a block's info.
 * 
 * @param bp pointer to the beginning of the block.
 * @param alloc The expected allocation bit for the block.
 * @param b_size The expected block size.
 */
void _assert_block_info(sf_block * bp, int alloc, size_t b_size)
{
    cr_assert(ALLOCATED(bp) == alloc,
              "Block %p has wrong allocation status (got %d, expected %d)",
              bp, ALLOCATED(bp), alloc);

    cr_assert(BLOCK_SIZE(bp) == b_size,
              "Block %p has wrong block_size (got %lu, expected %lu)",
              bp, BLOCK_SIZE(bp), b_size);
}

/**
 * Asserts payload pointer is not null.
 * 
 * @param pp payload pointer.
 */ 
void _assert_nonnull_payload_pointer(void * pp)
{
    cr_assert(pp != NULL, "Payload pointer should not be NULL");
}

/**
 * Asserts payload pointer is null.
 * 
 * @param pp payload pointer.
 */
void _assert_null_payload_pointer(void * pp)
{
    cr_assert(pp == NULL, "Payload pointer should be NULL");
}

/**
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 *  
 * @param size the size of free blocks to count.
 * @param count the expected number of free blocks to be counted.
 */
void _assert_free_block_count(size_t size, int count)
{
    int cnt = 0;
    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
        sf_block * bp = sf_free_list_heads[i].body.links.next;
        while (bp != &sf_free_list_heads[i])
        {
            if (size == 0 || size == (bp->header & BLOCK_SIZE_MASK))
            {
                cnt++;
            }
            bp = bp->body.links.next;
        }
    }
    if(size)
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    else
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
}

/**
 * Assert the sf_errno.
 * 
 * @param n the errno expected
 */
void _assert_errno_eq(int n)
{
    cr_assert_eq(sf_errno, n, "sf_errno has incorrect value (value=%d, exp=%d)", sf_errno, n);
}
