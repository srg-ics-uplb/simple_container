CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
TARGET = simple_container
SOURCE = simple_container.c
SHELL = /bin/bash

# Default target
all: $(TARGET)

# Build the container program
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Install to /usr/local/bin (requires root)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)

# Uninstall from /usr/local/bin
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Create a simple rootfs for testing
create-rootfs:
	@echo "Creating a minimal rootfs in ./rootfs..."
	mkdir -p rootfs/{bin,sbin,etc,proc,sys,tmp,usr/bin,usr/sbin,dev}
#	mkdir -p rootfs/{bin,sbin,etc,proc,sys,tmp,dev}
	@echo "Copying essential binaries..."
	@if [ -f /bin/bash ]; then cp /bin/bash rootfs/bin/; fi
	@if [ -f /bin/sh ]; then cp /bin/sh rootfs/bin/; fi
	@if [ -f /bin/ls ]; then cp /bin/ls rootfs/bin/; fi
	@if [ -f /bin/ps ]; then cp /bin/ps rootfs/bin/; fi
	@if [ -f /bin/cat ]; then cp /bin/cat rootfs/bin/; fi
	@if [ -f /bin/echo ]; then cp /bin/echo rootfs/bin/; fi
	@if [ -f /usr/bin/hostname ]; then cp /usr/bin/hostname rootfs/usr/bin/; fi
	@if [ -f /usr/bin/whoami ]; then cp /usr/bin/whoami rootfs/usr/bin/; fi
	@echo "Copying library dependencies..."
	@./copy_libs.sh
	@echo "Simple rootfs created in ./rootfs/"

# Remove the test rootfs
clean-rootfs:
	rm -rf rootfs

.PHONY: all clean install uninstall create-rootfs clean-rootfs
