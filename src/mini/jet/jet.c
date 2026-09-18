//! PSYQ=3.3

#include "types.h"
#include <game.h>

// Nine write cursors, each reset to the start of its own buffer below.
// PC: prim pools inside Class_coaster_D8
typedef struct {
    /* 0x0000 */ u_long* unk0;
    /* 0x0004 */ u_long* unk4;
    /* 0x0008 */ u_long* unk8;
    /* 0x000C */ u_long* unkC;
    /* 0x0010 */ u_long* unk10;
    /* 0x0014 */ u_long* unk14;
    /* 0x0018 */ u_long* unk18;
    /* 0x001C */ u_long* unk1C;
    /* 0x0020 */ u_long* unk20;
    /* 0x0024 */ u_long unk24[5];
    /* 0x0038 */ u_long unk38[6];
    /* 0x0050 */ u_long unk50[11200];
    /* 0xAF50 */ u_long unkAF50[270];
    /* 0xB388 */ u_long unkB388[8];
    /* 0xB3A8 */ u_long unkB3A8[3000];
    /* 0xE288 */ u_long unkE288[10];
    /* 0xE2B0 */ u_long unkE2B0[13];
    /* 0xE2E4 */ u_long unkE2E4[4];
} Unk800A7FAC; // size: 0xE2F4

// Offsets 0x00 and 0x5C are fixed by SetDefDrawEnv/SetDefDispEnv in
// func_800A7C88; both tables are sized by their ClearOTagR calls.
// PC: Class_coaster_D8 (renderer)
typedef struct {
    /* 0x0000 */ DRAWENV draw;
    /* 0x005C */ DISPENV disp;
    /* 0x0070 */ u_long* unk70[0x1000];
    /* 0x4070 */ u_long* unk4070[10];
    /* 0x4098 */ u_long* unk4098[0xB4];
    /* 0x4368 */ Unk800A7FAC unk4368;
} Unk800D1964; // size: 0x1265C

// Doubly linked list node. func_800A80A8 initialises the matrix to identity
// and func_800A8010 chains the arrays with a 0x38 stride.
// PC: t_coaster_Node
typedef struct Unk800EE1D4 {
    /* 0x00 */ s32 unk0;    // PC: pModel
    /* 0x04 */ MATRIX m;    // PC: sMatrixWorld
    /* 0x24 */ void* unk24; // PC: pParentNode
    /* 0x28 */ s16 unk28;   // PC: wModelId
    /* 0x2A */ s16 unk2A;   // PC: wIndex
    /* 0x2C */ u16 unk2C;   // PC: wDepth
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ struct Unk800EE1D4* unk30; // PC: pPrev
    /* 0x34 */ struct Unk800EE1D4* unk34; // PC: pNext
} Unk800EE1D4;                            // size: 0x38

// PC: t_coaster_Model
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ char pad4[0x1C];
} Unk800D0554; // size: 0x20

// PC: t_coaster_Quad
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ char pad4[0x24];
} Unk800D1968; // size: 0x28

// PC: t_coaster_Triangle
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ char pad4[0x20];
} Unk800A8CCC; // size: 0x24

// PC: t_coaster_GameObject
typedef struct {
    /* 0x00 */ u8 unk0[0xD4];
    /* 0xD4 */ Unk800EE1D4* unkD4; // PC: pNode
    /* 0xD8 */ s16 unkD8;          // PC: wObjIndex
    /* 0xDA */ s16 unkDA;          // PC: wIsActive
} Unk800A4390;                     // size: at least 0xDC

extern RECT D_800A0000;
extern s32 D_800A8310;
extern u32 D_800A8314;
extern s32 D_800A8318;
extern u32 D_800A831C;
extern s32 D_800A8320;
extern u32 D_800A8324;
extern s32 D_800A8328;
extern u32 D_800A832C;
extern s32 D_800A8338;
extern s32 D_800A833C;
extern s32 D_800A83C8;
extern s32 D_800A83CC;
extern s32 D_800A83D0;
extern s32 D_800A892C; // PC: sLNormal.vx
extern s32 D_800A8930; // PC: sLNormal.vy
extern s32 D_800A8934; // PC: sLNormal.vz
extern s32 D_800A893C; // PC: sRNormal.vx
extern s32 D_800A8940; // PC: sRNormal.vy
extern s32 D_800A8944; // PC: sRNormal.vz
extern s32 D_800A8988;
extern s32 D_800A89D8; // PC: D_00C5D0E4 (model info stream)
extern u_long D_800A89E4;
extern s32 D_800A8A5C;               // PC: dwLDistance
extern s32 D_800A8A64;               // PC: dwRDistance
extern s32 D_800A8A70;               // PC: D_00C5D0E0 (read triangles index)
extern u32 D_800A8A8C;               // PC: D_00C5D0EC (allocated models)
extern Unk800EE1D4 D_800A8A90[10];   // PC: D_00C60320 (list heads per depth)
extern s32* D_800A8CC0;              // PC: D_00C3F898 (track offsets, stream 5)
extern Unk800A8CCC* D_800A8CCC;      // PC: D_00C5D0E8 (triangles stream)
extern Unk800EE1D4 D_800A8CD0[0xC8]; // PC: D_00C5D590 (node pool)
extern Unk800D1964 D_800AB898[2];    // PC: Class_coaster_D8
extern Unk800D0554 D_800D0554[];     // PC: D_00C5BF60 (model pool)
extern Unk800EE1D4 D_800D16E4;       // PC: D_00C60150 (top node)
extern s32 D_800D171C;               // PC: D_00C5D320 (read quads index)
extern s32 D_800D1724;               // PC: D_00C3F894
extern s32 D_800D1730[];             // PC: D_00C5D0F0 (model pointer table)
extern s8 D_800D1960;                // PC: D_00C3F890 (release mode)
extern Unk800D1964* D_800D1964[1];   // PC: D_00C3F888 (renderer)
extern Unk800D1968* D_800D1968;      // PC: D_00C5BF58 (quads stream)
extern u16 D_800D1970[];             // PC: D_00C3FA80 (object index pool)
extern s16 D_800D1A40[0xC8];         // PC: D_00C60190 (node index pool)
extern u_long D_800D1BD4;
extern s32 D_800D1BD8;             // PC: xbin stream 1 (model info)
extern s32 D_800D1BE4;             // PC: xbin stream 4 (track data)
extern s32* D_800D1BE8;            // PC: xbin stream 5 (track offsets)
extern s32* D_800D1BEC;            // PC: xbin stream 6
extern Unk800A8CCC* D_800D1BFC;    // PC: xbin stream 0xA (triangles)
extern Unk800D1968* D_800D1C14;    // PC: xbin stream 0x10 (quads)
extern u16 D_800D9940;             // PC: D_00C3FA6C (next object index)
extern s16 D_800D9944;             // PC: D_00C60188 (next node index)
extern s32* D_800E2604;            // PC: D_00C3F8C0
extern Unk800EE1D4 D_800EE1D4[10]; // PC: D_00C5D360 (list tails per depth)
extern u16 D_800EE42C;             // PC: D_00C3FB54 (active object count)

void func_800A7E70(Unk800A7FAC* arg0);
void func_800A7FAC(Unk800A7FAC* arg0);
void func_800A80A8(Unk800EE1D4* arg0, s16 arg1);
void func_800A8264(s16);
void func_800A82F0(Unk800EE1D4*);
s16 func_800A8238(void);
void func_800A8290(Unk800EE1D4* arg0, Unk800EE1D4* arg1);
void func_800A2518();
void func_800A8204(Unk800EE1D4* arg0);
void func_800A442C(s16 arg0);
void func_800A2DE4(s32 arg0, s32 arg1);

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

void func_800A2420(void) {
    RECT unused;

    unused = D_800A0000;

    SystemLoadFileBySector(D_800A8310, D_800A8314, &D_800A89E4, NULL);
    while (SystemCdromReadChain())
        ;
    SystemLoadFileBySector(D_800A8318, D_800A831C, (u_long*)0x800F0000, NULL);
    while (SystemCdromReadChain())
        ;

    func_800A2518();

    SystemLoadFileBySector(D_800A8320, D_800A8324, &D_800D1BD4, NULL);
    while (SystemCdromReadChain())
        ;
    SysCdromStartLoadLzs(D_800A8328, D_800A832C, (u_long*)0x800F0000, NULL);
    while (SystemCdromReadChain())
        ;
}

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

// PC: C_005E9436, fade out music and SFX
void func_800A2938(void) {
    D_8009A000[0] = 0xC1;
    D_8009A004 = 0xF0;
    D_8009A008 = 0;
    SystemAkaoExecute();
    D_8009A000[0] = 0xB9;
    D_8009A004 = 0xF0;
    D_8009A008 = 0;
    SystemAkaoExecute();
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A29AC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2AA0);

// PC: part of C_005E938D, set channel volumes
void func_800A2B78(void) {
    D_8009A000[0] = 0xA2;
    D_8009A004 = D_800A8338;
    SystemAkaoExecute();
    D_8009A000[0] = 0xA3;
    D_8009A004 = D_800A833C;
    SystemAkaoExecute();
}

// PC: C_005EA8C0, camera/track module init
void func_800A2BE0(void) {
    D_800A83CC = -0x1B76;
    D_800A83C8 = 0;
    D_800A83D0 = 0xC8;
    D_800E2604 = D_800D1BEC;
    D_800A8CC0 = D_800D1BE8;
    func_800A2DE4(0, 3);
    D_800D1960 = 1;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2C50);

// PC: C_005EAAF3, select track data from stream 4
void func_800A2DE4(s32 arg0, s32 arg1) {
    s32 elem;
    s32 base;

    elem = D_800A8CC0[arg0 & 0xFF];
    base = D_800D1BE4;
    D_800A8988 = base + elem;
    D_800D1724 = *D_800E2604;
}

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

// PC: C_005EB2DF, release game object
void func_800A4390(Unk800A4390* arg0) {
    u16* temp;

    if (arg0->unkD8 != -1) {
        temp = &D_800EE42C;
        *temp -= 1;
        func_800A8204(arg0->unkD4);
        func_800A442C(arg0->unkD8);
        arg0->unkD8 = -1;
        arg0->unkDA = 0;
    }
}

// PC: C_005EB342, allocate object index
s16 func_800A4400(void) {
    u16* temp;
    s16 result;

    temp = &D_800D9940;
    result = *temp;
    *temp = D_800D1970[result];

    return result;
}

// PC: C_005EB36E, release object index
void func_800A442C(s16 arg0) {
    u16* temp;
    u16* temp2;

    temp2 = &D_800D1970[arg0];
    temp = &D_800D9940;
    *temp2 = *temp;
    *temp = arg0;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4458);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A45C0);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4650);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A46E8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6B08);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6BD8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A70D4);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7414);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7544);

// PC: __005EEEAD, half-space test against the left frustum plane
s32 func_800A7688(s32 arg0, s32 arg1, s32 arg2) {
    s32 a;
    s32 b;
    s32 c;

    a = D_800A892C;
    b = D_800A8930;
    c = D_800A8934;

    return (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + D_800A8A5C;
}

// PC: __005EEEEA, half-space test against the right frustum plane
s32 func_800A76DC(s32 arg0, s32 arg1, s32 arg2) {
    s32 a;
    s32 b;
    s32 c;
    a = D_800A893C;
    b = D_800A8940;
    c = D_800A8944;

    return (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + D_800A8A64;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7730);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7928);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7A10);

// PC: C_005EE7F0, model module init
void func_800A7AF8(void) {
    D_800A8A70 = 0;
    D_800D171C = 0;
    D_800A8A8C = 0;
    D_800A8CCC = D_800D1BFC;
    D_800D1968 = D_800D1C14;
    D_800A89D8 = D_800D1BD8;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7B48);

// PC: C_005EE9C2, allocate model
s32* func_800A7BF4(void) {
    u32* counter;
    Unk800D0554* base;
    s32 index;

    counter = &D_800A8A8C;
    index = *counter;
    base = D_800D0554;
    *counter = index + 1;
    return &base[index].unk0;
}

// PC: C_005EE9EC, read triangles
s32* func_800A7C20(s32 count) {
    s32* cursor;
    Unk800A8CCC* base;
    s32 index;

    cursor = &D_800A8A70;
    index = *cursor;
    *cursor = index + count;
    base = D_800A8CCC;
    return &base[index].unk0;
}

// PC: C_005EEA19, read quads
s32* func_800A7C54(s32 count) {
    s32* cursor;
    Unk800D1968* base;
    s32 index;

    cursor = &D_800D171C;
    index = *cursor;
    *cursor = index + count;
    base = D_800D1968;
    return &base[index].unk0;
}

// No PC counterpart; the port replaced the PSX double buffer with the DirectX driver
void func_800A7C88(void) {
    Unk800A7FAC* temp_s1;
    Unk800D1964* db;
    u_char* isbg;

    SetDefDrawEnv(&D_800AB898[0].draw, 0, 0, 0x140, 0xF0);
    SetDefDispEnv(&D_800AB898[0].disp, 0, 0xF0, 0x140, 0xF0);
    SetDefDrawEnv(&D_800AB898[1].draw, 0, 0xF0, 0x140, 0xF0);
    SetDefDispEnv(&D_800AB898[1].disp, 0, 0, 0x140, 0xF0);
    db = D_800AB898;
    D_800AB898[0].draw.isbg = 0;
    // Stored off the buffer base register; a direct field store folds to an absolute address.
    isbg = &db[1].draw.isbg;
    *isbg = 0;
    setRGB0(&D_800AB898[0].draw, 0, 0, 8);
    setRGB0(&D_800AB898[1].draw, 0, 0, 8);
    SetGeomOffset(0xA0, 0xA0);
    SetGeomScreen(0x100);
    SetDispMask(1);
    SetBackColor(0x80, 0x80, 0x80);
    SetFarColor(0, 0, 8);
    temp_s1 = &D_800AB898[0].unk4368;
    func_800A7E70(temp_s1);
    func_800A7E70(&D_800AB898[1].unk4368);
    func_800A7FAC(temp_s1);
    func_800A7FAC(&D_800AB898[1].unk4368);
    ClearOTagR((u_long*)&D_800AB898[0].unk70, LEN(D_800AB898[0].unk70));
    ClearOTagR((u_long*)&D_800AB898[1].unk70, LEN(D_800AB898[1].unk70));
    ClearOTagR((u_long*)&D_800AB898[0].unk4098, LEN(D_800AB898[0].unk4098));
    ClearOTagR((u_long*)&D_800AB898[1].unk4098, LEN(D_800AB898[1].unk4098));
    *D_800D1964 = &D_800AB898[0];
}

void func_800A7E1C(void) {
    ClearOTagR((u_long*)D_800D1964[0]->unk70, LEN(D_800D1964[0]->unk70));
    ClearOTagR((u_long*)D_800D1964[0]->unk4098, LEN(D_800D1964[0]->unk4098));
    func_800A7FAC(&D_800D1964[0]->unk4368);
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A7E70);

void func_800A7FAC(Unk800A7FAC* arg0) {
    arg0->unk0 = arg0->unk24;
    arg0->unk4 = arg0->unk38;
    arg0->unk8 = arg0->unk50;
    arg0->unkC = arg0->unkAF50;
    arg0->unk10 = arg0->unkB388;
    arg0->unk14 = arg0->unkB3A8;
    arg0->unk18 = arg0->unkE288;
    arg0->unk1C = arg0->unkE2B0;
    arg0->unk20 = arg0->unkE2E4;
}

// PC: C_005EF1C0, node module init
void func_800A8010(void) {
    Unk800EE1D4* a;
    Unk800EE1D4* b;
    s32 i;

    func_800A80A8(&D_800D16E4, 0);
    D_800D16E4.unk2C = 0;
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

// PC: C_005EF281, init node
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

// PC: C_005EF31E, allocate node (modelId, parent, x, y, z, rotation)
Unk800EE1D4* func_800A80F8(s16 arg0, s32 arg1, s32 arg2, s32 arg3, Unk800EE1D4* arg4, s32 arg5, s32 arg6, s32 arg7,
                           u16 arg8, u16 arg9, u16 arg10) {
    SVECTOR sp10;
    Unk800EE1D4* temp_s0;
    Unk800EE1D4* temp_v1;
    s16 temp_v0;

    temp_v0 = func_800A8238();
    temp_v1 = D_800A8CD0;
    temp_s0 = &temp_v1[temp_v0];
    func_800A8290(temp_s0, arg4);
    temp_s0->unk0 = D_800D1730[arg0];
    temp_s0->unk28 = arg0;
    temp_s0->unk2A = temp_v0;
    sp10.vx = arg8;
    sp10.vy = arg9;
    sp10.vz = arg10;
    RotMatrix(&sp10, &temp_s0->m);
    temp_s0->m.t[0] = arg5;
    temp_s0->m.t[1] = arg6;
    temp_s0->m.t[2] = arg7;
    return temp_s0;
}

// PC: C_005EF3BF, release node
void func_800A8204(Unk800EE1D4* arg0) {
    func_800A82F0(arg0);
    func_800A8264(arg0->unk2A);
}

// PC: C_005EF3E0, allocate node index
s16 func_800A8238(void) {
    s16* head;
    s16 result;

    head = &D_800D9944;
    result = *head;
    *head = D_800D1A40[result];

    return result;
}

// PC: C_005EF40C, release node index
void func_800A8264(s16 arg0) {
    s16* temp;
    s16* temp2;

    temp2 = &D_800D1A40[arg0];
    temp = &D_800D9944;
    *temp2 = *temp;
    *temp = arg0;
}

// PC: C_005EF42F, insert node at its parent's depth + 1
void func_800A8290(Unk800EE1D4* arg0, Unk800EE1D4* arg1) {
    Unk800EE1D4* temp_v0_2;
    Unk800EE1D4* temp_v1;
    s16 temp_v0;

    arg0->unk24 = arg1;
    temp_v0 = arg1->unk2C + 1;
    arg0->unk2C = temp_v0;
    temp_v1 = &D_800EE1D4[temp_v0];
    temp_v0_2 = temp_v1->unk30;
    arg0->unk30 = temp_v0_2;
    arg0->unk34 = temp_v0_2->unk34;
    temp_v1->unk30->unk34 = arg0;
    temp_v1->unk30 = arg0;
}

// PC: C_005EF49E, unlink node
void func_800A82F0(Unk800EE1D4* arg0) {
    arg0->unk30->unk34 = arg0->unk34;
    arg0->unk34->unk30 = arg0->unk30;
}
