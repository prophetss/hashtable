#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "hashtable.h"


#define SAMPLE_SIZE	1024*256


int main()
{

	static char key[SAMPLE_SIZE][160];
	static size_t klen[SAMPLE_SIZE];
	static char value[SAMPLE_SIZE][16];
	static size_t vlen[SAMPLE_SIZE];

	FILE *f = fopen("sample.txt", "a+");
    int i = 0;
    while (!feof(f)) { 
    	char buf[256];
       	if (NULL == fgets(buf, 256, f)) {
       		return 0;
       	}
        /*分别保存key-value和对应长度*/
        sscanf(buf, "%*[^ ]%s %s", key[i], value[i]);
        klen[i] = strlen(key[i]);
        vlen[i] = strlen(value[i]);
        i++;
    } 
    fclose(f);

    clock_t t0 = clock();
    hash_table_t *table = hash_table_new_n(SAMPLE_SIZE);
    for (int j = 0; j < i; j++) {
    	hash_table_add(table, key[j], klen[j], value[j], vlen[j]);
    }
    clock_t t1 = clock();
    printf("add %d keys used %fs\n", i, (double)(t1-t0)/CLOCKS_PER_SEC);

    for (int k = 0; k < 10000000; k++) {
    	char *res = hash_table_lookup(table, key[k%i], klen[k%i]);
    	assert(memcmp(res, value[k%i], vlen[k%i]) == 0);
    }
    clock_t t2 = clock();
    printf("lookup 10,000,000 times used %fs\n", (double)(t2-t1)/CLOCKS_PER_SEC);
    
}