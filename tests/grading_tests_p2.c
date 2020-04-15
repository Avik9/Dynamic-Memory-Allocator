/**
 * grading_tests_p2.c
 * @brief     part two for hw3 grading test cases
 * @editor    Xi Han
 * @edited    Nov 7 2019
 *
 */
#define TEST_TIMEOUT 15

#include "__grading_helpers.h"
#include "debug.h"

/*
 * Check LIFO discipline on free list
 */
Test(sf_memsuite_grading, malloc_free_lifo, .init = sf_mem_init, .fini = sf_mem_fini,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * u = sf_malloc(sz);
    _assert_nonnull_payload_pointer(u);
    _assert_block_info(PAYLOAD_TO_BLOCK(u), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * v = sf_malloc(sz);
    _assert_nonnull_payload_pointer(v);
    _assert_block_info(PAYLOAD_TO_BLOCK(v), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * z = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * w = sf_malloc(sz);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz));

    sf_free(x);
    sf_free(y);
    sf_free(z);

    void * z1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z1);
    _assert_block_info(PAYLOAD_TO_BLOCK(z1), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * y1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y1);
    _assert_block_info(PAYLOAD_TO_BLOCK(y1), 1, ADJUSTED_BLOCK_SIZE(sz));
    void * x1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x1);
    _assert_block_info(PAYLOAD_TO_BLOCK(x1), 1, ADJUSTED_BLOCK_SIZE(sz));

    cr_assert(x == x1 && y == y1 && z == z1,
              "malloc/free does not follow LIFO discipline");

    _assert_heap_is_valid();

    size_t exp_free_block_size = PAGE_SZ - ADJUSTED_BLOCK_SIZE(sz) * 6
	                                 - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_block_size, 1);

    _assert_errno_eq(0);
}

/*
 * Realloc tests.
 */
Test(sf_memsuite_grading, realloc_larger, .init = sf_mem_init, .fini = sf_mem_fini,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(nsz));

    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz), 1);

    size_t exp_sz = PAGE_SZ - ADJUSTED_BLOCK_SIZE(sz) - ADJUSTED_BLOCK_SIZE(nsz)
                            - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_smaller, .init = sf_mem_init, .fini = sf_mem_fini ,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 200;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(nsz));

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_heap_is_valid();

    size_t exp_sz = PAGE_SZ - ADJUSTED_BLOCK_SIZE(nsz) - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_sz, 1);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_same, .init = sf_mem_init, .fini = sf_mem_fini,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(nsz));

    cr_assert_eq(x, y, "realloc to same size did not return same payload pointer");

    _assert_heap_is_valid();

    size_t exp_sz = PAGE_SZ - ADJUSTED_BLOCK_SIZE(sz) - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_splinter, .init = sf_mem_init, .fini = sf_mem_fini ,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1020;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(nsz));

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_heap_is_valid();
    size_t exp_sz = PAGE_SZ - ADJUSTED_BLOCK_SIZE(nsz) - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_sz, 1);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_size_0, .init = sf_mem_init, .fini = sf_mem_fini ,  .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz));

    void * z = sf_realloc(x, 0);
    _assert_null_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 0, ADJUSTED_BLOCK_SIZE(sz));

    _assert_heap_is_valid();

    // after realloc x to (2) z, x is now a free block
    size_t exp_free_sz_x2z = ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz_x2z, 1);

    // the size of the remaining free block
    size_t exp_free_sz = PAGE_SZ - exp_free_sz_x2z - ADJUSTED_BLOCK_SIZE(sz)
                                 - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

/*
 * Illegal pointer tests.
 */
Test(sf_memsuite_grading, free_null, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    (void) sf_malloc(sz);
    sf_free(NULL);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_unallocated, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void *x = sf_malloc(sz);
    sf_free(x);
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_block_too_small, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);

    PAYLOAD_TO_BLOCK(x)->header = 0x0UL;

    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_prev_alloc, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);
    PAYLOAD_TO_BLOCK(x)->header &= ~PREV_BLOCK_ALLOCATED;
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

// random block assigments. Tried to give equal opportunity for each possible order to appear.
// But if the heap gets populated too quickly, try to make some space by realloc(half) existing
// allocated blocks.
Test(sf_memsuite_grading, stress_test, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    errno = 0;

    int order_range = 13;
    int nullcount = 0;

    void * tracked[100];

    for (int i = 0; i < 100; i++)
    {
        int order = (rand() % order_range);
        size_t extra = (rand() % (1 << order));
        size_t req_sz = (1 << order) + extra;

        tracked[i] = sf_malloc(req_sz);
        // if there is no free to malloc
        if (tracked[i] == NULL)
        {
            order--;
            while (order >= 0)
            {
                req_sz = (1 << order) + (extra % (1 << order));
                tracked[i] = sf_malloc(req_sz);
                if (tracked[i] != NULL)
                {
                    break;
                }
                else
                {
                    order--;
                }
            }
        }

        // tracked[i] can still be NULL
        if (tracked[i] == NULL)
        {
            nullcount++;
            // It seems like there is not enough space in the heap.
            // Try to halve the size of each existing allocated block in the heap,
            // so that next mallocs possibly get free blocks.
            for (int j = 0; j < i; j++)
            {
                if (tracked[j] == NULL)
                {
                    continue;
                }
                sf_block * bp = PAYLOAD_TO_BLOCK(tracked[j]);
                req_sz = BLOCK_SIZE(bp) >> 1;
                tracked[j] = sf_realloc(tracked[j], req_sz);
            }
        }
        errno = 0;
    }

    for (int i = 0; i < 100; i++)
    {
        if (tracked[i] != NULL)
        {
            sf_free(tracked[i]);
        }
    }

    _assert_heap_is_valid();

    // As allocations are random, there is a small probability that the entire heap
    // has not been used.  So only assert that there is one free block, not what size it is.
    //size_t exp_free_sz = MAX_SIZE - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(0, 1);
}
