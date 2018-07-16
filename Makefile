SRC = error.h xxhash.c xxhash.h hashtable.c hashtable.h test_hash.c
DEBUG =	-O2 -Wall -Wextra -ggdb
SAMPLE = tar -xjf sample.tar.bz2
SHELL_EXIST = $(shell if [ ! -e sample0.txt ]; then $(SAMPLE); fi;)

test: $(SRC)
	$(SHELL_EXIST)
	gcc -O2 -o $@ $^

debug: $(SRC)
	$(SHELL_EXIST)
	gcc $(DEBUG) -o $@ $^
	
.PHONY: clean
clean:
	-rm test debug -f
