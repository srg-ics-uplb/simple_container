#!/bin/sh
# mkrootfs.sh - build a minimal root filesystem by copying host binaries
# and the shared libraries they need. Usage: ./mkrootfs.sh ./rootfs
set -e
ROOT=${1:-./rootfs}
BINS="sh ls cat ps hostname mount id echo sleep mkdir head grep"
mkdir -p "$ROOT/bin" "$ROOT/proc" "$ROOT/tmp" "$ROOT/etc" "$ROOT/sys"
for b in $BINS; do
    p=$(which "$b")
    cp -L "$p" "$ROOT/bin/"
    # copy every library ldd reports, preserving its path
    for lib in $(ldd "$p" | grep -o '/[^ ]*'); do
        mkdir -p "$ROOT$(dirname "$lib")"
        cp -Ln "$lib" "$ROOT$lib" 2>/dev/null || true
    done
done
echo "root:x:0:0:root:/:/bin/sh" > "$ROOT/etc/passwd"
echo "rootfs ready in $ROOT ($(du -sh "$ROOT" | cut -f1))"

