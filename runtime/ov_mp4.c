// SDK overrides that Metroid Prime 4 needs and the TotK and FE3H hosts do not have. They live here, apart from the
// files taken from fe3h-pc-recomp (ov_system.c, ov_input.c, ...), so those stay close to their source; an override
// that turns out to be game independent can move into them later. The pattern follows totk-pc-recomp
// runtime/ov_system.c: the SDK function is replaced by symbol (host.h HOST_OVERRIDE), so the service behind it is
// never opened. fe3h-pc-recomp runtime/ov_fe3h.c has the same role for that game and the examples (offline nifm and
// socket stubs, older account calls); none of its entries applies here, as this game's main imports no network
// namespace (docs/plan.md, Phase 0).
//
// Each entry names the call that needs it and the game version it was seen in.
#include <stdio.h>
#include <string.h>

#include "host.h"
