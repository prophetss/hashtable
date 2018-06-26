#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_


#include <stddef.h>
#include "xxhash.h"

/*64位使用xx64，32位使用xx32*/
#ifdef __x86_64__
#define TABLE_BITS          64
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define hash_size_t         XXH64_hash_t
#else
#define TABLE_BITS          32
#define hash_size_t         XXH32_hash_t
#define XXHASH(a, b, c)     XXH32(a, b, c)
#endif

/* 
 * 表内添加value时的方式：引用模式value传入的方式为引用，
 * 删除时不会释放；拷贝模式内部申请内存拷贝value值，删除
 * 会释放表内value
 */
typedef enum {REF_MODE = 0, COPY_MODE} table_mode_t;


typedef struct hash_table_element hash_table_element_t;

typedef struct hash_table_element
{
	/* key长度 */
    size_t key_len;

	/* value长度 */
    size_t value_len;

    /* key数据 */
    void *key;

    /* value数据 */
    void *value;

    /* 下一个相同hash值数据，拉链法 */
    hash_table_element_t *next;

} hash_table_element_t;


typedef struct hash_table
{
	/*
	 * 初始表，参考redis哈希表分两个表以追求resize时负载均衡，
	 * 当表扩容时会逐次将first表内没饿元素移至second，当全部
     * 转移完后second表替换first表
	 */
    hash_table_element_t  **first_data_store;

    /*
     * 初始为NULL，发生表扩容时产生，大小为first表两倍，此后
     * 在插入查询删除时会逐个将first表中元素移至second表中
     */
    hash_table_element_t  **second_data_store;

    /*
     * 记录指向first表中按顺序第一个key不为空的序号，扩容后
     * first表元素移至second表时使用
     */
    size_t rehashidx;

    /* 表容量 */
    size_t table_capacity;

	/* 当前表内key数量 */
    size_t key_count;

    /*
     * 表内最大key数量，其值等于table_capacity*LOAD_FACTOR，当
     * key_count大于等于此值是会扩容
     */
    size_t max_key_count;

    /* 表内添加value时的方式：REF_MODE-为引用模式，COPY_MODE-为拷贝模式 */
    table_mode_t mode;

    /* 
     * 表容量对应xxhash结果偏移量，例如表容量为2的16次方，使
     * 用xx64，则rdigits为64-16，下次hash结果直接左移rdigits
     * 就为其实际hash值
     */
    unsigned char table_capacity_rdigits;

} hash_table_t;


/*以默认大小-DEFAULT_CAPACITY宏创建hash表，失败返回NULL*/
hash_table_t* hash_table_new(table_mode_t mode);

/*以指定大小n创建hash表，失败返回NULL*/
hash_table_t* hash_table_new_n(size_t n, table_mode_t mode);

/*删除表释放资源*/
void hash_table_delete(hash_table_t *table);

/*添加，失败返回-1，成功返回0，发生替换返回1*/
int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len);

/*删除，失败返回-1，成功返回0*/
int hash_table_remove(hash_table_t *table, void *key, size_t key_len);

/*查询，成功返回value指针，失败返回NULL*/
void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len);


#endif /*!_HASHTABLE_H_*/
