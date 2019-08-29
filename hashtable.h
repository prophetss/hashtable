#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_


#if defined (__cplusplus)
extern "C" {
#endif


#include "xxhash.h"


#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
	typedef uint32_t U32;
#else
	typedef unsigned int U32;
#endif

typedef struct hash_table_element hash_table_element_t;

typedef struct hash_table_element
{
	/* key长度 */
	U32 key_len;

	/* value长度 */
	U32 value_len;

	/* key数据 */
	void *key;

	/* value数据 */
	void *value;

	/* 下一个相同hash值数据，拉链法 */
	hash_table_element_t *next;

} hash_table_element_t;

/*
* 表内添加value时的方式：引用模式传入的方式为指针拷贝，删除时不会释放；
* 拷贝模式内部申请内存拷贝其值，删除会自己释放
*/
typedef enum 
{ 
	REF_MODE = 0,
	COPY_MODE
} table_mode_t;

typedef struct hash_table
{
	/* 比较函数， 默认为memcmp */
	int (*key_compare)(const void *key1, U32 key1_len, const void *key2, U32 key2_len);
		
	/*
	* 初始表，希表分两个表以追求resize时负载均衡，当表扩容时会逐
	* 次将first表内元素移至second，当全部转移完后second表替
	* 换first表
	*/
	hash_table_element_t  **first_data_store;

	/*
	* 初始为NULL，发生表扩容时产生，大小为first表两倍，此后
	* 在插入查询删除时会逐个将first表中元素移至second表中
	*/
	hash_table_element_t  **second_data_store;

	/* first表容量 */
	U32 table_capacity_1;

	/* second表容量 */
	U32 table_capacity_2;

	/* 当前表内key数量 */
	U32 key_count;

	/*
	* 记录指向first表中按顺序第一个key不为空的序号，扩容后
	* first表元素移至second表时使用
	*/
	U32 rehashidx;

	/* hash种子，默认为time(0) */
	U32 seed;

	/* 表内添加value时的方式(拷贝/引用) */
	table_mode_t mode;

} hash_table_t;

/*插入模式，模式1：相同key替换返回1，模式2：不替换返回-1*/
typedef enum { MODE_1 = 0, MODE_2 } set_mode_t;

/*表创建，n-表容量，mode-模式，表容量实际会向上扩至2的次幂*/
hash_table_t* hash_table_new_n(U32 n, table_mode_t mode);

int hash_table_set(hash_table_t* table, const void* key, U32 key_len, void* value, U32 value_len, set_mode_t mode);

/*查询，成功返回value指针，失败返回NULL*/
void* hash_table_lookup(hash_table_t* table, const void* key, U32 key_len);

/*删除，失败返回-1，成功返回0*/
int hash_table_remove(hash_table_t* table, const void* key, U32 key_len);

/*删除表释放资源*/
void hash_table_destory(hash_table_t* table);

/*
* 获取hash表内所有元素（table->key_count个元素），返回元素数组列表（内部有内存申请，自行释放）。
* 拷贝模式复制数据，引用模式返回指针数组，指针指向table表内数据，空表返回NULL
*/
hash_table_element_t* hash_table_elements(hash_table_t* table, table_mode_t mode);

/*以新hash种子重新映射表内元素*/
void hash_table_rehash(hash_table_t* table, U32 seed);

/*默认初始表容量*/
#define DEFAULT_CAPACITY        (8)

#define htnew()    hash_table_new_n(DEFAULT_CAPACITY, REF_MODE)

/*添加，成功返回0，key相同替换返回1*/
#define htset(table, key, key_len, value, value_len)	hash_table_set(table, key, key_len, value, value_len, MODE_1)

/*添加，成功返回0，key相同不替换返回-1*/
#define htsetnx(table, key, key_len, value, value_len)	hash_table_set(table, key, key_len, value, value_len, MODE_2)

#define htget(table, key, key_len)	hash_table_lookup(table, key, key_len)

#define htrm(table, key, key_len)	hash_table_remove(table, key, key_len)

#define htdel(table)	hash_table_destory(table)

#define htgetall(table, mode)	hash_table_elements(table, mode)

#define htrh(table, seed)	hash_table_rehash(table, seed)

/*设置键值比较函数(对应参数：key1, key1_len, key2, key2_len。返回值：不相同0，其他)。如无设置默认使用memcmp*/
void ht_set_compare_func(hash_table_t* table, int (*kc_func)(const void*, U32, const void*, U32));


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
