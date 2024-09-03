CC= clang
CFLAGS= -Wall -Wextra -Wunknown-pragmas -std=c99

ifeq ($(DEBUG), 1)
	CFLAGS+= -DCLOX_DEBUG -g 
else
	CFLAGS+= -O3
endif

SRC_DIR= src
SRC= $(wildcard $(SRC_DIR)/*.c)
INCLUDE_DIR= includes

main: $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@.o $^

clean:
	rm -rf *.o