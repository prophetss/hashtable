#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <linux/types.h>
#include "hashtable.h"


#define SAMPLE_SIZE	1024*1024

void test_add_and_remove()
{
	printf("add,remove and get all elements test start\n");
	hash_table_t *table = hash_table_new(REF_MODE);
	//当前表容量设置为4负载因子为0.75，8条数据会有扩容发生
	char* keys[] = {"Chinese Population", "India's Population", "American Population", "Indonesian Population",
	"Brazil Population", "Pakistan Population", "Nigerian Population", "Bangladeshi Population"};
	unsigned long long values[] = {1400000000L, 1300000000L, 320000000L, 250000000L, 200000000L, 190000000L, 
		180000000L, 160000000L};
	int ret;
	unsigned int i;
	for (i = 0; i < sizeof(keys)/sizeof(char*); ++i) {
		ret = hash_table_add(table, keys[i], strlen(keys[i]), &values[i], sizeof(unsigned long long));
		/*插入成功,无替换,返回0*/
		if (ret == 0) {
			/*成功反查*/
			char *res = hash_table_lookup(table, keys[i], strlen(keys[i]));
			assert(memcmp(res, &values[i], sizeof(unsigned long long)) == 0);
		}
		else {
			printf("failed to add a value. sad :( \n");
			return;
		}
	}

	/*插入相同key替换value*/
	char new_value[] = "1.4B";
	ret = hash_table_add(table, keys[0], strlen(keys[0]), new_value, sizeof(new_value));
	/*插入成功,发生替换,返回1*/
	if (ret == 1) {
		char *res = hash_table_lookup(table, keys[0], strlen(keys[0]));
		assert(memcmp(res, new_value, sizeof(new_value)) == 0);
	}
	else {
		printf("failed to change a value. sad :( \n");
		return;
	}

	printf("all of the add tests successfully. happy :) \n");

	/*删除*/
	ret = hash_table_remove(table, keys[0], strlen(keys[0]));
	if (ret == 0) {
		char *res = hash_table_lookup(table, keys[0], strlen(keys[0]));
		if (NULL == res) {
			printf("lookup result: value is null, remove successfully. happy :) \n");
		}
		else {
			printf("lookup result: value is not null, coding bugs. very sad :( :( :( \n");
			return;
		}
	}
	else {
		printf("failed to remove. sad :( \n");
		return;
	}

	//获取所有元素
	hash_table_element_t *all = hash_table_elements(table, COPY_MODE);
	for (i = 0; i < table->key_count; ++i) {
		assert(memcmp(hash_table_lookup(table, all[i].key, all[i].key_len), all[i].value, all[i].value_len) == 0);
		//拷贝模式调用者负责释放
		free(all[i].key);
		free(all[i].value);
	}
	free(all);
	printf("get all of elements successfully. happy :) \n");
	
	hash_table_delete(table);

	printf("add,remove and get all elements test end\n\n");
}

/*获取运行时钟周期，调用此函数在我本机实测消耗时间平均约为clock（）的十四分之一*/
inline __u64 rdtsc()
{
	__u32 lo, hi;
	__asm__ __volatile__
	(
	    "rdtsc":"=a"(lo), "=d"(hi)
	);
	return (__u64)hi << 32 | lo;
}

void test_performance(table_mode_t mode)
{
	const char *modes[2] = {"REF_MODE", "COPY_MODE"};

	printf("%s performance test start\n", modes[mode]);
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
	hash_table_t *table = hash_table_new(mode);

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
	printf("%s performance test end\n", modes[mode]);
}

int main()
{
	printf("all test start\n");

	/*插入、删除、获取所有元素测试*/
	test_add_and_remove();

	/*插入、查询性能测试(引用模式)*/
	test_performance(REF_MODE);

	/*拷贝模式*/
	test_performance(COPY_MODE);

	printf("all test end\n");
}