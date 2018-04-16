#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


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

/*由hash值获取对应元素链首地址*/
/*
static hash_table_element_t* get_elements(hash_table_t *table, void* key, size_t key_len)
{
	hash_size_t hash_key = XXHASH(key, key_len, key_len);
	hash_table_element_t *temp = NULL;
	if (NULL != table->second_data_store) {
		if (NULL == (temp = table->second_data_store[hash_key >> table->table_capacity_rdigits])) {
			temp = table->first_data_store[hash_key >> (table->table_capacity_rdigits + 1)];
		}
	}
	else {
		temp = table->first_data_store[hash_key >> table->table_capacity_rdigits];
	}
	return temp;
}
*/

hash_table_t* hash_table_new_n(size_t n)
{
	hash_table_t *new_hashtable = (hash_table_t*)malloc(sizeof(hash_table_t));
	size_t digits = get_digits(n);
	new_hashtable->table_capacity_rdigits = TABLE_BITS - digits;
	new_hashtable->table_capacity = (size_t)(1 << digits);
	new_hashtable->first_data_store = calloc(new_hashtable->table_capacity, sizeof(hash_table_element_t*));
	new_hashtable->table_capacity = new_hashtable->table_capacity * LOAD_FACTOR;
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
		data_store_delete(table->first_data_store, table->table_capacity / LOAD_FACTOR);
		data_store_delete(table->second_data_store, table->table_capacity / LOAD_FACTOR);
	}
	else {
		data_store_delete(table->first_data_store, table->table_capacity / LOAD_FACTOR);
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
	hash_table_element_t *temp = NULL;
	if (NULL != table->second_data_store) {
		if (NULL == (temp = table->second_data_store[hash_key >> table->table_capacity_rdigits])) {
			temp = table->first_data_store[hash_key >> (table->table_capacity_rdigits + 1)];
		}
	}
	else {
		temp = table->first_data_store[hash_key >> table->table_capacity_rdigits];
	}

	while (temp) {
		if (!key_compare(temp->key, temp->key_len, key, key_len)) {
			return temp->value;
		}
		temp = temp->next;
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
	if (NULL != table->second_data_store) {
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