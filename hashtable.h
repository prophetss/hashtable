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
	/* key���� */
	U32 key_len;

	/* value���� */
	U32 value_len;

	/* key���� */
	void *key;

	/* value���� */
	void *value;

	/* ��һ����ͬhashֵ���ݣ������� */
	hash_table_element_t *next;

} hash_table_element_t;

/*
* �������valueʱ�ķ�ʽ������ģʽ����ķ�ʽΪָ�뿽����ɾ��ʱ�����ͷţ�
* ����ģʽ�ڲ������ڴ濽����ֵ��ɾ�����Լ��ͷ�
*/
typedef enum 
{ 
	REF_MODE = 0,
	COPY_MODE
} table_mode_t;

typedef struct hash_table
{
	/* �ȽϺ����� Ĭ��Ϊmemcmp */
	int (*key_compare)(const void *key1, U32 key1_len, const void *key2, U32 key2_len);
		
	/*
	* ��ʼ��ϣ�����������׷��resizeʱ���ؾ��⣬��������ʱ����
	* �ν�first����Ԫ������second����ȫ��ת�����second����
	* ��first��
	*/
	hash_table_element_t  **first_data_store;

	/*
	* ��ʼΪNULL������������ʱ��������СΪfirst���������˺�
	* �ڲ����ѯɾ��ʱ�������first����Ԫ������second����
	*/
	hash_table_element_t  **second_data_store;

	/* first������ */
	U32 table_capacity_1;

	/* second������ */
	U32 table_capacity_2;

	/* ��ǰ����key���� */
	U32 key_count;

	/*
	* ��¼ָ��first���а�˳���һ��key��Ϊ�յ���ţ����ݺ�
	* first��Ԫ������second��ʱʹ��
	*/
	U32 rehashidx;

	/* hash���ӣ�Ĭ��Ϊtime(0) */
	U32 seed;

	/* �������valueʱ�ķ�ʽ(����/����) */
	table_mode_t mode;

} hash_table_t;

/*����ģʽ��ģʽ1����ͬkey�滻����1��ģʽ2�����滻����-1*/
typedef enum { MODE_1 = 0, MODE_2 } set_mode_t;

/*������n-��������mode-ģʽ��������ʵ�ʻ���������2�Ĵ���*/
hash_table_t* hash_table_new_n(U32 n, table_mode_t mode);

int hash_table_set(hash_table_t* table, const void* key, U32 key_len, void* value, U32 value_len, set_mode_t mode);

/*��ѯ���ɹ�����valueָ�룬ʧ�ܷ���NULL*/
void* hash_table_lookup(hash_table_t* table, const void* key, U32 key_len);

/*ɾ����ʧ�ܷ���-1���ɹ�����0*/
int hash_table_remove(hash_table_t* table, const void* key, U32 key_len);

/*ɾ�����ͷ���Դ*/
void hash_table_destory(hash_table_t* table);

/*
* ��ȡhash��������Ԫ�أ�table->key_count��Ԫ�أ�������Ԫ�������б��ڲ����ڴ����룬�����ͷţ���
* ����ģʽ�������ݣ�����ģʽ����ָ�����飬ָ��ָ��table�������ݣ��ձ���NULL
*/
hash_table_element_t* hash_table_elements(hash_table_t* table, table_mode_t mode);

/*����hash��������ӳ�����Ԫ��*/
void hash_table_rehash(hash_table_t* table, U32 seed);

/*Ĭ�ϳ�ʼ������*/
#define DEFAULT_CAPACITY        (8)

#define htnew()    hash_table_new_n(DEFAULT_CAPACITY, REF_MODE)

/*��ӣ��ɹ�����0��key��ͬ�滻����1*/
#define htset(table, key, key_len, value, value_len)	hash_table_set(table, key, key_len, value, value_len, MODE_1)

/*��ӣ��ɹ�����0��key��ͬ���滻����-1*/
#define htsetnx(table, key, key_len, value, value_len)	hash_table_set(table, key, key_len, value, value_len, MODE_2)

#define htget(table, key, key_len)	hash_table_lookup(table, key, key_len)

#define htrm(table, key, key_len)	hash_table_remove(table, key, key_len)

#define htdel(table)	hash_table_destory(table)

#define htgetall(table, mode)	hash_table_elements(table, mode)

#define htrh(table, seed)	hash_table_rehash(table, seed)

/*���ü�ֵ�ȽϺ���(��Ӧ������key1, key1_len, key2, key2_len������ֵ������ͬ0������)����������Ĭ��ʹ��memcmp*/
void ht_set_compare_func(hash_table_t* table, int (*kc_func)(const void*, U32, const void*, U32));


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
