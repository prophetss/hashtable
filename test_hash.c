#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#if defined(_MSC_VER)
#include "xxhash.h"
#include <intrin.h>
#pragma intrinsic(__rdtsc) 
#else
#include <linux/types.h>
#endif //  defined(_MSC_VER)
#include "hashtable.h"


//插入数据量
#define SAMPLE_SIZE	1000000

//随机生成每个key长度
#define KEY_LEN 	8

//随机随机生成每个value长度
#define VALUE_LEN 	8

//查询次数，必须是查询数量的整数倍
#define LOOKUPS 	100000000

//打开会计算每次调用时间和结果校验，打开会有10%到20的性能损失
//#define DEBUG


void test_all()
{
	printf("add,remove and get all elements test start\n");
	hash_table_t *table = hash_table_new_n(4, 0.75, REF_MODE);
	//当前表容量设置为4负载因子为0.75，8条数据会有扩容发生
	char* keys[] = { "Chinese Population", "India's Population", "American Population", "Indonesian Population",
		"Brazil Population", "Pakistan Population", "Nigerian Population", "Bangladeshi Population" };
	unsigned long long values[] = { 1400000000L, 1300000000L, 320000000L, 250000000L, 200000000L, 190000000L,
		180000000L, 160000000L };
	int ret;
	unsigned int i;
	for (i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
		ret = hash_table_add(table, keys[i], strlen(keys[i]), &values[i], sizeof(unsigned long long));
		/*插入成功,无替换,返回0*/
		if (ret == 0) {
			/*成功反查*/
			char *res = (char*)hash_table_lookup(table, keys[i], strlen(keys[i]));
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
		char *res = (char*)hash_table_lookup(table, keys[0], strlen(keys[0]));
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
		char *res = (char*)hash_table_lookup(table, keys[0], strlen(keys[0]));
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

/*获取运行时钟周期，比clock()快一个数量级*/
#if defined(_MSC_VER)
#define rdtsc()	__rdtsc()
#else
__u64 rdtsc()
{
	__u32 lo, hi;
	__asm__ __volatile__
		(
			"rdtsc":"=a"(lo), "=d"(hi)
			);
	return (__u64)hi << 32 | lo;
}
#endif //  defined(_MSC_VER)

void test_performance(table_mode_t mode, hash_size_t capacity, float load_factor)
{
	const char *modes[2] = { "REF_MODE", "COPY_MODE" };

	static char key[SAMPLE_SIZE][KEY_LEN], value[SAMPLE_SIZE][VALUE_LEN];

	int i = 0;

#if defined(_WIN32) || defined(_WIN32_WCE)
#define HASH_LEN	sizeof(hash_size_t)
	//基于xxhash生成随机字符串
	hash_size_t h, j;
	for (; i < SAMPLE_SIZE; ++i) {
		j = KEY_LEN;
		while (j > HASH_LEN) {
			h = XXHASH(&j, HASH_LEN, rdtsc());
			memcpy(&key[i][KEY_LEN - j], &h, HASH_LEN);
			j -= HASH_LEN;
		}
		h = XXHASH(&j, HASH_LEN, rdtsc());
		memcpy(&key[i][KEY_LEN - j], &h, j);
		j = VALUE_LEN;
		while (j > HASH_LEN) {
			h = XXHASH(&j, HASH_LEN, rdtsc());
			memcpy(&key[i][VALUE_LEN - j], &h, HASH_LEN);
			j -= HASH_LEN;
		}
		h = XXHASH(&j, HASH_LEN, rdtsc());
		memcpy(&key[i][VALUE_LEN - j], &h, j);
	}
#else
	FILE *f = fopen("/dev/urandom", "a+");
	while (!feof(f)) {
		if (!fgets(key[i], KEY_LEN, f) || !fgets(value[i], VALUE_LEN, f)) {
			continue;
		}
		if (++i == SAMPLE_SIZE) {
			break;
		}
	}
	fclose(f);
#endif //  defined(_WIN32) || defined(_WIN32_WCE)

	/*创建*/
	hash_table_t *table = hash_table_new_n(capacity, load_factor, mode);

	hash_size_t x = 0, y = 0;
	/*添加*/
	unsigned long long max = 0;
	clock_t t0 = clock();
	unsigned long long tlast = rdtsc(), tnext = tlast;
	for (int j = 0; j < i; j++) {
#ifdef DEBUG
		tlast = tnext;
#endif
		int ret = hash_table_add(table, key[j], KEY_LEN, value[j], VALUE_LEN);
#ifdef DEBUG
		if (ret == 1) x++;
		tnext = rdtsc();
		if (tnext - tlast > max) {
			max = tnext - tlast;
		}
#endif
	}
	clock_t t1 = clock();
	double dt1 = (double)(t1 - t0) / CLOCKS_PER_SEC;
	/*下面3.2G是我的电脑的主频，自己根据自己电脑修改，1000000是将s转化为us*/
	printf("add %d keys in %fs, %fus on average, and the max simple time is %.2fus.\n",
		i, dt1, dt1 * 1000000 / i, max / (double)(3200000000L / 1000000));

	/*查询*/
	max = 0;
	tlast = rdtsc();
	tnext = tlast;
	for (int k = 0; k < LOOKUPS; k++) {
#ifdef DEBUG
		tlast = tnext;
#endif
		char *res = (char*)hash_table_lookup(table, key[k % i], KEY_LEN);
#ifdef DEBUG
		if (memcmp(res, value[k % i], VALUE_LEN) != 0) ++y;
		tnext = rdtsc();
		if (tnext - tlast > max) {
			max = tnext - tlast;
		}
#endif
	}
	//当前，查询失败的次数/插入替换的次数应等于查询次数/插入个数，执行数次几乎可以保证无差错
	assert(y == x*(LOOKUPS / SAMPLE_SIZE));
	clock_t t2 = clock();
	double dt2 = (double)(t2 - t1) / CLOCKS_PER_SEC;
	printf("lookup 100,000,000 times in %fs, %fus on average, and the max simple time is %.2fus.\n",
		dt2, dt2 * 1000000 / LOOKUPS, max / (double)(3200000000L / 1000000));

	/*释放*/
	hash_table_delete(table);
	printf("%s performance test end. capacity is %llu, load factor is %f\n\n", modes[mode], capacity, load_factor);
}

int main()
{
	printf("all test start\n");

	for (int i = 0; i < 10000; ++i) {
		test_all();
	}

	//不同的模式和起始表大小和负载因子,添加和查询性能测试
	test_performance(COPY_MODE, 32, 0.75);

	test_performance(REF_MODE, 32, 0.75);

	test_performance(REF_MODE, 32, 2);

	test_performance(REF_MODE, 32, 0.25);


	printf("all test end\n");
}