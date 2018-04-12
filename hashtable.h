#include <stddef.h>
#include "xxhash.h"

#ifdef __x86_64__
#define TABLE_BITS          64
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define hash_size_t         XXH64_hash_t
#else
#define TABLE_BITS          32
#define hash_size_t         XXH32_hash_t
#define XXHASH(a, b, c)     XXH32(a, b, c)
#endif

typedef struct hash_table_element hash_table_element_t;

typedef struct hash_table_element
{
	/*key长度*/
    size_t key_len;

	/*value长度*/
    size_t value_len;

    /*key数据*/
    void *key;

   /*value数据*/
    void *value;

    /*下一个相同hash值数据，拉链法*/
    hash_table_element_t *next;

} hash_table_element_t;


typedef struct hash_table
{
    hash_table_element_t  **first_data_store;

    hash_table_element_t  **second_data_store;

    size_t key_count;

    size_t rehashidx;

    size_t table_capacity;

    unsigned char table_capacity_rdigits;

} hash_table_t;


hash_table_t* hash_table_new_n(size_t n);

int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len);

void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len);



