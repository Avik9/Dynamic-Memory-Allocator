# Dynamic Memory Allocator
#### Avik Kadakia and Eugene Stark

# Overview

I have created an allocator for the x86-64 architecture with the following features:

- Free lists segregated by size class, using first-fit policy within each size class.
- Immediate coalescing of blocks on free with adjacent free blocks.
- Boundary tags to support efficient coalescing.
- Block splitting without creating splinters.
- Allocated blocks with client-specified alignment.
- Free lists maintained using **last in first out (LIFO)** discipline.

I implemented my own versions of the **malloc**, **realloc**,
**free**, and **memalign** functions.

I wrote 25 Criterion unit tests to help debug my implementation.

## Free List Management Policy

My allocator uses the following scheme to manage free blocks:
Free blocks will be stored in a fixed array of `NUM_FREE_LISTS` free lists,
segregated by size class.
Each individual free list will be organized as a **circular, doubly linked list**
(more information below).
The size classes are based on a Fibonacci sequence (1, 2, 3, 5, 8, 13, ...),
according to the following scheme:
The first free list (at index 0) holds blocks of the minimum size `M`
(where `M = 64` for this program).
The second list (at index 1) holds blocks of size `2M`.
The third list (at index 2) holds blocks of size `3M`.
The fourth list holds blocks whose size is in the interval `(3M, 5M]`.
The fifth list holds blocks whose size is in the interval `(5M, 8M]`,
and so on.
This pattern continues up to the interval `(21M, 34M]`,
and then the second-to-last list (at index `NUM_FREE_LISTS-2`; *i.e.* 8)
holds blocks of size greater than `34M`.
The last list (at index `NUM_FREE_LISTS-1`; *i.e.* 9) is only used to contain
the so-called "wilderness block", which is the free block at the end of the heap
that will be extended when the heap is grown.
Allocation requests will be satisfied by searching the free lists in increasing
order of size class; considering the last list with the wilderness block only
if no suitable block has been found in the earlier lists.

> :nerd: This policy means that the wilderness block will effectively be treated
> as larger than any other free block, regardless of its actual size.
> This "wilderness preservation" heuristic tends to prevent the heap from growing
> unnecessarily.

## Block Placement Policy

When allocating memory in response to an allocation request, I used a
**segregated fits policy** as follows:
First, the smallest size class that is sufficiently large to satisfy the request
is determined.  The free lists are then searched, starting from the list for the
determined size class and continuing in increasing order of size, until a nonempty
list is found.  The request is then satisfied by the first block in that list
that is sufficiently large; *i.e.* a **first-fit policy** is applied within each individual free list.
The last list, which contains only the wilderness block (if there currently is one), is considered
only if no sufficiently large block is found in any of the earlier lists.

If there is no free list that contains a sufficiently large block,
then `sf_mem_grow` is called to extend the heap by an additional page of memory.
After **coalescing** this page (see below) with any free block that immediately precedes
it (*i.e.* with the wilderness block), I use the resulting block of memory
to satisfy the allocation request;
splitting it if it is too large and no splinter (*i.e.* a block of less than the minimum
block size) would result.  If the block of memory is still not large enough, another call to
`sf_mem_grow` is made; continuing to grow the heap until either a large enough block
is obtained or the return value from `sf_mem_grow` indicates that there is no more memory.

Segregated free lists allow the allocator to approximate a
best-fit policy, with lower overhead than would be the case if an exact best-fit policy
were implemented.

## Splitting Blocks & Splinters

The allocator splits blocks at allocation time to reduce the amount of
internal fragmentation. Due to alignment and overhead constraints, there is a minimum useful block size
that the allocator can support.  **For this program, pointers returned by the allocator
in response to allocation requests are aligned to 64-byte boundaries**;
*i.e.* the pointers returned will be addresses that are multiples of 2^6.
Note that this is a more stringent alignment requirement than what is imposed by the
underlying hardware, which only requires alignment to 16-byte boundaries.
The 64-byte alignment requirement implies that the minimum block size for the allocator
will be 64 bytes.  No "splinters" of smaller size than this are ever to be created.
If splitting a block to be allocated would result in a splinter, then the block should
not be split; rather, the block should be used as-is to satisfy the allocation request
(*i.e.*, "over-allocate" by issuing a block slightly larger than that required).

## Freeing a Block

When a block is freed, an attempt is first made to **coalesce** the block with
any free block that immediately precedes or follows it in the heap.
Once the block has been coalesced, it should be inserted at the **front** of the free
list for the appropriate size class (based on the size after coalescing).
The reason for performing coalescing is to combat the external fragmentation
that would otherwise result due to the splitting of blocks upon allocation.

## Block Headers & Footers

A block header is defined as 2 words
(32 bits) to hold the block size and allocated bit. In this program, the header
will be 4 words (i.e. 64 bits or 1 memory row). The header fields will be similar
to those in the textbook but I will keep an extra field for recording whether
or not the previous block is allocated.
Each free block will also have a footer, which occupies the last memory row of the block.
The footer of a free block contains exactly the same information as the header.
In an allocated block, the footer is not present, and the space that it would otherwise
occupy may be used for payload.

**Block Header Format:**
```c
    +---------------------------------------------------------------------+---------+---------+ <- header
    |                                       block_size                    |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |  (0/1)  |  (0/1)  |
    |                                        (1 row)                      |  1 bit  |  1 bit  |
    +--------------------------------------------+------------------------+---------+---------+ <- (aligned)
```

- The `block_size` field gives the number of bytes for the **entire** block (including header/footer,
  payload, and padding).  It occupies the entire 64 bits of the block header or footer,
  except that the two least-significant bits of the block size, which would normally always be zero
  due to alignment requirements, are used to store the allocation status (free/allocated) of that
  block and of the immediately preceding block in the heap.  This means that these two bits have
  to be masked when retrieving the block size from the header and when the block size is stored
  in the header the previously existing values of these two bits have to be preserved.
- The `alloc` bit is a boolean. It is 1 if the block is allocated and 0 if it is free.
- The `prev_alloc` bit is also a boolean. It is 1 if the **immediately preceding** block
in the heap is allocated and 0 if it is not.

> :thinking:  Here is an example of determining the block size required to satisfy
> a particular requested payload size.  Suppose the requested size is 25 bytes.
> An additional 8 bytes will be required to store the block header, which must always
> be present.  This means that a block of at least 33 bytes must be used, however due
> to alignment requirements this has to be rounded up to the next multiple of the
> alignment size.  If the alignment size were 16 bytes (which would be just large enough
> to enable the memory returned by the allocator to store in an aligned fashion any of
> the basic data types supported by the x86-64 architecture), then a block of at least
> 48 bytes would have to be used.  As a result, there would be 15 bytes of "padding"
> at the end of the payload area, which contributes to internal fragmentation.
> Besides the header, when the block is free it is also necessary to store a footer,
> as well and next and previous links for the freelist.
> These will take an additional 24 bytes of space, however when the block is free there
> is no payload so the payload area can be used to store this information, assuming that
> the payload area is big enough in the first place.  But the payload area is 40 bytes
> (25 bytes plus 15 bytes of padding), which is certainly bigger than 24 bytes,
> so a block of total size 48 would be fine.
> Note that a block cannot be smaller than 32 bytes, as there there would not then
> be enough space to store the header, footer, and freelist links when the block is free.
> 
> Although 16-byte alignment would be adequate for the x86-64 architecture,
> in this program I have arbitrarily set the alignment requirement to 64 bytes.
> In this case, even an allocation request of size 1 will require a block size of at
> least 64 bytes, so the minimum block size for the allocator will be 64 bytes.
> The 64 byte alignment requirement is not motivated by any practical considerations;
> it has been imposed for no reason other than to cause the program parameters to vary
> from what was used in previous semesters.

## Directory Structure

<pre>
.
├── hw3.sublime-project
├── include
│   ├── debug.h
│   └── sfmm.h
├── lib
│   └── sfutil.o
├── Makefile
├── src
│   ├── main.c
│   └── sfmm.c
└── tests
    └── sfmm_tests.c
</pre>

The `lib` folder contains the object file for the `sfutil` library. This
library provides several functions to aid with the implementation
of the allocator. <span style="color:red">**Do NOT delete this file **</span>

The provided `Makefile` creates object files from the `.c` files in the `src`
directory, places the object files inside the `build` directory, and then links
the object files together, including `lib/sfutil.o`, to make executables that
are stored to the `bin` directory.

**Note:** `make clean` will not delete `sfutil.o` or the `lib` folder, but it
will delete all other contained `.o` files.

The `sfmm.h` header file contains function prototypes and defines the format
of the various data structures that are to be used.

> :scream: **DO NOT modify `sfmm.h` or the Makefile.**

All functions for the allocator (`sf_malloc`, `sf_realloc`, `sf_free`, and `sf_memalign`)
**are** implemented in `src/sfmm.c`.

The program in `src/main.c` contains a basic example of using the initialization
and allocation functions together. Running `make` will create a `sfmm`
executable in the `bin` directory. This can be run using the command `bin/sfmm`.

# Allocation Functions

The following four functions in the file `src/sfmm.c` have been implemented.
The file `include/sfmm.h` contains the prototypes and documentation found here.

Standard C library functions set `errno` when there is an error. To avoid
conflicts with these functions, the allocation functions will set `sf_errno`, a
variable declared as `extern` in `sfmm.h`.

```c
/*
 * This is the implementation of sf_malloc. It acquires uninitialized memory that
 * is aligned and padded properly for the underlying system.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If size is 0, then NULL is returned without setting sf_errno.
 * If size is nonzero, then if the allocation is successful a pointer to a valid region of
 * memory of the requested size is returned.  If the allocation is not successful, then
 * NULL is returned and sf_errno is set to ENOMEM.
 */
void *sf_malloc(size_t size);

/*
 * Resizes the memory pointed to by ptr to size bytes.
 *
 * @param ptr Address of the memory region to resize.
 * @param size The minimum size to resize the memory to.
 *
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and sf_errno is set appropriately.
 *
 *   If sf_realloc is called with an invalid pointer sf_errno should be set to EINVAL.
 *   If there is no memory available sf_realloc should set sf_errno to ENOMEM.
 *
 * If sf_realloc is called with a valid pointer and a size of 0 it should free
 * the allocated block and return NULL without setting sf_errno.
 */
void* sf_realloc(void *ptr, size_t size);

/*
 * Marks a dynamically allocated region as no longer in use.
 * Adds the newly freed block to the free list.
 *
 * @param ptr Address of memory returned by the function sf_malloc.
 *
 * If ptr is invalid, the function calls abort() to exit the program.
 */
void sf_free(void *ptr);

/*
 * Allocates a block of memory with a specified alignment.
 *
 * @param align The alignment required of the returned pointer.
 * @param size The number of bytes requested to be allocated.
 *
 * @return If align is not a power of two or is less than the minimum block size,
 * then NULL is returned and sf_errno is set to EINVAL.
 * If size is 0, then NULL is returned without setting sf_errno.
 * Otherwise, if the allocation is successful a pointer to a valid region of memory
 * of the requested size and with the requested alignment is returned.
 * If the allocation is not successful, then NULL is returned and sf_errno is set
 * to ENOMEM.
 */
void *sf_memalign(size_t size, size_t align);
```

# Initialization Functions

In the `build` directory, there is a `sfutil.o` object file.
When linked with the program, this object file allows to access the
`sfutil` library, which contains the following functions:

This library contains the following functions:

```c
/*
 * Any program using the sfmm library must call this function ONCE
 * before issuing any allocation requests. This function DOES NOT
 * allocate any space to the allocator.
 */
void sf_mem_init();

/*
 * Any program using the sfmm library must call this function ONCE
 * after all allocation requests are complete. If implemented cleanly,
 * the program should have no memory leaks in valgrind after this function
 * is called.
 */
void sf_mem_fini();

/*
 * This function increases the size of the heap by adding one page of
 * memory to the end.
 *
 * @return On success, this function returns a pointer to the start of the
 * additional page, which is the same as the value that would have been returned
 * by sf_mem_end() before the size increase.  On error, NULL is returned
 * and sf_errno is set to ENOMEM.
 */
void *sf_mem_grow();

/* The size of a page of memory returned by sf_mem_grow(). */
#define PAGE_SZ 4096

/*
 * @return The starting address of the heap for the allocator.
 */
void *sf_mem_start();

/*
 * @return The ending address of the heap for the allocator.
 */
void *sf_mem_end();
```

The function `sf_mem_init` **MUST** be used to initialize memory.  It is to be
called once **BEFORE** using any of the other functions from `sfutil.o`.
The function `sf_mem_grow` is to be invoked by `sf_malloc`, at the time of the
first allocation request to set up the heap prologue and epilogue and obtain
an initial free block, and on subsequent allocations when a large enough block
to satisfy the request is not found.

> :scream: As these functions are provided in a pre-built .o file, the source
> is not available.
> They must be treated as black boxes.

# sf_mem_grow

For this program, my implementation **ONLY** uses `sf_mem_grow` to
extend the heap.  THere are **NO** system calls such as **brk** or **sbrk**
to do this.

Function `sf_mem_grow` returns memory to the allocator in pages.
Each page is 4096 bytes (4 KB) and there are a limited, small number of pages
available (the actual number may vary, so do not hard-code any particular limit
into the program).  Each call to `sf_mem_grow` extends the heap by one page and
returns a pointer to the new page (this will be the same pointer as would have
been obtained from `sf_mem_end` before the call to `sf_mem_grow`.

The `sf_mem_grow` function also keeps track of the starting and ending addresses
of the heap. One can get these addresses through the `sf_mem_start` and
`sf_mem_end` functions.

> :smile: A real allocator would typically use the **brk**/**sbrk** system calls
> calls for small memory allocations and the **mmap**/**munmap** system calls
> for large allocations.  To allow the program to use other functions provided by
> glibc, which rely on glibc's allocator (*i.e.* `malloc`), we have provided
> `sf_mem_grow` as a safe wrapper around **sbrk**.  This makes it so the heap and
> the one managed by glibc do not interfere with each other.

# Implementation Details

## Memory Row Size

The table below lists the sizes of data types (following Intel standard terminlogy)
on x86-64 Linux Mint:

| C declaration | Data type | x86-64 Size (Bytes) |
| :--------------: | :----------------: | :----------------------: |
| char  | Byte | 1 |
| short | Word | 2 |
| int   | Double word | 4 |
| long int | Quadword | 8 |
| unsigned long | Quadword | 8 |
| pointer | Quadword | 8 |
| float | Single precision | 4 |
| double | Double precision | 8 |
| long double | Extended precision | 16

> :nerd: One can find these sizes themself using the sizeof operator.
> For example, `printf("%lu\n", sizeof(int))` prints 4.

In this program, one can assume that each "memory row" is 8 bytes (64 bits) in size.
Note that, although pointers aligned to addresses that are multiples of 16 would be adequate
to store values of the largest data type (`long double`) supported by the hardware,
all pointers returned by `sf_malloc` are to be 64-byte aligned; that is, they will be
addresses that are multiples of 64.

## Block Header & Footer Fields

The various header and footer formats are specified in `include/sfmm.h`:

```
                                 Format of an allocated memory block
    +-----------------------------------------------------------------------------------------+
    |                                    64-bit-wide row                                      |
    +-----------------------------------------------------------------------------------------+

    +---------------------------------------------------------------------+---------+---------+ <- header
    |                                       block_size                    |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |  (0/1)  |   (1)   |
    |                                        (1 row)                      |  1 bit  |  1 bit  |
    +---------------------------------------------------------------------+---------+---------+ <- (aligned)
    |                                                                                         |
    |                                   Payload and Padding                                   |
    |                                        (N rows)                                         |
    |                                                                                         |
    |                                                                                         |
    +--------------------------------------------+------------------------+---------+---------+

    NOTE: For an allocated block, there is no footer (it is used for payload).
```

```
                                     Format of a free memory block
    +---------------------------------------------------------------------+---------+---------+ <- header
    |                                       block_size                    |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |  (0/1)  |   (0)   |
    |                                        (1 row)                      |  1 bit  |  1 bit  |
    +--------------------------------------------+------------------------+---------+---------+ <- (aligned)
    |                                                                                         |
    |                                Pointer to next free block                               |
    |                                        (1 row)                                          |
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |                               Pointer to previous free block                            |
    |                                        (1 row)                                          |
    +-----------------------------------------------------------------------------------------+
    |                                                                                         | 
    |                                         Unused                                          | 
    |                                        (N rows)                                         |
    |                                                                                         |
    |                                                                                         |
    +--------------------------------------------+------------------------+---------+---------+ <- footer
    |                                       block_size                    |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |  (0/1)  |   (0)   |
    |                                         (1 row)                     |  1 bit  |  1 bit  |
    +---------------------------------------------------------------------+---------+---------+

    NOTE: For a free block, footer contents must always be identical to header contents.
```

The `sfmm.h` header file contains C structure definitions corresponding to the above diagrams:

```c
#define THIS_BLOCK_ALLOCATED  0x1
#define PREV_BLOCK_ALLOCATED  0x2
#define BLOCK_SIZE_MASK 0xfffffffc

typedef size_t sf_header;
typedef size_t sf_footer;

/* Structure of a block. */
typedef struct sf_block {
    sf_footer prev_footer;  // NOTE: This actually belongs to the *previous* block.
    sf_header header;       // This is where the current block really starts.
    union {
        /* A free block contains links to other blocks in a free list. */
        struct {
            struct sf_block *next;
            struct sf_block *prev;
        } links;
        /* An allocated block contains a payload (aligned), starting here. */
        char payload[0];   // Length varies according to block size.
    } body;
} sf_block;
```

For `sf_block`, the `body` field is a `union`, which has been used to emphasize
the difference between the information contained in a free block and that contained
in an allocated block.  If the block is free, then its `body` has a `links` field,
which is a `struct` containing `next` and `prev` pointers.  If the block is
allocated, then its `body` does not have a `links` field, but rather has a `payload`,
which starts at the same address that the `links` field would have started if the
block were free.  The size of the `payload` is obviously not zero, but as it is
variable and only determined at run time, the `payload` field has been declared
to be an array of length 0 just to enable the use of `bp->body.payload` to obtain
a pointer to the payload area, if `bp` is a pointer to `sf_block`.

> :thumbsup:  One can use casts to convert a generic pointer value to one
> of type `sf_block *` or `sf_header *`, in order to make use of the above
> structure definitions to easily access the various fields.

When a block is free, it must have a valid footer whose contents are identical to the
header contents.  We will use a "footer optimization" technique that permits a footer
to be omitted from allocated blocks; thereby making the space that would otherwise
be occupied by the footer available for use by payload.  The footer optimization
technique involves maintaining a bit in the header of each block which can be checked
to find out if the immediately preceding block is allocated or free.
If the preceding block is free, then its footer can be examined to find out its
size and then the size can be used to calculate the block's starting address for the
purpose of performing coalescing.
If the preceding block is **not** free, then it has no footer, but as we can only
coalesce with a free block there is no need for the information that we would have
found in the footer, anyway.

> :scream:  Note that the `prev_footer` field in the `sf_block` structure is actually
> part of the **previous** block in the heap.  In order to initialize an `sf_block`
> pointer to correctly access the fields of a block, it is necessary to compute the
> address of the footer of the immediately preceding block in the heap and then cast
> that address to type `sf_block *`.  The footer of a particular block can be obtained
> by first getting an `sf_block *` pointer for that block and then using the contained
> information (*i.e.* the block size) to obtain the `prev_footer` field of the
> **next** block in the heap.  The `sf_block` structure has been specified this way
> so as to permit it to be defined with a fixed size, even though the payload size
> is unknown and will vary.

## Free List Heads

In the file `include/sfmm.h`, one will see the following declaration:

```c
#define NUM_FREE_LISTS 10
struct sf_block sf_free_list_heads[NUM_FREE_LISTS];
```

The array `sf_free_list_heads` contains the heads of the free lists,
which are maintained as **circular, doubly linked lists**.
Each node in a free list contains a `next` pointer that points to the next
node in the list, and a `prev` pointer that points the previous node.
For each index `i` with `0 <= i < NUM_FREE_LISTS` the variable `sf_free_list_head[i]`
is a dummy, "sentinel" node, which is used to connect the beginning and the end of
the list at index `i`.  This sentinel node is always present and (aside from its `next`
and `free` pointers) does **not** contain any other data.  If the list is empty,
then the fields `sf_freelist_heads[i].body.links.next` and `sf_freelist_heads[i].body.links.prev`
both contain `&sf_freelist_heads[i]` (*i.e.* the sentinel node points back to itself).
If the list is nonempty, then `sf_freelist_heads[i].body.links.next` points to the
first node in the list and `sf_freelist_heads[i].body.links.prev` points to the
last node in the list.
Inserting into and deleting from a circular doubly linked list is done
in the usual way, except that, owing to the use of the sentinel, there
are no edge cases for inserting or removing at the beginning or the end
of the list.
If one needs a further introduction to this data structure, one can readily
find information on it by googling ("circular doubly linked lists with sentinel").

> :scream:  The `sf_free_list_heads` array has been used for the heads
> of the free lists and maintained these lists as circular,
> doubly linked lists.
> The helper functions discussed later, as well as the unit tests,
> will assume that has been done when accessing the free lists.

## Overall Structure of the Heap: Prologue and Epilogue

The heap should use a prologue and epilogue to
arrange for the proper block alignment and to avoid edge cases when coalescing blocks.
The overall organization of the heap is as shown below:

```c
                                         Format of the heap
    +-----------------------------------------------------------------------------------------+
    |                                    64-bit-wide row                                      |
    +-----------------------------------------------------------------------------------------+

                                                                                                   heap start
    +-----------------------------------------------------------------------------------------+ <- (aligned)
    |                                                                                         |
    |                                         unused                                          | padding
    |                                        (7 rows)                                         |
    +--------------------------------------------+------------------------+---------+---------+ <- prologue start
    |                                     block_size (= 64)               |prv alloc|  alloc  | 
    |                                  (2 LSB's implicitly 0)             |   (1)   |   (1)   | prologue
    |                                         (1 row)                     |  1 bit  |  1 bit  | header
    +--------------------------------------------+------------------------+---------+---------+ <- (aligned)
    |                                                                                         |
    |                                         unused                                          | padding
    |                                        (6 rows)                                         |
    +-----------------------------------------------------------------------------------------+
    |                                     block_size (= 64)               |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |   (1)   |   (1)   | prologue
    |                                         (1 row)                     |  1 bit  |  1 bit  | footer
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |                                                                                         |
    |                                                                                         |
    |                                                                                         |
    |                                 Allocated and free blocks                               |
    |                                                                                         |
    |                                                                                         |
    |                                                                                         |
    +---------------------------------------------------------------------+---------+---------+
    |                                     block_size (= 0)                |prv alloc|  alloc  |
    |                                  (2 LSB's implicitly 0)             |  (0/1)  |   (1)   | epilogue
    |                                         (1 row)                     |  1 bit  |  1 bit  |
    +---------------------------------------------------------------------+---------+---------+ <- heap end
                                                                                                   (aligned)
    NOTE: Prologue footer contents must always be identical to prologue header contents.
```

The heap begins with unused "padding", so that the header of each block will start
`sizeof(sf_header)` bytes before an alignment boundary.
The first block of the heap is the "prologue", which is an allocated block of minimum
size that is otherwise unused.

At the end of the heap is an "epilogue", which consists only of an allocated header,
with block size set to 0.
The prologue and epilogue are never used to satisfy allocation requests and they
are never freed.
Whenever the heap is extended, a new epilogue is created at the end of the
newly added region and the old epilogue becomes the header of the new block.
This is as described in the book.

We did not make any separate C structure definitions for the prologue and epilogue.
They have been manipulated using the existing `sf_block` structure, and care has been taken
not to access fields that are not valid for these special blocks
(*i.e.* `prev_footer` for the prologue and anything other than `header` and `prev_footer`
for the epilogue).

As the heap is initially empty, at the time of the first call to `sf_malloc`
I made one call to `sf_mem_grow` to obtain a page of memory
within which to set up the prologue and initial epilogue.
The remainder of the memory in this first page should then be inserted into
the free list as a single block (note that this will be a "wilderness block").

## Notes on sf_malloc

When implementing the `sf_malloc` function, I first determined if the request size
is 0.  If so, then `NULL` was returned without setting `sf_errno`.
If the request size is non-zero, then I determined the size of the
block to be allocated by adding the header size and the size of any necessary
padding to reach a size that is a multiple of 64 to maintain proper alignment.
Remember that the block is big enough to store the footer
(which will always be present), as well as the `next` and `prev` pointers when
the block is free.
As the `next` and `prev` pointers are not present in an allocated block this
space is overlapped with the payload area.
As has already been discussed, the above constraints lead to a minimum block size
of 64 bytes, so an attempt to allocate any block smaller than this has not been made.
After having determined the required block size, I determined the
smallest free list that would be able to satisfy a request of that size.
Search that free list from the beginning until the first sufficiently large
block is found.  If there is no such block, continue with the next larger
size class.
If a big enough block is found, then after splitting it (if it will not leave
a splinter), I inserted the remainder part back into the appropriate
freelist.  When splitting a block, the "lower part" was used to
satisfy the allocation request and the "upper part" became the remainder.
If a big enough block is not found in any of the other freelists, then as a
last resort consider the "wilderness block" (if any) in the last freelist.
If this block does not currently exist or is also not big enough, then 
`sf_mem_grow` was used to request more memory
(for requests larger than a page, more than one such call might be required).
If the allocator ultimately cannot satisfy the request, the `sf_malloc` function
must set `sf_errno` to `ENOMEM` and return `NULL`.

### Notes on sf_mem_grow

After each call to `sf_mem_grow`, one must attempt to coalesce the newly
allocated page with any wilderness block immediately preceding it, in order to build
blocks larger than one page.  Insert the new (wilderness) block at the beginning of
the last freelist.

**Note:** It does not coalesce with the prologue or epilogue, or past the beginning
or end of the heap.

## Notes on sf_free

When implementing `sf_free`, I first verify that the pointer being
passed to the function belongs to an allocated block. This can be done by
examining the fields in the block header and footer.  In this program,
we consider the following cases to be invalid pointers:

- The pointer is `NULL`.
- The pointer is not aligned to a 64-byte boundary.
- The `allocated` bit in the header is 0.
- The header of the block is before the end of the prologue, or the footer
  of the block is after the beginning of the epilogue.
- The `prev_alloc` field is 0, indicating that the previous block is free,
  but the `alloc` field of the previous block header is not 0.

If an invalid pointer is passed to the function, `abort` is called to exit
the program.  Use the man page for the `abort` function to learn more about this.

After confirming that a valid pointer was given, it must free the block.
First, the block must be coalesced with any adjacent free block.
Then, determine the size class appropriate for the (now-coalesced) block and
inserting the block at the beginning of the free list for that size class.
A wilderness block is inserted in the last free list; all other blocks
must be inserted in lists before the last.

Note that blocks in a free list are **not** be marked as allocated,
and they must have a valid footer with contents identical to the block header.

# Notes on sf_realloc

When implementing the `sf_realloc` function, it must first verify that the
pointer and size parameters passed to the function are valid. The criteria for
pointer validity are the same as those described in the 'Notes on sf_free'
section above.  If the pointer is valid but the size parameter is 0,
free the block and return `NULL`.

After verifying both parameters, consider the cases described below.
Note that in some cases, `sf_realloc` is more complicated than calling `sf_malloc`
to allocate more memory, `memcpy` to move the old memory to the new memory, and
`sf_free` to free the old memory.

## Reallocating to a Larger Size

When reallocating to a larger size, it follows these three steps:

1. Call `sf_malloc` to obtain a larger block.

2. Call `memcpy` to copy the data in the block given by the client to the block
returned by `sf_malloc`.  Be sure to copy the entire payload area, but no more.

3. Call `sf_free` on the block given by the client (coalescing if necessary).

4. Return the block given to the by `sf_malloc` to the client.

If `sf_malloc` returns `NULL`, `sf_realloc` must also return `NULL`. Note that
it does not need to set `sf_errno` in `sf_realloc` because `sf_malloc` should
take care of this.

## Reallocating to a Smaller Size

When reallocating to a smaller size, the allocator must use the block that was
passed by the caller.  It must attempt to split the returned block. There are
two cases for splitting:

- Splitting the returned block results in a splinter. In this case, it does not
split the block. Leaves the splinter in the block, updates the header field
if necessary, and returns the same block back to the caller.

**Example:**

<pre>
            b                                               b
+----------------------+                       +------------------------+
| allocated            |                       |   allocated.           |
| Blocksize: 64 bytes  |   sf_realloc(b, 32)   |   Block size: 64 bytes |
| payload: 48 bytes    |                       |   payload: 32 bytes    |
|                      |                       |                        |
|                      |                       |                        |
+----------------------+                       +------------------------+
</pre>

In the example above, splitting the block would have caused a 24-byte splinter.
Therefore, the block is not split.

- The block can be split without creating a splinter. In this case, it splits the
block and updates the block size fields in both headers.  Free the remaining block
by inserting it into the appropriate free list (after coalescing, if possible).
Returns a pointer to the payload of the now-smaller block to the caller.

Note that in both of these sub-cases, it returns a pointer to the same block
that was given to it.

**Example:**

<pre>
            b                                              b
+----------------------+                       +------------------------+
| allocated            |                       | allocated |  free      |
| Blocksize: 128 bytes |   sf_realloc(b, 50)   | 64 bytes  |  64 bytes. |
| payload: 80 bytes    |                       | payload:  |            |
|                      |                       | 50 bytes  | goes into  |
|                      |                       |           | free list  |
+----------------------+                       +------------------------+
</pre>

# Notes on sf_memalign

The `sf_memalign` function permits the caller to request a region of memory that
satisfies a more restrictive alignment requirement than the default 64-byte
alignment.  Such a facility is sometimes required in system programming; for example,
if the memory address returned by the allocator is to be given to an I/O device
that is only capable of accessing memory regions with alignments greater than the
default.

When `sf_memalign` is called, it must check that the requested alignment is at
least as large as the minimum block size.  It must also check that the requested
alignment is a power of two.  If either of these tests fail, `sf_errno` should
be set to `EINVAL` and `sf_memalign` should return `NULL`.
If these tests pass, then `sf_memalign` should treat the requested size in the
say way as for `malloc`.

In order to obtain memory with the requested alignment, `sf_memalign` allocates
a larger block than requested.  Specifically, it attempts to allocate a block
whose size is at least the requested size, plus the alignment size, plus the minimum
block size, plus the size required for a block header.  A block of this size
will have the following property: either the normal payload address of the block
will satisfy the requested alignment (in which case nothing further need be done),
or else there will be a larger address within the block that satisfies the requested
alignment, has sufficient space after it to hold the requested size payload,
and in addition is sufficiently far from the beginning of the block that the initial
portion of the block can itself be split off into a block of at least minimum size
and freed:

<pre>
     large block
+-------------------------+                    +----+--------------------+
|                         |                    |    |                    |
|                         |    split 1         |    |                    |
|                         |      ==>           |free|hdr payload         |
|                         |                    |  1 |    ^               |
|                         |                    |    |    |               |
+---------+---------------+                    +----+----+---------------+
        aligned                                          aligned
</pre>

Once any initial portion of the block has been split off and freed, if the block
is still too large for the requested size then it is subjected to splitting in
the normal way (*i.e.* as in `malloc`) and the unused remainder is freed.

<pre>
+----+--------------------+                    +----+---------------+----+
|    |                    |                    |    |               |    |
|    |                    |                    |    |allocated block|    |
|free|hdr payload         |   split 2          |free|hdr payload    |free|
|  1 |    ^               |     ==>            |  1 |    ^          |  2 |
|    |    |               |                    |    |    |          |    |
+----+----+---------------+                    +----+----+----------+----+
          aligned                                        aligned
</pre>


# Helper Functions

The `sfutil` library additionally contains the following helper functions,
which should be self explanatory.  They all output to `stderr`.

```c
void sf_show_block(sf_block *bp);
void sf_show_blocks();
void sf_show_free_list(int index);
void sf_show_free_lists();
void sf_show_heap();
```

These functions have been provided to help visualize the free lists and
allocated blocks.