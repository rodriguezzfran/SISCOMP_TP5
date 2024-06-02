# Makefile for GPIO button driver

# Compiler
CC := gcc

# Flags
CFLAGS := -Wall

# Module name
MODULE_NAME := CDD

# Source files
SRC := $(MODULE_NAME).c

# Object files
OBJ := $(SRC:.c=.o)

# Targets
all: $(MODULE_NAME).ko

$(MODULE_NAME).ko: $(OBJ)
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(OBJ)

.PHONY: all clean
