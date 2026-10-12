# RootsMagic 11/12 Wine compatibility patches (source only)

This repository contains Wine source patches, synthetic regression probes, and
scripts for building and staging **private** Wine runtimes. It does not include
RootsMagic software, Wine binaries, fonts, databases, credentials, or a
ready-to-use patched DLL download. RootsMagic is proprietary; obtain your own
installer and license. This is an independent, unsupported compatibility
experiment, not an official RootsMagic or Wine project.

## Easiest way to follow this guide with a CLI AI agent

If you use an AI coding agent in a terminal, give it this repository and your
own RootsMagic installer files. It can inspect your machine, build the patches,
and guide the installation rather than having you manually translate every Wine
path and command. For example, paste this request into your agent:

> Help me install my licensed RootsMagic 11 and/or 12 on Linux using Wine and
> https://github.com/rodhuffaker/rootsmagic-wine-patches. Read the entire README
> first. Inspect my current Wine setup and the installers I provide; identify
> each application's 32- or 64-bit architecture. Do not overwrite my system
> Wine, existing prefixes, launchers, or genealogy databases. Download the
> official Wine 11.17 source archive and verify its checksum, run `build.sh`,
> stage the resulting DLLs in a new private runtime with `stage-runtime.sh`,
> and run `test-probes.py` in an isolated prefix. For RM12 Browse, use only a
> verified x64 version-6 `comctl32.dll` from my own Wine prefix if available;
> it is not supplied by this repository. Install my own RootsMagic installer
> into the test prefix, set 192 DPI if I use 200% scaling, use Vulkan for RM11
> and test OpenGL for RM12, then check the actual UI and file picker with a
> disposable tree. Once that version passes, create a separate launcher script and
> Desktop `.desktop` icon for it, pointing to the tested private Wine runner,
> prefix, executable and per-version renderer (Vulkan for RM11, OpenGL for RM12).
> Do not replace an existing icon without asking. Validate the desktop entry,
> launch it from the icon, and verify it still opens the app and RM12's Browse
> picker—not just from a terminal. Let me handle registration and FamilySearch
> sign-in myself. Ask before changing my daily-use setup or accessing a real
> database. Report what passed and what remains untested; do not claim the
> patches were rebuilt against Wine 11.19 source or that all features work
> everywhere.

An agent is a helper, not a one-command installer: review its proposed changes
and make it verify each result. Never paste license keys, passwords, genealogy
files, or raw authentication logs into a chat or public issue. The repository
contains source patches and scripts, **not** the RootsMagic installer, the
version-6 common-controls DLL, or an automatically validated setup for your
machine.

## What was tested

* Linux Mint 22.3 (x86_64); RootsMagic 11 (32-bit) and RootsMagic 12 (64-bit).
* Daily use: a Wine 11.0 runtime with i386 Direct2D, DirectWrite, DXGI, and
  WinHTTP fixes for RM11; an RM12-only runtime with x64 WinHTTP and version-6
  common controls, plus OpenGL for RM12 Person Tools. Windows DPI: 192.
* A separate Wine 11.19 development-runtime trial reused the replacement DLLs
  built from Wine 11.17 source. RM12 opened a database; RM11 launched and drew
  its main window. The Direct2D and 32/64-bit WinHTTP probes passed. This **does
  not** validate compiling the patches against 11.19 source, or every app
  workflow on 11.19. RM11 database opening was not tested there because the
  available databases had been upgraded to RM12 format.
* On the daily setup, RM12 Browse opened and FamilySearch Person Tools rendered
  with OpenGL. Those interactive features were **not** rechecked on 11.19.

Do not present a passing synthetic probe as a substitute for testing the
application itself. Do not open one real database from two Wine processes.

## Patches

The five numbered patches in `patches/exact/series` apply to **Wine 11.17
source**. They modify i386 `d2d1.dll`, `dwrite.dll`, `dxgi.dll`, `winhttp.dll`,
and x64 `winhttp.dll`. The Direct2D/DXGI changes include application-specific
compatibility behavior; they are not upstream-ready as a single patch.

| Component | Why it matters here |
| --- | --- |
| Direct2D (`d2d1`) | Shared bitmap drawing, transient resource churn, disjoint layer masks, collinear connector joins |
| DirectWrite (`dwrite`) | `IDWriteTextLayout::HitTestTextRange` implementation and count initialization |
| DXGI (`dxgi`) | Known-good FireMonkey frame/presentation behavior; experimental GDI overlay remains off |
| WinHTTP (`winhttp`) | Gzip decoding and connection reuse after chunked compressed responses |

RM12's **Browse** fix is separate: an x64 Wine version-6 `comctl32.dll` from the
installed prefix was used in an RM12-only private runtime. It is **not** built
by these patches or supplied here. Copying it beside RootsMagic and setting a
native DLL override did not solve the observed class-registration failure.
Do not substitute an arbitrary Microsoft DLL or copy anything out of another
person's Windows installation.

## Build from official Wine source

Tested with the official WineHQ `wine-11.17.tar.xz` source archive, SHA-256
`8e752e29bba2901ad9f915139345c6ee67184b099d425704d915971ca5c055fc`.
The source hash is in `manifests/source-sha256.txt` and `build.sh` checks it.

Install the Wine build prerequisites for your distribution (on Mint/Ubuntu,
this includes `build-essential`, `mingw-w64`, `bison`, `flex`, and `patch`;
additional Wine configure dependencies vary). Do **not** run these scripts as
root. Run from a checkout:

```sh
curl -fL -o wine-11.17.tar.xz \
  https://dl.winehq.org/wine/source/11.x/wine-11.17.tar.xz
bash ./build.sh ./wine-11.17.tar.xz "$HOME/rootsmagic-wine-build"
```

The build applies the exact patch series with no fuzz, configures a PE-only
Wine build, and writes five PE DLLs plus checksums under
`$HOME/rootsmagic-wine-build/overrides/`. It does not deploy them. The build
requires a *new*, otherwise nonexistent output directory each time. Source
archives and DLLs are ignored by Git. The patches were developed against
Wine 11.17 and are not promised to apply unchanged to later Wine source.

## Stage in an isolated runtime

Obtain a matching x86_64 Wine runtime with both 32-bit and 64-bit support.
`stage-runtime.sh` copies it to a **new** destination and replaces only the
five listed Wine PE DLLs there, retaining `.original` backups in that copy.
It refuses to overwrite a pre-existing destination. Never point it at your
system Wine installation as the destination.

```sh
bash ./stage-runtime.sh /path/to/unmodified/wine-runtime \
  "$HOME/rootsmagic-wine-build/overrides" \
  "$HOME/.local/share/rootsmagic-wine-private"
```

The optional fourth argument is a tested x64 version-6 `comctl32.dll` from
*your own* Wine prefix's `drive_c/windows/winsxs/amd64_microsoft.windows.common-controls_*`
assembly. Supply it only after confirming its architecture and provenance;
without that DLL the script does not claim to fix RM12 Browse. For the known
RM12 Browse fix, use a separate RM12 private runtime and that fourth argument.
Back up your old launcher/runtime before switching. Mixing DLLs built from
11.17 source with a different Wine runtime (including 11.19) is an
**experiment**, not a supported ABI guarantee.

Create a new test prefix with the staged runner and set 192 DPI, then run
synthetic probes. Replace paths below with your own **isolated** locations:

```sh
RUNTIME="$HOME/.local/share/rootsmagic-wine-private"
TESTPREFIX="$HOME/.local/share/rootsmagic-wine-test-prefix"
WINEPREFIX="$TESTPREFIX" "$RUNTIME/bin/wineboot" -u
WINEPREFIX="$TESTPREFIX" "$RUNTIME/bin/wine" reg add \
  'HKCU\Control Panel\Desktop' /v LogPixels /t REG_DWORD /d 192 /f
python3 ./test-probes.py "$RUNTIME/bin/wine" "$TESTPREFIX"
```

The probes use synthetic drawing and localhost HTTP, not accounts or genealogy
data. Once those pass, install your own RM11/RM12 installers into a test
prefix; try a disposable tree, Browse, and FamilySearch in the actual apps.
Keep RM11 on Vulkan and use `WINE_D3D_CONFIG=renderer=gl` for RM12 on the tested
machine. Your own renderer result may differ. Preserve the working Wine setup
until these checks pass in the launcher you intend to use.

## Scope, licensing, and privacy

The patches modify Wine source, which is LGPL-2.1-or-later; see `LICENSE` and
the Wine source headers. These changes and the scripts were developed with AI
assistance and have not been merged upstream. Review them before use. No
registration key, FamilySearch token, private genealogy data, diagnostic trace,
RootsMagic installer, proprietary font, or Wine binary should be committed.
Open an issue with a sanitized reproducer and exact Wine/app versions, not raw
network logs or a real tree.
