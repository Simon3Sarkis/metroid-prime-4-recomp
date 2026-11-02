// From fe3h-pc-recomp runtime/game_patch_stubs.c @ 47ef99126793dbd13e4a475feb910a13ef6639b8 (MIT, Copyright (c) 2026 Jeff Aigner), copied unchanged.
// Stand-ins for totk-pc-recomp runtime/frame_rate.c and runtime/resolution.c, which hold the host half of TotK's
// 60 fps and resolution patch sets and were not taken over. The rest of the runtime still calls their interface
// (host.h); these definitions keep the game at its own frame pacing and resolution:
//
// - frame step: no real-time step, so nvn_api_obj.c present_period() follows the game's present interval. The world
//   clock and the 60 Hz tick count (input scripts, profiler, A/B blocks) run from the first present.
// - resolution: no extra heap, no grown HeapDef table and no wide UI overlay; the output and scene sizes the overlay
//   shows are a placeholder until FE3H's own sizes are measured.
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "host.h"

// ---------- Frame step

static _Atomic uint64_t g_first_present_ns;   // display time of the first present, 0 before it
static _Atomic uint64_t g_last_present_ns;

void frame_step_init(void) {}

int frame_step_realtime(void) { return 0; }

uint64_t frame_step_cap_ns(void) { return 0; }

double frame_step_limit_hz(void) { return 0; }   // the overlay shows no limit

void frame_step_present(uint64_t display_ns, int interval) {
    (void)interval;
    uint64_t zero = 0;
    atomic_compare_exchange_strong(&g_first_present_ns, &zero, display_ns ? display_ns : 1);
    atomic_store(&g_last_present_ns, display_ns);
}

int64_t frame_step_world_us(void) {
    uint64_t first = atomic_load(&g_first_present_ns), last = atomic_load(&g_last_present_ns);
    return first && last >= first ? (int64_t)((last - first) / 1000) : -1;
}

uint64_t frame_step_ticks(void) {
    int64_t us = frame_step_world_us();
    return us < 0 ? 0 : (uint64_t)us * 60 / 1000000;
}

// ---------- Resolution

uint64_t res_heap_extra_kib(void) { return 0; }

uint64_t res_heap_extra_bytes(void) { return 0; }

int res_heapdef_build(const void *in, size_t n, void **out, size_t *out_n) {
    (void)in, (void)n, (void)out, (void)out_n;
    return 0;   // never reached: romfs.c heapdef_overlay() returns first when res_heap_extra_kib() is 0
}

void res_heapdef_served(void) {}

void res_init(void) {}

int res_is_16x9(unsigned w, unsigned h) {
    // Within 1% of 16:9, the test resolution.c uses.
    double r = h ? (double)w / h : 0, want = 16.0 / 9.0;
    return r > want * 0.99 && r < want * 1.01;
}

int res_wants_wide_overlay(const unsigned out[2]) {
    (void)out;
    return 0;
}

void res_wanted(unsigned out[2]) { out[0] = 1920, out[1] = 1080; }

void res_sizes(unsigned out[2], unsigned scene[2]) {
    out[0] = scene[0] = 1920;
    out[1] = scene[1] = 1080;
}
