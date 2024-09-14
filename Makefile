CC= clang
CFLAGS= -Wall -Wextra -Wunknown-pragmas -std=c99

ifeq ($(DEBUG), 1)
	CFLAGS+= -DCLOX_DEBUG -g -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wfloat-equal -Wuninitialized -Wnull-dereference -Wpointer-sign -Wcast-qual 
else
	CFLAGS+= -O3 -static
endif

SRC_DIR= src
SRC= $(wildcard $(SRC_DIR)/*.c)
INCLUDE_DIR= includes

main: $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@.o $^

clean:
	rm -rf *.o