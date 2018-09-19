#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_


#if defined (__cplusplus)
extern "C" {
#endif


#include "xxhash.h"

/*64位使用xx64，32位使用xx32*/
#if defined(__x86_64__) || defined(_WIN64)
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define hash_size_t         XXH64_hash_t
#else
#define hash_size_t         XXH32_hash_t
#define XXHASH(a, b, c)     XXH32(a, b, c)
#endif

/*默认初始表容量*/
#define DEFAULT_CAPACITY        (1024*2048)

/*
 * 负载因子，负载因子乘以表容量为实际最大可容纳元素数量值，
 * 超过此值就会发生表扩容
 */
#define LOAD_FACTOR              (0.75)

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
	 * 初始表，希表分两个表以追求resize时负载均衡，当表扩容时会逐
	 * 次将first表内没饿元素移至second，当全部转移完后second表替
     * 换first表
	 */
    hash_table_element_t  **first_data_store;

    /*
     * 初始为NULL，发生表扩容时产生，大小为first表两倍，此后
     * 在插入查询删除时会逐个将first表中元素移至second表中
     */
    hash_table_element_t  **second_data_store;

    /* 表容量 */
    hash_size_t table_capacity;

	/* 当前表内key数量 */
    hash_size_t key_count;

    /*
     * 表内最大key数量，其值等于table_capacity*LOAD_FACTOR，
     * 当key_count大于等于此值时会扩容
     */
    hash_size_t max_key_count;

    /*
     * 记录指向first表中按顺序第一个key不为空的序号，扩容后
     * first表元素移至second表时使用
     */
    hash_size_t rehashidx;

    /* 表内添加value时的方式：REF_MODE-为引用模式，COPY_MODE-为拷贝模式 */
    table_mode_t mode;

    /* hash种子，默认为time(0) */
    hash_size_t seed;

} hash_table_t;



/*表创建，n-表容量，lf-负载因子，mode-模式，表容量实际会向上扩至2的次幂*/
hash_table_t* hash_table_new_n(size_t n, float lf, table_mode_t mode);

/*同上表创建，手动设置seed-哈希种子*/
hash_table_t* hash_table_new_ns(size_t n, float lf, table_mode_t mode, hash_size_t seed);

#define hash_table_new(mode)    hash_table_new_n(DEFAULT_CAPACITY, LOAD_FACTOR, mode)

#define hash_table_new_s(mode, seed)    hash_table_new_ns(DEFAULT_CAPACITY, LOAD_FACTOR, mode, seed)

/*删除表释放资源*/
void hash_table_delete(hash_table_t *table);

/*添加，失败返回-1，成功返回0，发生替换返回1*/
int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len);

/*删除，失败返回-1，成功返回0*/
int hash_table_remove(hash_table_t *table, void *key, size_t key_len);

/*查询，成功返回value指针，失败返回NULL*/
void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len);

/*
 * 获取hash表内所有元素（table->key_count个元素），返回元素数组列表。拷贝模式复制数据，
 * 引用模式返回指针数组，指针指向table表内数据，空表返回NULL
 */
hash_table_element_t* hash_table_elements(hash_table_t *table, table_mode_t mode);


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
