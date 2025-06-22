#!/bin/bash

# Script to copy shared libraries needed by binaries in rootfs

ROOTFS_DIR="./rootfs"
LIB_DIR="$ROOTFS_DIR/lib"
LIB64_DIR="$ROOTFS_DIR/lib64"

# Create lib directories
mkdir -p "$LIB_DIR" "$LIB64_DIR"

# Function to copy library and its dependencies
copy_lib_deps() {
    local binary="$1"
    
    if [ ! -f "$binary" ]; then
        return
    fi
    
    echo "Processing $binary..."
    
    # Get library dependencies
    ldd "$binary" 2>/dev/null | while read line; do
        # Extract library path
        lib_path=$(echo "$line" | grep -o '/[^ ]*' | head -1)
        
        if [ -n "$lib_path" ] && [ -f "$lib_path" ]; then
            # Determine destination directory based on library path
            if [[ "$lib_path" == *"/lib64/"* ]]; then
                dest_dir="$LIB64_DIR"
            else
                dest_dir="$LIB_DIR"
            fi
            
            # Create subdirectories if needed
            lib_subdir=$(dirname "$lib_path" | sed 's|/lib64|/lib|g' | sed 's|/lib||g')
            if [ -n "$lib_subdir" ] && [ "$lib_subdir" != "/" ]; then
                mkdir -p "$dest_dir$lib_subdir"
                cp -L "$lib_path" "$dest_dir$lib_subdir/" 2>/dev/null
            else
                cp -L "$lib_path" "$dest_dir/" 2>/dev/null
            fi
        fi
    done
}

# Copy libraries for all binaries in rootfs
for binary in "$ROOTFS_DIR/bin"/* "$ROOTFS_DIR/usr/bin"/* "$ROOTFS_DIR/sbin"/* "$ROOTFS_DIR/usr/sbin"/*; do
    copy_lib_deps "$binary"
done

# Copy dynamic linker
if [ -f /lib64/ld-linux-x86-64.so.2 ]; then
    cp /lib64/ld-linux-x86-64.so.2 "$LIB64_DIR/"
fi

if [ -f /lib/ld-linux.so.2 ]; then
    cp /lib/ld-linux.so.2 "$LIB_DIR/"
fi

echo "Library copying completed."