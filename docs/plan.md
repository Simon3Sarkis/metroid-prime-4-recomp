# Plan

## Phase 0: survey

Answer these from the user's dump before writing any port code. Record answers here (facts about the game only, no game data).

| Question | Answer |
|---|---|
| Release in scope, final update version and title ids | Switch 1 release. Base XCI: title id `010019A01E2F2000`, version 0, display version 1.0.0. Update NSP: Meta title id `010019A01E2F2800`, version 131072 (`v131072`, the second update), display version 1.1.0; its Program NCA is a BKTR patch over the Switch 1 base (same title id, NVN and SDK 19.3 inside), so it is the Switch 1 code even though its `control.nacp` names the application with the "Nintendo Switch 2 Edition" suffix. Both packages verified by `tools/extract_exefs.py` (FS header, hash table and every ExeFS block; signatures not checked). Extracted to `build/extracted/v0/exefs` and `build/extracted/v131072/exefs`. Target: 1.1.0 (`v131072`) |
| ExeFS modules and their sizes | `rtld`, `main`, `subsdk0`, `sdk`: one subsdk fewer than TotK, two fewer than FE3H. NSO file sizes in 1.1.0 (bytes): `main` 15,426,217; `sdk` 6,248,767; `subsdk0` 3,429,165; `rtld` 8,550. Loaded segment sizes (text / ro / data): `main` 20,508,192 / 4,067,328 / 751,600; `sdk` 6,612,672 / 5,160,960 / 345,312; `subsdk0` 3,509,104 / 1,982,464 / 199,560. The update changes every module (`sdk` and `subsdk0` keep their sizes: an SDK patch-level rebuild, 19.3.13 to 19.3.15). `main` is `GameFramework.nss` (Retro's build name, module path under `RS15`), `subsdk0` is `multimedia` (`nn::movie`), `sdk` is `nnSdk`. `main` needs `multimedia.nss` and `nnSdk.nss`. `main` exports `malloc`, `free`, `calloc`, `realloc`, `aligned_alloc` and the C++ `new`/`delete` set, so the game supplies the allocator, as TotK does. `main.npdm`: 3 cores (0 to 2), main thread priority 44, 1 MiB main stack, handle table 512, `Allow Debug` set, retail ACID |
| SDK version string in `main` | `main` carries no SDK MW tag. `sdk` is `NintendoSdk_nnSdk-19_3_13-Release` in 1.0.0 and `19_3_15` in 1.1.0, with the `NintendoSDK_NVN` and `NintendoSDK_libz` tags of the same version; `subsdk0` is `NintendoSDK_movie` of the same version. So SDK 19.3, as the listings said. The `sdk` module also holds `nn::nifm`, `nn::socket` and `nvrm_gpu` code, as every SDK build does; what matters is what `main` imports (below) |
| Graphics API: NVN, Vulkan or OpenGL imports | NVN, exactly as TotK and FE3H reach it: `main` imports `nvnBootstrapLoader`, `nv::InitializeGraphics`, `nv::FinalizeGraphics`, `nv::SetGraphicsAllocator` and `nv::SetGraphicsDevtoolsAllocator`, and takes every other NVN entry through the bootstrap loader (`nvnDevice*` builder and `nvnDeviceSetIntermediateShaderCache` names appear in `main`'s strings). No `glslc*` import in `main` (unlike FE3H): the runtime GLSL compiler is not used, only `sdk`'s own glslc allocator hooks exist. `subsdk0` imports the `NvDdkVic*`, `NvBlit*` and `NvOs*` symbols for hardware video decoding, as FE3H's `multimedia` module did |
| How the shaders ship (precompiled Maxwell code, GLSL source for glslc, or both) | Precompiled Maxwell code. The base RomFS root holds `MaterialArchive.arc` (30 MiB, `RFRM` form of type `MTRL`), `main` names the file. It is 77 KB of plain header followed by 10,518 zstd frames. The first 3,558 decoded frames are `DXBC` blobs (chunks `RDEF`, `ISGN`, `OSGN`, `SHEX`, `STAT`; built by "Microsoft (R) HLSL Shader Compiler 6.3.9"), that is Direct3D 11 bytecode with reflection, one per shader. They are followed by 1,734 shader pairs for the GPU: a block headed by a Maxwell shader program header word and a 256-byte-aligned block headed by `0x12345678` holding the SASS (3,500 Maxwell `EXIT` encodings counted across them). Pass names in the plain header (post-processing, shadows, clustered culling, indirect draw, stream-out) describe Retro's renderer. No GLSL source, SPIR-V or `DKSH` anywhere in the archive. So TotK's shader path applies: Maxwell to SPIR-V through the translator in `tools/xlate/`; the DXBC copies are reflection or a leftover of the Windows toolchain, to be confirmed in Phase 1 from how `main` reads the archive. The exact frame packaging (how the `0x12345678` block and the header block are paired, and what the 3,480 frames that `zstd -d` refuses are) is Phase 1 work too |
| Middleware (audio, video, compression, physics) | Retro Studios' own engine (`RetroBuildInfoStamp`, `RetroFMV` strings), no Havok, PhysX, Wwise, FMOD, CRI or Bink strings. Compression: zstd linked statically into `main` (the two `ZSTD_trace_*` imports are its weak trace hooks) and used for the shader archive; `LZ4` and `libz` strings also present. Audio: `nn::audio` audio renderer, 175 imported functions, including effect types newer than FE3H's SDK (compressor, light limiter, I3DL2 reverb, splitter, aux send/return, circular buffer sink) and `ParseAdpcmHeader`; no `nn::codec` (no Opus in `main`, although `hwopus` is in the service list). Video: `nn::movie` through `subsdk0` (`movie::Decoder`, `Extractor`, `MediaData`, `Buffer::SetNvnDev`), with the FMV container handled by `RetroFMV` code in `main`; codec not yet known (no movie files in the RomFS tree, they are inside the paks) |
| `nn::*` functions imported, and how many the TotK and FE3H hosts already implement | `main` imports 449 symbols: 64 are libc, libm, C++ runtime and the NVN entries above, 434 are `nn::*` and `movie::*` (`subsdk0`'s own 665 imports are `nn::*` and NVIDIA `NvOs*`/`NvDdkVic*`). By namespace: `nn::audio` 175, `nn::os` 71, `nn::hid` 42, `nn::fs` 23, `movie` 20, `nn::oe` 13, `nn::nfp` 12, `nn::account` 8, `nn::vi` 7, `nn::mem` 6, `nn::time` 4, `nn::ec` 2, `nn::err` 2, `nn::settings` 2, `nn::pl`, `nn::ro`, `nn::util`, `nn::result` 1 each. By symbol, the TotK host replaces 75 and the FE3H host 83 (`tools/survey_imports.py`); the rest run as lifted `sdk` code over SVCs and IPC, as in both ports. Of note for the host: `nn::hid` vibration player, node and mixer (HD rumble patterns), six-axis sensors, touch screen and `ShowControllerSupport`; `nn::oe` display resolution and its change event, performance mode and configuration, CPU boost; `nn::pl::IsRunningOnOunce` (Switch 2 detection, must say no); `nn::ro::ProtectRelro`; `nn::fs` save data (`EnsureSaveData`, `MountSaveData`, `GetSaveDataSize`, `Commit`), `MountRom` plus `QueryMountRomCacheSize`, and `SetAllocator`; `nn::ec::ShowShopApplicationInformationForUpgrade` and `nn::err::ShowUnacceptableApplicationVersionError` (upgrade and version applets). `sdk` is the only module that issues SVCs: 58 distinct numbers, of which the FE3H host does not handle 0x13, 0x14 (shared memory), 0x23, 0x28, 0x2A, 0x2F, 0x32, 0x33, 0x40 to 0x45, 0x4B, 0x50 to 0x52, 0x70 and 0x72; which of those the game reaches is a Phase 1 question |
| Lifter run over `main`: unknown instructions, unresolved jump tables | FE3H `recomp/gen.py` @ `47ef991` (TotK's, unchanged) over 1.1.0, nothing adapted: `main` 99,388 functions, 5,127,048 instructions, 344 MB of C in 19 s; `subsdk0` 14,152 functions, 61 MB; `sdk` 40,076 functions, 115 MB. Unsupported instructions: 0 in every module. Trap sites (`rt_udf`): 24,509 / 7,744 / 26,116, all on zero padding words, `udf #1` words or two undecodable data words per module, none on real code. Jump tables resolved: 613 / 334 / 574; all-label fallbacks: 12 / 0 / 30. `tests/jumptables_check.py` against the Unicorn oracle: `main` 500 agree, 2 disagree (the resolver lists more targets than the oracle at both), 1 table only the oracle finds, 9 computed sites unresolved; `subsdk0` 310 agree, 0 disagree; `sdk` 469 agree, 0 disagree, 2 only the oracle (1 of them an oracle error), 27 computed sites unresolved. Every hoisted table base verified, no PLT stub miscounted. The check exits 1 for the disagreements and only-oracle tables, the same class of leftover FE3H carried into its Phase 1 |
| RomFS layout (archive format, compression) | Base RomFS: 27,242,247,956 bytes, 443 files, no per-file compression at the RomFS level. Everything is Retro's `RFRM` form container: 441 `.pak` files (`PACK` form with `TOCC`, `META` and `STRG` chunks, the Metroid Prime Remastered layout), one `.arc` and one `.bin`. Root: `Kernel.pak`, `MiscData.pak`, `StartupUI.pak`, `MaterialArchive.arc`, `AreaPakManagerCacheNX.bin`. `Preload/` holds two paks, 3.7 GB, `UI/` two, `Worlds/` 434 (22 GB): one pak per room, per zone, per zone transition and per vista. Compression inside the paks: zstd frames and some LZ4 blocks seen in the sampled paks. The update's RomFS (BKTR patch) is not yet read: `tools/extract_romfs.py` with hactool does that in Phase 1 |
| Frame-rate assumptions in game logic (the game runs at 60 fps on Switch 1) | Not decidable from strings: `main`'s rodata holds both 1/60 and 1/30 as float constants (2 each), 60.0f eleven times and 30.0f six times. `nn::oe::GetPerformanceMode` and `SetPerformanceConfiguration` are imported (docked and handheld profiles). Phase 1 question, to answer from the frame loop once it runs |
| Online and amiibo services linked | amiibo: yes, `nn::nfp` (12 functions: initialize, list devices, mount, start/stop detection, model info and the three attach-event calls). Accounts: `nn::account` (open preselected user, user selector, list users). Online: none in `main`'s imports (no `nn::nifm`, `nn::socket`, `nn::friends`, `nn::bcat`, `nn::ldn`, `nn::prepo`, `nn::ssl`), although the NPDM service list grants the usual set (`nifm:u`, `bsd:u`, `ssl`, `friend:u`, `bcat:u`, `ldn:u`, `prepo:u` and others; 50 services in all). So no network stubbing beyond what the SDK's own initialization asks for; `nn::ec` shop applet and the version-error applet are the only store-facing calls |

## Phase 1: boot to the title screen

Lift all modules, link them on the host, bring up the services the game calls, and draw the first frames.

What FE3H hit between the first black frames and its title screen is in fe3h-pc-recomp `docs/plan.md`, Phase 1.
Expect the SDK-version class of problem again, in the other direction: FE3H's SDK was older than TotK's and this
game's is newer.

**Started 2026-11-02.** The lifter, runtime, tests and build tools came over from fe3h-pc-recomp @ `47ef991` in one
copy (see "Shared core"); the translator built as `build/xlate/mp4_xlate.exe`. Game-bound tables rewritten for this
game before the first build: the boot driver (`runtime/boot.c`: three modules, their init_array sizes, the 1 MiB main
stack, the first `nvnBootstrapLoader` return address per version, and SDK 19.3's three-callback `nn::init::Start`,
which is TotK's form again after FE3H's two-callback 6.5.0), the main stack in `runtime/kernel.c`, the version names in
`tools/build_game.py`, the module bases in `tests/diff/lift.py`, and the window title. `runtime/ov_mp4.c` starts
empty: no entry of FE3H's `ov_fe3h.c` applies, as `main` imports no network namespace.

**First build (2026-11-02):** `mp4.exe` compiles in 90 s and links; `--selftest` passes (153,616 functions). The
differential tests pass on this game's code: `tests/diff/lift.py` 24,000 encodings, 71,967 cases, 0 failures;
`tests/diff/func.py` 410 whole functions, 1,859 cases, 0 failures (run with the Windows venv, as the Linux side has no
clang). Host probes: `--probe-loader` passes with this game's measured tables; `--probe-kernel`, `--probe-gfx-abi`,
`--probe-astc`, `--probe-dpi` pass; `--probe-ipc` reaches the SDK client's render loop with the SDK 19.3 heap
constructor (`sdk+0x217770`) and a fall-back for `GetAudioRendererElapsedFrameCount`, which this game does not
import; its one failure is the frame period (9.8 ms against 5 ms), which the FE3H build reproduces on this machine,
whose USB audio device opens with a 480-sample period. `--probe-services` lists the overrides this game does not
import (`nn::oe` focus and volume calls, `nn::prepo`, `nn::codec` hardware Opus, `nn::swkbd`, mouse and keyboard)
and those it links but the probe does not exercise; the host's own checks there pass. `tests/excl_mt.py`'s ABA cases
fail with this machine's clang 23.1.4 on the FE3H tree too, so that is not this port's doing. A pitfall of the rename
worth recording: the settings code assumed the five-character `FE3H_` prefix in four places (`cfg.c`, `input_map.c`,
`ui/binds_page.cpp`, `ui/menu.cpp`); the length now follows the prefix string. The TotK-era `RT_V143` switch is pinned
on, as SDK 19.3 belongs to the 20.5 generation.

`--boot` runs through `nn::init::Start`, `nnosInitialize`, the rtld callback and `nninitInitializeSdkModule`, and stops
where the host wants its RomFS, whose merge (`tools/extract_romfs.py`: the 27 GB base NCA copied out of the XCI, then
hactool's BKTR merge with verification) takes most of an hour on this machine.

**Reached (2026-11-02, later):** `--boot` passes: the first `nvnBootstrapLoader` call comes from `main+0xa20f54`,
the address Phase 0 found, after 170 SVCs, 12 threads and 256 host calls, 3.4 s after the process starts. The RomFS
merge took 24 min (28.2 GB, 445 files, two more than the base). What stood between the SDK init and that call, in
the order found, each a host answer to a call the TotK and FE3H games never made:

- `SetHeapSize`: the game's start-up binary-searches the largest heap the kernel grants between 0 and 8 GiB in 2 MiB
  steps, so it expects a clean failure for a size too large. The SDK's wrapper (sdk+0x1455f0) maps OutOfResource,
  OutOfMemory and LimitReached to its own ResultOutOfMemory and aborts on any other result; the host answered
  InvalidCurrentMemory. It now answers OutOfMemory (`memory.c`). The search settles on the application pool
  (0xCD500000 less the images), the hardware's answer too.
- `nn::oe::GetPerformanceConfiguration`, lifted, read the apm session the overridden `Initialize` never opened: it now
  returns the configuration the game last requested per mode (`ov_system.c`, `ov_mp4.c`), and
  `GetDefaultDisplayResolution` and its change event are answered (1080p docked, 720p handheld; an event that never
  fires).
- `nn::fs::MountRom` and `QueryMountRomCacheSize` take an extra `int` in SDK 19.3; the same handlers serve both forms
  (`romfs.c`). `GetSaveDataSize` answers the control data's 10 MiB save and 4 MiB journal (`fs_host.c`).
- `nn::account`: the game lists the open users (`ListOpenUsers`), opens one by Uid and can show the user selector; all
  answer with the host's one local user (`ov_system.c`).
- `nn::hid`: `SetGyroscopeZeroDriftMode` opened the hid service; it, `IsSixAxisSensorAtRest`, `InitializeTouchScreen`
  and `GetTouchScreenStates<16>` are answered at the SDK's detail layer, where ov_input.c replaces the others
  (`ov_mp4.c`). The SDK's vibration player, node and mixer classes run lifted and reach the host's `SendVibrationValue`.

**Into the world (2026-11-02, night):** the game walks its RomFS, opens every area pak and the update's patch pak,
loads its save, presents frames at 58 fps and plays its intro movie. What stood in the way, in order:

- Paths. The game spells its paths with backslashes and "." components (`rom:\StartupUI.pak`, `rom:\./Worlds`),
  which Horizon accepts and the host's path code refused; it normalises them at its fs entry now (`fs_host.c`).
- File handles. The game keeps its open mode in the low three bits of a file handle and masks them off before
  closing, so handles must be 8-byte aligned like Horizon's pointers; the host's are multiples of 8 now.
- RomFS directories. The game lists the whole RomFS tree to find its paks (TotK and FE3H never listed it); listings
  are built from the index (`romfs.c`).
- The audio renderer's `SetVoiceDropParameter` command (12), which this game sends.
- Movies. The game plays H.264 in `nn::movie`'s texture mode: the decoder writes into the game's own NVN textures
  (RGBA8_SRGB, `VIDEO_DECODE`), passed to `GetOutputBuffer` as the buffer's data pointer; the SDK's decoder ran
  lifted into the NVIDIA driver and aborted. The host decodes with openh264 (BSD, linked statically from MSYS2) and
  writes the frames through the back end's texel write (`ov_movie.c`, beside TotK's VP9 buffer-mode decoder); in
  texture mode the game never releases an output index, so the slot is freed after the write. A null decoder stands
  in for any other type, taking the stream and dropping it so the player reaches its end. The conversion to RGBA is
  SSE2; a 1080p60 stream decodes in 4 ms, two at once in 15-19 ms, and a few percent of frames drop.
- Audio. The game's final mix goes through an Aux effect to the game's own DSP code and back, into a 5.1 device sink;
  the host folds it to stereo. The sound was wrong, and clean with the Aux round trip bypassed (`MP4_AUDIO_AUX=0`).
  The ring trace showed why: the Aux ring holds exactly two renderer frames (2,880 samples for six channels), so the
  game's DSP code must run between every two consecutive renders, which hardware's fixed 5 ms cadence guarantees;
  the host rendered frames in bursts whenever its output queue fell below target and overwrote the ring before the
  game read it. The render thread now keeps one frame per period even when the device asks for more (`audren.c`);
  with that the game reads and writes a whole frame between every two renders.

Diagnostics added on the way: `MP4_BREAK_AT` and `MP4_BREAK_WATCH` (a log at chosen lifted function entries with the
arguments and a watched global), the fs trace's open, directory and read lines, the movie decoder's call log. Settings
reach a Windows process started from WSL only through `WSLENV`.

**The window origin (2026-11-03):** the opening cinematic's text crawl showed a faded, vertically flipped copy of
itself. The frame dump (`MP4_DUMP`) placed it: the bloom chain's first downsample came out upside down while the
tone map from the same source, same vertex shader, same quad and same uniforms did not; and with no host change
between runs the tone map came out upside down as well, so the quads themselves map texture v=0 to the bottom of the
viewport. That is GL's render-to-texture convention, and the game keeps NVN's default window origin, LOWER_LEFT
(`nvnDeviceSetWindowOriginMode` is logged now; TotK sets UPPER_LEFT, so the host never met the default). In that mode
NDC +y lands on the last memory row of a render target and the display turns the window texture over; window y is the
memory row in both modes, so viewports, scissors, `gl_FragCoord` and the front face need no change. The host now keeps
Vulkan's own viewport orientation for the lower-left origin instead of the negative-height viewport it uses for the
upper-left one, turns the frame over in the present blit (and the upscaler's), and reverses the rows of screenshots
and dump PNGs so they read as the screen does (`nvn_origin_lower_left` in `nvn_api_core.c`; `vk_exec_draw.c`,
`gfx_window.c`, `vk_fsr.c`, `boot.c`, `vk_dump.c`). Two wrong turns on the way, both reverted: mirroring viewport
and scissor rectangles against the target height plus the translator's YNegate for `gl_FragCoord`, and mirroring the
front face; the dump showed the bloom shader reads no `gl_FragCoord` at all.

**The cinematic crash (2026-11-03):** a few seconds into the opening cinematic the game faulted in `main+0x115980`,
a pop from an empty pool of GPU timer queries (three per profiler object; the pop with a count of zero overwrites the
count with a pointer and the next pop reads wild). A breakpoint trace (`MP4_BREAK_AT` on the pop, the query's
IsReady and its fence setter, `MP4_BREAK_BRIEF=1`, `MP4_BREAK_FOLLOW=x1:0x978@0x340`) showed the bookkeeping: each
query carries the game's submit count at issue, the game's reclaim (`main+0xbb880`) polls its syncs in order with a
zero timeout and advances a completed count, and a query returns to the pool once the completed count reaches its
value. Each of two profilers pops one query per frame, so at a frame's first pop the submits of two frames ago must
have completed. Hardware's GPU pace gives that; the host's replay, stalled by a pipeline build, let the CPU run up to
five submits ahead (seen in the trace) and the pool ran dry in three of six runs. The window acquire now bounds the
distance: it waits for the GPU to finish the image's last present (as the display's release implies on hardware) and
for every submit made before the previous acquire (`MP4_GPU_AHEAD`, default 1 frame of overlap; 0 serialises the
frames). Three scripted runs through the cinematic pass at 59 fps. `nvnSyncWait` logs a blocking wait that times
out and a poll that fails eight times on the same fence, which ruled out a stuck sync first. On the way the game also
asked for `nvnTextureBuilderGetSparseTileLayout`, answered with Maxwell's 64 KiB tile sizes by bytes per block.

## Shared core

As pieces turn out to be identical to fe3h-pc-recomp's (and through it to totk-pc-recomp's), list them here. They are
candidates for a shared Switch recomp library.

Identical at the start (Phase 0):

- `recomp/nso.py`: unchanged through all three projects.
- `tools/check_content.py`: only the marker name differs per project.
- `tools/build_hactool.py`, `tools/extract_exefs.py`, `tools/extract_romfs.py`, `tools/romfs_reader.py`: unchanged
  from fe3h-pc-recomp apart from their headers. The title id and version come from the dump, so nothing in them
  names a game.

The copy of 2026-11-02 (Phase 1), from fe3h-pc-recomp @ `47ef991`, 176 text files plus the vendored translator sources:

- 86 files copied unchanged: all of `recomp/` (the lifter ran unchanged over this game's three modules in Phase 0 with
  no unsupported instruction), `tests/`, the NVN front end's core, the Vulkan back end's larger part, the texture
  code, the platform layer, the shaders and their SPIR-V headers, `tools/xlate/`.
- 90 files that differ only by the rename of `fe3h` to `mp4` and `FE3H_` to `MP4_` (the executable, settings file,
  environment variables, window): the kernel, SVCs, IPC, fs, RomFS, audio renderer, the rest of the NVN and Vulkan
  code, the menu, the build tools. Each file's first line says which.
- Rewritten for this game: the tables listed under Phase 1. Not taken: FE3H's overrides, patch tables, expected-hash
  files, installer and release tools.

The answer so far: a third game needed one mechanical copy and about a hundred lines of game-bound tables to reach
a build. What the boot reveals goes under Phase 1.
