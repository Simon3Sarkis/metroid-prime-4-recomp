# Metroid Prime 4 PC Recomp

Research project: static recompilation of *Metroid Prime 4: Beyond* (Switch 1 release, ARM64) into a native Windows
program, by the method of [totk-pc-recomp](https://github.com/valentderah/totk-pc-recomp) as carried to a second game
by [fe3h-pc-recomp](https://github.com/jaigner-hub/fe3h-pc-recomp). The game's code is lifted to C, compiled for
x86-64 and linked with a host runtime that stands in for the Switch OS and for NVN, its graphics API, on Vulkan.

This is the third game on that code. Besides a port, the project answers how much of a Switch recompilation is shared
code that a fourth game gets for free: the lifter, runtime and renderer came over from fe3h-pc-recomp with the game
name renamed and a handful of game-bound tables rewritten ([docs/plan.md](docs/plan.md), "Shared core").

## Status

Phase 1: bringing the game up to its first frames. Phase 0, the survey of the binaries, is recorded in
[docs/plan.md](docs/plan.md). Version 1.1.0, the final Switch 1 update, is the target.

This repository contains no game content. You need your own dump of the game (XCI or NSP), its 1.1.0 update NSP, and
the keys dumped from your own console. The content check (`tools/check_content.py`, run by the pre-commit hook and
CI) keeps it that way.

## What you need

Windows 10 or later, a Vulkan 1.3 GPU, and the toolchain totk-pc-recomp's `docs/building.md` describes: MSYS2
UCRT64 (clang, lld, ninja, SDL3, opus, libvpx, Vulkan, vulkan-memory-allocator), the .NET 10 SDK and Visual Studio
Build Tools for the shader translator, and Python 3.12. The lifter and its tests also run under WSL with the
`.venv` of `tools/requirements.txt`.

## Building

1. Copy `config.example.toml` to `config.local.toml` and point it at your game, update and keys.
2. `python tools/build_hactool.py`, then `python tools/extract_exefs.py <game> --keys <keys> --output build/extracted/v0/exefs`
   for the base and, with `--update`, `--output build/extracted/v131072/exefs` for the update;
   `python tools/extract_romfs.py` merges the update's RomFS (needs about three times the RomFS size, 27 GB, free).
3. `python recomp/gen.py --exefs build/extracted/v131072/exefs --out generated/v131072 --modules main,subsdk0,sdk`
4. `python tools/build_xlate.py`, then `python tools/build_game.py` (Windows Python; MSYS2 clang does the compiling).
5. Run `build\v131072\mp4.exe`, in the foreground: the game pauses when its window loses focus. `--boot` runs to the
   first NVN call and the `--probe-*` modes check the host without a window (`runtime/main.c`).

Settings: `mp4.ini.example`.

## Third-party code

Everything taken from elsewhere is listed with its source, commit and license in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). fe3h-pc-recomp is MIT, Copyright (c) 2026 Jeff Aigner;
totk-pc-recomp is MIT, Copyright (c) 2026 Valiantsin Dzerakh; their own upstreams (LAS Recomp, MacNativeGames,
d2r-mobile, nxnative, Ryujinx's shader translator) keep their notices.
