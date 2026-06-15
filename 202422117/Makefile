# Root Makefile for simple-ipc

APP_DIR   := apps
KERN_DIR  := kernel
INC_DIR   := include
BUILD_DIR := .build

KDIR ?= /lib/modules/$(shell uname -r)/build

CC      := gcc
CFLAGS  := -Wall -Wextra -g -I$(INC_DIR)
LDFLAGS :=

APPS := client server
APP_BINS := $(addprefix $(BUILD_DIR)/,$(APPS))

KMOD_NAME := simipc
KMOD := $(BUILD_DIR)/$(KMOD_NAME).ko

.PHONY: all apps kernel clean

all: $(APP_BINS) $(KMOD)

apps: $(APP_BINS)

kernel: $(KMOD)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: $(APP_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(KMOD): | $(BUILD_DIR)
	$(MAKE) -C $(KDIR) M=$(PWD)/$(KERN_DIR) modules
	cp $(KERN_DIR)/$(KMOD_NAME).ko $(KMOD)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/$(KERN_DIR) clean || true
	rm -rf $(BUILD_DIR)