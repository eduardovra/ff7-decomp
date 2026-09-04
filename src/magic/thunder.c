//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ char pad[0x10];
} ThunderData; // size = 0x20

typedef struct {
    /* 0x00 */ char pad[0x10000];
} ThunderPrimPage; // size = 0x10000

extern void* ThunderBufferPtr;
extern ThunderData D_80162978[];
extern ThunderPrimPage ThunderPrimBuffer[];

void func_801B06CC(s32 arg0, s32 arg1);

void func_801B0000(s32 arg0, s32 arg1) { func_801B06CC(arg0, arg1); }

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B0020);

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B0180);

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B023C);

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B0350);

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B05A4);

void func_801B066C(void) {
    ThunderData* data = &D_80162978[D_8015169C];

    ThunderBufferPtr = &ThunderPrimBuffer[data->AnimationFrame];
    data->AnimationFrame = (u16)data->AnimationFrame ^ 1;
    if (D_80162080 < 2) {
        data->StartFrame = -1;
    }
}

void func_801B05A4(s32); // extern

extern u32 D_801B0DAC;

void func_801B06CC(s32 arg0, s32 arg1) {
    func_800D2980((u_long*)&D_801B0DAC, 0, 0, 0);
    BattleEffectRegister(func_801B066C);
    MagicAnimationRegister(arg0, arg1, 2, func_801B05A4);
}
