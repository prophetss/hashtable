#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "hashtable.h"


#define SAMPLE_SIZE	1024*1024

void test_add_and_remove()
{
	printf("add and remove test start\n");
	hash_table_t *table = hash_table_new();
	char key[] = "Chinese Population";
	size_t key_len = strlen(key)+1;
	unsigned long long value = 1400000000L;
	printf("%s:%llu\n", key, value);

	int ret = hash_table_add(table, key, key_len, &value, sizeof(unsigned long long));
	if (ret >= 0) {
		printf("add success. happy :) \n");
	}
	char *res = hash_table_lookup(table, key, key_len);
	printf("lookup result: %s:%llu\n", key, *(unsigned long long*)res);

	ret = hash_table_remove(table, key, key_len);
	if (ret != 0) {
		printf("remove failed. sad :( \n");
		return;
	}
	res = hash_table_lookup(table, key, key_len);
	if (NULL == res) {
		printf("lookup result: value is null, remove success. happy :) \n");
	}
	else {
		printf("lookup result: value is not null, coding bugs. very sad :( :( :( \n\n");
	}
	hash_table_delete(table);
	printf("add and remove test end\n");
}

void test_performance()
{
	printf("performance test start\n");
	static char key[SAMPLE_SIZE][160];
	static size_t klen[SAMPLE_SIZE];
	static char value[SAMPLE_SIZE][16];
	static size_t vlen[SAMPLE_SIZE];

	int i = 0;
	/*读取样本数据，一共102万多条*/
	for (int fn = 0; fn < 6; fn++) {
		char fname[16];
		sprintf(fname, "sample%d.txt", fn);
		FILE *f = fopen(fname, "a+");
		while (!feof(f)) {
			char buf[256];
			if (NULL == fgets(buf, 256, f)) {
				return;
			}
			/*分别保存key-value和对应长度*/
			sscanf(buf, "%*[^ ]%s %s", key[i], value[i]);
			klen[i] = strlen(key[i]);
			vlen[i] = strlen(value[i]);
			i++;
		}
		fclose(f);
	}

	clock_t t0 = clock();
	/*创建*/
	hash_table_t *table = hash_table_new_n(SAMPLE_SIZE);

	/*添加*/
	for (int j = 0; j < i; j++) {
		hash_table_add(table, key[j], klen[j], value[j], vlen[j]);
	}
	clock_t t1 = clock();
	double dt1 = (double)(t1 - t0) / CLOCKS_PER_SEC;
	printf("add %d keys in %fs, %fus on average.\n", i, dt1, dt1 * 1000000 / i);

	/*查询*/
	for (int k = 0; k < 100000000L; k++) {
		char *res = hash_table_lookup(table, key[k % i], klen[k % i]);
		assert(memcmp(res, value[k % i], vlen[k % i]) == 0);
	}
	clock_t t2 = clock();
	double dt2 = (double)(t2 - t1) / CLOCKS_PER_SEC;
	printf("lookup 100,000,000 times in %fs, %fus on average.\n", dt2, dt2 / 100);

	/*释放*/
	hash_table_delete(table);
	printf("performance test end\n");
}

int main()
{
	printf("all test start\n");

	/*插入删除测试*/
	test_add_and_remove();

	/*插入查询性能测试*/
	test_performance();

	printf("all test end\n");
}