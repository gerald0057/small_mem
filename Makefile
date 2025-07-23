
CC = gcc

INC_DIR = inc
SRC_DIR = src
BUILD_DIR = build

CFLAGS = -I$(INC_DIR) -Wall -Wextra -g

SRCS = $(wildcard $(SRC_DIR)/*.c) main.c
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = smem_demo

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) TARGET

.PHONY: all clean
