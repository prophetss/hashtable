#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "hashtable.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4996) //strerror _CRT_SECURE_NO_WARNINGS
#endif //  defined(_MSC_VER)

#define sys_exit_throw() do {													\
    fprintf(stderr, "Errno : %d, msg : %s\n", errno, strerror(errno));          \
    exit(errno);                                                                \
} while(0)

/*64位使用xx64，32位使用xx32*/
#if defined(__x86_64__) || defined(_WIN64)
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define XXH_hash_t	XXH64_hash_t
#else
#define XXHASH(a, b, c)     XXH32(a, b, c)
#define XXH_hash_t	XXH32_hash_t
#endif

#define MAX_COUNT	0x3fffffffU

/*
* 最大负载因子百分比，乘以表容量为实际最大可容纳元素数量值,
* 超过此值表扩容
*/
#define MAX_LOAD_FACTOR              (75)

/*
* 最小负载因子百分比，表内元素数量少于此百分比,表缩减
*/
#define MIN_LOAD_FACTOR              (15)

#define INDEX(hash, cap) (((U32)hash) & (((U32)cap)-1))

/*默认key比较*/
static inline int key_cmp(const void *key1, U32 key1_len, const void *key2, U32 key2_len)
{
	return ((key1_len == key2_len) && !memcmp(key1, key2, key1_len));
}

//向上取2的幂(http://graphics.stanford.edu/~seander/bithacks.html#OperationCounting)
static inline U32 ROUND_UP(U32 v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return ++v;
}

static void* ht_alloc(size_t len)
{
	void* data = malloc(len);
	if (!data) sys_exit_throw();
	return data;
}

static void* ht_calloc(size_t len, size_t size)
{
	void* data = calloc(len, size);
	if (!data) sys_exit_throw();
	return data;
}

static hash_table_element_t* element_new(hash_table_t *table, const void *key, U32 key_len, void *value, U32 value_len)
{
	hash_table_element_t* element = (hash_table_element_t*)ht_alloc(sizeof(hash_table_element_t));
	element->key = ht_alloc(key_len);
	memcpy(element->key, key, key_len);
	if (table->mode == COPY_MODE) {
		element->value = ht_alloc(value_len);
		memcpy(element->value, value, value_len);
	}
	else element->value = value;
	element->value_len = value_len;
	element->key_len = key_len;
	element->next = NULL;
	return element;
}

static void element_delete(hash_table_t *table, hash_table_element_t *element)
{
	if (table->mode == COPY_MODE) free(element->value);
	free(element->key);
	free(element);
}

static void data_store_delete(hash_table_t *table, hash_table_element_t **data_store, U32 capacity)
{
	if (!data_store) return;
	for (U32 i = 0; i < capacity; i++) {
		while (NULL != data_store[i]) {
			hash_table_element_t *temp = data_store[i];
			data_store[i] = data_store[i]->next;
			element_delete(table, temp);
		}
	}
}

hash_table_t* hash_table_new_n(U32 n, table_mode_t mode)
{
	hash_table_t *new_hashtable = (hash_table_t*)ht_alloc(sizeof(hash_table_t));
	new_hashtable->table_capacity_1 = ROUND_UP(n);
	if (new_hashtable->table_capacity_1 > MAX_COUNT) {
		new_hashtable->table_capacity_1 >>= 2;
		fprintf(stderr, "Capacity Overflow! New Capacity(%u)\n", new_hashtable->table_capacity_1);
	}
	new_hashtable->first_data_store = (hash_table_element_t * *)ht_calloc(new_hashtable->table_capacity_1, sizeof(hash_table_element_t*));
	new_hashtable->second_data_store = NULL;
	new_hashtable->table_capacity_2 = 0;
	new_hashtable->key_compare = key_cmp;
	new_hashtable->rehashidx = 0;
	new_hashtable->key_count = 0;
	new_hashtable->mode = mode;
	new_hashtable->seed = (U32)time(0);
	return new_hashtable;
}

void hash_table_destory(hash_table_t *table)
{
	data_store_delete(table, &table->first_data_store[table->rehashidx], table->table_capacity_1 - table->rehashidx);
	data_store_delete(table, table->second_data_store, table->table_capacity_2);
	free(table);
}

/*将first表内的count个元素转移至second表*/
static int move_elements(hash_table_t *table, U32 count)
{
	hash_table_element_t *ftemp;
	while (table->rehashidx < table->table_capacity_1) {
		if (!(ftemp = table->first_data_store[table->rehashidx])) {
			table->rehashidx++;
			continue; 
		}
		/*重新hash，插入链首*/
		U32 rehash_key = INDEX(XXHASH(ftemp->key, ftemp->key_len, table->seed), table->table_capacity_2);
		hash_table_element_t *stemp = table->second_data_store[rehash_key];
		table->second_data_store[rehash_key] = table->first_data_store[table->rehashidx];
		table->first_data_store[table->rehashidx] = ftemp->next;
		table->second_data_store[rehash_key]->next = stemp;
		if (--count == 0) return 0;
	}
	/*全部转移完毕*/
 	free(table->first_data_store);
	table->first_data_store = table->second_data_store;
	table->table_capacity_1 = table->table_capacity_2;
	table->table_capacity_2 = 0;
	table->second_data_store = NULL;
	table->rehashidx = 0;
	return 1;
}

/*扩大缩小只重新建表，不进行元素转移*/
static void hash_table_expand(hash_table_t *table, U32 new_size)
{
	if (new_size > MAX_COUNT || new_size < DEFAULT_CAPACITY) return;
	table->second_data_store = (hash_table_element_t**)ht_calloc(new_size, sizeof(void*));
	table->table_capacity_2 = new_size;
}

static void get_tables(hash_table_t* table, const void* key, U32 key_len, hash_table_element_t** ht, U32* h)
{
	XXH_hash_t hash = XXHASH(key, key_len, table->seed);
	h[1] = (table->table_capacity_2 == 0 ? 0 : INDEX(hash, table->table_capacity_2));
	ht[1] = (NULL != table->second_data_store && 0 == move_elements(table,2) ? table->second_data_store[h[1]] : NULL);
	h[0] = INDEX(hash, table->table_capacity_1);
	ht[0] = table->first_data_store[h[0]];
}

void* hash_table_lookup(hash_table_t *table, const void *key, U32 key_len)
{
	hash_table_element_t* temp[2] = { 0 };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, key_len, temp, hash_key);
	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (table->key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				return temp[i]->value;
			}
			temp[i] = temp[i]->next;
		}
	}
	return NULL;
}

static void _expand_if_needed(hash_table_t *table)
{
	if (!table->second_data_store && table->key_count/MAX_LOAD_FACTOR * 100 > table->table_capacity_1)
		hash_table_expand(table, table->table_capacity_1*2);
}

static void replace_element(void** val, void* new_val, size_t len, table_mode_t mode)
{
	if (mode == COPY_MODE) {
		free(*val);
		*val = ht_alloc(len);
		memcpy(*val, new_val, len);
	}
	else *val = new_val;
}

int hash_table_set(hash_table_t *table, const void *key, U32 key_len, void *value, U32 value_len, set_mode_t mode)
{
	_expand_if_needed(table);
	hash_table_element_t *prev = NULL, *temp[2] = { 0 };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, key_len, temp, hash_key);
	/*检索key发现相同替换返回1，否则加入链表末尾count+1返回0*/
	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (table->key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				if (mode == MODE_2) return -1;
				replace_element(&(temp[i]->value), value, value_len, table->mode);
				return 1;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}

	//未查到新建插入
	table->key_count++;
	hash_table_element_t *element = element_new(table, key, key_len, value, value_len);
	if (prev) {
		prev->next = element;
		return 0;
	}

	if (table->second_data_store) table->second_data_store[hash_key[1]] = element;
	else table->first_data_store[hash_key[0]] = element;

	return 0;
}

static void _resize_if_needed(hash_table_t *table)
{
	if (!table->second_data_store && table->key_count > DEFAULT_CAPACITY && (table->key_count/MIN_LOAD_FACTOR * 100 < table->table_capacity_1))
		hash_table_expand(table, table->table_capacity_1/2);
}

int hash_table_remove(hash_table_t *table, const void *key, U32 key_len)
{
	hash_table_element_t *temp[2] = { NULL }, **ptemp[2] = { NULL };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, key_len, temp, hash_key);
	if (temp[1]) ptemp[1] = &(table->second_data_store[hash_key[1]]);
	if (temp[0]) ptemp[0] = &(table->first_data_store[hash_key[0]]);
	for (int i = 0; i < 2; ++i) {
		hash_table_element_t *prev = NULL;
		while (temp[i]) {
			if (table->key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				if (!prev) *ptemp[i] = temp[i]->next;
				else prev->next = temp[i]->next;
				element_delete(table, temp[i]);
				table->key_count--;
				_resize_if_needed(table);
				return 0;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}
	
	return -1;
}

static void get_elements(hash_table_element_t **p, hash_table_element_t **data_store, U32 n, table_mode_t mode)
{
	for (U32 i = 0; i < n; ++i) {
		hash_table_element_t *tmp = data_store[i];
		while (tmp) {
			memcpy(*p, tmp, sizeof(hash_table_element_t));
			(*p)->key = ht_alloc(tmp->key_len);
			memcpy((*p)->key, tmp->key, tmp->key_len);
			if (mode == COPY_MODE) {
				(*p)->value = ht_alloc(tmp->value_len);
				memcpy((*p)->value, tmp->value, tmp->value_len);
			}
			tmp = tmp->next;
			++(*p);
		}
	}
}

hash_table_element_t* hash_table_elements(hash_table_t *table, table_mode_t mode)
{
	/*表内无元素*/
	if (0 == table->key_count) return NULL;

	/*创建返回列表*/
	hash_table_element_t *head = (hash_table_element_t*)ht_calloc(table->key_count, sizeof(hash_table_element_t));
	

	hash_table_element_t *ptmp = head;

	/*筛出first表元素*/
	get_elements(&ptmp, table->first_data_store, table->table_capacity_1, mode);

	/*筛出second表元素*/
	get_elements(&ptmp, table->second_data_store, table->table_capacity_2, mode);

	return head;
}

void hash_table_rehash(hash_table_t* table, U32 seed)
{
	if (table->key_count == 0) return;
	table->seed = seed;
	if (!table->second_data_store) {
		hash_table_expand(table, table->table_capacity_1);
		move_elements(table, MAX_COUNT);
	}
	else {
		hash_table_element_t** p = table->first_data_store;
		U32 p_cap = table->table_capacity_1;
		table->first_data_store = table->second_data_store;
		table->table_capacity_1 = table->table_capacity_2;
		hash_table_expand(table, table->table_capacity_2);
		move_elements(table, MAX_COUNT);
		table->second_data_store = table->first_data_store;
		table->first_data_store = p;
		table->table_capacity_1 = p_cap;
		move_elements(table, MAX_COUNT);
	}
}

void ht_set_compare_func(hash_table_t* table, int (*kc_func)(const void*, U32, const void*, U32))
{
	table->key_compare = kc_func;
}


