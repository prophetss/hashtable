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


//����������
#define SAMPLE_SIZE	1000000

//�������ÿ��key����
#define KEY_LEN 	16

//����������ÿ��value����
#define VALUE_LEN 	16

static char key[SAMPLE_SIZE][KEY_LEN], value[SAMPLE_SIZE][VALUE_LEN];

static const char ZEROS[VALUE_LEN] = { 0 };

//�򿪻�ͳ��ÿ�ε���ʱ�䣨���ؾ�����ԣ�������10%��20��������ʧ
#ifndef DEBUG
//#define DEBUG
#endif // !DEBUG

#ifdef DEBUG
#define BEGIN tlast = rdtsc()
#define END	if ((tlast = rdtsc() - tlast) > max_time) max_time = tlast
#else
#define BEGIN
#define END
#endif

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

/*��ȡ����ʱ�����ڣ���clock()������һ��������*/
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

static double DOMINANT_FREQUENCY;

//��ȡ������Ƶ����һ����ʵ�����1%
U64 get_dominant_frequency()
{
	U64 t1 = rdtsc();
	sleep(1);
	return rdtsc() - t1;
}

void data_gen()
{
	int i = 0;
#if defined(_WIN32) || defined(_WIN32_WCE)
#define HASH_LEN	sizeof(U32)
	//����xxhash��������ַ���
	XXH_hash_t h, j, k = 0, lens[2] = { KEY_LEN, VALUE_LEN };
	for (; k < 2; ++k) {
		for (; i < SAMPLE_SIZE; ++i) {
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
	while (i != SAMPLE_SIZE) {
		if (!fgets(key[i], KEY_LEN, f) || !fgets(value[i], VALUE_LEN, f))	continue;
		i++;
	}
	fclose(f);
#endif //  defined(_WIN32) || defined(_WIN32_WCE)
}

void test_set(ht_t* table)
{
	U64 max_time = 0, tlast = 0, t0 = rdtsc();
	for (U32 i = 0; i < SAMPLE_SIZE; i++) {
		BEGIN;
		if (0 != htsetnx(table, key[i], KEY_LEN, value[i], VALUE_LEN))	memset(value[i], 0, VALUE_LEN);	// �����ظ�valueȫ��0�����
		END;
	}
	double t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("add %d keys in %fs, %fus on average, and the ht_max simple time is %fus.\n", SAMPLE_SIZE, t, t * 1000000 / SAMPLE_SIZE, (double)max_time * 1000000 / DOMINANT_FREQUENCY);
	UNUSED(tlast);
}

void test_lookup(ht_t* table)
{
	U64 max_time = 0, tlast = 0, t0 = rdtsc();
	char *res;
	for (U32 i = 0; i < SAMPLE_SIZE; i++) {
		BEGIN;
		res = (char*)htget(table, key[i], KEY_LEN);
		END;
		assert(res && (!memcmp(res, value[i], VALUE_LEN) || !memcmp(value[i], ZEROS, VALUE_LEN)));	//valueȫΪ0�ǲ������key���ظ����ı��
	}
	double t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("lookup %d times in %fs, %fus on average, and the ht_max simple time is %fus.\n", SAMPLE_SIZE, t, t * 1000000 / SAMPLE_SIZE, (double)max_time * 1000000 / DOMINANT_FREQUENCY);
	UNUSED(res);
	UNUSED(tlast);
}

void test_remove(ht_t* table)
{
	U64 t0 = rdtsc();
	ht_elem_t* elems = htgetall(table, REF_MODE);
	printf("get all of elements in %fs\n", (double)(rdtsc() - t0) / DOMINANT_FREQUENCY);

	/*ȫ��Ԫ�����¹�ϣ*/
	t0 = rdtsc();
	htrh(table, (U32)t0);
	printf("rehashing in %fs\n", (double)(rdtsc() - t0) / DOMINANT_FREQUENCY);

	/*ɾ��*/
	U64 max_time = 0, tlast = 0;;
	t0 = rdtsc();
	ht_elem_t* p = elems;
	int ret = 0;
	while(p) {
		BEGIN;
		ret = htrm(table, p->key, KEY_LEN);
		END;
		assert(ret == 0 && !htget(table, p->key, KEY_LEN));
		p = p->next;
	}
	double t = (double)(rdtsc() - t0) / DOMINANT_FREQUENCY;
	printf("remove all elements in %fs, %fus on average, and the ht_max simple time is %fus.\n" ,t, t * 1000000 / SAMPLE_SIZE, (double)max_time * 1000000 / DOMINANT_FREQUENCY);

	//ȫ��ɾ�������ʱ��ӦΪ�ձ�
	assert(htgetall(table, REF_MODE) == NULL);

	//�ͷ�Ԫ�ػ�ȡ
	htrmall(elems, REF_MODE);

	UNUSED(ret);
	UNUSED(tlast);
}

void test_performance()
{
	DOMINANT_FREQUENCY = (double)get_dominant_frequency();

	//��ȡ�������
	data_gen();

	/*����*/
	ht_t* table = htnew();

	/*���*/
	test_set(table);

	/*��ѯ*/
	test_lookup(table);
	
	/*ɾ��*/
	test_remove(table);
	
	int mode = table->mode;
	/*�ͷ�*/
	htdel(table);

	static const char* modes[2] = { "REF_MODE", "COPY_MODE" };
	printf("%s performance test end.\n", modes[mode]);

	UNUSED(ZEROS);
} 

int main()
{
	printf("all test start\n");

	//��ӡ���ѯ����ɢ�С�ɾ���ͻ�ȡ����Ԫ�����ܲ���
	test_performance();

	printf("all test end\n");
}