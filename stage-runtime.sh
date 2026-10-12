#!/usr/bin/env bash
# Stage the built PE DLLs in a copy of a Wine runtime. Never patch a donor in place.
set -euo pipefail
if [[ $# != 3 && $# != 4 ]]; then
    echo "Usage: $0 /existing/wine-runtime /build/overrides /new/private-runtime [x64-v6-comctl32.dll]" >&2
    exit 2
fi
donor=$(realpath -- "$1")
overrides=$(realpath -- "$2")
dest=$(realpath -m -- "$3")
[[ -x "$donor/bin/wine" && -d "$donor/lib/wine/i386-windows" && -d "$donor/lib/wine/x86_64-windows" ]] || { echo 'Donor must contain bin/wine and both PE architectures' >&2; exit 1; }
[[ -f "$overrides/SHA256SUMS" ]] || { echo 'Build output missing SHA256SUMS' >&2; exit 1; }
(cd "$overrides" && sha256sum -c SHA256SUMS)
[[ ! -e "$dest" && "$dest" != "$donor"/* && "$dest" != "$overrides"/* ]] || { echo 'Destination must be a new directory outside the inputs' >&2; exit 1; }
if [[ $# == 4 ]]; then
    comctl=$(realpath -- "$4")
    [[ -f "$comctl" && $(file -b "$comctl") == *PE32+* ]] || { echo 'Need an x64 version-6 comctl32.dll' >&2; exit 1; }
fi
mkdir -p "$(dirname -- "$dest")"
cp -a --reflink=auto -- "$donor" "$dest"
for name in d2d1 dwrite dxgi winhttp; do
    arch=i386-windows
    cp -a "$dest/lib/wine/$arch/$name.dll" "$dest/lib/wine/$arch/$name.dll.original"
    install -m 0644 "$overrides/$arch/$name.dll" "$dest/lib/wine/$arch/$name.dll"
done
arch=x86_64-windows
cp -a "$dest/lib/wine/$arch/winhttp.dll" "$dest/lib/wine/$arch/winhttp.dll.original"
install -m 0644 "$overrides/$arch/winhttp.dll" "$dest/lib/wine/$arch/winhttp.dll"
if [[ $# == 4 ]]; then
    cp -a "$dest/lib/wine/$arch/comctl32.dll" "$dest/lib/wine/$arch/comctl32.dll.original"
    install -m 0644 "$comctl" "$dest/lib/wine/$arch/comctl32.dll"
else
    echo 'RM12 Browse may need the separate x64 version-6 comctl32 DLL; not supplied.' >&2
fi
printf 'Private runtime staged: %s\n' "$dest"
"$dest/bin/wine" --version
