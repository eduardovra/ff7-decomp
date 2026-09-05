//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ SVECTOR unk04;
    /* 0x0C */ SVECTOR unk0C;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ char pad1E[2];
} MabariaData; // size:0x20

typedef struct {
    /* 0x0000 */ char pad[0x10000];
} MabariaPrimPage;

extern s32 D_801B0CA0;
extern s32 D_801B0CA4;
extern s16 D_8015169C;
extern s16 D_80162080;
extern MabariaData D_80162978[];
extern Unk801B0C98 MabariaRenderDesc;
extern MabariaPrimPage MabariaPrimBuffer[];
extern void* MabariaBufferPtr;

void func_801B0378();
void MabariaMainSetup(s32 arg0, s32 arg1);

void MAGIC_MBarrier(s32 arg0, s32 arg1) { MabariaMainSetup(arg0, arg1); }

// Draws the embedded model through the model path, growing it over the 16
// animation frames and fading it to black over the last 8.
void MabariaRenderModel(void)
{
    MabariaData* effect = &D_80162978[D_8015169C];
    // D_801B0CA0 is always 0x2000, making the scale (growth + 0x400) * 2:
    // 0.5x on frame 0 to 7.5x on frame 15. 0x3BFF is sized so the notional
    // frame 16 lands on 0x7FFE, one short of the 0x7FFF s16 matrix ceiling.
    s32 growth = effect->AnimationFrame * 0x3BFF / 16;
    MATRIX matrix;
    s32 fade;
    s16 nextFrame;
    VECTOR scale;

    scale.vx = scale.vy = scale.vz = ((growth + 0x400) * D_801B0CA0) >> 12;
    // Depth cue toward SetFarColor, black here: the last 8 frames fade the
    // model out, reaching 0xE00 on frame 15. D_801B0CA4 holds 0, so the
    // lerp below passes the value through.
    if (effect->AnimationFrame < 8) {
        fade = 0;
    } else {
        fade = (effect->AnimationFrame - 8) << 9;
    }
    fade += ((0x1000 - fade) * D_801B0CA4) >> 12;
    RotMatrixYXZ(&effect->unk0C, &matrix);
    matrix.t[0] = (s32) effect->unk04.vx;
    matrix.t[1] = (s32) effect->unk04.vy;
    matrix.t[2] = (s32) effect->unk04.vz;
    ScaleMatrix(&matrix, &scale);
    CompMatrix(&D_800FA63C, &matrix, &matrix);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    SetFarColor(0, 0, 0);
    MabariaRenderDesc.desc.uA.depthCue = fade;
    MabariaBufferPtr = func_800D29D4(&MabariaRenderDesc, g_cDb->unk70, 0xC, MabariaBufferPtr);
    if (D_80062D98 == 0) {
        nextFrame = effect->AnimationFrame + 1;
        effect->AnimationFrame = nextFrame;
        if (nextFrame >= 16) {
            effect->StartFrame = -1;
        }
    }
}

INCLUDE_ASM("asm/us/magic/nonmatchings/mabaria", func_801B01C0);

INCLUDE_ASM("asm/us/magic/nonmatchings/mabaria", func_801B0378);

void MabariaDoubleBufferFlip(void)
{
    MabariaData *effect = &D_80162978[D_8015169C];

    MabariaBufferPtr = &MabariaPrimBuffer[effect->AnimationFrame];
    effect->AnimationFrame = effect->AnimationFrame ^ 1;
    if (D_80162080 < 2) {
        effect->StartFrame = -1;
    }
}

void MabariaMainSetup(s32 arg0, s32 arg1) {
    D_801B0CA0 = 0x2000;
    D_801B0CA4 = 0;
    BattleEffectRegister(MabariaDoubleBufferFlip);
    MagicAnimationRegister(arg0, arg1, 0, func_801B0378);
    BattleCommandSend(0x20, 0x40, 0x43);
}
