SRC = xxhash.c xxhash.h hashtable.c hashtable.h test_hash.c
DEBUG =	-DDEBUG -Og -Wall -Wextra -ggdb

sample_r: $(SRC)
	gcc -O3 -o $@ $^

sample_d: $(SRC)
	gcc $(DEBUG) -o $@ $^
	
.PHONY: clean
clean:
	-rm sample_r sample_d -f
