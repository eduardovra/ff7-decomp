// Native PSY-Z entry point.
//
// The PS1 build reaches battle through the overlay loader at a fixed address;
// linked natively the overlays are ordinary symbols, called directly. Most of
// the game is still a generated stub, so -battle is expected to misbehave.

#include <psyz.h>
#include <libgpu.h>
#include <libetc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define OTSIZE 8

typedef struct {
    DRAWENV draw;
    DISPENV disp;
    OT_TYPE ot[OTSIZE];
} DoubleBuffer;

// Decompiled game code, declared here so this file stays clear of the PS1
// headers: game.h and the PSY-Z headers disagree about u_long.
extern int SysCountActiveBits(unsigned int value);
extern int SysGetLsbNumber(unsigned int value);
extern void SysMemCopy32(void* dst, const void* src, const int len);
extern void func_800148B4(void);
extern void func_80014934(void);
extern void BATINI_Main(int sceneID);
extern void BATTLE_RunFrame(void);

#define PS1_RAM_BASE 0x80000000
#define PS1_RAM_SIZE 0x00200000

static DoubleBuffer db[2];
static DoubleBuffer* cdb;

// Casts like (u8*)0x801B0000 survive all over the decompiled C, so map the
// PS1's 2MB of RAM where the console had it and they all become valid.
static int MapPs1Ram(void) {
    void* ram = mmap((void*)PS1_RAM_BASE, PS1_RAM_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

    if (ram == MAP_FAILED) {
        return 0;
    }
    return ram == (void*)PS1_RAM_BASE;
}

static void InitGraphics(void) {
    int i;

    for (i = 0; i < 2; i++) {
        SetDefDrawEnv(&db[i].draw, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        SetDefDispEnv(&db[i].disp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    SetVideoMode(MODE_NTSC);
    ResetGraph(0);
    PutDrawEnv(&db[0].draw);
    PutDispEnv(&db[0].disp);
    ClearOTag(db[0].ot, OTSIZE);
    ClearOTag(db[1].ot, OTSIZE);
    SetDispMask(1);
    cdb = &db[0];
}

static void Display(void) {
    if (cdb == &db[0]) {
        cdb = &db[1];
    } else {
        cdb = &db[0];
    }
    FntFlush(-1);
    ClearOTag(cdb->ot, OTSIZE);
    DrawSync(0);
    VSync(0);
    ClearImage(&cdb->draw.clip, 0, 0, 40);
    DrawOTag(&cdb->ot[0]);
}

// Runs three pure routines out of src/main/btlinit.c, so the screen is
// showing real decompiled game code rather than PSY-Z alone.
static void SelfTest(int* activeBits, int* lsb, unsigned int* copied) {
    static const unsigned int source[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};

    *activeBits = SysCountActiveBits(0xF0F0);
    *lsb = SysGetLsbNumber(0x80);
    memset(copied, 0, sizeof(unsigned int) * 4);
    SysMemCopy32(copied, source, sizeof(source));
}

static void DrawSelfTest(void) {
    unsigned int copied[4];
    int activeBits;
    int lsb;

    SelfTest(&activeBits, &lsb, copied);
    FntPrint("FF7 PSY-Z PROTOTYPE\n\n");
    FntPrint("SysCountActiveBits(0xF0F0) = %d\n", activeBits);
    FntPrint("SysGetLsbNumber(0x80)      = %d\n", lsb);
    FntPrint("SysMemCopy32 last word     = %08x\n", copied[3]);
}

// No window, no GPU: enough to prove the decompiled code links and runs, and
// the only mode that works over a plain ssh session or in CI.
static int RunHeadless(void) {
    unsigned int copied[4];
    int activeBits;
    int lsb;

    SelfTest(&activeBits, &lsb, copied);
    printf("SysCountActiveBits(0xF0F0) = %d\n", activeBits);
    printf("SysGetLsbNumber(0x80)      = %d\n", lsb);
    printf("SysMemCopy32 last word     = %08x\n", copied[3]);
    return 0;
}

int main(int argc, char* argv[]) {
    int frameLimit = 0;
    int intoBattle = 0;
    int headless = 0;
    int sceneID = 0;
    int frame = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-frames") && i + 1 < argc) {
            frameLimit = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-headless")) {
            headless = 1;
        } else if (!strcmp(argv[i], "-battle")) {
            intoBattle = 1;
        } else if (!strcmp(argv[i], "-scene") && i + 1 < argc) {
            sceneID = atoi(argv[++i]);
        }
    }

    if (headless) {
        return RunHeadless();
    }

    InitGraphics();
    FntLoad(960, 256);
    SetDumpFnt(FntOpen(16, 16, SCREEN_WIDTH - 32, SCREEN_HEIGHT - 32, 0, 512));

    if (intoBattle && !MapPs1Ram()) {
        printf("could not map PS1 RAM at %08x\n", PS1_RAM_BASE);
        return 1;
    }

    if (intoBattle) {
        // func_800148B4 is the boot init: it loads the LBA table and every
        // KERNEL.BIN section, including the weapon, armor and materia tables
        // that BATINI_Main reads. func_80014934 then reseeds Savemap.party
        // from the kernel's new-game blob.
        printf("booting into scene %d\n", sceneID);
        func_800148B4();
        func_80014934();
        BATINI_Main(sceneID);
    }

    while (!Psyz_QuitRequested()) {
        if (intoBattle) {
            BATTLE_RunFrame();
        } else {
            DrawSelfTest();
            FntPrint("\nframe %d\n", frame);
        }
        Display();
        if (frameLimit && ++frame >= frameLimit) {
            break;
        }
    }
    printf("ran %d frames\n", frame);
    return 0;
}
