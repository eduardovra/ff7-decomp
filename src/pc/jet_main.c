// Native entry point that runs the jet minigame on its own.

#include <game.h>
#include <libgte.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define DEFAULT_DISK_CUE "disks/Final Fantasy VII (USA) (Disc 1).cue"

extern u16 MINI_Jet(void);
extern void InputInit(void);
extern void SysCdromInit(void);

static void ReserveAddrRam(uintptr_t addr, size_t size) {
    void* p = mmap((void*)addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
        ERRORF("failed to reserve 0x%08X with len 0x%x", addr, size);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ReserveAddrRam(0x80010000, 0x1F0000);
    ReserveAddrRam(0x1F800000, 0x400);
    if (Psyz_CdSetDiskPath(DEFAULT_DISK_CUE) < 0) {
        ERRORF("failed to open disk image '%s'", DEFAULT_DISK_CUE);
        return 1;
    }
    // What main's boot init does before any overlay runs.
    ResetGraph(0);
    InitGeom();
    SysCdromInit();
    InputInit();
    printf("jet result %d\n", MINI_Jet());
    return 0;
}
