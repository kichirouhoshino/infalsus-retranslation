#!/usr/bin/env bash
set -e

echo "=== Building In Falsus Proxy DLL (version.dll) ==="

SRC="src/proxy_version.c"
OUT="version.dll"

# Wine import libraries and headers
WINE_INC_WINDOWS="/usr/include/wine/windows"
WINE_INC_MSVCRT="/usr/include/wine/msvcrt"
WINE_LIB="/usr/lib/wine/x86_64-windows"

clang -target x86_64-w64-windows-gnu \
    -fuse-ld=lld \
    -shared \
    -O2 \
    -Wall -Wextra -Wno-unused-parameter -Wno-inconsistent-dllimport \
    -nostdlib \
    -Wl,-e,DllMain \
    version.def \
    -DMINIZ_NO_STDIO \
    -DMINIZ_NO_TIME \
    -DMINIZ_NO_ARCHIVE_APIS \
    -DMINIZ_NO_DEFLATE_APIS \
    -Isrc \
    -I"$WINE_INC_MSVCRT" \
    -I"$WINE_INC_WINDOWS" \
    -L"$WINE_LIB" \
    -lkernel32 \
    -lmsvcrt \
    "$SRC" \
    src/miniz_tinfl.c \
    -o "$OUT"

echo "Built $OUT successfully!"
file "$OUT"
ls -lh "$OUT"
