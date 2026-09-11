//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"
#include "magic_private.h"

extern s32 g_dbIndex;

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ s16 unk4;        // inferred
    /* 0x06 */ s16 unk6;        // inferred
    /* 0x08 */ char pad8[0xA];  // maybe part of unk6[6]?
    /* 0x12 */ s16 unk12;       // inferred
    /* 0x14 */ char pad14[0xC]; // maybe part of unk12[7]?
} FireData;                     // size:0x20

typedef struct {
    /* 0x00 */ char pad[0x4000];
} FirePrimPage; // size:0x4000

extern FireData g_BattleEffectSlots[];
static FirePrimPage fire_prim_buffer[2];
static void* fire_buffer_ptr;
extern s16 D_80151774;
extern u_long g_FireTexture[]; // 4bpp TIM + four 16-colour CLUTs, uploaded on setup

INCLUDE_ASM("asm/us/magic/nonmatchings/fire", func_801B0000);

void func_801B00B8(); // extern
INCLUDE_ASM("asm/us/magic/nonmatchings/fire", func_801B00B8);

void func_801B0210(s32 arg0, s32 arg1) {
    FireData* effect;

    effect = &g_BattleEffectSlots[BattleEffectRegister(func_801B00B8)];
    effect->unk4 = arg0;
    effect->unk6 = 0;
    effect->unk12 = -D_801518E4[arg0].unk12;
}

void func_801B0294(void) {
    FireData* effect;
    FirePrimPage* buf;

    effect = &g_BattleEffectSlots[g_BattleEffectCursor];
    buf = &fire_prim_buffer[1];
    if (g_dbIndex != 0) {
        buf = &fire_prim_buffer[0];
    }
    fire_buffer_ptr = buf;

    if (g_BattleEffectCount < 2) {
        effect->StartFrame = -1;
    }
}

void func_801B02EC(s32 arg0, s32 arg1) {
    func_800D2980(g_FireTexture, 0, 0, 0);
    MagicAnimationRegister(arg0, arg1, 0, func_801B0210);
    BattleEffectRegister(func_801B0294);
    BattleCommandSend(0x20, BattleEntityGetStereoPan((s32)D_80151774), 9);
}

void func_8001C3C4();

void func_801B037C(void) { func_8001C3C4(); }
