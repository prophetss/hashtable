SRC = xxhash.c hashtable.c test_hash.c
DEBUG =	-O0 -Wall -Wextra -ggdb
SAMPLE = tar -xjf sample.tar.bz2

test: $(SRC)
	$(SAMPLE)
	gcc -O2 -o $@ $^

debug: $(SRC)
	$(SAMPLE)
	gcc $(DEBUG) -o $@ $^
	
.PHONY: clean
clean:
	-rm test
