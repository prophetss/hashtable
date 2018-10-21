#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_


#if defined (__cplusplus)
extern "C" {
#endif


#include "xxhash.h"


	/*64λʹ��xx64��32λʹ��xx32*/
#if defined(__x86_64__) || defined(_WIN64)
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define hash_size_t         XXH64_hash_t
#else
#define hash_size_t         XXH32_hash_t
#define XXHASH(a, b, c)     XXH32(a, b, c)
#endif

	/*Ĭ�ϳ�ʼ������*/
#define DEFAULT_CAPACITY        (32)

	/*
	* �������ӣ��������ӳ��Ա�����Ϊʵ����������Ԫ������ֵ
	* (Ӧ��֤����ڵ���1��Ϊ����)��������ֵ�ͻᷢ��������
	*/
#define LOAD_FACTOR              (0.75)

	/*
	* �������valueʱ�ķ�ʽ������ģʽvalue����ķ�ʽΪ���ã�
	* ɾ��ʱ�����ͷţ�����ģʽ�ڲ������ڴ濽��valueֵ��ɾ��
	* ���ͷű���value
	*/
	typedef enum { REF_MODE = 0, COPY_MODE } table_mode_t;

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

		/* ������ */
		hash_size_t table_capacity;

		/* ��ǰ����key���� */
		hash_size_t key_count;

		/*
		* �������key��������ֵ����table_capacity*LOAD_FACTOR��
		* ��key_count���ڵ��ڴ�ֵʱ������
		*/
		hash_size_t max_key_count;

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



	/*������n-��������lf-�������ӣ�mode-ģʽ��������ʵ�ʻ���������2�Ĵ���*/
	hash_table_t* hash_table_new_n(hash_size_t n, float lf, table_mode_t mode);

	/*ͬ�ϱ������ֶ�����seed-��ϣ����*/
	hash_table_t* hash_table_new_ns(hash_size_t n, float lf, table_mode_t mode, hash_size_t seed);

	#define hash_table_new(mode)    hash_table_new_n(DEFAULT_CAPACITY, LOAD_FACTOR, mode)

	#define hash_table_new_s(mode, seed)    hash_table_new_ns(DEFAULT_CAPACITY, LOAD_FACTOR, mode, seed)

	/*ɾ�����ͷ���Դ*/
	void hash_table_delete(hash_table_t *table);

	/*��ӣ�ʧ�ܷ���-1���ɹ�����0�������滻����1*/
	int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len);

	/*ɾ����ʧ�ܷ���-1���ɹ�����0*/
	int hash_table_remove(hash_table_t *table, void *key, size_t key_len);

	/*��ѯ���ɹ�����valueָ�룬ʧ�ܷ���NULL*/
	void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len);

	/*
	* ��ȡhash��������Ԫ�أ�table->key_count��Ԫ�أ�������Ԫ�������б�����ģʽ�������ݣ�
	* ����ģʽ����ָ�����飬ָ��ָ��table�������ݣ��ձ���NULL
	*/
	hash_table_element_t* hash_table_elements(hash_table_t *table, table_mode_t mode);


#if defined (__cplusplus)
}
#endif


#endif /*!_HASHTABLE_H_*/
