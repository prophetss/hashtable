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

/*key比较*/
#define key_compare(key, key_len, key0, key0_len) ((key_len == key0_len) && !memcmp(key, key0, key_len))

static unsigned char get_digits(hash_size_t n)
{
	unsigned char count = 0;
	do {
		count++;
	} while ((n = n >> 1));
	return count;
}

static hash_table_element_t* element_new(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len)
{
	hash_table_element_t *element = (hash_table_element_t*)malloc(sizeof(hash_table_element_t));
	if (!element) sys_exit_throw();

	element->key = malloc(key_len);
	if (!element->key) sys_exit_throw();

	memcpy(element->key, key, key_len);

	if (table->mode == COPY_MODE) {
		element->value = malloc(value_len);
		if (!element->value) sys_exit_throw();
		memcpy(element->value, value, value_len);
	}
	else {
		element->value = value;
	}
	element->key_len = key_len;
	element->value_len = value_len;
	element->next = NULL;

	return element;
}

static void element_delete(hash_table_t *table, hash_table_element_t *element)
{
	if (table->mode == COPY_MODE) {
		free(element->value);
	}
	free(element->key);
	free(element);
}

static void data_store_delete(hash_table_t *table, hash_table_element_t **data_store, const hash_size_t capacity)
{
	for (hash_size_t i = 0; i < capacity; i++) {
		while (NULL != data_store[i]) {
			hash_table_element_t *temp = data_store[i];
			data_store[i] = data_store[i]->next;
			element_delete(table, temp);
		}
	}
}

hash_table_t* hash_table_new_ns(hash_size_t n, float lf, table_mode_t mode, hash_size_t seed)
{
	hash_table_t *new_hashtable = (hash_table_t*)malloc(sizeof(hash_table_t));
	if (!new_hashtable) sys_exit_throw();
	unsigned char digits = get_digits(n);
	new_hashtable->table_capacity = (hash_size_t)(1 << digits);
	new_hashtable->first_data_store = (hash_table_element_t**)calloc(new_hashtable->table_capacity, sizeof(hash_table_element_t*));
	if (!new_hashtable->first_data_store) sys_exit_throw();
	new_hashtable->max_key_count = (hash_size_t)(new_hashtable->table_capacity * lf);
	new_hashtable->second_data_store = NULL;
	new_hashtable->rehashidx = 0;
	new_hashtable->key_count = 0;
	new_hashtable->mode = mode;
	new_hashtable->seed = seed;
	return new_hashtable;
}

hash_table_t* hash_table_new_n(hash_size_t n, float lf, table_mode_t mode)
{
	return hash_table_new_ns(n, lf, mode, (hash_size_t)time(0));
}

void hash_table_delete(hash_table_t *table)
{
	if (NULL != table->second_data_store) {
		data_store_delete(table, &(table->first_data_store[table->rehashidx]), table->table_capacity / 2 - table->rehashidx);
		data_store_delete(table, table->second_data_store, table->table_capacity);
	}
	else {
		data_store_delete(table, table->first_data_store, table->table_capacity);
	}
	free(table);
}

/*将first表内相同hash的一个元素转移至second表*/
static int move_element(hash_table_t *table)
{
	while (table->rehashidx < table->table_capacity / 2) {
		hash_table_element_t *ftemp = table->first_data_store[table->rehashidx];
		if (!ftemp) {
			table->rehashidx++;
			continue;
		}
		/*重新hash，插入链首*/
		hash_size_t rehash_key = (table->table_capacity - 1) & XXHASH(ftemp->key, ftemp->key_len, table->seed);
		hash_table_element_t *stemp = table->second_data_store[rehash_key];
		table->second_data_store[rehash_key] = table->first_data_store[table->rehashidx];
		table->first_data_store[table->rehashidx] = ftemp->next;
		table->second_data_store[rehash_key]->next = stemp;
		return 0;
	}
	/*全部转移完毕*/
	free(table->first_data_store);
	table->first_data_store = table->second_data_store;
	table->second_data_store = NULL;
	table->rehashidx = 0;
	return 1;
}

/*只负责重新建表，不负责元素转移。只扩大（翻倍），不缩小*/
static void hash_table_expand(hash_table_t *table)
{
	table->second_data_store = (hash_table_element_t**)calloc((table->table_capacity * 2), sizeof(hash_table_element_t*));
	if (!table->second_data_store)  sys_exit_throw();
	table->max_key_count *= 2;
	table->table_capacity *= 2;
}

void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash = XXHASH(key, key_len, table->seed);

	hash_table_element_t *temp[2] = { NULL };
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		temp[1] = table->second_data_store[hash & (table->table_capacity - 1)];
		temp[0] = table->first_data_store[hash & (table->table_capacity / 2 - 1)];
	}
	else {
		temp[0] = table->first_data_store[hash & (table->table_capacity - 1)];
	}

	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				return temp[i]->value;
			}
			temp[i] = temp[i]->next;
		}
	}

	return NULL;
}

int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len)
{
	if (table->key_count >= table->max_key_count && NULL == table->second_data_store) {
		hash_table_expand(table);
	}

	hash_size_t hash = XXHASH(key, key_len, table->seed);
	hash_size_t shash_key = hash & (table->table_capacity - 1);
	hash_table_element_t *temp[2] = { NULL };
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		temp[1] = table->second_data_store[shash_key];
		temp[0] = table->first_data_store[hash & (table->table_capacity / 2 - 1)];
	}
	else {
		temp[0] = table->first_data_store[shash_key];
	}

	/*检索key发现相同替换返回1，否则加入链表末尾count+1返回0*/
	hash_table_element_t *prev = NULL;
	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				//替换value
				if (table->mode == COPY_MODE) {
					free(temp[i]->value);
					temp[i]->value = malloc(value_len);
					if (!temp[i]->value) sys_exit_throw();
					memcpy(temp[i]->value, value, value_len);
				}
				else {
					temp[i]->value = value;
				}
				return 1;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}

	//未查到新建插入
	hash_table_element_t *element = element_new(table, key, key_len, value, value_len);
	if (prev) {
		prev->next = element;
	}
	else {
		if (NULL != table->second_data_store) {
			table->second_data_store[shash_key] = element;
		}
		else {
			table->first_data_store[shash_key] = element;
		}
	}
	table->key_count++;

	return 0;
}

int hash_table_remove(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash = XXHASH(key, key_len, table->seed);
	hash_table_element_t *temp[2] = { NULL }, **ptemp[2] = { NULL };
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		if ((temp[0] = table->first_data_store[hash & (table->table_capacity / 2 - 1)])) {
			ptemp[0] = &(table->first_data_store[hash & (table->table_capacity / 2 - 1)]);
		}
		if ((temp[1] = table->second_data_store[hash & (table->table_capacity - 1)])) {
			ptemp[1] = &(table->second_data_store[hash & (table->table_capacity - 1)]);
		}
	}
	else {
		if ((temp[0] = table->first_data_store[hash & (table->table_capacity - 1)])) {
			ptemp[0] = &(table->first_data_store[hash & (table->table_capacity - 1)]);
		}
	}

	for (int i = 0; i < 2; ++i) {
		hash_table_element_t *prev = NULL;
		while (temp[i]) {
			if (key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				if (!prev) {
					*ptemp[i] = temp[i]->next;
				}
				else {
					prev->next = temp[i]->next;
				}
				element_delete(table, temp[i]);
				table->key_count--;
				return 0;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}

	return -1;
}

static void get_elements(hash_table_element_t **p, hash_table_element_t **data_store, hash_size_t n, table_mode_t mode)
{
	for (hash_size_t i = 0; i < n; ++i) {
		hash_table_element_t *tmp = data_store[i];
		while (tmp) {
			memcpy(*p, tmp, sizeof(hash_table_element_t));
			if (mode == COPY_MODE) {
				(*p)->key = malloc(tmp->key_len);
				if (!(*p)->key) sys_exit_throw();
				memcpy((*p)->key, tmp->key, tmp->key_len);
				(*p)->value = malloc(tmp->value_len);
				if (!(*p)->value) sys_exit_throw();
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
	if (0 == table->key_count) {
		return NULL;
	}

	/*创建返回列表*/
	hash_table_element_t *head = (hash_table_element_t*)calloc(table->key_count, sizeof(hash_table_element_t));
	if (!head) sys_exit_throw();

	hash_table_element_t *ptmp = head;

	hash_size_t sn = table->second_data_store ? table->table_capacity : 0;

	/*second表不为空则first表大小为table_capacity/2*/
	hash_size_t fn = sn == 0 ? table->table_capacity : sn / 2;

	/*筛出first表元素*/
	get_elements(&ptmp, table->first_data_store, fn, mode);

	/*筛出second表元素*/
	get_elements(&ptmp, table->second_data_store, sn, mode);

	return head;
}
