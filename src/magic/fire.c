//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"
#include "../battle/battle_private.h"
#include "magic_private.h"

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ char pad4[0x1C];
} FireData; // size:0x20

typedef struct {
    /* 0x00 */ char pad[0x4000];
} FirePrimPage; // size:0x4000

static FirePrimPage fire_prim_buffer[2];
static void* fire_buffer_ptr;

void func_801B0210(s32, s32);

INCLUDE_ASM("asm/us/magic/nonmatchings/fire", func_801B0000);

INCLUDE_ASM("asm/us/magic/nonmatchings/fire", func_801B00B8);

INCLUDE_ASM("asm/us/magic/nonmatchings/fire", func_801B0210);

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

extern s16 D_80151774;
extern u32 D_801B039C;

void func_801B02EC(s32 arg0, s32 arg1) {
    func_800D2980(&D_801B039C, 0, 0, 0);
    MagicAnimationRegister(arg0, arg1, 0, func_801B0210);
    BattleEffectRegister(func_801B0294);
    BattleCommandSend(0x20, BattleEntityGetStereoPan((s32)D_80151774), 9);
}

void func_8001C3C4();

void func_801B037C(void) { func_8001C3C4(); }
