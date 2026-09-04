//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ SVECTOR pos;
    /* 0x0C */ char pad0C[8];
    /* 0x14 */ s16 unk14;
    /* 0x16 */ char pad16[4];
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ char pad1E[2];
} ThunderData; // size = 0x20

typedef struct {
    /* 0x00 */ char pad[0x10000];
} ThunderPrimPage; // size = 0x10000

extern void* ThunderBufferPtr;
extern ThunderData D_80162978[];
extern ThunderPrimPage ThunderPrimBuffer[];
extern u32 D_801B0DAC;

extern ModelRenderDesc ThunderRenderDesc0;
extern ModelRenderDesc ThunderRenderDesc1;

void func_801B06CC(s32 arg0, s32 arg1);

void func_801B0000(s32 arg0, s32 arg1) { func_801B06CC(arg0, arg1); }

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B0020);

void func_801B0180(void)
{
    ThunderData* temp_s0;
    s16 temp_v0;

    temp_s0 = &D_80162978[D_8015169C];
    func_800D4368(&temp_s0->pos, 0x2000, temp_s0->unk1C);
    ThunderRenderDesc0.unk8 = (s16) (u16) temp_s0->AnimationFrame >> 1;
    ThunderBufferPtr = func_800D4D90(&ThunderRenderDesc0, g_cDb->unk70, 0xC, ThunderBufferPtr);
    if (D_80062D98 == 0) {
        temp_v0 = (u16) temp_s0->AnimationFrame + 1;
        temp_s0->AnimationFrame = temp_v0;
        if (temp_v0 == 9) {
            temp_s0->StartFrame = -1;
        }
    }
}

void func_801B023C(void) {
    MATRIX* temp_s0;
    ThunderData* temp_s1 = &D_80162978[D_8015169C];
    s16 temp_v0;

    temp_s0 = func_800D4368(&temp_s1->pos, 0x2000, temp_s1->unk1C);
    if (temp_s1->unk1A & 1) {
        temp_s0->m[0][0] = (u16) - (s32)temp_s0->m[0][0];
    }
    if (temp_s1->unk1A & 2) {
        temp_s0->m[1][1] = (u16) - (s32)temp_s0->m[1][1];
    }
    SetRotMatrix(temp_s0);
    SetTransMatrix(temp_s0);
    ThunderRenderDesc1.unk8 = temp_s1->AnimationFrame;
    ThunderBufferPtr = func_800D4D90(&ThunderRenderDesc1, g_cDb->unk70, 0xC, ThunderBufferPtr);
    if (D_80062D98 == 0) {
        temp_v0 = (u16) temp_s1->AnimationFrame + 1;
        temp_s1->AnimationFrame = temp_v0;
        if (temp_v0 == 8) {
            temp_s1->StartFrame = -1;
        }
    }
}

INCLUDE_ASM("asm/us/magic/nonmatchings/thunder", func_801B0350);

void func_801B0350();

void func_801B05A4(s32 target) {
    ThunderData* data = &D_80162978[BattleEffectRegister(func_801B0350)];

    BattleGetPartPosition(target, D_801518E4[target].D_8015190F, &data->pos);
    data->unk14 = target;
    data->unk1C = -D_801518E4[target].unk12;
    BattleCommandSend(0x20, BattlePositionToStereoPan(&data->pos), 0xB);
}

void func_801B066C(void) {
    ThunderData* data = &D_80162978[D_8015169C];

    ThunderBufferPtr = &ThunderPrimBuffer[data->AnimationFrame];
    data->AnimationFrame = data->AnimationFrame ^ 1;
    if (D_80162080 < 2) {
        data->StartFrame = -1;
    }
}

void func_801B06CC(s32 arg0, s32 arg1) {
    func_800D2980((u_long*)&D_801B0DAC, 0, 0, 0);
    BattleEffectRegister(func_801B066C);
    MagicAnimationRegister(arg0, arg1, 2, func_801B05A4);
}
