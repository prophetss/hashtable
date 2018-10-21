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


//����������
#define SAMPLE_SIZE	1000000

//�������ÿ��key����
#define KEY_LEN 	8

//����������ÿ��value����
#define VALUE_LEN 	8

//��ѯ�����������ǲ�ѯ������������
#define LOOKUPS 	100000000

//�򿪻����ÿ�ε���ʱ��ͽ��У�飬�򿪻���10%��20��������ʧ
//#define DEBUG


void test_all()
{
	printf("add,remove and get all elements test start\n");
	hash_table_t *table = hash_table_new_n(4, 0.75, REF_MODE);
	//��ǰ����������Ϊ4��������Ϊ0.75��8�����ݻ������ݷ���
	char* keys[] = { "Chinese Population", "India's Population", "American Population", "Indonesian Population",
		"Brazil Population", "Pakistan Population", "Nigerian Population", "Bangladeshi Population" };
	unsigned long long values[] = { 1400000000L, 1300000000L, 320000000L, 250000000L, 200000000L, 190000000L,
		180000000L, 160000000L };
	int ret;
	unsigned int i;
	for (i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
		ret = hash_table_add(table, keys[i], strlen(keys[i]), &values[i], sizeof(unsigned long long));
		/*����ɹ�,���滻,����0*/
		if (ret == 0) {
			/*�ɹ�����*/
			char *res = (char*)hash_table_lookup(table, keys[i], strlen(keys[i]));
			assert(memcmp(res, &values[i], sizeof(unsigned long long)) == 0);
		}
		else {
			printf("failed to add a value. sad :( \n");
			return;
		}
	}

	/*������ͬkey�滻value*/
	char new_value[] = "1.4B";
	ret = hash_table_add(table, keys[0], strlen(keys[0]), new_value, sizeof(new_value));
	/*����ɹ�,�����滻,����1*/
	if (ret == 1) {
		char *res = (char*)hash_table_lookup(table, keys[0], strlen(keys[0]));
		assert(memcmp(res, new_value, sizeof(new_value)) == 0);
	}
	else {
		printf("failed to change a value. sad :( \n");
		return;
	}

	printf("all of the add tests successfully. happy :) \n");

	/*ɾ��*/
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

	//��ȡ����Ԫ��
	hash_table_element_t *all = hash_table_elements(table, COPY_MODE);
	for (i = 0; i < table->key_count; ++i) {
		assert(memcmp(hash_table_lookup(table, all[i].key, all[i].key_len), all[i].value, all[i].value_len) == 0);
		//����ģʽ�����߸����ͷ�
		free(all[i].key);
		free(all[i].value);
	}
	free(all);
	printf("get all of elements successfully. happy :) \n");

	hash_table_delete(table);

	printf("add,remove and get all elements test end\n\n");
}

/*��ȡ����ʱ�����ڣ���clock()��һ��������*/
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
	//����xxhash��������ַ���
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

	/*����*/
	hash_table_t *table = hash_table_new_n(capacity, load_factor, mode);

	hash_size_t x = 0, y = 0;
	/*���*/
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
	/*����3.2G���ҵĵ��Ե���Ƶ���Լ������Լ������޸ģ�1000000�ǽ�sת��Ϊus*/
	printf("add %d keys in %fs, %fus on average, and the max simple time is %.2fus.\n",
		i, dt1, dt1 * 1000000 / i, max / (double)(3200000000L / 1000000));

	/*��ѯ*/
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
	//��ǰ����ѯʧ�ܵĴ���/�����滻�Ĵ���Ӧ���ڲ�ѯ����/���������ִ�����μ������Ա�֤�޲��
	assert(y == x*(LOOKUPS / SAMPLE_SIZE));
	clock_t t2 = clock();
	double dt2 = (double)(t2 - t1) / CLOCKS_PER_SEC;
	printf("lookup 100,000,000 times in %fs, %fus on average, and the max simple time is %.2fus.\n",
		dt2, dt2 * 1000000 / LOOKUPS, max / (double)(3200000000L / 1000000));

	/*�ͷ�*/
	hash_table_delete(table);
	printf("%s performance test end. capacity is %llu, load factor is %f\n\n", modes[mode], capacity, load_factor);
}

int main()
{
	printf("all test start\n");

	for (int i = 0; i < 10000; ++i) {
		test_all();
	}

	//��ͬ��ģʽ����ʼ���С�͸�������,��ӺͲ�ѯ���ܲ���
	test_performance(COPY_MODE, 32, 0.75);

	test_performance(REF_MODE, 32, 0.75);

	test_performance(REF_MODE, 32, 2);

	test_performance(REF_MODE, 32, 0.25);


	printf("all test end\n");
}