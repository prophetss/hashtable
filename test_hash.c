#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include<linux/types.h>
#include "hashtable.h"


#define SAMPLE_SIZE	1024*1024

void test_add_and_remove()
{
	printf("add and remove test start\n");
	hash_table_t *table = hash_table_new();
	char key[] = "Chinese Population";
	unsigned long long value = 1400000000L;
	printf("%s:%llu\n", key, value);

	int ret = hash_table_add(table, key, sizeof(key), &value, sizeof(unsigned long long));
	/*插入成功,无替换,返回0*/
	if (ret == 0) {
		char *res = hash_table_lookup(table, key, sizeof(key));
		printf("lookup result: %s:%llu. add successfully. happy :) \n", key, *(unsigned long long*)res);
	}
	else {
		printf("failed to add a value. sad :( \n");
		return;
	}

	/*插入相同key替换value*/
	char new_value[] = "1.4B";
	ret = hash_table_add(table, key, sizeof(key), new_value, sizeof(new_value));
	/*插入成功,发生替换,返回1*/
	if (ret == 1) {
		char *res = hash_table_lookup(table, key, sizeof(key));
		printf("lookup result: %s:%s. change a value successfully. happy :) \n", key, res);
	}
	else {
		printf("failed to change a value. sad :( \n");
		return;
	}

	/*删除*/
	ret = hash_table_remove(table, key, sizeof(key));
	if (ret == 0) {
		char *res = hash_table_lookup(table, key, sizeof(key));
		if (NULL == res) {
			printf("lookup result: value is null, remove successfully. happy :) \n");
		}
		else {
			printf("lookup result: value is not null, coding bugs. very sad :( :( :( \n");
		}
	}
	else {
		printf("failed to remove. sad :( \n");
		return;
	}

	hash_table_delete(table);
	printf("add and remove test end\n");
}

/*获取运行时钟周期，调用此函数在我本机实测消耗时间约为clock（）的十分之一*/
inline __u64 rdtsc()
{
	__u32 lo, hi;
	__asm__ __volatile__
	(
	    "rdtsc":"=a"(lo), "=d"(hi)
	);
	return (__u64)hi << 32 | lo;
}

void test_performance()
{
	printf("performance test start\n");
	static char key[SAMPLE_SIZE][160];
	static size_t klen[SAMPLE_SIZE];
	static char value[SAMPLE_SIZE][16];
	static size_t vlen[SAMPLE_SIZE];

	int i = 0;
	/*读取样本数据，一共1,038,000条*/
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

	/*创建*/
	hash_table_t *table = hash_table_new();

	/*添加*/
	unsigned long long max = 0;
	clock_t t0 = clock();
	unsigned long long tlast = rdtsc(), tnext = tlast;
	for (int j = 0; j < i; j++) {
		tlast = tnext;
		hash_table_add(table, key[j], klen[j], value[j], vlen[j]);
		tnext = rdtsc();
		if (tnext - tlast > max) {
			max = tnext - tlast;
		}
	}
	clock_t t1 = clock();
	double dt1 = (double)(t1 - t0) / CLOCKS_PER_SEC;
	/*下面3.2G是我的电脑的主频，自己根据自己电脑修改，1000000是将s转化为us*/
	printf("add %d keys in %fs %fus on average, and the max simple time is %.2fus.\n",
	       i, dt1, dt1 * 1000000 / i, max / (double)(3200000000L / 1000000));

	/*查询*/
	max = 0;
	tlast = rdtsc();
	tnext = tlast;
	for (int k = 0; k < 100000000L; k++) {
		tlast = tnext;
		char *res = hash_table_lookup(table, key[k % i], klen[k % i]);
		assert(memcmp(res, value[k % i], vlen[k % i]) == 0);
		tnext = rdtsc();
		if (tnext - tlast > max) {
			max = tnext - tlast;
		}
	}
	clock_t t2 = clock();
	double dt2 = (double)(t2 - t1) / CLOCKS_PER_SEC;
	printf("lookup 100,000,000 times in %fs %fus on average, and the max simple time is %.2fus.\n",
	       dt2, dt2 / 100, max / (double)(3200000000L / 1000000));

	/*释放*/
	hash_table_delete(table);
	printf("performance test end\n");
}

int main()
{
	printf("all test start\n");

	/*插入删除测试*/
	test_add_and_remove();

	/*插入、查询性能测试*/
	test_performance();

	printf("all test end\n");
}