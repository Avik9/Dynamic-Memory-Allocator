#define BLOCK_SZ  64
#define INITIAL_PADDING BLOCK_SZ - (2 * sizeof(sf_header))

#define GET(p) (*(unsigned int *)(p))
#define GET_ALLOC(p) (GET(p) & THIS_BLOCK_ALLOCATED)
#define IS_ALLOC(p) (GET(p) & THIS_BLOCK_ALLOCATED)
#define IS_PREV_ALLOC(p) (GET_PREV_ALLOC(p) == 2)
#define GET_PREV_ALLOC(p) (GET(p) & PREV_BLOCK_ALLOCATED)
#define GET_BLOCK_SIZE(p) (GET(p) & BLOCK_SIZE_MASK)
#define GET_BLOCK_FROM_PAYLOAD(pp) ((sf_block *)((void *)(pp) - (2 * sizeof(sf_header))))
#define GET_NEXT_BLOCK(pp) (sf_block *)((void *)(pp) + GET_BLOCK_SIZE(&pp->header))

int list_position(int num_bytes);
int first_call_to_sf_malloc();
void init_lists();
void add_epilogue();
int round_to_64(size_t size);
int search_empty_block(size_t size);
void split_block(size_t size, int is_wilderness);
int valid_pointer(void *pp);
void add_block_to_list(sf_block *block_to_add, int is_wilderness);
int remove_block(sf_block *block_to_remove);
void coalesce();
void count_num_blocks(int pos, int size);
void clear_empty_blocks();
void get_blocks();
int is_power_of_2(int value);
void remove_block_from_list();