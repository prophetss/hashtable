#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


#define DEFAULT_CAPACITY       	  32

#define LOAD_FACTOR		     	0.75


static unsigned char get_digits(size_t n)
{
	unsigned char count = 0;
	do {
		count++;
	} while ((n = n >> 1));
	return count;
}

static int key_compare(void *key, size_t key_len, void *key0, size_t key0_len)
{
	if (key_len != key0_len) {
		return -1;
	}
	return memcmp(key, key0, key_len);
}

static hash_table_element_t* element_new(void *key, size_t key_len, void *value, size_t value_len)
{
	hash_table_element_t *element = (hash_table_element_t*)malloc(sizeof(hash_table_element_t));
	if (!element) {
		return NULL;
	}

	element->key = malloc(key_len);
	element->value = malloc(value_len);
	if (NULL == element->key || NULL == element->value) {
		return NULL;
	}

	memcpy(element->key, key, key_len);
	memcpy(element->value, value, value_len);

	element->key_len = key_len;
	element->value_len = value_len;
	element->next = NULL;

	return element;
}

static void element_delete(hash_table_element_t *element)
{
	free(element->value);
	free(element->key);
	free(element);
}

static void data_store_delete(hash_table_element_t **data_store, const size_t capacity)
{
	for (size_t i = 0; i < capacity; i++) {
		while (NULL != data_store[i]) {
			hash_table_element_t *temp = data_store[i];
			data_store[i] = data_store[i]->next;
			element_delete(temp);
		}
	}
}

hash_table_t* hash_table_new_n(size_t n)
{
	hash_table_t *new_hashtable = (hash_table_t*)malloc(sizeof(hash_table_t));
	if (NULL == new_hashtable) {
		return NULL;
	}
	size_t digits = get_digits(n);
	new_hashtable->table_capacity_rdigits = TABLE_BITS - digits;
	new_hashtable->table_capacity = (size_t)(1 << digits);
	new_hashtable->first_data_store = calloc(new_hashtable->table_capacity, sizeof(hash_table_element_t*));
	if (NULL == new_hashtable->first_data_store) {
		free(new_hashtable);
		return NULL;
	}
	new_hashtable->max_key_count = new_hashtable->table_capacity * LOAD_FACTOR;
	new_hashtable->second_data_store = NULL;
	new_hashtable->rehashidx = 0;
	new_hashtable->key_count = 0;
	return new_hashtable;
}

hash_table_t* hash_table_new()
{
	return hash_table_new_n(DEFAULT_CAPACITY);
}

void hash_table_delete(hash_table_t *table)
{
	if (NULL != table->second_data_store) {
		data_store_delete(&(table->first_data_store[table->rehashidx]), table->table_capacity/2 - table->rehashidx);
		data_store_delete(table->second_data_store, table->table_capacity);
	}
	else {
		data_store_delete(table->first_data_store, table->table_capacity);
	}
	free(table);
}

/*将first表内相同hash的一个元素转移至second表*/
static int move_element(hash_table_t *table)
{
	while (table->rehashidx < table->table_capacity / 2) {
		hash_table_element_t *ftemp = table->first_data_store[table->rehashidx];
		if (NULL == ftemp) {
			table->rehashidx++;
			continue;
		}
		/*重新hash*/
		hash_size_t rehash = XXHASH(ftemp->key, ftemp->key_len, ftemp->key_len);
		hash_size_t rehash_key = rehash >> table->table_capacity_rdigits;

		hash_table_element_t *stemp = table->second_data_store[rehash_key];
		if (NULL == stemp) {
			table->second_data_store[rehash_key] = table->first_data_store[table->rehashidx];
			table->first_data_store[table->rehashidx] = ftemp->next;
			(table->second_data_store[rehash_key])->next = NULL;
		}
		else {
			/*已存在元素，追加至链尾*/
			while (stemp->next) {
				stemp = stemp->next;
			}
			stemp->next = table->first_data_store[table->rehashidx];
			table->first_data_store[table->rehashidx] = (table->first_data_store[table->rehashidx])->next;
			(stemp->next)->next = NULL;
		}
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
	table->second_data_store = calloc((table->table_capacity * 2), sizeof(hash_table_element_t*));
	if (NULL == table->second_data_store) {
		return;
	}
	table->table_capacity_rdigits--;
	table->max_key_count <<= 1;
	table->table_capacity <<= 1;
}

void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash = XXHASH(key, key_len, key_len);

	hash_table_element_t *temp[2] = {NULL};
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		temp[1] = table->second_data_store[hash >> table->table_capacity_rdigits];
		temp[0] = table->first_data_store[hash >> (table->table_capacity_rdigits + 1)];
	}
	else {
		temp[0] = table->first_data_store[hash >> table->table_capacity_rdigits];
	}

	/*先查表second，再查first，查找速度更快*/
	for (int i = 1; i >= 0; i--) {
		while (temp[i]) {
			if (!key_compare(temp[i]->key, temp[i]->key_len, key, key_len)) {
				return temp[i]->value;
			}
			temp[i] = temp[i]->next;
		}
	}

	return NULL;
}

int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len)
{
	hash_table_element_t **data_store;
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		data_store = table->second_data_store;
	}
	else {
		data_store = table->first_data_store;
	}

	hash_size_t hash = XXHASH(key, key_len, key_len);
	hash_size_t hash_key = hash >> table->table_capacity_rdigits;

	if (table->key_count >= table->max_key_count && NULL == table->second_data_store) {
		hash_table_expand(table);
	}

	hash_table_element_t *element = element_new(key, key_len, value, value_len);
	if (NULL == element) {
		return -1;
	}

	if (NULL == data_store[hash_key]) {
		data_store[hash_key] = element;
		table->key_count++;
		return 0;
	}

	/*检索key发现相同替换返回1，否则加入链表末尾count+1返回0*/
	hash_table_element_t *prev = NULL, *temp = data_store[hash_key];
	while (temp) {
		if (!key_compare(temp->key, temp->key_len, element->key, element->key_len)) {
			hash_table_element_t *to_delete = temp;
			/*temp为局部指针，element不能赋至temp*/
			if (temp == data_store[hash_key]) {
				data_store[hash_key] = element;
			}
			else {
				prev->next = element;
			}
			element->next = to_delete->next;
			element_delete(to_delete);
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	prev->next = element;
	table->key_count++;

	return 0;
}

int hash_table_remove(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash_key = XXHASH(key, key_len, key_len);
	hash_table_element_t *prev = NULL, *temp = NULL;
	hash_table_element_t **phead = NULL;
	if (NULL != table->second_data_store && 0 == move_element(table)) {
		if (NULL == (temp = table->second_data_store[hash_key >> table->table_capacity_rdigits])) {
			phead = &(table->first_data_store[hash_key >> (table->table_capacity_rdigits + 1)]);

		}
	}
	else {
		phead = &(table->first_data_store[hash_key >> table->table_capacity_rdigits]);
	}
	temp = *phead;

	while (temp) {
		if (!key_compare(temp->key, temp->key_len, key, key_len)) {
			if (*phead == temp) {
				*phead = temp->next;
			}
			else {
				prev->next = temp->next;
			}
			table->key_count--;
			element_delete(temp);
			return 0;
		}
		prev = temp;
		temp = temp->next;
	}
	return -1;
}