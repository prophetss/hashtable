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
	/*
	 * 后期计划参考redis哈希表分两个表以追求resize时负载均衡，
	 * 目前固定使用first，不自动扩容
	 */
    hash_table_element_t  **first_data_store;

    /*暂时没用*/
    hash_table_element_t  **second_data_store;

	/*暂时没用*/
    size_t key_count;

	/*暂时没用*/
    size_t rehashidx;

    /*表容量*/
    size_t table_capacity;

    /* 
     * 表容量对应xxhash结果偏移量，例如表容量为2的16次方，使
     * 用xx64，则rdigits为64-16，下次hash结果直接左移rdigits
     * 就为其实际hash值
     */
    unsigned char table_capacity_rdigits;

} hash_table_t;

hash_table_t* hash_table_new();

hash_table_t* hash_table_new_n(size_t n);

void hash_table_delete(hash_table_t *table);

int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len);

int hash_table_remove(hash_table_t *table, void *key, size_t key_len);

void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len);



