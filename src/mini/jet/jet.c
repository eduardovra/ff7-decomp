//! PSYQ=3.3

// "PC:" comments source https://github.com/ergonomy-joe/ff7-coaster

#include "types.h"
#include <game.h>
#include <libetc.h>

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

// PC: t_coaster_ModelInfo
typedef struct {
    /* 0x00 */ s16 unk0;     // PC: wNumTri
    /* 0x02 */ s16 unk2;     // PC: wNumQua
    /* 0x04 */ SVECTOR unk4; // PC: f_04
    /* 0x0C */ SVECTOR unkC; // PC: f_0c
} Unk800A89D8;               // size: 0x14

// PC: t_coaster_Model
typedef struct {
    /* 0x00 */ s16 unk0; // PC: wNumPoly
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4; // PC: wNumTri
    /* 0x06 */ s16 unk6; // PC: wNumQua
    /* 0x08 */ s16 unk8;
    /* 0x0A */ char padA[2];
    /* 0x0C */ s32* unkC;  // PC: pTriangles
    /* 0x10 */ s32* unk10; // PC: pQuads
    /* 0x14 */ s16 unk14;  // PC: f_14
    /* 0x16 */ s16 unk16;  // PC: f_16
    /* 0x18 */ s16 unk18;  // PC: f_18
    /* 0x1A */ s16 unk1A;  // PC: f_1a
    /* 0x1C */ char pad1C[4];
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

// PC: t_coaster_LinkedList
typedef struct {
    /* 0x0 */ u16 unk0; // PC: wPrev
    /* 0x2 */ u16 unk2; // PC: wNext
} Unk800E2608;          // size: 0x4

// PC: t_coaster_GameObject
typedef struct {
    /* 0x00 */ VECTOR unk0; // PC: sPos
    /* 0x10 */ char pad10[0x18];
    /* 0x28 */ s32 unk28; // PC: f_028.dwType
    /* 0x2C */ s32 unk2C; // PC: f_028.dwIsHit
    /* 0x30 */ s32 unk30; // PC: f_028.dwModelId
    /* 0x34 */ char pad34[4];
    /* 0x38 */ s32 unk38; // PC: f_028.dwMustInit
    /* 0x3C */ char pad3C[0x6C];
    /* 0xA8 */ s32 unkA8; // PC: f_028.f_50[0xC]
    /* 0xAC */ s32 unkAC;
    /* 0xB0 */ char padB0[0x24];
    /* 0xD4 */ Unk800EE1D4* unkD4; // PC: pNode
    /* 0xD8 */ s16 unkD8;          // PC: wObjIndex
    /* 0xDA */ s16 unkDA;          // PC: wIsActive
    /* 0xDC */ char padDC[0x60];
} Unk800A4390; // size: 0x13C

extern RECT D_800A0000;
// .data, in ROM order. Sector and size pairs feed the four loads in func_800A2420
s32 D_800A8310 = 0x9D8;
u32 D_800A8314 = 0x28;
s32 D_800A8318 = 0x9D9;
u32 D_800A831C = 0x4DE8;
s32 D_800A8320 = 0x9E3;
u32 D_800A8324 = 0x44;
s32 D_800A8328 = 0x9E4;
u32 D_800A832C = 0xA7958;
s32 D_800A8330 = 0x7F;
s32 D_800A8334 = 0x7F;
s32 D_800A8338 = 0;
s32 D_800A833C = 0;
MATRIX D_800A8340 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}}; // PC: __009014B0
MATRIX D_800A8360 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}}; // PC: __009014D0
MATRIX D_800A8380 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}}; // PC: D_00C3F8A0 (world matrix)
SVECTOR D_800A83A0 = {0, 0, 0, 0};                                                 // world rotation
VECTOR D_800A83A8 = {0, 0, 0, 0};                                                  // PC: D_00C3F8D8 (view position)
VECTOR D_800A83B8 = {0, 0, 0, 0}; // PC: D_00C3F8E8 (starfield position)
VECTOR D_800A83C8 = {0, 0, 0, 0}; // PC: D_00C3F8F8
VECTOR D_800A83D8 = {0, 0, 0, 0}; // PC: D_00C3F908
s32 D_800A83E8[2] = {0, 0};

extern s32 D_800A892C;          // PC: sLNormal.vx
extern s32 D_800A8930;          // PC: sLNormal.vy
extern s32 D_800A8934;          // PC: sLNormal.vz
extern s32 D_800A893C;          // PC: sRNormal.vx
extern s32 D_800A8940;          // PC: sRNormal.vy
extern s32 D_800A8944;          // PC: sRNormal.vz
extern s32 D_800A8950;          // PC: dwLHelper
extern SVECTOR* D_800A8954;     // PC: D_00C3FB60 (current path)
extern s32 D_800A897C;          // PC: D_00C3F768 (speed)
extern s32 D_800A8968;          // PC: dwRHelper
extern s32 D_800A8984;          // PC: D_00C476E0 (current path length)
extern SVECTOR* D_800A8988;     // PC: D_00C3F8D0 (selected track path)
extern s32 D_800A898C;          // PC: D_00C3F91C (current object number)
extern s32 D_800A89D0;          // fog near
extern s32 D_800A89D4;          // fog far
extern Unk800A89D8* D_800A89D8; // PC: D_00C5D0E4 (model info stream)
extern u16 D_800A89DC;          // PC: D_00C503A4 (track list head)
extern s32 D_800A89E0;          // PC: D_00C476D8 (object stream index)
extern s32 D_800A8958;
extern u_long D_800A89E4;
extern s32 D_800A8A5C;          // PC: dwLDistance
extern u16 D_800A8A60;          // PC: D_00C5BF44 (bg triangle list head)
extern s32 D_800A8A64;          // PC: dwRDistance
extern s32 D_800A8A70;          // PC: D_00C5D0E0 (read triangles index)
extern Unk800EE1D4* D_800A8A74; // PC: D_00C3F880 (score node)
extern s16 D_800A8A88;
extern u32 D_800A8A8C;             // PC: D_00C5D0EC (allocated models)
extern Unk800EE1D4 D_800A8A90[10]; // PC: D_00C60320 (list heads per depth)
extern s32* D_800A8CC0;            // PC: D_00C3F898 (track offsets, stream 5)
extern Unk800A8CCC* D_800A8CCC;    // PC: D_00C5D0E8 (triangles stream)
extern s32 D_800A8CC8;
extern Unk800EE1D4 D_800A8CD0[0xC8]; // PC: D_00C5D590 (node pool)
extern s32 D_800AB890;               // PC: dwLNormalLength
extern Unk800D1964 D_800AB898[2];    // PC: Class_coaster_D8
extern s32 D_800D0550;               // PC: dwRNormalLength
extern Unk800D0554 D_800D0554[];     // PC: D_00C5BF60 (model pool)
extern s32 D_800D16D8;               // PC: D_00C3F76C (last shoot score)
extern u8 D_800D16DC;                // PC: D_00C3F760 (pause mode)
extern s32 D_800D16E0;
extern Unk800EE1D4 D_800D16E4; // PC: D_00C60150 (top node)
extern s32 D_800D171C;         // PC: D_00C5D320 (read quads index)
extern s32 D_800D1724;         // PC: D_00C3F894
extern s32 D_800D172C;
extern s32 D_800D1730[];           // PC: D_00C5D0F0 (model pointer table)
extern s8 D_800D1960;              // PC: D_00C3F890 (release mode)
extern Unk800D1964* D_800D1964[1]; // PC: D_00C3F888 (renderer)
extern void* D_800D196C;
extern Unk800D1968* D_800D1968; // PC: D_00C5BF58 (quads stream)
extern u16 D_800D1970[];        // PC: D_00C3FA80 (object index pool)
extern s16 D_800D1A40[0xC8];    // PC: D_00C60190 (node index pool)
extern u_long D_800D1BD4;
extern Unk800A89D8* D_800D1BD8; // PC: xbin stream 1 (model info)
extern void* D_800D1BDC;        // PC: xbin stream 2
extern void* D_800D1BE0;        // PC: xbin stream 3
extern u8* D_800D1BE4;          // PC: xbin stream 4 (track data)
extern s32* D_800D1BE8;         // PC: xbin stream 5 (track offsets)
extern s32* D_800D1BEC;         // PC: xbin stream 6
extern void* D_800D1BF0;        // PC: xbin stream 7
extern void* D_800D1BF4;        // PC: xbin stream 8
extern void* D_800D1BF8;        // PC: xbin stream 9
extern Unk800A8CCC* D_800D1BFC; // PC: xbin stream 0xA (triangles)
extern u8* D_800D1C00;          // PC: xbin stream 0xB (object paths)
extern s32* D_800D1C04;         // PC: xbin stream 0xC (path offsets)
extern s32* D_800D1C08;         // PC: xbin stream 0xD (path lengths)
extern Unk800D1968* D_800D1C14; // PC: xbin stream 0x10 (quads)
extern s8 D_800D1C4C;           // PC: D_00C3FA70 (shoot)
extern u16 D_800D1C50;          // PC: D_00C5BF30 (track element count)
extern SVECTOR* D_800D1C58;     // PC: D_00C3F874 (left track vectors)
extern u16 D_800D1C5C;          // PC: D_00C3FB50 (shoot power)
extern void* D_800D1C60;
extern u16 D_800D1C78;               // PC: D_00C5039C (bg triangle count)
extern s8 D_800D1C7C;                // PC: D_00C3FA74 (shoot repeat counter)
extern u16 D_800D1C80;               // PC: D_00C5BF38 (track list tail)
extern Unk800A4390 D_800D1C84;       // PC: D_00C3F930 (object being built)
extern Unk800A4390 D_800D1DC0[0x64]; // PC: D_00C3FB68 (object pool)
extern u16 D_800D9930;               // PC: D_00C503A8 (bg triangle list tail)
extern DR_MODE D_800D9934;
extern u16 D_800D9940;           // PC: D_00C3FA6C (next object index)
extern s16 D_800D9944;           // PC: D_00C60188 (next node index)
extern Unk800E2608 D_800D9948[]; // PC: D_00C476F0 (track list nodes)
extern s16 D_800E25EC;           // PC: D_00C3FB58 (cursor X)
extern s16 D_800E25F0;           // PC: D_00C3FB5C (cursor Y)
extern u8 D_800E25F4;
extern s8 D_800E25F8;
extern s32 D_800E25FC;
extern u8 D_800E2600;
extern s32* D_800E2604;        // PC: D_00C3F8C0
extern Unk800E2608 D_800E2608; // PC: D_00C503B0 (bg triangle list nodes)
extern void* D_800EE188;
extern SVECTOR* D_800EE194;        // PC: D_00C3F878 (right track vectors)
extern Unk800EE1D4 D_800EE1D4[10]; // PC: D_00C5D360 (list tails per depth)
extern SVECTOR* D_800EE424;        // PC: D_00C3F870 (track camera)
extern void* D_800EE428;
extern u16 D_800EE42C; // PC: D_00C3FB54 (active object count)

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
s16 func_800A40F4(Unk800A4390* arg0, s16 arg1);
u_long* func_800A8734(Unk800A8CCC* arg0, u_long* arg1, u_long** arg2, Unk800A8CCC* arg3);
u_long* func_800A882C(SVECTOR* arg0, u_long* arg1, u_long** arg2, SVECTOR* arg3);
Unk800D0554* func_800A7BF4(void);
s32* func_800A7C20(s32 count);
s32* func_800A7C54(s32 count);
void func_800A6BD8(Unk800A4390* arg0);
void func_800A12EC(void);
void func_800A13AC(void);
void func_800A1450();
void func_800A1A64();
void func_800A1B64(Unk800D1964* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_800A1CD8(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_800A1F18(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, s32 arg9);
void func_800A2058();
void func_800A2214();
void func_800A2860(void);
void func_800A2B78(void);
void func_800A2C50(s32 arg0);
void func_800A2E38();
void func_800A35DC(s32 arg0);
void func_800A372C(s32 arg0);
void func_800A46E8(Unk800D1964* arg0);
Unk800EE1D4* func_800A80F8(s16 arg0, s32 arg1, s32 arg2, s32 arg3, Unk800EE1D4* arg4, s32 arg5, s32 arg6, s32 arg7,
                           u16 arg8, u16 arg9, u16 arg10);

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", MINI_Jet);
#else
// Off by instruction scheduling only: the load-delay slot in the prologue and
// the address form of the D_800E25FC store.
u16 MINI_Jet(void) {
    volatile s32 dummy;
    Unk800D1964** db;
    Unk800D1964* var_a2;
    s32* speed;
    SVECTOR** path;
    s32 temp_s0;

    func_800A2214();
    SetDrawMode(&D_800D9934, 0, 1, GetTPage(1, 1, 0x300, 0) & 0xFFFF, NULL);
    db = D_800D1964;
    speed = &D_800A897C;
    D_800EE424 = D_800D1BF0;
    func_800A2DE4(0, 0);
    path = &D_800A8988;
    D_800D1C58 = *path;
    func_800A2DE4(1, 0);
    D_800EE194 = *path;
    temp_s0 = 0x20;
    func_800A2860();
    SetFogNearFar(D_800A89D0, D_800A89D4, 0x100);
    D_800A8A74 = func_800A80F8(0x1E, 0, 0, 1, &D_800D16E4, 0x4B0, 0x32, 0xBB8, 0, 0x3E8, 0);
    // A loop keyword makes gcc duplicate the exit test and hoist loop constants;
    // the target has neither.
loop:
    if ((D_800D16E0 * 4) > (D_800D1724 - 0x10) || D_800E2600 == 1) {
        goto done;
    }
    {
        func_800A2E38();
        if (D_800D16DC == 0) {
            func_800A2C50(9);
            func_800A35DC(*speed);
            func_800A1450();
            func_800A13AC();
            func_800A12EC();
            func_800A1B64(db[0], D_800A8A88, 5, 0x28, 0);
            func_800A372C(*speed);
            func_800A46E8(db[0]);
            func_800A1CD8(D_800D16D8, 0xF4, 0xC8, 0, 0);
            func_800A1F18(7, 0xCC, 0xC8, 0x27, 0x11, 0, 0, 0x27, 0x11, 0);
            func_800A1F18(0xB, 0x12, 0x56, 0xC, 0x8C, 0, 0x70, 0xC, 0x8C, 0);
            func_800A1A64();
            if (*speed < 0x4000) {
                D_800A8338 = 0;
            } else {
                D_800A8338 = 0x7F;
            }
        } else {
            func_800A1F18(9, 0xCA, 0xC0, 0x60, temp_s0, 0, 0x50, 0x60, temp_s0, 0);
            D_800A8338 = 0;
            D_800A833C = 0;
        }
        func_800A2B78();
        func_800A2058();
        func_800A1F18(0xA, 0xC8, 0xC0, 0x6F, 0x1F, 0, 0x30, 0x70, temp_s0, 0);
        DrawSync(0);
        VSync(0);
        ResetGraph(1);
        PutDrawEnv(&db[0]->draw);
        PutDispEnv(&db[0]->disp);
        ClearImage(&db[0]->draw.clip, 0, 0, 0);
        if (D_800E25F4 != 0) {
            DrawOTag((u_long*)&db[0]->unk70[0xFFF]);
            DrawOTag((u_long*)&db[0]->unk4098[0xB3]);
        }
        var_a2 = D_800AB898;
        D_800E25FC = 0;
        if (db[0] == var_a2) {
            var_a2++;
        }
        db[0] = var_a2;
        ClearOTagR((u_long*)var_a2->unk70, 0x1000);
        ClearOTagR((u_long*)db[0]->unk4098, 0xB4);
        func_800A7FAC(&db[0]->unk4368);
        goto loop;
    }
done:
    *D_8009A000 = 0xB8;
    D_8009A004 = 0;
    SystemAkaoExecute();
    return D_800D16D8;
}
#endif

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0874);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0D78);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1198);

// Draw every background triangle on the draw list, front to back.
// PC: C_005E9E7E, render every background triangle on the draw list
void func_800A12EC(void) {
    Unk800E2608* list;
    Unk800A8CCC* tris;
    u16 triId;
    u_long* ot;

    ot = D_800D1964[0]->unk4368.unk8;
    tris = D_800D1BFC;
    if (D_800D1C78) {
        triId = D_800A8A60;
        list = &D_800E2608;
        do {
            ot = func_800A8734(&tris[triId], ot, D_800D1964[0]->unk70, &tris[triId]);
            triId = list[triId].unk2;
        } while (triId != 0xFFFF);
    }
    D_800D1964[0]->unk4368.unk8 = ot;
}

// Draw every track element on the draw list, front to back.
// PC: C_005E9F33, render every track element on the draw list
void func_800A13AC(void) {
    Unk800E2608* list;
    SVECTOR* left;
    SVECTOR* right;
    u16 trackId;
    u_long* ot;

    trackId = D_800A89DC;
    ot = D_800D1964[0]->unk4368.unk14;
    list = D_800D9948;
// This needs to be refactored to get rid of the goto.
loop:
    left = D_800D1C58;
    right = D_800EE194;
    ot = func_800A882C(&left[trackId], ot, D_800D1964[0]->unk70, &right[trackId]);
    trackId = list[trackId].unk2;
    if (trackId != 0xFFFF) {
        goto loop;
    }
    D_800D1964[0]->unk4368.unk14 = ot;
}

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

void func_800A2860(void) {
    D_8009A000[0] = 0x10;
    D_8009A004 = D_800D1BD4;
    SystemAkaoExecute();
    D_8009A000[0] = 0xC0;
    D_8009A004 = 0x7F;
    SystemAkaoExecute();
    D_8009A000[0] = 0xB8;
    D_8009A004 = 0x7F;
    SystemAkaoExecute();
    D_8009A000[0] = 0xBC;
    D_8009A004 = 0;
    SystemAkaoExecute();
    D_8009A000[0] = 0xA2;
    D_8009A004 = 0;
    SystemAkaoExecute();
    D_8009A000[0] = 0x2A;
    D_8009A004 = 0x40;
    D_8009A008 = 0x177;
    SystemAkaoExecute();
}

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

void func_800A2AA0(s32 arg0) {
    s32* temp;
    s32 temp_s0;

    temp = &D_800A8958;
    if (*temp == 0) {
        if (arg0 & 0xFF) {
            *D_8009A000 = 0x2B;
            D_8009A004 = 0x40;
            D_8009A008 = 0x22B;
            SystemAkaoExecute();
        } else {
            *D_8009A000 = 0x2B;
            D_8009A004 = 0x40;
            D_8009A008 = 0;
            SystemAkaoExecute();
            D_800A8958 = 0;
            return;
        }
    }
    temp_s0 = arg0 & 0xFF;
    if (temp_s0) {
        D_800A833C = temp_s0;
        *D_8009A000 = 0xB3;
        D_8009A004 = temp_s0;
        SystemAkaoExecute();
        *temp = temp_s0;
    } else {
        *D_8009A000 = 0x2B;
        D_8009A004 = 0x40;
        D_8009A008 = 0;
        SystemAkaoExecute();
        D_800A8958 = 0;
    }
}

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
    D_800A83C8.vy = -0x1B76;
    D_800A83C8.vx = 0;
    D_800A83C8.vz = 0xC8;
    D_800E2604 = D_800D1BEC;
    D_800A8CC0 = D_800D1BE8;
    func_800A2DE4(0, 3);
    D_800D1960 = 1;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2C50);

// PC: C_005EAAF3, select track data from stream 4
void func_800A2DE4(s32 arg0, s32 arg1) {
    s32 elem;
    u8* base;

    elem = D_800A8CC0[arg0 & 0xFF];
    base = D_800D1BE4;
    D_800A8988 = (SVECTOR*)(base + elem);
    D_800D1724 = *D_800E2604;
}

void func_800A2E30(void) {}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2E38);

// Reset both draw lists and the object streams for a new run.
void func_800A334C(void) {
    Unk800E2608* list;
    s32 i;

    D_800D172C = 0xFFFE;
    D_800A8CC8 = 0;
    D_800D1C60 = D_800D1BF4;
    D_800EE428 = D_800D1BF8;
    D_800D196C = D_800D1BDC;
    D_800EE188 = D_800D1BE0;
    list = &D_800E2608;
    for (i = 0; i < 0x2EE0; i++) {
        list[i].unk0 = 0xFFFF;
        list[i].unk2 = 0xFFFF;
    }
    D_800D1C78 = 0;
    list = D_800D9948;
    for (i = 0; i < 0x2328; i++) {
        list[i].unk0 = 0xFFFF;
        list[i].unk2 = 0xFFFF;
    }
    D_800D1C50 = 0;
    D_800E25F8 = 1;
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3414);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A35DC);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A372C);

// PC: C_005EDE71, append a background triangle to the draw list
void func_800A385C(u16 arg0) {
    Unk800E2608* elem;
    u16* pCount;
    u16* pTail;
    u16 newCount;
    u16 count;
    u16 tail;

    elem = (&D_800E2608) + arg0;
    pCount = &D_800D1C78;
    count = *pCount;
    if (count == 0) {
        D_800A8A60 = arg0;
        D_800D9930 = arg0;
        *pCount = 1;
    } else {
        pTail = &D_800D9930;
        tail = *pTail;
        newCount = count + 1;
        elem->unk0 = tail;
        (&D_800E2608)[tail].unk2 = arg0;
        *pTail = arg0;
        *pCount = newCount;
    }
}

// PC: C_005EDF18, unlink a background triangle from the draw list
void func_800A38D4(u16 arg0) {
    Unk800E2608* list;
    Unk800E2608* node;
    u16* pCount;
    u16 prev;
    u16 next;

    node = (&D_800E2608) + arg0;
    prev = node->unk0;
    next = node->unk2;
    list = &D_800E2608;
    if (prev != 0xFFFF) {
        list[prev].unk2 = next;
    } else {
        D_800A8A60 = next;
    }
    if (next != 0xFFFF) {
        (&D_800E2608)[next].unk0 = prev;
    } else {
        D_800D9930 = prev;
    }
    {
        Unk800E2608* self;

        self = &D_800E2608;
        self[arg0].unk0 = 0xFFFF;
        self[arg0].unk2 = 0xFFFF;
    }
    pCount = &D_800D1C78;
    *pCount = *pCount - 1;
}

// PC: C_005EDFE7, append a track element to the draw list
void func_800A3980(u16 arg0) {
    u16* pCount;
    u16 count;

    pCount = &D_800D1C50;
    count = *pCount;
    if (count == 0) {
        Unk800E2608* list;
        Unk800E2608* node;

        list = D_800D9948;
        node = &list[arg0];
        node->unk0 = 0xFFFF;
        node->unk2 = 0xFFFF;
        D_800A89DC = arg0;
        D_800D1C80 = arg0;
        *pCount = 1;
    } else {
        Unk800E2608* list;
        Unk800E2608* node;
        u16* pTail;
        u16 newCount;
        u16 tail;

        newCount = count + 1;
        list = D_800D9948;
        node = &list[arg0];
        pTail = &D_800D1C80;
        tail = *pTail;
        node->unk0 = tail;
        node->unk2 = 0xFFFF;
        list[tail].unk2 = arg0;
        *pTail = arg0;
        *pCount = newCount;
    }
}

// PC: C_005EE09A, unlink a track element from the draw list
void func_800A3A20(u16 arg0) {
    Unk800E2608* list;
    Unk800E2608* node;
    u16* pCount;
    u16 prev;
    u16 next;

    node = &D_800D9948[arg0];
    prev = node->unk0;
    next = node->unk2;
    list = D_800D9948;
    if (prev != 0xFFFF) {
        list[prev].unk2 = next;
    } else {
        D_800A89DC = next;
    }
    if (next != 0xFFFF) {
        D_800D9948[next].unk0 = prev;
    } else {
        D_800D1C80 = prev;
    }
    pCount = &D_800D1C50;
    *pCount = *pCount - 1;
}

// PC: C_005EAB70, init the game object pool
void func_800A3AAC(void) {
    Unk800A4390* obj;
    s32 i;

    obj = D_800D1DC0;
    for (i = 0; i < 0x64; i++) {
        obj[i].unkD8 = -1;
        obj[i].unkDA = 0;
    }
    D_800D9940 = 0;
    for (i = 0; i < 0x64; i++) {
        D_800D1970[i] = i + 1;
    }
    D_800D1C5C = 0x80;
    D_800D1C4C = 0;
    D_800D1C7C = 0;
    D_800E25EC = 0xA0;
    D_800E25F0 = 0x78;
    D_800A898C = 0;
    D_800A89E0 = 0;
    D_800EE42C = 0;
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3B58);
#else
// PC: C_005EAC30, start path (mode 0: objects, mode 1: player car).
// Off by the operand order of the final add only.
void func_800A3B58(u8 pathIndex, u8 mode) {
    s32 offset;
    s32* offsets;
    s32* lengths;

    if (mode == 0) {
        offsets = D_800D1C04;
        lengths = D_800D1C08;
        offset = offsets[pathIndex];
        D_800A8984 = lengths[pathIndex];
        D_800A8954 = (SVECTOR*)(D_800D1C00 + offset);
    }
    if (mode == 1) {
        offsets = D_800D1BE8;
        lengths = D_800D1BEC;
        offset = offsets[pathIndex];
        D_800A8984 = lengths[pathIndex];
        D_800A8954 = (SVECTOR*)(D_800D1BE4 + offset);
    }
}
#endif

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

// PC: C_005EB507, create an object at a position
void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    setVector(&D_800D1C84.unk0, arg0, arg1, arg2);
    D_800D1C84.unk28 = arg3;
    D_800D1C84.unk38 = 1;
    D_800D1C84.unk30 = arg4;
    D_800D1C84.unk2C = 0;
    func_800A40F4(&D_800D1C84, 0);
}

// PC: C_005EB566, create an object at a position, clearing f_50[0xC]
void func_800A4650(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    setVector(&D_800D1C84.unk0, arg0, arg1, arg2);
    D_800D1C84.unk28 = arg3;
    D_800D1C84.unk38 = 1;
    D_800D1C84.unk30 = arg4;
    D_800D1C84.unk2C = 0;
    D_800D1C84.unkA8 = 0;
    func_800A40F4(&D_800D1C84, 0);
}

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A46E8);

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6B08);
#else
// Spawn an object at this one's position once its timer runs out.
// Off by the register allocation of arg0 and the object base only.
void func_800A6B08(Unk800A4390* arg0) {
    u8 amount;
    s32 x;
    s32 y;
    s32 z;

    amount = D_800D1C5C >> 5;
    if (amount == 0) {
        amount = 1;
    }
    arg0->unkAC -= amount;
    if (arg0->unkAC < 0) {
        func_800A6BD8(arg0);
        return;
    }
    x = arg0->unk0.vx;
    y = arg0->unk0.vy;
    z = arg0->unk0.vz;
    D_800D1C84.unk28 = 0xCA;
    D_800D1C84.unk38 = 1;
    D_800D1C84.unk30 = 0x3F;
    D_800D1C84.unk2C = 0;
    D_800D1C84.unkA8 = 0;
    setVector(&D_800D1C84.unk0, (s16)x, (s16)y, (s16)z);
    func_800A40F4(&D_800D1C84, 0);
}
#endif

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6BD8);

INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A70D4);

// PC: C_005EECB5, is a point inside both frustum planes
s32 func_800A7414(VECTOR* arg0) {
    s32 hsLeft;
    s32 rightOk;
    s32 leftOk;
    s32 hsRight;
    s32 lx;
    s32 ly;
    s32 lz;
    s32 rx;
    s32 ry;
    s32 rz;

    leftOk = 0;
    rightOk = 0;
    lx = D_800A892C;
    ly = D_800A8930;
    lz = D_800A8934;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + D_800A8A5C;
    rx = D_800A893C;
    ry = D_800A8940;
    rz = D_800A8944;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + D_800A8A64;
    if (hsLeft > 0 && D_800A8950 > 0) {
        leftOk = 1;
    }
    if (hsLeft < 0 && D_800A8950 < 0) {
        leftOk = 1;
    }
    if (hsRight > 0 && D_800A8968 > 0) {
        rightOk = 1;
    }
    if (hsRight < 0 && D_800A8968 < 0) {
        rightOk = 1;
    }
    return leftOk & rightOk;
}

// PC: __005EEDAE, is a point inside both frustum planes
s32 func_800A7544(SVECTOR* arg0) {
    s32 hsLeft;
    s32 rightOk;
    s32 leftOk;
    s32 hsRight;
    s32 lx;
    s32 ly;
    s32 lz;
    s32 rx;
    s32 ry;
    s32 rz;

    leftOk = 0;
    rightOk = 0;
    lx = D_800A892C;
    ly = D_800A8930;
    lz = D_800A8934;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + D_800A8A5C;
    rx = D_800A893C;
    ry = D_800A8940;
    rz = D_800A8944;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + D_800A8A64;
    if (hsLeft > 0 && D_800A8950 > 0) {
        leftOk = 1;
    }
    if (hsLeft < 0 && D_800A8950 < 0) {
        leftOk = 1;
    }
    if (hsRight > 0 && D_800A8968 > 0) {
        rightOk = 1;
    }
    if (hsRight < 0 && D_800A8968 < 0) {
        rightOk = 1;
    }
    return leftOk & rightOk;
}

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

// PC: __005EEF27, is a sphere inside both frustum planes
s32 func_800A7730(VECTOR* arg0, s16 arg1) {
    s32 leftOk;
    s32 hsLeft;
    s32 rightOk;
    s32 hsRight;
    s32 planeDistance;
    s32 lx;
    s32 ly;
    s32 lz;
    s32 rx;
    s32 ry;
    s32 rz;
    s32 len;

    leftOk = 0;
    rightOk = 0;
    lx = D_800A892C;
    ly = D_800A8930;
    lz = D_800A8934;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + D_800A8A5C;
    if (D_800A8950 > 0 && hsLeft >= 0) {
        leftOk = 1;
    }
    if (D_800A8950 < 0 && hsLeft <= 0) {
        leftOk = 1;
    }
    if (leftOk == 0) {
        len = D_800AB890;
        planeDistance = ((hsLeft < 0) ? -hsLeft : hsLeft) / len;
        if (planeDistance < arg1) {
            leftOk = 1;
        }
    }
    rx = D_800A893C;
    ry = D_800A8940;
    rz = D_800A8944;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + D_800A8A64;
    if (D_800A8968 > 0 && hsRight >= 0) {
        rightOk = 1;
    }
    if (D_800A8968 < 0 && hsRight <= 0) {
        rightOk = 1;
    }
    if (rightOk == 0) {
        len = D_800D0550;
        planeDistance = ((hsRight < 0) ? -hsRight : hsRight) / len;
        if (planeDistance < arg1) {
            rightOk = 1;
        }
    }
    return leftOk & rightOk;
}

// PC: __005EF071, sphere test against the left frustum plane
s32 func_800A7928(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 hs;
    s32 ok;
    s32 len;

    a = D_800A892C;
    b = D_800A8930;
    c = D_800A8934;
    ok = 0;
    hs = (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + D_800A8A5C;
    if (D_800A8950 > 0 && hs >= 0) {
        ok = 1;
    }
    if (D_800A8950 < 0 && hs <= 0) {
        ok = 1;
    }
    if (ok == 0) {
        len = D_800AB890;
        if (hs < 0) {
            hs = -hs;
        }
        if (hs / len < arg3) {
            ok = 1;
        }
    }
    return ok;
}

// PC: __005EF114, sphere test against the right frustum plane
s32 func_800A7A10(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 hs;
    s32 ok;
    s32 len;

    a = D_800A893C;
    b = D_800A8940;
    c = D_800A8944;
    ok = 0;
    hs = (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + D_800A8A64;
    if (D_800A8968 > 0 && hs >= 0) {
        ok = 1;
    }
    if (D_800A8968 < 0 && hs <= 0) {
        ok = 1;
    }
    if (ok == 0) {
        len = D_800D0550;
        if (hs < 0) {
            hs = -hs;
        }
        if (hs / len < arg3) {
            ok = 1;
        }
    }
    return ok;
}

// PC: C_005EE7F0, model module init
void func_800A7AF8(void) {
    D_800A8A70 = 0;
    D_800D171C = 0;
    D_800A8A8C = 0;
    D_800A8CCC = D_800D1BFC;
    D_800D1968 = D_800D1C14;
    D_800A89D8 = D_800D1BD8;
}

// PC: C_005EE8CF, build a model from its info entry
Unk800D0554* func_800A7B48(s32 arg0) {
    Unk800D0554* model;
    Unk800A89D8* info;
    s32 numTri;
    s32 numQua;

    model = func_800A7BF4();
    info = &D_800A89D8[arg0];
    numTri = info->unk0;
    numQua = info->unk2;
    model->unk16 = info->unk4.vx;
    model->unk14 = info->unkC.vx;
    model->unk1A = info->unk4.vz;
    model->unk18 = info->unkC.vz;
    model->unk2 = 0;
    model->unk4 = numTri;
    model->unk6 = numQua;
    model->unk8 = 0;
    model->unk0 = numTri + numQua;
    model->unkC = func_800A7C20(numTri);
    model->unk10 = func_800A7C54(numQua);
    return model;
}

// PC: C_005EE9C2, allocate model
Unk800D0554* func_800A7BF4(void) {
    u32* counter;
    Unk800D0554* base;
    s32 index;

    counter = &D_800A8A8C;
    index = *counter;
    base = D_800D0554;
    *counter = index + 1;
    return &base[index];
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
    setVector(&sp10, arg8, arg9, arg10);
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
