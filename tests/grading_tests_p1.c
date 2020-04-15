/**
 * grading_tests_p1.c
 * @brief      part 1 for hw3 grading test cases
 */
#define TEST_TIMEOUT 15
#include "__grading_helpers.h"

/*
 * Do one malloc and check that the prologue and epilogue are correctly initialized
 */
Test(sf_memsuite_grading, initialization, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    (void) sf_malloc(sz);

    _assert_prologue_is_valid();
    _assert_epilogue_is_valid();
    _assert_heap_is_valid();
}

/*
 * Single malloc tests, up to the size that forces a non-minimum block size.
 */
Test(sf_memsuite_grading, single_malloc_1, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_4, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 4;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_8, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 8;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_16, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 16;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_32, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 32;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}


/*
 * Single malloc test, of a size exactly equal to what is left after initialization.
 */
Test(sf_memsuite_grading, single_malloc_4024, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue)
	                - (sizeof(sf_block) - sizeof(sf_header));
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    _assert_free_block_count(0, 0);

    _assert_errno_eq(0);
}

/*
 * Single malloc test, of a size just larger than what is left after initialization.
 */
Test(sf_memsuite_grading, single_malloc_4040, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = PAGE_SZ - sizeof(_sf_epilogue) - sizeof(_sf_prologue)
	                - (sizeof(sf_block) - sizeof(sf_header)) + (1 << ALIGN_SHIFT);
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ * 2 - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

/*
 * Single malloc test, of multiple pages.
 */
Test(sf_memsuite_grading, single_malloc_12000, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 12000;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz));
    _assert_heap_is_valid();

    size_t exp_free_sz = PAGE_SZ * 3 - sizeof(_sf_epilogue) - sizeof(_sf_prologue) - ADJUSTED_BLOCK_SIZE(sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(0);
}

/*
 * Single malloc test, unsatisfiable.
 * There should be left one single large block.
 */
Test(sf_memsuite_grading, single_malloc_max, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = MAX_SIZE;
    void * x = sf_malloc(sz);
    _assert_null_payload_pointer(x);
    _assert_heap_is_valid();

    size_t exp_free_sz = MAX_SIZE - sizeof(_sf_epilogue) - sizeof(_sf_prologue);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_errno_eq(ENOMEM);
}

/*
 * Malloc/free with/without coalescing.
 */
Test(sf_memsuite_grading, malloc_free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = 300;
    size_t sz3 = 400;

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    void * z = sf_malloc(sz3);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz3));

    sf_free(y);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 0, ADJUSTED_BLOCK_SIZE(sz2));
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz3));
    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz2), 1);

    size_t exp_free_2_sz = PAGE_SZ -
                           ADJUSTED_BLOCK_SIZE(sz1) - ADJUSTED_BLOCK_SIZE(sz2) - ADJUSTED_BLOCK_SIZE(sz3) -
                           sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_2_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_lower, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = 300;
    size_t sz3 = 400;
    size_t sz4 = 500;

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    void * z = sf_malloc(sz3);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz3));

    void * w = sf_malloc(sz4);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz4));

    sf_free(y);
    sf_free(z);

    size_t sz = ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 0, sz);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz4));
    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3), 1);

    size_t exp_free_2_sz = PAGE_SZ -
                           ADJUSTED_BLOCK_SIZE(sz1) - ADJUSTED_BLOCK_SIZE(sz2) -
                           ADJUSTED_BLOCK_SIZE(sz3) - ADJUSTED_BLOCK_SIZE(sz4) -
                           sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_2_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_upper, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = 300;
    size_t sz3 = 400;
    size_t sz4 = 500;

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    void * z = sf_malloc(sz3);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz3));

    void * w = sf_malloc(sz4);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz4));

    sf_free(z);
    sf_free(y);
    size_t sz = ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 0, sz);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz4));
    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3), 1);

    size_t exp_free_2_sz = PAGE_SZ -
                           ADJUSTED_BLOCK_SIZE(sz1) - ADJUSTED_BLOCK_SIZE(sz2) -
                           ADJUSTED_BLOCK_SIZE(sz3) - ADJUSTED_BLOCK_SIZE(sz4) -
                           sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_2_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_both, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = 300;
    size_t sz3 = 400;
    size_t sz4 = 500;

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    void * z = sf_malloc(sz3);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(PAYLOAD_TO_BLOCK(z), 1, ADJUSTED_BLOCK_SIZE(sz3));

    void * w = sf_malloc(sz4);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(PAYLOAD_TO_BLOCK(w), 1, ADJUSTED_BLOCK_SIZE(sz4));

    sf_free(x);
    sf_free(z);
    sf_free(y);
    size_t sz = ADJUSTED_BLOCK_SIZE(sz1) + ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 0, sz);
    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz1) + ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3), 1);

    size_t exp_free_2_sz = PAGE_SZ -
                           ADJUSTED_BLOCK_SIZE(sz1) - ADJUSTED_BLOCK_SIZE(sz2) -
                           ADJUSTED_BLOCK_SIZE(sz3) - ADJUSTED_BLOCK_SIZE(sz4) -
                           sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_2_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_first_block, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = 300;

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    sf_free(x);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 0, ADJUSTED_BLOCK_SIZE(sz1));
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));
    _assert_heap_is_valid();

    _assert_free_block_count(ADJUSTED_BLOCK_SIZE(sz1), 1);

    size_t exp_free_2_sz = PAGE_SZ -
                           ADJUSTED_BLOCK_SIZE(sz1) - ADJUSTED_BLOCK_SIZE(sz2) -
                           sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_free_block_count(exp_free_2_sz, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_last_block, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
    size_t sz2 = PAGE_SZ - ADJUSTED_BLOCK_SIZE(sz1)
	                 - sizeof(_sf_prologue) - sizeof(_sf_epilogue) - (1 << ALIGN_SHIFT);

    void * x = sf_malloc(sz1);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    void * y = sf_malloc(sz2);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 1, ADJUSTED_BLOCK_SIZE(sz2));

    sf_free(y);

    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz1));

    size_t exp_free_sz = PAGE_SZ - ADJUSTED_BLOCK_SIZE(sz1) - sizeof(_sf_prologue) - sizeof(_sf_epilogue);
    _assert_block_info(PAYLOAD_TO_BLOCK(y), 0, exp_free_sz);
    _assert_free_block_count(exp_free_sz, 1);

    _assert_heap_is_valid();

    _assert_errno_eq(0);
}

/*
 * Check that malloc leaves no splinter.
 */
Test(sf_memsuite_grading, malloc_with_splinter, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = PAGE_SZ - sizeof(_sf_prologue) - sizeof(_sf_epilogue) - MIN_BLOCK_SIZE;
    void * x = sf_malloc(sz);
    fprintf(stderr, "sz: %ld\n", sz);

    _assert_nonnull_payload_pointer(x);
    // Change added S2020.
    _assert_block_info(PAYLOAD_TO_BLOCK(x), 1, ADJUSTED_BLOCK_SIZE(sz) /*+ (1 << ALIGN_SHIFT)*/);
    _assert_heap_is_valid();

    _assert_free_block_count(0, 0);

    _assert_errno_eq(0);
}

/*
 *  Allocate small blocks until memory exhausted.
 */
Test(sf_memsuite_grading, malloc_to_exhaustion, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t sz = 100;

    // Increased limit for S2020 as MAX_SIZE is now larger
    int limit = 600;
    int exp_mallocation = (MAX_SIZE - sizeof(_sf_prologue) - sizeof(_sf_epilogue)) / ADJUSTED_BLOCK_SIZE(sz);
    int exp_limit = limit - exp_mallocation;

    void * x;
    while ((x = sf_malloc(sz)) != NULL && limit--)
    {
        sf_block * bp = PAYLOAD_TO_BLOCK(x);
        size_t size = BLOCK_SIZE(bp);
        // Not all blocks will be the same size due to splitting restrictions.
        cr_assert(size == ADJUSTED_BLOCK_SIZE(sz) ||
                  size == (ADJUSTED_BLOCK_SIZE(sz) + (1 << ALIGN_SHIFT)),
                  "block has incorrect size (size=%lu, exp=%lu or %lu)",
                  size, ADJUSTED_BLOCK_SIZE(sz),
                  ADJUSTED_BLOCK_SIZE(sz) + (1 << ALIGN_SHIFT));
    }

    fprintf(stderr, "limit: %d, exp_limit: %d, exp_mallocation: %d\n",
        limit, exp_limit, exp_mallocation);
        cr_assert_eq(limit, exp_limit, "Memory not exhausted when it should be");

    _assert_heap_is_valid();

    size_t exp_free_sz = MAX_SIZE - sizeof(_sf_prologue) - sizeof(_sf_epilogue)
                                  - (ADJUSTED_BLOCK_SIZE(sz) * exp_mallocation);

    // This check was added S2020.
    int exp_block_count = 1;
    if (exp_free_sz == 0)
        exp_block_count = 0;
    _assert_free_block_count(exp_free_sz, exp_block_count);

    _assert_errno_eq(ENOMEM);
}

/*
 *  Test sf_memalign handling invalid arguments:
 *  If align is not a power of two or is less than the minimum block size,
 *  then NULL is returned and sf_errno is set to EINVAL.
 *  If size is 0, then NULL is returned without setting sf_errno.
 */
Test(sf_memsuite_grading, sf_memalign_test_1, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    /* Test size is 0, then NULL is returned without setting sf_errno */
    int old_errno = sf_errno;  // Save the current errno just in case
    sf_errno = ENOTTY;  // Set errno to something it will never be set to as a test (in this case "not a typewriter")
    size_t arg_align = MIN_BLOCK_SIZE;
    size_t arg_size = 0;
    void *actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed a size of 0");
    _assert_errno_eq(ENOTTY);  // Assert that the errno didn't change
    sf_errno = old_errno;  // Restore the old errno

    /* Test align less than the minimum block size */
    arg_align = 1U << 2;  // A power of 2 that is still less than MIN_BLOCK_SIZE
    arg_size = 25;  // Arbitrary
    actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed align that was less than the minimum block size");
    _assert_errno_eq(EINVAL);

    /* Test align that isn't a power of 2 */
    arg_align = (MIN_BLOCK_SIZE << 1) - 1;  // Greater than MIN_BLOCK_SIZE, but not a power of 2
    arg_size = 65;  // Arbitrary
    actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed align that wasn't a power of 2");
    _assert_errno_eq(EINVAL);
}

/*
Test that memalign returns an aligned address - using minimum block size for alignment
 */
Test(sf_memsuite_grading, sf_memalign_test_2, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t arg_align = 1U << 6; // Use the minimum block size for alignment
    size_t arg_size = 25;  // Arbitrary
    void *x = sf_memalign(arg_size, arg_align);
    _assert_nonnull_payload_pointer(x);
    if (((unsigned long)x & 0x3F) != 0) {  // Test if any of the lower-order 6 bits are 1 - if so, then the address is not aligned
        cr_assert(1 == 0, "sf_memalign didn't return an aligned address!");
    }
}

/*
Test that memalign returns aligned, usable memory
 */
Test(sf_memsuite_grading, sf_memalign_test_3, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = TEST_TIMEOUT)
{
    size_t arg_align = 1U << 7; // Use larger than minimum block size for alignment
    size_t arg_size = 129;  // Arbitrary
    void *x = sf_memalign(arg_size, arg_align);
    _assert_nonnull_payload_pointer(x);
    if (((unsigned long)x & 0x7F) != 0) {  // Test if any of the lower-order 6 bits are 1 - if so, then the address is not aligned
        cr_assert(1 == 0, "sf_memalign didn't return an aligned address!");
    }
    _assert_heap_is_valid();
}
