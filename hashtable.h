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
	/* key���� */
	U32 kl;

	/* value���� */
	U32 vl;

	/* key���� */
	void *key;

	/* value���� */
	void *val;

	/* ��һ����ͬhashֵ���ݣ������� */
	ht_elem_t *next;

} ht_elem_t;

typedef struct tb
{
	ht_elem_t** ds;
	U32 tc;
}tb_t;

/*
* �������valueʱ�ķ�ʽ������ģʽ����ķ�ʽΪָ�뿽��,
* ����ģʽ�ڲ������ڴ濽����ֵ��ɾ�����Լ��ͷ�
*/
typedef enum { REF_MODE = 0, COPY_MODE } tmode_t;

typedef struct ht
{
	/* �ȽϺ����� Ĭ��Ϊmemcmp */
	int (*kc)(const void *k1, U32 kl1, const void *k2, U32 kl2);

	/*
	* ��ʼ��ϣ�����������׷��resizeʱ���ؾ��⣬��������ʱ
	* ����ν�0����Ԫ������1����ȫ��ת�����1���滻0��,
	* 1��ʼΪNULL������������ʱ��������СΪ0���������˺�
	* �ڲ����ѯɾ��ʱ�������0����Ԫ������1����
	*/
	tb_t	tbs[2];

	/* ��ǰkey���� */
	U32 used;

	/* �����������Ԫ������ */
	U32 ht_max;

	/* ������������Ԫ������ */
	U32 ht_min;

	/*
	* ��¼ָ��first���а�˳���һ��key��Ϊ�յ���ţ����ݺ�
	* first��Ԫ������second��ʱʹ��
	*/
	U32 rehashidx;

	/* hash���ӣ�Ĭ��Ϊtime(0) */
	U32 seed;

	/* �������valueʱ�ķ�ʽ(����/����) */
	tmode_t mode;

} ht_t;

/**
 * ������ϣ��
 * @param n-��������������ʵ�ʻ���������2�Ĵ��ݣ�
 * @param tmode-ģʽ
 * @return ʧ�ܷ���NULL��ֻ��n����Żᷢ����
 */
ht_t* hash_table_new_n(U32 n, tmode_t tmode);

/*����ģʽ��ģʽ1����ͬkey�滻����1��ģʽ2�����滻����-1*/
typedef enum { MODE_1 = 0, MODE_2 } smode_t;

int hash_table_set(ht_t* table, const void* key, U32 kl, void* val, U32 vl, smode_t smode);

/**
 * ��ѯ
 * @return �ɹ�����valueָ�룬ʧ�ܷ���NULL
 */
void* hash_table_lookup(ht_t* table, const void* key, U32 kl);

/**
 * ɾ��
 * @return ʧ�ܷ���-1���ɹ�����0
 */
int hash_table_remove(ht_t* table, const void* key, U32 kl);

/*ɾ�����ͷ���Դ*/
void hash_table_destory(ht_t* table);

/**
 * ��ȡhash��������Ԫ�أ�keyΪ���ݿ���
 * @param tmode-REF_MODE��value����ָ��, COPY_MODE��value��������
 * @return Ԫ������
 */
ht_elem_t* hash_table_elements(ht_t* table, tmode_t tmode);

/*�ͷ�hash_table_elements����ָ���ָ���ڴ棬tmode�����ǰ��һ��*/
void hash_table_remove_elements(ht_elem_t* elems, tmode_t tmode);

/*����hash��������ӳ�����Ԫ��*/
void hash_table_rehash(ht_t* table, U32 seed);

/*���ü�ֵ�ȽϺ���(��Ӧ������k1, kl1, k2, kl2������ֵ������ͬ0������)����������Ĭ��ʹ��memcmp*/
void hash_table_set_compare_func(ht_t* table, int (*kc_func)(const void*, U32, const void*, U32));

#define htnew()    hash_table_new_n(0, REF_MODE)	//����0����չ��Ĭ����Сֵ8

/*��ӣ��ɹ�����0��key��ͬ�滻����1*/
#define htset(table, key, kl, val, vl)	hash_table_set(table, key, kl, val, vl, MODE_1)

/*��ӣ��ɹ�����0��key��ͬʧ�ܲ��滻����-1*/
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
