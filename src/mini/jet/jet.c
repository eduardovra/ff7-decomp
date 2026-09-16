//! PSYQ=3.3

#include "common.h"
#include "libgpu.h"
#include "libgte.h"

// Offsets 0x00 and 0x5C are fixed by SetDefDrawEnv/SetDefDispEnv in
// func_800A7C88; both tables are sized by their ClearOTagR calls.
typedef struct {
    /* 0x0000 */ DRAWENV draw;
    /* 0x005C */ DISPENV disp;
    /* 0x0070 */ u_long* unk70[0x1000];
    /* 0x4070 */ u_long* unk4070[10];
    /* 0x4098 */ u_long* unk4098[0xB4];
    /* 0x4368 */ u_long* unk4368[6];
} Unk800D1964; // size: at least 0x4380

// Doubly linked list node. func_800A80A8 initialises the matrix to identity
// and func_800A8010 chains the arrays with a 0x38 stride.
typedef struct Unk800EE1D4 {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ MATRIX m;
    /* 0x24 */ void* unk24;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ struct Unk800EE1D4* unk30;
    /* 0x34 */ struct Unk800EE1D4* unk34;
} Unk800EE1D4; // size: 0x38

extern Unk800D1964* D_800D1964[1];
extern Unk800EE1D4 D_800A8A90[10];
extern Unk800EE1D4 D_800D16E4;
extern s16 D_800D1710;
extern s16 D_800D1A40[0xC8];
extern s16 D_800D9944;
extern Unk800EE1D4 D_800EE1D4[10];

void func_800A7FAC(u_long**);
void func_800A80A8(Unk800EE1D4* arg0, s16 arg1);
void func_800A8264(s16);
void func_800A82F0(Unk800EE1D4*);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0450);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0874);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0D78);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1198);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A12EC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A13AC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1450);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A16A4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1A64);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1B64);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1CD8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1F18);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2058);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2214);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2420);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2518);

void func_800A27F0(u_long* addr) {
    TIM_IMAGE timimg;

    OpenTIM(addr);

    while (ReadTIM(&timimg)) {
        if (timimg.caddr) {
            LoadImage(timimg.crect, timimg.caddr);
        }
        if (timimg.paddr) {
            LoadImage(timimg.prect, timimg.paddr);
        }
    }
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2860);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2938);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A29AC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2AA0);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2B78);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2BE0);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2C50);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2DE4);

void func_800A2E30(void) {}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2E38);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A334C);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3414);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A35DC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A372C);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A385C);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A38D4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3980);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3A20);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3AAC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3B58);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3C04);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3D50);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3E58);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A40F4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4390);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4400);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A442C);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4458);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A45C0);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4650);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A46E8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6B08);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6BD8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A70D4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7414);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7544);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7688);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A76DC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7730);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7928);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7A10);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7AF8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7B48);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7BF4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7C20);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7C54);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7C88);

void func_800A7E1C(void) {
    ClearOTagR((u_long*)D_800D1964[0]->unk70, LEN(D_800D1964[0]->unk70));
    ClearOTagR((u_long*)D_800D1964[0]->unk4098, LEN(D_800D1964[0]->unk4098));
    func_800A7FAC(D_800D1964[0]->unk4368);
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7E70);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7FAC);

void func_800A8010(void) {
    Unk800EE1D4* a;
    Unk800EE1D4* b;
    s32 i;

    func_800A80A8(&D_800D16E4, 0);
    D_800D1710 = 0;
    D_800D9944 = 0;
    for (i = 0; i < LEN(D_800D1A40); i++) {
        D_800D1A40[i] = i + 1;
    }
    for (i = 0; i < LEN(D_800A8A90); i++) {
        a = &D_800A8A90[i];
        b = &D_800EE1D4[i];
        a->unk30 = NULL;
        a->unk34 = b;
        b->unk30 = a;
        b->unk34 = NULL;
    }
}

void func_800A80A8(Unk800EE1D4* arg0, s16 arg1) {
    arg0->m.m[0][0] = 0x1000;
    arg0->m.m[1][1] = 0x1000;
    arg0->m.m[2][2] = 0x1000;
    arg0->m.t[0] = 0;
    arg0->m.t[1] = 0;
    arg0->m.t[2] = 0;
    arg0->m.m[0][1] = 0;
    arg0->m.m[0][2] = 0;
    arg0->m.m[1][0] = 0;
    arg0->m.m[1][2] = 0;
    arg0->m.m[2][0] = 0;
    arg0->m.m[2][1] = 0;
    arg0->unk24 = &D_800D16E4;
    arg0->unk2A = arg1;
    arg0->unk30 = 0;
    arg0->unk34 = 0;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A80F8);

void func_800A8204(Unk800EE1D4* arg0) {
    func_800A82F0(arg0);
    func_800A8264(arg0->unk2A);
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A8238);

void func_800A8264(s16 arg0) {
    s16* temp;
    s16* temp2;

    temp2 = &D_800D1A40[arg0];
    temp = &D_800D9944;
    *temp2 = *temp;
    *temp = arg0;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A8290);

void func_800A82F0(Unk800EE1D4* arg0) {
    arg0->unk30->unk34 = arg0->unk34;
    arg0->unk34->unk30 = arg0->unk30;
}
