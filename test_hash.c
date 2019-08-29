#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#if defined(_MSC_VER)
#include "xxhash.h"	//random string
#include <windows.h> //sleep
#include <intrin.h>	//rdtsc
#pragma intrinsic(__rdtsc) 
#else
#include <linux/types.h>	//rdtsc
#include <unistd.h>	//sleep
#endif //  defined(_MSC_VER)
#include "hashtable.h"


//插入数据量
#define SAMPLE_SIZE	1000000

//随机生成每个key长度
#define KEY_LEN 	16

//随机随机生成每个value长度
#define VALUE_LEN 	16

//打开会计算每次调用时间和每次结果校验（负载均衡测试），打开会有10%到20的性能损失
#ifndef DEBUG
#define DEBUG
#endif // !DEBUG


#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
typedef uint32_t U32;
typedef uint64_t U64;
#else
typedef unsigned int U32;
typedef unsigned long long  U64;
#endif

#if defined(__x86_64__) || defined(_WIN64)
#define XXHASH(a, b, c)     XXH64(a, b, c)
#define XXH_hash_t	XXH64_hash_t
#else
#define XXHASH(a, b, c)     XXH32(a, b, c)
#define XXH_hash_t	XXH32_hash_t
#endif

#define UNUSED(x)	(void)((x)+1)

/*获取运行时钟周期，比clock()快至少一个数量级*/
#if defined(_MSC_VER)
#define rdtsc()	__rdtsc()
#define sleep(t)	Sleep(t*1000)
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

//获取电脑主频，有一定误差，实测低于1%
U64 get_dominant_frequency()
{
	U64 t1 = rdtsc();
	sleep(1);
	return rdtsc() - t1;
}

void data_gen(char key[][KEY_LEN], char val[][VALUE_LEN], int size)
{
	int i = 0;
#if defined(_WIN32) || defined(_WIN32_WCE)
#define HASH_LEN	sizeof(U32)
	//基于xxhash生成随机字符串
	XXH_hash_t h, j, k = 0, lens[2] = { KEY_LEN, VALUE_LEN };
	for (; k < 2; ++k) {
		for (; i < size; ++i) {
			j = lens[k];
			while (j > HASH_LEN) {
				h = XXHASH(&j, HASH_LEN, rdtsc());
				memcpy(&key[i][lens[k] - j], &h, HASH_LEN);
				j -= HASH_LEN;
			}
			h = XXHASH(&j, HASH_LEN, rdtsc());
			memcpy(&key[i][lens[k] - j], &h, j);
		}
	}
#else
	FILE* f = fopen("/dev/urandom", "r");
	while (i != size) {
		if (!fgets(key[i], KEY_LEN, f) || !fgets(val[i], VALUE_LEN, f))	continue;
		i++;
	}
	fclose(f);
#endif //  defined(_WIN32) || defined(_WIN32_WCE)
}

void test_performance(table_mode_t mode)
{
	static char key[SAMPLE_SIZE][KEY_LEN], value[SAMPLE_SIZE][VALUE_LEN];
	const double DOMINANT_FREQUENCY = (double)get_dominant_frequency();
	//获取随机数据
	data_gen(key, value, SAMPLE_SIZE);
	
	/*创建*/
	hash_table_t* table = htnew();
	/*添加*/
#ifdef DEBUG
	const char ZEROS[VALUE_LEN] = { 0 };
	U64 max_time = 0, tlast, t0 = rdtsc();
#else
	U64 max_time = 0, t0 = rdtsc();
#endif
	int i;
	for ( i = 0;i < SAMPLE_SIZE; i++) {
#ifdef DEBUG
		tlast = rdtsc();
		if (0 != htsetnx(table, key[i], KEY_LEN, value[i], VALUE_LEN))	memset(value[i], 0, VALUE_LEN);	// 出现重复value全置0记录
		tlast = rdtsc() - tlast;
		if (tlast > max_time) max_time = tlast;
#else
		htsetnx(table, key[i], KEY_LEN, value[i], VALUE_LEN);
#endif
	}
	double t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("add %d keys in %fs, %fus on average, and the max simple time is %fus.\n",i, t, t * 1000000 / i, (double)max_time * 1000000 / DOMINANT_FREQUENCY);

	/*查询*/
	max_time = 0;
	t0 = rdtsc();
	for (i = 0; i < SAMPLE_SIZE; i++) {
#ifdef DEBUG
		tlast = rdtsc();
		char* res = (char*)htget(table, key[i], KEY_LEN);
		tlast = rdtsc() - tlast;
		if (tlast > max_time) max_time = tlast;
		assert(res && (!memcmp(res, value[i], VALUE_LEN) || !memcmp(value[i], ZEROS, VALUE_LEN)));
		UNUSED(ZEROS);
		UNUSED(res);
#else
		htget(table, key[i], KEY_LEN);
#endif
	}
	t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("lookup %d times in %fs, %fus on average, and the max simple time is %fus.\n",i, t, t * 1000000 / i, (double)max_time * 1000000 / DOMINANT_FREQUENCY);
	
	/*获取所有元素*/
	t0 = rdtsc();
	htgetall(table, REF_MODE);
	printf("get all of elements in %fs\n", (double)(rdtsc() - t0) / DOMINANT_FREQUENCY);

	/*全部元素重新哈希*/
	t0 = rdtsc();
	htrh(table, (U32)t0);
	printf("rehashing in %fs\n", (double)(rdtsc() - t0) / DOMINANT_FREQUENCY);

	/*删除*/
	max_time = 0;
	t0 = rdtsc();
	for (i = 0; i < SAMPLE_SIZE; i++) {
#ifdef DEBUG
		tlast = rdtsc();
		int ret = htrm(table, key[i], KEY_LEN);
		tlast = rdtsc() - tlast;
		if (tlast > max_time) max_time = tlast;
		assert((ret == 0 && !htget(table, key[i], KEY_LEN)) || !memcmp(value[i], ZEROS, VALUE_LEN));
		UNUSED(ret);
#else
		hash_table_remove(table, key[i], KEY_LEN);
#endif
	}
	t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("remove %d times in %fs, %fus on average, and the max simple time is %fus.\n", i, t, t * 1000000 / i, (double)max_time * 1000000 / DOMINANT_FREQUENCY);
	
	/*释放*/
	htdel(table);
	static const char *modes[2] = { "REF_MODE", "COPY_MODE" };
	printf("%s performance test end.\n\n", modes[mode]);
} 

int main()
{
	printf("all test start\n");

	//两种模式添加、查询、重散列、删除和获取所有元素性能测试
	test_performance(COPY_MODE);
	test_performance(REF_MODE);

	printf("all test end\n");
}