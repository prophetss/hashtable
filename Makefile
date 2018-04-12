SRC =  test_hash.c hashtable.c xxhash.c

test: $(SRC)
	gcc -O2 -Wall -o $@ $^
	
.PHONY: clean
clean:
	-rm test