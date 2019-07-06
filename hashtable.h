#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_


#if defined (__cplusplus)
extern "C" {
#endif


#include "xxhash.h"


/*64λʹ��xx64��32λʹ��xx32*/
#if defined(__x86_64__) || defined(_WIN64)
#define hash_size_t         XXH64_hash_t
#else
#define hash_size_t         XXH32_hash_t
#endif

/*Ĭ�ϳ�ʼ������*/
#define DEFAULT_CAPACITY        (8)

/*
* ��������Ӱٷֱȣ����Ա�����Ϊʵ����������Ԫ������ֵ,
* ������ֵ������
*/
#define MAX_LOAD_FACTOR              (75)

/*
* ��С�������Ӱٷֱȣ�����Ԫ���������ڴ˰ٷֱ�,������
*/
#define MIN_LOAD_FACTOR              (15)

/*
* �������valueʱ�ķ�ʽ������ģʽvalue����ķ�ʽΪ���ã�
* ɾ��ʱ�����ͷţ�����ģʽ�ڲ������ڴ濽��valueֵ��ɾ��
* ���ͷű���value
*/
typedef enum { REF_MODE = 0, COPY_MODE } table_mode_t;

/*����ģʽ��ģʽ1����ͬkey�滻����1��ģʽ2�����滻����-1*/
typedef enum { MODE_1 = 0, MODE_2} set_mode_t;

typedef struct hash_table_element hash_table_element_t;

typedef struct hash_table_element
{
	/* key���� */
	size_t key_len;

	/* value���� */
	size_t value_len;

	/* key���� */
	void *key;

	/* value���� */
	void *value;

	/* ��һ����ͬhashֵ���ݣ������� */
	hash_table_element_t *next;

} hash_table_element_t;

typedef struct hash_table
{
	/* �ȽϺ����� Ĭ��Ϊmemcmp */
	int (*key_compare)(const void *key1, size_t key1_len, const void *key2, size_t key2_len);
		
	/*
	* ��ʼ��ϣ�����������׷��resizeʱ���ؾ��⣬��������ʱ����
	* �ν�first����Ԫ������second����ȫ��ת�����second����
	* ��first��
	*/
	hash_table_element_t  **first_data_store;

	/* first������ */
	hash_size_t table_capacity_1;

	/*
	* ��ʼΪNULL������������ʱ��������СΪfirst���������˺�
	* �ڲ����ѯɾ��ʱ�������first����Ԫ������second����
	*/
	hash_table_element_t  **second_data_store;

	/* second������ */
	hash_size_t table_capacity_2;

	/* ��ǰ����key���� */
	hash_size_t key_count;

	/*
	* ��¼ָ��first���а�˳���һ��key��Ϊ�յ���ţ����ݺ�
	* first��Ԫ������second��ʱʹ��
	*/
	hash_size_t rehashidx;

	/* �������valueʱ�ķ�ʽ��REF_MODE-Ϊ����ģʽ��COPY_MODE-Ϊ����ģʽ */
	table_mode_t mode;

	/* hash���ӣ�Ĭ��Ϊtime(0) */
	hash_size_t seed;

} hash_table_t;


/*������n-��������mode-ģʽ��������ʵ�ʻ���������2�Ĵ���*/
hash_table_t* hash_table_new_n(hash_size_t n, table_mode_t mode);

int hash_table_set(hash_table_t* table, const void* key, size_t key_len, void* value, size_t value_len, int mode);

/*��ѯ���ɹ�����valueָ�룬ʧ�ܷ���NULL*/
void* hash_table_lookup(hash_table_t* table, const void* key, size_t key_len);

/*ɾ����ʧ�ܷ���-1���ɹ�����0*/
int hash_table_remove(hash_table_t* table, const void* key, size_t key_len);

/*ɾ�����ͷ���Դ*/
void hash_table_destory(hash_table_t* table);

/*
* ��ȡhash��������Ԫ�أ�table->key_count��Ԫ�أ�������Ԫ�������б��ڲ����ڴ����룬�����ͷţ���
* ����ģʽ�������ݣ�����ģʽ����ָ�����飬ָ��ָ��table�������ݣ��ձ���NULL
*/
hash_table_element_t* hash_table_elements(hash_table_t* table, table_mode_t mode);

/*����hash��������ӳ�����Ԫ��*/
void hash_table_rehash(hash_table_t* table, hash_size_t seed);

#define htnew(mode)    hash_table_new_n(DEFAULT_CAPACITY, mode)

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
void ht_set_compare_func(hash_table_t* table, int (*kc_func)(const void*, size_t, const void*, size_t));


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
