#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


#define capacity_expand(n)	((n & (n-1)) == 0 ? n : (n+(~n)+1))

#define DEFAULT_CAPACITY    8

#define LOAD_FACTOR		 1


static unsigned char get_digits(size_t n)
{
	unsigned char count = 0;
	do {
		count++;
	} while ((n = n >> 1));
	return count;
}

hash_table_t* hash_table_new_n(size_t n)
{
	hash_table_t *new_hashtable = (hash_table_t*)malloc(sizeof(hash_table_t));
	size_t digits = get_digits(n);
	new_hashtable->table_capacity_rdigits = TABLE_BITS - digits;
	new_hashtable->table_capacity = (size_t)((1 << digits) * LOAD_FACTOR);
	new_hashtable->first_data_store = calloc(new_hashtable->table_capacity, sizeof(hash_table_element_t));
	new_hashtable->second_data_store = NULL;
	new_hashtable->rehashidx = 0;
	new_hashtable->key_count = 0;
	return new_hashtable;
}

hash_table_t* hash_table_new()
{
	return hash_table_new_n(DEFAULT_CAPACITY);
}

static void element_delete(hash_table_element_t *element)
{
	free(element->value);
	free(element->key);
}

static void data_store_delete(hash_table_element_t **data_store, const unsigned char rdigits)
{
	for (int i = 0; i < (1 << rdigits); i++) {
		while (NULL != data_store[i]) {
			hash_table_element_t *temp = data_store[i];
			data_store[i] = data_store[i]->next;
			element_delete(temp);
		}
	}
}

void hash_table_delete(hash_table_t *table)
{
	if (NULL != table->second_data_store) {
		data_store_delete(table->first_data_store, table->table_capacity_rdigits / 2);
		data_store_delete(table->second_data_store, table->table_capacity_rdigits);
	}
	else {
		data_store_delete(table->first_data_store, table->table_capacity_rdigits);
	}
	free(table);
}

/*
int hash_table_resize(hash_table_t *table, size_t len)
{

}

*/

/*

static void move_element(hash_table_t *table)
{
	while (table->rehashidx < table->table_capacity / 2) {
		if (NULL != table->first_data_store[table->rehashidx]) {
			hash_size_t hash_key = XXHASH(table->first_data_store[table->rehashidx]->key, table->first_data_store[table->rehashidx]->key_len,
			                              table->first_data_store[table->rehashidx]->key_len);
			move_element(table->second_data_store, hash_key << table->table_capacity_rdigits, table->first_data_store[table->rehashidx]);
			if (NULL != table->first_data_store[table->rehashidx]->next) {
				table->first_data_store[table->rehashidx] = table->first_data_store[table->rehashidx]->next;
				break;
			}
		}
		table->rehashidx++;
	}
	if (table->rehashidx == table->table_capacity / 2)
	{

	}
}
*/

void* hash_table_lookup(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash_key = XXHASH(key, key_len, key_len);
	hash_size_t first_hash, second_hash;
	if (NULL == table->second_data_store) {
		first_hash = hash_key >> table->table_capacity_rdigits;
	}
	else {
		second_hash = hash_key >> table->table_capacity_rdigits;
		first_hash = second_hash >> 1;
	}

	hash_table_element_t *temp = table->first_data_store[first_hash];
	if (NULL == temp) {
		if (NULL == table->second_data_store) {
			return NULL;
		}
		temp = table->second_data_store[second_hash];
		if (NULL == temp) {
			return NULL;
		}
	}

	while (temp) {
		/*过滤不相等的key_len*/
		if (temp->key_len != key_len) {
			temp = temp->next;
			continue;
		}
		if (!memcmp(temp->key, key, key_len)) {
			return temp->value;
		}
		else {
			temp = temp->next;
		}
	}
	return NULL;
}

int hash_table_add(hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len)
{
	hash_table_element_t **data_store;
	if (NULL != table->second_data_store) {
		data_store = table->second_data_store;
		//move_element(table);
	}
	else {
		data_store = table->first_data_store;
	}

	if (table->key_count >= table->table_capacity) {
		//hash_table_resize(table, table->key_num * 2);
	}

	hash_size_t hash = XXHASH(key, key_len, key_len);
	hash_size_t hash_key = hash >> table->table_capacity_rdigits;

	hash_table_element_t *element = (hash_table_element_t*)malloc(sizeof(hash_table_element_t));
	if (!element) {
		exit(0);
	}

	element->key = malloc(key_len);
	element->value = malloc(value_len);
	if (NULL == element->key || NULL == element->value) {
		exit(0);
	}

	memcpy(element->key, key, key_len);
	memcpy(element->value, value, value_len);

	element->key_len = key_len;
	element->value_len = value_len;
	element->next = NULL;

	if (NULL == data_store[hash_key]) {
		data_store[hash_key] = element;
		table->key_count++;
	}
	else {
		hash_table_element_t *temp = data_store[hash_key];
		/*if (!memcmp(temp->key, key, key_len)) {
			hash_table_element_t *to_delete = temp;
			temp = element;
			element->next = to_delete->next;
			element_delete(to_delete);
			return 0;
		}*/
		while (temp->next) {
			if (temp->next->key_len != key_len) {
				temp = temp->next;
				continue;
			}
			if (temp->next) {
				/*key相同替换*/
				if (!memcmp(temp->next->key, key, key_len)) {
					hash_table_element_t *to_delete = temp->next;
					temp->next = element;
					element->next = to_delete->next;
					element_delete(to_delete);
					return 0;
				}
				else {
					temp = temp->next;
				}
			}
		}
		temp->next = element;
		table->key_count++;
	}
	return 0;
}
/*
int hash_table_remove(hash_table_t *table, void *key, size_t key_len)
{
	hash_size_t hash_key = XXHASH(key, key_len, key_len);
	hash_size_t first_hash, second_hash;
	hash_table_element_t *temp, *prev, **data_store;;
	if (NULL == table->second_data_store) {
		first_hash = hash_key >> table->table_capacity_rdigits;
	}
	else {
		second_hash = hash_key >> table->table_capacity_rdigits;
		first_hash = second_hash >> 1;
	}

	if (NULL != table->first_data_store[first_hash]) {
		temp = table->first_data_store[first_hash];
		data_store = table->first_data_store;
		hash_key = second_hash;
	}
	else if (NULL != table->second_data_store && NULL != table->second_data_store[second_hash]) {
		temp = table->second_data_store[second_hash];
		data_store = table->second_data_store;
		hash_key = first_hash;
	}
	else {
		//没找到
		return -1;
	}

	prev = temp;

	while (temp) {
		while (temp && temp->key_len != key_len) {
			prev = temp;
			temp = temp->next;
		}
		if (temp) {
			if (!memcmp(temp->key, key, key_len)) {
				if (prev == data_store[hash_key]) {
					data_store[hash_key] = temp->next;
				}
				else {
					prev->next = temp->next;
				}
				element_delete(temp);
				table->key_count--;
				return 0;
			}
			temp = temp->next;
		}
	}
	return -1;
}

*/