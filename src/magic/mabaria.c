//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"

// Growth is spread across the animation: the notional frame MABARIA_LIFETIME
// lands on (GROWTH_TOTAL + SCALE_BASE) * 2 == 0x7FFE, one short of the 0x7FFF
// ceiling ScaleMatrix's s16 matrix entries allow.
#define GROWTH_TOTAL 0x3BFF
#define SCALE_BASE 0x400

// The model renders on frames 0..15 and retires after the last one, fading
// out over the second half. func_800D29D4 loads the fade as the GTE's
// depth-cue factor, driving the vertex colour toward SetFarColor, black
// here, and reaching 0xE00 on the last frame. The primitive is drawn
// semi-transparent and the GPU blends additively, so black adds nothing and
// the model fades to invisible rather than to a dark shape.
#define MABARIA_LIFETIME 16
#define FADE_START_FRAME 8
#define FADE_PER_FRAME 0x200

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ SVECTOR Pos;
    /* 0x0C */ SVECTOR Rot;
    /* 0x14 */ s16 TargetIndex;
    // Nothing in the overlay reads or writes past 0x16.
    /* 0x16 */ char pad16[0xA];
} MabariaData; // size:0x20

// PSX fixed point: 1.0 == 1 << FIXED_SHIFT.
#define FIXED_SHIFT 12
#define FIXED_ONE (1 << FIXED_SHIFT)

// The primitive buffer holds two pages; MabariaDoubleBufferFlip alternates
// between them so the GPU can read last frame's primitives while this one
// builds.
#define MABARIA_PAGE_SIZE 0x10000

typedef struct {
    /* 0x0000 */ char pad[MABARIA_PAGE_SIZE];
} MabariaPrimPage;

extern s32 D_801B0CA0;
extern s32 D_801B0CA4;
extern MabariaData D_80162978[];
extern Unk801B0C98 MabariaRenderDesc;
extern MabariaPrimPage MabariaPrimBuffer[];
extern void* MabariaBufferPtr;

void MabariaMainSetup(s32 arg0, s32 arg1);

void MAGIC_MBarrier(s32 arg0, s32 arg1) { MabariaMainSetup(arg0, arg1); }

// Draws the embedded model through the model path, growing it over the 16
// animation frames and fading it out over the last 8.
void MabariaRenderModel(void) {
    MabariaData* effect = &D_80162978[D_8015169C];
    // D_801B0CA0 is always 0x2000, making the scale (growth + SCALE_BASE) * 2:
    // 0.5x on frame 0 to 7.5x on frame 15.
    s32 growth = effect->AnimationFrame * GROWTH_TOTAL / MABARIA_LIFETIME;
    MATRIX matrix;
    s32 fade;
    s16 nextFrame;
    VECTOR scale;

    scale.vx = scale.vy = scale.vz = ((growth + SCALE_BASE) * D_801B0CA0) >> FIXED_SHIFT;
    // D_801B0CA4 holds 0, so the lerp below passes the fade through.
    if (effect->AnimationFrame < FADE_START_FRAME) {
        fade = 0;
    } else {
        fade = (effect->AnimationFrame - FADE_START_FRAME) * FADE_PER_FRAME;
    }
    fade += ((FIXED_ONE - fade) * D_801B0CA4) >> FIXED_SHIFT;
    RotMatrixYXZ(&effect->Rot, &matrix);
    matrix.t[0] = (s32)effect->Pos.vx;
    matrix.t[1] = (s32)effect->Pos.vy;
    matrix.t[2] = (s32)effect->Pos.vz;
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
        if (nextFrame >= MABARIA_LIFETIME) {
            effect->StartFrame = -1;
        }
    }
}

INCLUDE_ASM("asm/us/magic/nonmatchings/mabaria", func_801B01C0);
void func_801B01C0();

void MabariaAttachToTarget(s32 target) {
    MabariaData* temp_s1 = &D_80162978[BattleEffectRegister(func_801B01C0)];

    BattleGetPartPosition(target, D_801518E4[target].D_8015190F, &temp_s1->Pos);
    temp_s1->Pos.vx =
        temp_s1->Pos.vx - ((rsin(D_801518E4[target].unk160.vy) * D_801518E4[target].unk12) >> FIXED_SHIFT);
    temp_s1->Pos.vz =
        temp_s1->Pos.vz - ((rcos(D_801518E4[target].unk160.vy) * D_801518E4[target].unk12) >> FIXED_SHIFT);
    temp_s1->Rot = D_801518E4[target].unk160;
    temp_s1->TargetIndex = target;
}

void MabariaDoubleBufferFlip(void) {
    MabariaData* effect = &D_80162978[D_8015169C];

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
    MagicAnimationRegister(arg0, arg1, 0, MabariaAttachToTarget);
    BattleCommandSend(0x20, 0x40, 0x43);
}
