SRC = xxhash.c xxhash.h hashtable.c hashtable.h test_hash.c
DEBUG =	-O0 -Wall -Wextra -ggdb

test: $(SRC)
	gcc -O2 -o $@ $^

debug: $(SRC)
	gcc $(DEBUG) -o $@ $^
	
.PHONY: clean
clean:
	-rm test debug -f
