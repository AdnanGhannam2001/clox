CC= clang
CFLAGS= -Wall -Wextra -Wunknown-pragmas -std=c99 -lm

DEBUG ?= 0

ifeq ($(DEBUG), 0)
	CFLAGS+= -O3 -static
else
	CFLAGS+= -DCLOX_DEBUG -g -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wfloat-equal -Wuninitialized -Wnull-dereference -Wpointer-sign -Wcast-qual 

	ifeq ($(DEBUG), 2)
		CFLAGS+= -DCLOX_DEBUG_PRINT
	endif
endif

SRC_DIR= src
SRC= $(wildcard $(SRC_DIR)/*.c)
INCLUDE_DIR= includes

main: $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@.o $^

clean:
	rm -rf *.o