TARGET = gpu-miner
SOURCE = gpu-miner.c
CC = gcc
CFLAGS = -O2

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $^ -lOpenCL

clean:
	rm -f $(TARGET)

.PHONY: all clean
