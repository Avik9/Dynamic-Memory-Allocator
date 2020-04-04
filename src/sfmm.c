/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

#include <errno.h> // Added so sf_errno could be set to ENOMEM
#include "helper.h"

void *mem_grow_ptr;
void *mem_end_ptr;
int position_in_list;

sf_block *prologue_ptr;
sf_block *epilogue_ptr;
sf_block *curr_block_ptr;
sf_block *block_to_coalesce;

sf_block *prev_block_get_blocks;
sf_block *temp_block_get_blocks;
sf_block *next_block_get_blocks;

void *sf_malloc(size_t size)
{
    if (size <= 0)
        return NULL;

    if ((sf_mem_start() == sf_mem_end()) && first_call_to_sf_malloc() == -1)
    {
        sf_errno = ENOMEM;
        return NULL;
    }

    int rounded_size = round_to_64(size + sizeof(sf_header));
    curr_block_ptr = NULL;
    int found_empty_block = search_empty_block(rounded_size);

    if (found_empty_block == -1)
    {
        sf_errno = ENOMEM;
        return NULL;
    }

    if (GET_BLOCK_SIZE(&curr_block_ptr->header) - rounded_size >= BLOCK_SZ) // Split the current block
    {
        if (found_empty_block >= NUM_FREE_LISTS - 1)
            split_block(rounded_size, 1);
        else
            split_block(rounded_size, 0);
    }
    else
    {
        remove_block_from_list();
        get_blocks();

        if (IS_ALLOC(prev_block_get_blocks) == 1)
            curr_block_ptr->header += 2;
        curr_block_ptr->header += 1;
    }

    clear_empty_blocks();
    return curr_block_ptr->body.payload;
}

void sf_free(void *pp)
{
    if (!valid_pointer(pp))
        abort();

    curr_block_ptr->header -= 1; // Free the block
    sf_block *wilderness_block = sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next;

    // Case 1: Prev and Next
    if ((GET_ALLOC(&prev_block_get_blocks->header)) && (GET_ALLOC(&next_block_get_blocks->header)))
        debug("Prev and Next");
    
    // Case 2: Prev and !Next
    else if ((&next_block_get_blocks->header != &epilogue_ptr->header) && (GET_ALLOC(&prev_block_get_blocks->header)) && (!GET_ALLOC(&next_block_get_blocks->header)))
    {
        block_to_coalesce = next_block_get_blocks;

        if (!remove_block(block_to_coalesce))
            debug("ERROR IN PREV AND !NEXT");

        coalesce();
    }

    // Case 3: !Prev and Next
    else if ((&prev_block_get_blocks->header != &prologue_ptr->header) && (!GET_ALLOC(&prev_block_get_blocks->header)) && (GET_ALLOC(&next_block_get_blocks->header)))
    {
        block_to_coalesce = curr_block_ptr;
        curr_block_ptr = prev_block_get_blocks;

        if (!remove_block(prev_block_get_blocks))
            debug("ERROR IN !PREV AND NEXT");

        coalesce();
    }

    // Case 4: !Prev and !Next
    else if ((!GET_ALLOC(&prev_block_get_blocks->header)) && (!GET_ALLOC(&next_block_get_blocks->header)))
    {
        if (&prev_block_get_blocks->header != &prologue_ptr->header)
        {
            block_to_coalesce = curr_block_ptr;
            curr_block_ptr = prev_block_get_blocks;
            if (!remove_block(prev_block_get_blocks))
                debug("ERROR IN !PREV AND NEXT PART OF !PREV AND !NEXT");
            
            coalesce();
        }
        if (&next_block_get_blocks->header != &epilogue_ptr->header)
        {
            block_to_coalesce = next_block_get_blocks;

            if (!remove_block(block_to_coalesce))
                debug("ERROR IN PREV AND !NEXT");
            
            coalesce();
        }
    }

    (GET_NEXT_BLOCK(curr_block_ptr))->prev_footer = curr_block_ptr->header;

    if (wilderness_block != block_to_coalesce && &sf_free_list_heads[NUM_FREE_LISTS - 1] != sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next)
        add_block_to_list(curr_block_ptr, 0);
    else
        add_block_to_list(curr_block_ptr, 1);
}

void *sf_realloc(void *pp, size_t rsize)
{
    if (valid_pointer(pp))
    {
        if (rsize == 0)
        {
            sf_free(pp);
            return NULL;
        }
    }
    else
        abort();

    sf_block *pp_block = GET_BLOCK_FROM_PAYLOAD(pp);
    int rounded_rsize = round_to_64(rsize + sizeof(sf_header));

    if (GET_BLOCK_SIZE(&pp_block->header) < rounded_rsize)
    {
        sf_block *tmp_ptr = sf_malloc(rsize);

        if (tmp_ptr == NULL)
            return NULL;

        int x = memcmp(tmp_ptr, pp, rsize);
        x += 2;

        sf_free(pp);

        return tmp_ptr;
    }
    else if (GET_BLOCK_SIZE(&pp_block->header) > rounded_rsize)
    {
        if (GET_BLOCK_SIZE(&pp_block->header) >= rounded_rsize + BLOCK_SZ)
        {
            sf_block *temp_block_ptr = (sf_block *)(((void *)pp_block) + (rounded_rsize));

            temp_block_ptr->header = pp_block->header - rounded_rsize;
            pp_block->header = pp_block->header - GET_BLOCK_SIZE(&pp_block->header) + rounded_rsize;
            temp_block_ptr->prev_footer = pp_block->header;

            (GET_NEXT_BLOCK(pp_block))->prev_footer = pp_block->header;

            sf_free(temp_block_ptr->body.payload);
        }
    }

    return pp;
}

void *sf_memalign(size_t size, size_t align)
{
    if (align < BLOCK_SZ || !is_power_of_2(align))
    {
        sf_errno = EINVAL;
        return NULL;
    }

    int malloc_size = size + align + BLOCK_SZ; // sizeof(sf_header) is already added in malloc
    curr_block_ptr = GET_BLOCK_FROM_PAYLOAD(sf_malloc(malloc_size));

    int new_align = align - INITIAL_PADDING + (2 * sizeof(sf_header));

    sf_block *block_to_free_after = (sf_block *)(((void *)curr_block_ptr) + new_align);

    if (align == BLOCK_SZ)
    {
        if(GET_BLOCK_SIZE(&block_to_free_after->header) >= round_to_64(size) + BLOCK_SZ)
            return sf_realloc(block_to_free_after->body.payload, size);
        return curr_block_ptr->body.payload;
    }
    else
    {
        if ((curr_block_ptr->header & PREV_BLOCK_ALLOCATED) == 2)
            curr_block_ptr->header = new_align + 3;
        else
            curr_block_ptr->header = new_align + 1;

        block_to_free_after->prev_footer = curr_block_ptr->header;
        
        block_to_free_after->header = round_to_64(malloc_size) - GET_BLOCK_SIZE(&curr_block_ptr->header) + 3;

        (GET_NEXT_BLOCK(block_to_free_after))->prev_footer = block_to_free_after->header;    
        sf_free(curr_block_ptr->body.payload);
        sf_realloc(block_to_free_after->body.payload, size);
    }
    
    return curr_block_ptr->body.payload;
}

void coalesce()
{
    curr_block_ptr->header += GET_BLOCK_SIZE(&block_to_coalesce->header);
    (GET_NEXT_BLOCK(curr_block_ptr))->prev_footer = curr_block_ptr->header;
}

int valid_pointer(void *pp)
{
    curr_block_ptr = GET_BLOCK_FROM_PAYLOAD(pp);
    get_blocks();

    if (pp == NULL) // The pointer is NULL
        abort();
    
    if (((((void *)&epilogue_ptr->header) - ((void *)&curr_block_ptr->header)) % 64) != 0) // The pointer is not aligned to a 64-byte boundary
        abort();
    
    if (!IS_ALLOC(&curr_block_ptr->header)) // The allocated bit in the header is 0
        abort();
    
    if (curr_block_ptr >= epilogue_ptr) // The header of the block is before the end of the prologue
        abort();
    
    if (curr_block_ptr <= prologue_ptr) // The footer of the block is after the beginning of the epilogue
        abort();
    
    if ((IS_PREV_ALLOC(&curr_block_ptr->header) == 0) && (IS_ALLOC(&prev_block_get_blocks->header) != 0)) // The prev_alloc field is 0 but the alloc field of the previous block header is not 0.
        abort();
    
    return 1;
}

void split_block(size_t size, int is_wilderness)
{
    sf_block *new_block_ptr = (sf_block *)(((void *)curr_block_ptr) + size);

    new_block_ptr->header = curr_block_ptr->header - size;
    curr_block_ptr->header = size + 3;
    new_block_ptr->prev_footer = curr_block_ptr->header;

    (GET_NEXT_BLOCK(new_block_ptr))->prev_footer = new_block_ptr->header;

    add_block_to_list(new_block_ptr, is_wilderness);
}

int search_empty_block(size_t size)
{
    position_in_list = list_position(size);
    int i = position_in_list;
    sf_block *traverse_block;

    for (; i < NUM_FREE_LISTS; i++)
    {
        if (&sf_free_list_heads[i] == sf_free_list_heads[i].body.links.next) {}
        else
        {
            traverse_block = sf_free_list_heads[i].body.links.next;
            while (traverse_block != &sf_free_list_heads[i])
            {
                if (GET_BLOCK_SIZE(&traverse_block->header) >= size)
                {
                    curr_block_ptr = traverse_block;
                    break;
                }
                traverse_block = traverse_block->body.links.next;
            }
            if (&curr_block_ptr->header == &traverse_block->header)
                break;
        }
    }

    if (curr_block_ptr == traverse_block)
    {
        remove_block_from_list();
        return i;
    }

    if (&sf_free_list_heads[NUM_FREE_LISTS - 1] != sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next)
        curr_block_ptr = sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next;
    else
        curr_block_ptr = epilogue_ptr;

    int available_space = GET_BLOCK_SIZE(&curr_block_ptr->header);

    while (size > available_space)
    {
        if (sf_mem_grow() == NULL)
            return -1;

        available_space += PAGE_SZ;
        curr_block_ptr->header += PAGE_SZ;

        if (&sf_free_list_heads[NUM_FREE_LISTS - 1] != sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next)
            add_block_to_list(curr_block_ptr, 1);

        add_epilogue();
    }
    return i;
}

void remove_block_from_list()
{
    sf_block *prev_block = curr_block_ptr->body.links.prev;
    sf_block *next_block = curr_block_ptr->body.links.next;

    next_block->body.links.prev = prev_block;
    prev_block->body.links.next = next_block;
}

void add_block_to_list(sf_block *block_to_add, int is_wilderness)
{
    if (is_wilderness)
    {
        sf_block *tp = curr_block_ptr;
        curr_block_ptr = block_to_add;
        get_blocks();
        curr_block_ptr = tp;

        block_to_add->header = GET_BLOCK_SIZE(&block_to_add->header) + GET_PREV_ALLOC(&block_to_add->header);
        block_to_add->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS - 1];
        block_to_add->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS - 1];
        sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next = block_to_add;
        sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.prev = block_to_add;

        epilogue_ptr->prev_footer = block_to_add->header;
    }
    else
    {
        int position = list_position(GET_BLOCK_SIZE(&block_to_add->header));

        if (sf_free_list_heads[position].body.links.next != &sf_free_list_heads[position]) // Something already in the list
        {
            block_to_add->body.links.next = sf_free_list_heads[position].body.links.next;
            block_to_add->body.links.prev = &sf_free_list_heads[position];
            sf_free_list_heads[position].body.links.next = block_to_add;
        }
        else // Nothing in the list
        {
            block_to_add->body.links.next = &sf_free_list_heads[position];
            block_to_add->body.links.prev = &sf_free_list_heads[position];
            sf_free_list_heads[position].body.links.next = block_to_add;
            sf_free_list_heads[position].body.links.prev = block_to_add;
        }
    }

    clear_empty_blocks();
}

int remove_block(sf_block *block_to_remove)
{
    int position = list_position(GET_BLOCK_SIZE(&block_to_remove->header));
    sf_block *cursor = sf_free_list_heads[position].body.links.next;

    while (&cursor->header != (&block_to_remove->header))
    {
        cursor = cursor->body.links.next;

        if (cursor == &sf_free_list_heads[position])
            return 0;
    }

    block_to_remove->body.links.next->body.links.prev = block_to_remove->body.links.prev;
    block_to_remove->body.links.prev->body.links.next = block_to_remove->body.links.next;

    return 1;
}

int list_position(int num_bytes)
{
    int upper_limit[] = {1, 2, 3, 5, 8, 13, 21, 34};

    for (int i = 0; i < 8; i++)
    {
        if (upper_limit[i] * BLOCK_SZ >= num_bytes)
            return i;
    }

    return 8;
}

int round_to_64(size_t size)
{
    if (size % BLOCK_SZ == 0)
        return size;

    return size + BLOCK_SZ - (size % BLOCK_SZ);
}

void add_epilogue()
{
    epilogue_ptr = (sf_block *)(sf_mem_end() - (2 * sizeof(sf_header))); // Heap end - 8
    epilogue_ptr->header = 1;
    epilogue_ptr->prev_footer = curr_block_ptr->header;
}

int first_call_to_sf_malloc()
{
    if (sf_mem_grow() == NULL) // No memory is left
        return -1;

    mem_grow_ptr = sf_mem_start();
    init_lists();

    // Insert padding (48) and Prologue (64)
    prologue_ptr = (sf_block *)(mem_grow_ptr + INITIAL_PADDING); // INITIAL_PADDING  = 48
    prologue_ptr->header = BLOCK_SZ + 3;                         // 64 for the block size and 3 for prev and current block being allocated

    curr_block_ptr = (sf_block *)(sf_mem_start() + INITIAL_PADDING + BLOCK_SZ);
    curr_block_ptr->header = PAGE_SZ - (INITIAL_PADDING + BLOCK_SZ + (2 * sizeof(sf_header))) + 2; // PAGE_SZ - sizeof(initial padding) - sizeof(prologue) - sizeof(epilogue)
    curr_block_ptr->prev_footer = prologue_ptr->header;

    curr_block_ptr->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS - 1];
    curr_block_ptr->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS - 1];
    sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.prev = curr_block_ptr;
    sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next = curr_block_ptr;

    add_epilogue();

    return 1;
}

void init_lists()
{
    // If the list is empty, then the fields sf_free_list_heads[i].body.links.next
    // and sf_free_list_heads[i].body.links.prev both contain &sf_free_list_heads[i]

    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

void count_num_blocks(int pos, int size)
{
    sf_block *tmp = sf_free_list_heads[pos].body.links.next;
    int counter = 0;

    while (tmp != &sf_free_list_heads[pos])
    {
        counter += 1;
        tmp = tmp->body.links.next;
    }
}

void clear_empty_blocks()
{
    sf_block *tmp;

    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
        tmp = sf_free_list_heads[i].body.links.next;

        if (tmp != &sf_free_list_heads[i])
        {
            if (GET_BLOCK_SIZE(&tmp->header) == 0)
            {
                sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
                sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
            }
        }
    }
}

void get_blocks()
{
    prev_block_get_blocks = prologue_ptr;
    temp_block_get_blocks = prologue_ptr;

    do
    {
        if (temp_block_get_blocks == epilogue_ptr)
            break;

        prev_block_get_blocks = temp_block_get_blocks;
        temp_block_get_blocks = GET_NEXT_BLOCK(temp_block_get_blocks);
    } while ((&((temp_block_get_blocks))->header) != &(curr_block_ptr->header));

    next_block_get_blocks = GET_NEXT_BLOCK(curr_block_ptr);
}

int is_power_of_2(int value)
{

    if (value == 0 || value == 1)
        return 1;
    
    int old_value = value;
    while (value > 1)
    {
        old_value = value;
        value /= 2;
        if (value * 2 != old_value)
            return 0;
    }
    if (value != 1)
        return 0;
    return value;
}