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

typedef struct ht_elem ht_elem_t;

typedef struct ht_elem
{
	/* key长度 */
	U32 kl;

	/* value长度 */
	U32 vl;

	/* key数据 */
	void *key;

	/* value数据 */
	void *val;

	/* 下一个相同hash值数据，拉链法 */
	ht_elem_t *next;

} ht_elem_t;

typedef struct tb
{
	ht_elem_t** ds;
	U32 tc;
}tb_t;

/*
* 表内添加value时的方式：引用模式传入的方式为指针拷贝,
* 拷贝模式内部申请内存拷贝其值，删除会自己释放
*/
typedef enum { REF_MODE = 0, COPY_MODE } tmode_t;

typedef struct ht
{
	/* 比较函数， 默认为memcmp */
	int (*kc)(const void *k1, U32 kl1, const void *k2, U32 kl2);

	/*
	* 初始哈希表分两个表以追求resize时负载均衡，当表扩容时
	* 会逐次将0表内元素移至1，当全部转移完后1表替换0表,
	* 1初始为NULL，发生表扩容时产生，大小为0表两倍，此后
	* 在插入查询删除时会逐个将0表中元素移至1表中
	*/
	tb_t	tbs[2];

	/* 当前key数量 */
	U32 used;

	/* 表内允许最大元素数量 */
	U32 ht_max;

	/* 表内允许最少元素数量 */
	U32 ht_min;

	/*
	* 记录指向first表中按顺序第一个key不为空的序号，扩容后
	* first表元素移至second表时使用
	*/
	U32 rehashidx;

	/* hash种子，默认为time(0) */
	U32 seed;

	/* 表内添加value时的方式(拷贝/引用) */
	tmode_t mode;

} ht_t;

/**
 * 创建哈希表
 * @param n-表容量（表容量实际会向上扩至2的次幂）
 * @param tmode-模式
 * @return 失败返回NULL（只有n过大才会发生）
 */
ht_t* hash_table_new_n(U32 n, tmode_t tmode);

/*插入模式，模式1：相同key替换返回1，模式2：不替换返回-1*/
typedef enum { MODE_1 = 0, MODE_2 } smode_t;

int hash_table_set(ht_t* table, const void* key, U32 kl, void* val, U32 vl, smode_t smode);

/**
 * 查询
 * @return 成功返回value指针，失败返回NULL
 */
void* hash_table_lookup(ht_t* table, const void* key, U32 kl);

/**
 * 删除
 * @return 失败返回-1，成功返回0
 */
int hash_table_remove(ht_t* table, const void* key, U32 kl);

/*删除表释放资源*/
void hash_table_destory(ht_t* table);

/**
 * 获取hash表内所有元素，key为数据拷贝
 * @param tmode-REF_MODE：value拷贝指针, COPY_MODE：value拷贝数据
 * @return 元素链表
 */
ht_elem_t* hash_table_elements(ht_t* table, tmode_t tmode);

/*释放hash_table_elements返回指针的指向内存，tmode必须和前者一致*/
void hash_table_remove_elements(ht_elem_t* elems, tmode_t tmode);

/*以新hash种子重新映射表内元素*/
void hash_table_rehash(ht_t* table, U32 seed);

/*设置键值比较函数(对应参数：k1, kl1, k2, kl2。返回值：不相同0，其他)。如无设置默认使用memcmp*/
void hash_table_set_compare_func(ht_t* table, int (*kc_func)(const void*, U32, const void*, U32));

#define htnew()    hash_table_new_n(0, REF_MODE)	//容量0会扩展成默认最小值8

/*添加，成功返回0，key相同替换返回1*/
#define htset(table, key, kl, val, vl)	hash_table_set(table, key, kl, val, vl, MODE_1)

/*添加，成功返回0，key相同失败不替换返回-1*/
#define htsetnx(table, key, kl, val, vl)	hash_table_set(table, key, kl, val, vl, MODE_2)

#define htget(table, key, kl)	hash_table_lookup(table, key, kl)

#define htrm(table, key, kl)	hash_table_remove(table, key, kl)

#define htdel(table)	hash_table_destory(table)

#define htgetall(table, mode)	hash_table_elements(table, mode)

#define htrmall(elems, tmode)	hash_table_remove_elements(elems, tmode)

#define htrh(table, seed)	hash_table_rehash(table, seed)

#define  htsetcmp(table, kc_func)	hash_table_set_compare_func(table, kc_func);


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
