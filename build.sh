#!/usr/bin/env bash
# Build source-only RootsMagic compatibility DLLs; never modify installed Wine.
set -euo pipefail
base=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ $# != 2 ]]; then
    echo "Usage: $0 /path/to/wine-11.17.tar.xz /new/build-directory" >&2
    exit 2
fi
archive=$(realpath -- "$1")
work=$(realpath -m -- "$2")
[[ -f "$archive" && ! -e "$work" ]] || { echo 'Source must exist; build directory must be new' >&2; exit 1; }
expected=$(awk -v file="$(basename -- "$archive")" '$2 == file {print $1}' "$base/manifests/source-sha256.txt")
[[ -n "$expected" ]] || { echo 'Unknown Wine source archive name' >&2; exit 1; }
actual=$(sha256sum -- "$archive")
[[ ${actual%% *} == "$expected" ]] || { echo 'Wine 11.17 source checksum mismatch' >&2; exit 1; }
for command in gcc make bison flex patch i686-w64-mingw32-gcc x86_64-w64-mingw32-gcc; do
    command -v "$command" >/dev/null || { echo "Missing build dependency: $command" >&2; exit 1; }
done
mkdir -p "$work/source" "$work/build" "$work/overrides/i386-windows" "$work/overrides/x86_64-windows"
tar -xJf "$archive" -C "$work/source" --strip-components=1
while IFS= read -r patchfile; do
    patch --batch --fuzz=0 -d "$work/source" -p1 < "$base/patches/exact/$patchfile"
done < "$base/patches/exact/series"
(
    cd "$work/build"
    ../source/configure --enable-archs=i386,x86_64 --without-x --without-wayland --without-freetype
    make -j"${JOBS:-4}" \
        dlls/d2d1/i386-windows/d2d1.dll \
        dlls/dwrite/i386-windows/dwrite.dll \
        dlls/dxgi/i386-windows/dxgi.dll \
        dlls/winhttp/i386-windows/winhttp.dll \
        dlls/winhttp/x86_64-windows/winhttp.dll
)
for name in d2d1 dwrite dxgi winhttp; do
    cp "$work/build/dlls/$name/i386-windows/$name.dll" "$work/overrides/i386-windows/"
done
cp "$work/build/dlls/winhttp/x86_64-windows/winhttp.dll" "$work/overrides/x86_64-windows/"
(cd "$work/overrides" && sha256sum i386-windows/*.dll x86_64-windows/*.dll > SHA256SUMS)
printf 'Built five Wine PE DLLs in %s/overrides\n' "$work"
echo 'Do not install them without testing a private Wine runtime and a disposable prefix.'
