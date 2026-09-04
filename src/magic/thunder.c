//! PSYQ=3.3 CC1=2.6.3

#include "common.h"
#include "../battle/battle.h"

// Bolt (サンダー / Thunder), tier 1. Three passes share the effect slot array:
// a model drawn through func_800D29D4, and two textured-quad passes drawn
// through func_800D4D90, all double-buffered into a 0x20000 primitive page.

typedef struct {
    /* 0x00 */ s16 StartFrame;
    /* 0x02 */ s16 AnimationFrame;
    /* 0x04 */ SVECTOR Pos;
    /* 0x0C */ SVECTOR unk0C;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
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
extern u_long ThunderTexture[]; // 8bpp TIM + CLUT, uploaded on setup
extern ModelRenderDesc ThunderRenderDesc0;
extern ModelRenderDesc ThunderRenderDesc1;
extern MATRIX ThunderModelMatrix;
extern s16 ThunderModelMatrixM21; // = ThunderModelMatrix.m[2][1]
extern Unk801B0C98 ThunderModelDesc;

static void ThunderMainSetup(s32 arg0, s32 arg1);

void MAGIC_Thunder(s32 arg0, s32 arg1) { ThunderMainSetup(arg0, arg1); }

// Draws the embedded model through the model path, spinning it up over the
// first 8 frames and fading it out over the next 8.
static void ThunderRenderModel(void) {
    MATRIX matrix;
    ThunderData* effect = &D_80162978[D_8015169C];
    s16 frame = effect->AnimationFrame;
    u16* scale; // read through a pointer; a plain field read does not match

    if (frame < 8) {
        ThunderModelDesc.desc.unkA = 0x80;
    } else if (frame < 16) {
        ThunderModelDesc.desc.unkA = 0x80 - ((frame - 8) * 0x10);
    } else {
        effect->StartFrame = -1;
        return;
    }

    scale = &effect->unk16;
    ThunderModelMatrix.m[0][0] = ThunderModelMatrixM21 = *scale;
    ThunderModelMatrix.m[1][2] = -(s16)*scale;
    ThunderModelMatrix.t[0] = (s32)effect->Pos.vx;
    ThunderModelMatrix.t[1] = (s32)effect->Pos.vy;
    ThunderModelMatrix.t[2] = (s32)effect->Pos.vz;
    CompMatrix(&D_800FA63C.m, &ThunderModelMatrix, &matrix);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    ThunderBufferPtr = func_800D29D4(&ThunderModelDesc, g_cDb->unk70, 0xC, ThunderBufferPtr);
    if (D_80062D98 == 0) {
        effect->AnimationFrame = (u16)effect->AnimationFrame + 1;
        effect->unk16 += effect->unk18;
    }
}

// Quad pass over ThunderRenderDesc0, 9 frames, quad count ramping with the
// frame. Not renamed: what it draws is not established.
static void func_801B0180(void) {
    ThunderData* effect;
    s16 nextFrame;

    effect = &D_80162978[D_8015169C];
    func_800D4368(&effect->Pos, 0x2000, effect->unk1C);
    ThunderRenderDesc0.QuadCount = (s16)(u16)effect->AnimationFrame >> 1;
    ThunderBufferPtr = func_800D4D90(&ThunderRenderDesc0, g_cDb->unk70, 0xC, ThunderBufferPtr);
    if (D_80062D98 == 0) {
        nextFrame = (u16)effect->AnimationFrame + 1;
        effect->AnimationFrame = nextFrame;
        if (nextFrame == 9) {
            effect->StartFrame = -1;
        }
    }
}

// Quad pass over ThunderRenderDesc1, 8 frames, mirrored per instance by the
// random bits in unk1A. Not renamed: what it draws is not established.
static void func_801B023C(void) {
    MATRIX* matrix;
    ThunderData* effect = &D_80162978[D_8015169C];
    s16 nextFrame;

    matrix = func_800D4368(&effect->Pos, 0x2000, effect->unk1C);
    if (effect->unk1A & 1) {
        matrix->m[0][0] = (u16) - (s32)matrix->m[0][0];
    }
    if (effect->unk1A & 2) {
        matrix->m[1][1] = (u16) - (s32)matrix->m[1][1];
    }
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    ThunderRenderDesc1.QuadCount = effect->AnimationFrame;
    ThunderBufferPtr = func_800D4D90(&ThunderRenderDesc1, g_cDb->unk70, 0xC, ThunderBufferPtr);
    if (D_80062D98 == 0) {
        nextFrame = (u16)effect->AnimationFrame + 1;
        effect->AnimationFrame = nextFrame;
        if (nextFrame == 8) {
            effect->StartFrame = -1;
        }
    }
}

// Spawn callback for the bolt. On frame 0 it registers the two one-shot
// renderers; from frame 2 on it scatters a spark each frame, retiring at 16.
static void ThunderSpawnBolt(void) {
    ThunderData* next;
    ThunderData* effect = &D_80162978[D_8015169C];
    s16 nextFrame;

    if (D_80062D98 == 0) {
        if (effect->AnimationFrame == 0) {
            next = &D_80162978[BattleEffectRegister(func_801B0180)];
            next->Pos = effect->Pos;
            next->Pos.vy = 0;
            next->unk1C = (u16)effect->unk1C;
            func_800D5774(effect->unk14);
            if (effect->AnimationFrame == 0) {
                next = &D_80162978[BattleEffectRegister(ThunderRenderModel)];
                next->Pos = effect->Pos;
                next->unk16 = 0x1000;
                next->Pos.vy = 0;
                next->unk18 = 0x200;
                next->unk1C = (u16)effect->unk1C;
            }
        }
        if (effect->AnimationFrame >= 2) {
            next = &D_80162978[BattleEffectRegister(func_801B023C)];
            next->Pos.vx = ((u16)effect->Pos.vx + rand() % 1000) - 500;
            next->Pos.vy = ((u16)effect->Pos.vy + rand() % 1000) - 500;
            next->Pos.vz = ((u16)effect->Pos.vz + rand() % 1000) - 500;
            next->unk1A = rand() & 3;
            next->unk1C = (u16)effect->unk1C;
        }
        nextFrame = (u16)effect->AnimationFrame + 1;
        effect->AnimationFrame = nextFrame;
        if (nextFrame == 16) {
            effect->StartFrame = -1;
        }
    }
}

static void ThunderAttachToTarget(s32 target) {
    ThunderData* effect = &D_80162978[BattleEffectRegister(ThunderSpawnBolt)];

    BattleGetPartPosition(target, D_801518E4[target].D_8015190F, &effect->Pos);
    effect->unk14 = target;
    effect->unk1C = -D_801518E4[target].unk12;
    BattleCommandSend(0x20, BattlePositionToStereoPan(&effect->Pos), 0xB);
}

static void ThunderDoubleBufferFlip(void) {
    ThunderData* data = &D_80162978[D_8015169C];

    ThunderBufferPtr = &ThunderPrimBuffer[data->AnimationFrame];
    data->AnimationFrame = data->AnimationFrame ^ 1;
    if (D_80162080 < 2) {
        data->StartFrame = -1;
    }
}

static void ThunderMainSetup(s32 arg0, s32 arg1) {
    func_800D2980(ThunderTexture, 0, 0, 0);
    BattleEffectRegister(ThunderDoubleBufferFlip);
    MagicAnimationRegister(arg0, arg1, 2, ThunderAttachToTarget);
}
