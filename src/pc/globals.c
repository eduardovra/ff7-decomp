// Globals the native build needs at their real size.
//
// On PS1 these live at fixed linker-script addresses, so the decompiled C
// only ever declares them. Anything whose header knows its true type is
// defined here; the rest is guessed byte arrays in the generated stubs.c.

#include <game.h>

SaveWork Savemap;
