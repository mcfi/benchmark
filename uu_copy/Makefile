# Makefile for umemcpy_bench

CC      := clang
CFLAGS  := -O3 -march=native -mtune=native -Wall -Wextra
LDFLAGS := 

TARGET  := uu_copy_bench
SRC     := uu_copy_bench.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

