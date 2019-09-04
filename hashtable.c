#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "hashtable.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4996) //strerror _CRT_SECURE_NO_WARNINGS
#endif //  defined(_MSC_VER)

/*64位使用xx64，32位使用xx32*/
#if defined(__x86_64__) || defined(_WIN64)
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define XXH_hash_t	XXH64_hash_t
#else
#define XXHASH(a, b, c)     XXH32(a, b, c)
#define XXH_hash_t	XXH32_hash_t
#endif

#define sys_exit_throw() {														\
    fprintf(stderr, "Errno : %d, msg : %s\n", errno, strerror(errno));          \
    exit(errno);                                                                \
}

#define MAX_NUMS	0x80000000U

#define U32_MAX		0xffffffffU

/*
* 最大负载因子百分比，乘以表容量为实际最大可容纳元素数量值,
* 超过此值表扩容
*/
#define MAX_LOAD_FACTOR              (1.0f)

/*默认初始表容量*/
#define DEFAULT_CAPACITY        (8)

#define INDEX(hash, cap) ((U32)((hash) & ((cap)-1)))

/*默认key比较*/
static int key_cmp(const void *k1, U32 kl1, const void *k2, U32 kl2)
{
	return ((kl1 == kl2) && !memcmp(k1, k2, kl1));
}

//向上取2的幂
static U32 round_up(U32 n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return ++n;
}

static void* ht_dup(const void* src, size_t len)
{
	void* data = malloc(len);
	if (!data) sys_exit_throw();
	memcpy(data, src, len);
	return data;
}

static void* ht_calloc(size_t len)
{
	void* data = calloc(len, 1);
	if (!data) sys_exit_throw();
	return data;
}

static ht_elem_t* element_new(ht_t *table, const void *key, U32 kl, void *val, U32 vl)
{
	ht_elem_t* element = (ht_elem_t*)ht_calloc(sizeof(ht_elem_t));
	element->key = ht_dup(key, kl);
	if (table->mode == COPY_MODE) element->val = ht_dup(val, vl);
	else element->val = val;
	element->vl = vl;
	element->kl = kl;
	element->next = NULL;
	return element;
}

static void element_delete(tmode_t tmode, ht_elem_t *element)
{
	if (tmode == COPY_MODE) free(element->val);
	free(element->key);
	free(element);
}

static void data_store_delete(tmode_t tmode, ht_elem_t **data_store, U32 capacity)
{
	for (U32 i = 0; i < capacity; i++) {
		while (data_store[i]) {
			ht_elem_t *temp = data_store[i];
			data_store[i] = data_store[i]->next;
			element_delete(tmode, temp);
		}
	}
}

ht_t* hash_table_new_n(U32 n, tmode_t tmode)
{
	ht_t *nh = (ht_t*)ht_calloc(sizeof(ht_t));
	if (n >= MAX_NUMS) nh->tbs[0].tc = MAX_NUMS;
	else if(n <= DEFAULT_CAPACITY ) nh->tbs[0].tc = DEFAULT_CAPACITY;
	else nh->tbs[0].tc = round_up(n);
	nh->ht_max = (U32)((double)nh->tbs[0].tc * MAX_LOAD_FACTOR);
	nh->ht_min = nh->ht_max >> 3;		//最小为最大值除以8
	nh->tbs[0].ds = (ht_elem_t * *)ht_calloc(nh->tbs[0].tc*sizeof(void*));
	nh->tbs[1].ds = NULL;
	nh->tbs[1].tc = 0;
	nh->kc = key_cmp;
	nh->rehashidx = 0;
	nh->used = 0;
	nh->mode = tmode;
	nh->seed = (U32)time(0);
	return nh;
}

void hash_table_destory(ht_t *table)
{
	data_store_delete(table->mode, &table->tbs[0].ds[table->rehashidx], table->tbs[0].tc - table->rehashidx);
	if (table->tbs[1].ds) data_store_delete(table->mode, table->tbs[1].ds, table->tbs[1].tc);
	free(table);
}

/*将first表内的count个元素转移至second表*/
static void move_elements(ht_t *table, U32 count)
{
	ht_elem_t *ftemp;
	while (table->rehashidx < table->tbs[0].tc) {
		if (!(ftemp = table->tbs[0].ds[table->rehashidx])) {
			table->rehashidx++;
			continue; 
		}
		/*重新hash，插入链首*/
		U32 rehash_key = INDEX(XXHASH(ftemp->key, ftemp->kl, table->seed), table->tbs[1].tc);
		ht_elem_t *stemp = table->tbs[1].ds[rehash_key];
		table->tbs[1].ds[rehash_key] = table->tbs[0].ds[table->rehashidx];
		table->tbs[0].ds[table->rehashidx] = ftemp->next;
		table->tbs[1].ds[rehash_key]->next = stemp;
		if (--count == 0) return;
	}

	/*全部转移完毕*/
 	free(table->tbs[0].ds);
	table->tbs[0].ds = table->tbs[1].ds;
	table->tbs[0].tc = table->tbs[1].tc;
	table->tbs[1].tc = 0;
	table->ht_max = (U32)((double)table->tbs[0].tc * MAX_LOAD_FACTOR);
	table->ht_min = table->ht_max >> 3;
	table->tbs[1].ds = NULL;
	table->rehashidx = 0;
}

static void get_tables(ht_t* table, const void* key, U32 kl, ht_elem_t** ht, U32* h)
{
	if (table->tbs[1].ds) move_elements(table, 2);	//一次转移2个元素
	XXH_hash_t hash = XXHASH(key, kl, table->seed);
	h[0] = INDEX(hash, table->tbs[0].tc);
	ht[0] = table->tbs[0].ds[h[0]];
	if (table->tbs[1].ds) {
		h[1] = INDEX(hash, table->tbs[1].tc);
		ht[1] = table->tbs[1].ds[h[1]];
	}
}

void* hash_table_lookup(ht_t *table, const void *key, U32 kl)
{
	ht_elem_t* temp[2] = { 0 };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, kl, temp, hash_key);
	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (table->kc(temp[i]->key, temp[i]->kl, key, kl)) {
				return temp[i]->val;
			}
			temp[i] = temp[i]->next;
		}
	}
	return NULL;
}

/*扩大缩小只重新建表，不进行元素转移*/
static void hash_table_expand(tb_t* tb1, U32 new_size)
{
	tb1->ds = (ht_elem_t * *)ht_calloc(new_size * sizeof(void*));
	tb1->tc = new_size;
}

static void _expand_if_needed(ht_t* table)
{
	if (!table->tbs[1].ds && table->used > table->ht_max && table->tbs[0].tc != MAX_NUMS) {
		hash_table_expand(&table->tbs[1], table->tbs[0].tc * 2);
	}
}

static void replace_element(void** val, void* new_val, size_t len, tmode_t mode)
{
	if (mode == COPY_MODE) {
		free(*val);
		*val = ht_dup(new_val, len);
	}
	else *val = new_val;
}

int hash_table_set(ht_t *table, const void *key, U32 kl, void *val, U32 vl, smode_t smode)
{
	_expand_if_needed(table);
	ht_elem_t *prev = NULL, *temp[2] = { 0 };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, kl, temp, hash_key);
	/*检索key发现相同替换返回1，否则加入链表末尾count+1返回0*/
	for (int i = 0; i < 2; ++i) {
		while (temp[i]) {
			if (table->kc(temp[i]->key, temp[i]->kl, key, kl)) {
				if (smode == MODE_2) return -1;
				replace_element(&temp[i]->val, val, vl, table->mode);
				return 1;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}
	//未查到新建插入
	table->used++;
	ht_elem_t *element = element_new(table, key, kl, val, vl);
	if (prev) {
		prev->next = element;
		return 0;
	}
	if (table->tbs[1].ds) table->tbs[1].ds[hash_key[1]] = element;
	else table->tbs[0].ds[hash_key[0]] = element;
	return 0;
}

static void _resize_if_needed(ht_t *table)
{
	if (!table->tbs[1].ds && table->used < table->ht_min && table->tbs[0].tc != DEFAULT_CAPACITY) {
		hash_table_expand(&table->tbs[1], table->tbs[0].tc / 2);
	}
}

int hash_table_remove(ht_t *table, const void *key, U32 kl)
{
	ht_elem_t *temp[2] = { 0 };
	U32 hash_key[2] = { 0 };
	get_tables(table, key, kl, temp, hash_key);
	for (int i = 0; i < 2; ++i) {
		if (!temp[i]) continue;
		ht_elem_t *prev = NULL, **ptemp = &table->tbs[i].ds[hash_key[i]];
		while (temp[i]) {
			if (table->kc(temp[i]->key, temp[i]->kl, key, kl)) {
				if (!prev) *ptemp = temp[i]->next;
				else prev->next = temp[i]->next;
				element_delete(table->mode, temp[i]);
				table->used--;
				_resize_if_needed(table);
				return 0;
			}
			prev = temp[i];
			temp[i] = temp[i]->next;
		}
	}
	return -1;
}

ht_elem_t* hash_table_elements(ht_t *table, tmode_t mode)
{
	if (table->used == 0) return NULL;
	/*创建返回链表*/
	ht_elem_t head = { 0 }, * ptmp = &head;
	for (U32 i = 0; i < 2; i++) {
		ht_elem_t **ds = table->tbs[i].ds;
		if (!ds) continue;
		for (U32 j = 0; j < table->tbs[i].tc; ++j) {
			ht_elem_t* p = ds[j];
			while (p) {
				ptmp->next = ht_dup(p, sizeof(ht_elem_t));
				ptmp->next->key = ht_dup(p->key, p->kl);
				if (mode == COPY_MODE) ptmp->next->val = ht_dup(p->val, p->vl);
				ptmp = ptmp->next;
				p = p->next;
			}
		}
	}
	return head.next;
}

void hash_table_remove_elements(ht_elem_t* elems, tmode_t tmode)
{
	ht_elem_t* p = elems;
	while (p) {
		free(p->key);
		if (tmode == COPY_MODE) free(p->val);
		p = p->next;
	}
	free(elems);
}

void hash_table_rehash(ht_t* table, U32 seed)
{
	if (table->used == 0) return;
	if (!table->tbs[1].ds) {
		table->seed = seed;
		hash_table_expand(&table->tbs[1], table->tbs[0].tc);
		move_elements(table, U32_MAX);
	}
	else {
		move_elements(table, U32_MAX);
		hash_table_rehash(table, seed);
	}
}

void hash_table_set_compare_func(ht_t* table, int (*kc_func)(const void*, U32, const void*, U32))
{
	table->kc = kc_func;
}

