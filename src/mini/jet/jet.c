//! PSYQ=3.3

// "PC:" comments source https://github.com/ergonomy-joe/ff7-coaster

#include "types.h"
#include <game.h>
#include <libetc.h>
#include <libc.h>

// Nine write cursors, each reset to the start of its own buffer below.
typedef struct {
    /* 0x0000 */ void* unk0;
    /* 0x0004 */ void* unk4;
    /* 0x0008 */ void* unk8;
    /* 0x000C */ void* unkC;
    /* 0x0010 */ void* unk10;
    /* 0x0014 */ void* unk14;
    /* 0x0018 */ void* unk18;
    /* 0x001C */ void* unk1C;
    /* 0x0020 */ void* unk20;
    /* 0x0024 */ POLY_F3 unk24[1];
    /* 0x0038 */ POLY_F4 unk38[1];
    /* 0x0050 */ POLY_G3 unk50[0x640];
    /* 0xAF50 */ POLY_G4 unkAF50[0x1E];
    /* 0xB388 */ POLY_FT3 unkB388[1];
    /* 0xB3A8 */ POLY_FT4 unkB3A8[0x12C];
    /* 0xE288 */ POLY_GT3 unkE288[1];
    /* 0xE2B0 */ POLY_GT4 unkE2B0[1];
    /* 0xE2E4 */ LINE_F2 unkE2E4[1];
} Unk800A7FAC; // size: 0xE2F4

// Offsets 0x00 and 0x5C are fixed by SetDefDrawEnv/SetDefDispEnv; both
// tables are sized by their ClearOTagR calls. PC: Class_coaster_D8
typedef struct {
    /* 0x0000 */ DRAWENV draw;
    /* 0x005C */ DISPENV disp;
    /* 0x0070 */ u_long unk70[0x1000];
    /* 0x4070 */ u_long unk4070[10];
    /* 0x4098 */ u_long unk4098[0xB4];
    /* 0x4368 */ Unk800A7FAC unk4368;
} Unk800D1964; // size: 0x1265C

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

// Doubly linked list node, chained by func_800A8010 with a 0x38 stride.
// PC: t_coaster_Node
typedef struct Unk800EE1D4 {
    /* 0x00 */ Unk800D0554* unk0;         // PC: pModel
    /* 0x04 */ MATRIX m;                  // PC: sMatrixWorld
    /* 0x24 */ struct Unk800EE1D4* unk24; // PC: pParentNode
    /* 0x28 */ s16 unk28;                 // PC: wModelId
    /* 0x2A */ s16 unk2A;                 // PC: wIndex
    /* 0x2C */ u16 unk2C;                 // PC: wDepth
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ struct Unk800EE1D4* unk30; // PC: pPrev
    /* 0x34 */ struct Unk800EE1D4* unk34; // PC: pNext
} Unk800EE1D4;                            // size: 0x38

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

// One scheduled object spawn, read from xbin stream 0xE.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ char pad2[2];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ char pad6[2];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10[0x14];
} Unk800D1C0C; // size: 0x60

// Argument block for the GTE renderers in jet_gte.s.
typedef struct {
    /* 0x0 */ s32* tris;
    /* 0x4 */ u_long* prim;
    /* 0x8 */ u_long* ot;
    /* 0xC */ Unk800D0554* model;
} Unk800A8604; // size: 0x10

// PC: t_coaster_LinkedList
typedef struct {
    /* 0x0 */ u16 unk0; // PC: wPrev
    /* 0x2 */ u16 unk2; // PC: wNext
} Unk800E2608;          // size: 0x4

// The behaviour state an object's type handler drives. PC: f_028
typedef struct {
    /* 0x00 */ s32 unk0; // PC: dwType
    /* 0x04 */ s32 unk4; // PC: dwIsHit
    /* 0x08 */ s32 unk8; // PC: dwModelId
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10; // PC: dwMustInit
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ char pad20[8];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ char pad38[0x18];
    /* 0x50 */ s32 unk50[0x14]; // PC: f_50
} Unk800D1CAC;                  // size: 0xA0

// PC: t_coaster_GameObject
typedef struct {
    /* 0x00 */ VECTOR unk0; // PC: sPos
    /* 0x10 */ char pad10[8];
    /* 0x18 */ SVECTOR unk18; // spawn rotation
    /* 0x20 */ char pad20[8];
    /* 0x28 */ Unk800D1CAC unk28; // PC: f_028
    /* 0xC8 */ s32 unkC8;         // the object path's length
    /* 0xCC */ SVECTOR* unkCC;    // the object path itself
    /* 0xD0 */ char padD0[4];
    /* 0xD4 */ Unk800EE1D4* unkD4; // PC: pNode
    /* 0xD8 */ s16 unkD8;          // PC: wObjIndex
    /* 0xDA */ s16 unkDA;          // PC: wIsActive
    /* 0xDC */ SVECTOR unkDC[6];   // the model bounding box's six face centres
    /* 0x10C */ char pad10C[0x10];
    /* 0x11C */ u_long unk11C[6]; // the same six points projected to the screen
    /* 0x134 */ char pad134[8];
} Unk800A4390; // size: 0x13C

extern RECT D_800A0000;
extern u8 D_800A0008;             // the rotation order the object matrices use
extern Unk800D0554* D_800D189C[]; // the models the animated objects switch between
// The four view frustum corner rays at the projection distance, screen order.
extern VECTOR D_800A0410; // bottom left
extern VECTOR D_800A0420; // bottom right
extern VECTOR D_800A0430; // top left
extern VECTOR D_800A0440; // top right
// .data in ROM order, the sector and size pairs feeding func_800A2420's loads.
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

extern u8 D_800A8928;
extern s32 D_800A8A84;
extern void* D_800A891C;
extern void* D_800A8920;
extern s32 D_800A892C; // PC: sLNormal.vx
extern s32 D_800A8930; // PC: sLNormal.vy
extern s32 D_800A8934; // PC: sLNormal.vz
extern s32 D_800A893C; // PC: sRNormal.vx
extern s32 D_800A8940; // PC: sRNormal.vy
extern s32 D_800A8944; // PC: sRNormal.vz
extern s32 D_800A894C; // PC: D_00C503A0 (track segments stepped this frame)
extern s32 D_800A8950; // PC: dwLHelper
extern s16 D_800A895C;
extern s16 D_800A8960;
extern s16 D_800A8964;
extern s16 D_800A896C;
extern s16 D_800A8970;
extern s16 D_800A8974;
extern s16 D_800A8978;
extern s16 D_800A8980;
extern SVECTOR* D_800A8954;     // PC: D_00C3FB60 (current path)
extern s32 D_800A897C;          // PC: D_00C3F768 (speed)
extern s32 D_800A8968;          // PC: dwRHelper
extern s32 D_800A8984;          // PC: D_00C476E0 (current path length)
extern SVECTOR* D_800A8988;     // PC: D_00C3F8D0 (selected track path)
extern s32 D_800A898C;          // PC: D_00C3F91C (current object number)
extern u16 D_800A8990[];        // sprite tpage table
extern s32 D_800A89D0;          // fog near
extern s32 D_800A89D4;          // fog far
extern Unk800A89D8* D_800A89D8; // PC: D_00C5D0E4 (model info stream)
extern u16 D_800A89DC;          // PC: D_00C503A4 (track list head)
extern s32 D_800A89E0;          // PC: D_00C476D8 (object stream index)
extern s32 D_800A8958;
extern u_long D_800A89E4; // loaded TIM address table
extern u16 D_800A8A68;    // background clut
extern s32 D_800A8A5C;    // PC: dwLDistance
extern u16 D_800A8A60;    // PC: D_00C5BF44 (bg triangle list head)
extern s32 D_800A8A64;    // PC: dwRDistance
extern s32 D_800A8A6C;    // pad direction code, 1..9 keypad layout
extern s32 D_800A8A70;    // PC: D_00C5D0E0 (read triangles index)
extern s32 D_800A8A7C;
extern s32 D_800A8A80; // frames R1 has been held
extern s16 D_800A89CC;
extern s16 D_800A8CC4;
extern Unk800EE1D4* D_800A8A74[1]; // PC: D_00C3F880 (score node)
extern s16 D_800A8A88;
extern u32 D_800A8A8C;             // PC: D_00C5D0EC (allocated models)
extern Unk800EE1D4 D_800A8A90[10]; // PC: D_00C60320 (list heads per depth)
extern s32* D_800A8CC0;            // PC: D_00C3F898 (track offsets, stream 5)
extern Unk800A8CCC* D_800A8CCC;    // PC: D_00C5D0E8 (triangles stream)
extern u32 D_800A8CC8;
extern Unk800EE1D4 D_800A8CD0[0xC8]; // PC: D_00C5D590 (node pool)
extern u16 D_800AB894;               // background tpage
extern s32 D_800AB890;               // PC: dwLNormalLength
extern Unk800D1964 D_800AB898[2];    // PC: Class_coaster_D8
extern s32 D_800D0550;               // PC: dwRNormalLength
extern Unk800D0554 D_800D0554[];     // PC: D_00C5BF60 (model pool)
extern s32 D_800D16D8;               // PC: D_00C3F76C (last shoot score)
extern u8 D_800D16DC;                // PC: D_00C3F760 (pause mode)
extern void* D_800D16D4;
extern s32 D_800D16E0;
extern Unk800EE1D4 D_800D16E4; // PC: D_00C60150 (top node)
extern s32 D_800D171C;         // PC: D_00C5D320 (read quads index)
extern u8 D_800D1720;          // laser beam texture scroll
extern s32 D_800D1724;         // PC: D_00C3F894
extern u32 D_800D172C;
extern Unk800D0554* D_800D1730[];  // PC: D_00C5D0F0 (model pointer table)
extern u8 D_800D1960;              // PC: D_00C3F890 (release mode)
extern Unk800D1964* D_800D1964[1]; // PC: D_00C3F888 (renderer)
extern u16* D_800D196C;
extern Unk800D1968* D_800D1968; // PC: D_00C5BF58 (quads stream)
extern u16 D_800D1970[];        // PC: D_00C3FA80 (object index pool)
extern s16 D_800D1A40[0xC8];    // PC: D_00C60190 (node index pool)
extern void* D_800D1A38;
extern void* D_800D1A3C;
extern Unk800D0554* D_800D186C; // shadow triangles
extern MATRIX* D_800D1728;      // view matrix
extern MATRIX* D_800D1BD0;      // world matrix
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
extern Unk800D1C0C* D_800D1C0C; // PC: xbin stream 0xE (object spawns)
extern u8* D_800D1C10;          // PC: xbin stream 0xF (spawns per segment)
extern Unk800D1968* D_800D1C14; // PC: xbin stream 0x10 (quads)
extern u8 D_800D1C4C;           // PC: D_00C3FA70 (shoot)
extern u16 D_800D1C50;          // PC: D_00C5BF30 (track element count)
extern s32 D_800D1C54;          // PC: D_00C3F764 (camera path position)
extern SVECTOR* D_800D1C58;     // PC: D_00C3F874 (left track vectors)
extern u16 D_800D1C5C;          // PC: D_00C3FB50 (shoot power)
extern u16* D_800D1C60;
extern u16 D_800D1C78;               // PC: D_00C5039C (bg triangle count)
extern u8 D_800D1C7C;                // PC: D_00C3FA74 (shoot repeat counter)
extern u16 D_800D1C80;               // PC: D_00C5BF38 (track list tail)
extern Unk800A4390 D_800D1C84;       // PC: D_00C3F930 (object being built)
extern Unk800A4390 D_800D1DC0[0x64]; // PC: D_00C3FB68 (object pool)
extern u16 D_800D9930;               // PC: D_00C503A8 (bg triangle list tail)
extern DR_MODE D_800D9934;
extern u16 D_800D9940;           // PC: D_00C3FA6C (next object index)
extern s16 D_800D9944;           // PC: D_00C60188 (next node index)
extern Unk800E2608 D_800D9948[]; // PC: D_00C476F0 (track list nodes)
extern u16 D_800E25EC;           // PC: D_00C3FB58 (cursor X)
extern u16 D_800E25F0;           // PC: D_00C3FB5C (cursor Y)
extern u8 D_800E25E8;
extern u8 D_800E25F4;
extern u8 D_800E25F8;
extern s32 D_800E25FC;
extern u8 D_800E2600;
extern s32* D_800E2604;        // PC: D_00C3F8C0
extern Unk800E2608 D_800E2608; // PC: D_00C503B0 (bg triangle list nodes)
extern u16* D_800EE188;
extern SVECTOR D_800EE18C;
extern u16 D_800EE198[];           // sprite clut table
extern SVECTOR* D_800EE194;        // PC: D_00C3F878 (right track vectors)
extern Unk800EE1D4 D_800EE1D4[10]; // PC: D_00C5D360 (list tails per depth)
extern MATRIX D_800EE404;          // camera rotation
extern SVECTOR* D_800EE424;        // PC: D_00C3F870 (track camera)
extern u16* D_800EE428;
extern u16 D_800EE42C; // PC: D_00C3FB54 (active object count)
extern void* D_80110BB8;

void func_800A7E70(Unk800A7FAC* arg0);
void func_800A7FAC(Unk800A7FAC* arg0);
void func_800A80A8(Unk800EE1D4* arg0, s16 arg1);
void func_800A8264(s16);
void func_800A82F0(Unk800EE1D4*);
s16 func_800A8238(void);
void func_800A8290(Unk800EE1D4* arg0, Unk800EE1D4* arg1);
void func_800A2518(void);
void func_800A27F0(u_long* addr);
void func_800A8204(Unk800EE1D4* arg0);
void func_800A442C(s16 arg0);
s16 func_800A4400(void);
void func_800A2DE4(s32 arg0, s32 arg1);
s16 func_800A40F4(Unk800A4390* arg0, s16 arg1);
void* func_800A84DC(Unk800A8604* arg0);
void func_800A84A4(s32* arg0, u_long* arg1);
void func_800A83F0(SVECTOR* arg0, u_long* arg1);
void func_800A0874(Unk800D1964* db, Unk800EE1D4* node, s16 otIndex, s32 arg3, Unk800A4390* obj);
void* func_800A8604(Unk800A8604* arg0);
u_long* func_800A8734(Unk800A8CCC* arg0, u_long* arg1, u_long* arg2, Unk800A8CCC* arg3);
u_long* func_800A882C(SVECTOR* arg0, u_long* arg1, u_long* arg2, SVECTOR* arg3);
Unk800D0554* func_800A7BF4(void);
s32* func_800A7C20(s32 count);
s32* func_800A7C54(s32 count);
void func_800A6BD8(Unk800A4390* arg0);
s32 func_800A7414(VECTOR* arg0);
void func_800A6B08(Unk800A4390* arg0);
void func_800A12EC(void);
void func_800A13AC(void);
void func_800A1450();
void func_800A1A64();
void func_800A1B64(Unk800D1964* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_800A2420(void);
void func_800A2BE0(void);
void func_800A334C(void);
void func_800A3AAC(void);
void func_800A70D4(void);
void func_800A7AF8(void);
Unk800D0554* func_800A7B48(s32 arg0);
void func_800A7C88(void);
void func_800A8010(void);
void func_800A16A4(u32 at, s32 lift, VECTOR* pos, SVECTOR* rot);
void func_800A1CD8(s32 value, u16 x, s16 y, s16 padWithZero, u16 v);
void func_800A1F18(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans);
void func_800A2058();
void func_800A2214();
void func_800A2860(void);
void func_800A2B78(void);
void func_800A2C50(s32 arg0);
void func_800A2E38();
void func_800A35DC(s32 arg0);
void func_800A385C(u16 arg0);
void func_800A38D4(u16 arg0);
void func_800A3980(u16 arg0);
void func_800A3A20(u16 arg0);
void func_800A372C(s32 arg0);
void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4);
void func_800A46E8(Unk800D1964* db);
void func_800A0D78(Unk800D1964* db, Unk800EE1D4* node, s16 otIndex, s32 arg3, Unk800A4390* obj);
Unk800EE1D4* func_800A80F8(s16 arg0, s32 arg1, s32 arg2, s32 arg3, Unk800EE1D4* arg4, s32 arg5, s32 arg6, s32 arg7,
                           s16 arg8, s16 arg9, s16 arg10);

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", MINI_Jet);
#else
// The dummy local reproduces the target stack frame.
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
    D_800A8A74[0] = func_800A80F8(0x1E, 0, 0, 1, &D_800D16E4, 0x4B0, 0x32, 0xBB8, 0, 0x3E8, 0);
    // A loop keyword makes gcc duplicate the exit test and hoist loop constants.
loop:
    if ((D_800D16E0 * 4) > (D_800D1724 - 0x10) || D_800E2600 == 1) {
        goto done;
    }
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
        DrawOTag(&db[0]->unk70[0xFFF]);
        DrawOTag(&db[0]->unk4098[0xB3]);
    }
    var_a2 = D_800AB898;
    D_800E25FC = 0;
    if (db[0] == var_a2) {
        var_a2++;
    }
    db[0] = var_a2;
    ClearOTagR(var_a2->unk70, 0x1000);
    ClearOTagR(db[0]->unk4098, 0xB4);
    func_800A7FAC(&db[0]->unk4368);
    goto loop;
done:
    *D_8009A000 = 0xB8;
    D_8009A004 = 0;
    SystemAkaoExecute();
    return D_800D16D8;
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0874);
#else
// Draw one object's model, project its bounding box and flag a cursor hit.
void func_800A0874(Unk800D1964* db, Unk800EE1D4* node, s16 otIndex, s32 arg3, Unk800A4390* obj) {
    Unk800A8604 args;
    s16 xs[6];
    s16 ys[6];
    MATRIX** world;
    MATRIX* m;
    MATRIX* wm;
    MATRIX* cam;
    u_long x;
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;
    s16 i;

    world = &D_800D1BD0;
    m = world[0];
    m->m[0][0] = node->m.m[0][0];
    m->m[0][1] = node->m.m[0][1];
    m->m[0][2] = node->m.m[0][2];
    m->m[1][0] = node->m.m[1][0];
    m->m[1][1] = node->m.m[1][1];
    m->m[1][2] = node->m.m[1][2];
    m->m[2][0] = node->m.m[2][0];
    m->m[2][1] = node->m.m[2][1];
    m->m[2][2] = node->m.m[2][2];
    m->t[0] = node->m.t[0];
    m->t[1] = node->m.t[1];
    m->t[2] = node->m.t[2];
    if (node->unk24 != &D_800D16E4) {
        CompMatrix(&node->unk24->m, m, m);
    }
    wm = world[0];
    wm->t[0] -= D_800A83A8.vx;
    wm->t[1] -= D_800A83A8.vy;
    wm->t[2] -= D_800A83A8.vz;
    cam = &D_800EE404;
    gte_SetRotMatrix2(cam);
    gte_ldclmv2(&world[0]->m[0][0]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][0]);
    gte_ldclmv2(&world[0]->m[0][1]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][1]);
    gte_ldclmv2(&world[0]->m[0][2]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][2]);
    gte_SetTransMatrix2(cam);
    gte_ldlv0_2(&world[0]->t[0]);
    gte_rtv0();
    gte_stlvnl2(&world[0]->t[0]);
    gte_SetRotMatrix2(world[0]);
    gte_SetTransMatrix2(world[0]);
    args.tris = node->unk0->unkC;
    args.prim = db->unk4368.unk8;
    args.ot = &db->unk70[otIndex];
    args.model = node->unk0;
    db->unk4368.unk8 = func_800A84DC(&args);
    func_800A83F0(obj->unkDC, obj->unk11C);
    ys[0] = obj->unk11C[0] >> 16;
    minY = ys[0];
    maxY = minY;
    x = obj->unk11C[0];
    xs[0] = x;
    minX = x;
    maxX = minX;
    for (i = 1; i < 6; i++) {
        ys[i] = (obj->unk11C[i] & 0xFFFF0000) >> 16;
        x = obj->unk11C[i];
        xs[i] = x;
        if (xs[i] < minX) {
            minX = x;
        }
        if (maxX < xs[i]) {
            maxX = x;
        }
        if (ys[i] < minY) {
            minY = ys[i];
        }
        if (maxY < ys[i]) {
            maxY = ys[i];
        }
    }
    if (func_800A7414((VECTOR*)&D_800D1BD0->t[0])) {
        obj->unk28.unk4 = 0;
        if ((s16)D_800E25EC < maxX && minX < (s16)D_800E25EC && (s16)D_800E25F0 < maxY && minY < (s16)D_800E25F0 &&
            D_800D1C4C == 1) {
            obj->unk28.unk4 = D_800D1C4C;
        }
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0D78);
#else
// The two locals before screen exist only to reproduce the target stack frame.
void func_800A0D78(Unk800D1964* db, Unk800EE1D4* node, s16 otIndex, s32 arg3, Unk800A4390* obj) {
    Unk800A8604 args;
    MATRIX unused;
    u_long screen[12];
    MATRIX** world;
    MATRIX* m;
    MATRIX* wm;
    MATRIX* cam;
    Unk800D0554** shadow;
    u_long xy1;
    u_long xy2;

    world = &D_800D1BD0;
    m = world[0];
    m->m[0][0] = node->m.m[0][0];
    m->m[0][1] = node->m.m[0][1];
    m->m[0][2] = node->m.m[0][2];
    m->m[1][0] = node->m.m[1][0];
    m->m[1][1] = node->m.m[1][1];
    m->m[1][2] = node->m.m[1][2];
    m->m[2][0] = node->m.m[2][0];
    m->m[2][1] = node->m.m[2][1];
    m->m[2][2] = node->m.m[2][2];
    m->t[0] = node->m.t[0];
    m->t[1] = node->m.t[1];
    m->t[2] = node->m.t[2];
    if (node->unk24 != &D_800D16E4) {
        CompMatrix(&node->unk24->m, m, m);
    }
    wm = world[0];
    wm->t[0] -= D_800A83A8.vx;
    wm->t[1] -= D_800A83A8.vy;
    wm->t[2] -= D_800A83A8.vz;
    cam = &D_800EE404;
    gte_SetRotMatrix2(cam);
    gte_ldclmv2(&world[0]->m[0][0]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][0]);
    gte_ldclmv2(&world[0]->m[0][1]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][1]);
    gte_ldclmv2(&world[0]->m[0][2]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][2]);
    gte_SetTransMatrix2(cam);
    gte_ldlv0_2(&world[0]->t[0]);
    gte_rtv0();
    gte_stlvnl2(&world[0]->t[0]);
    gte_SetRotMatrix2(world[0]);
    gte_SetTransMatrix2(world[0]);
    args.tris = node->unk0->unkC;
    args.prim = db->unk4368.unk8;
    args.ot = &db->unk4098[otIndex];
    args.model = node->unk0;
    db->unk4368.unk8 = func_800A84DC(&args);
    shadow = &D_800D186C;
    func_800A84A4(shadow[0]->unkC, screen);
    xy1 = screen[1];
    D_800A8964 = xy1 >> 16;
    D_800A895C = xy1;
    xy1 = screen[2];
    D_800A896C = xy1 >> 16;
    D_800A8960 = xy1;
    func_800A84A4(shadow[0]->unkC + 9, screen);
    xy1 = screen[1];
    xy2 = screen[2];
    D_800A8978 = xy1 >> 16;
    D_800A8970 = xy1;
    D_800A8980 = xy2 >> 16;
    D_800A8974 = xy2;
}
#endif

// Load a node's matrix into the GTE and draw its model's triangles.
void func_800A1198(Unk800D1964* db, Unk800EE1D4* node, s16 otIndex, s32 arg3, s32 arg4) {
    Unk800A8604 args;
    MATRIX** world;
    MATRIX* m;

    world = &D_800D1BD0;
    m = world[0];
    m->m[0][0] = node->m.m[0][0];
    m->m[0][1] = node->m.m[0][1];
    m->m[0][2] = node->m.m[0][2];
    m->m[1][0] = node->m.m[1][0];
    m->m[1][1] = node->m.m[1][1];
    m->m[1][2] = node->m.m[1][2];
    m->m[2][0] = node->m.m[2][0];
    m->m[2][1] = node->m.m[2][1];
    m->m[2][2] = node->m.m[2][2];
    m->t[0] = node->m.t[0];
    m->t[1] = node->m.t[1];
    m->t[2] = node->m.t[2];
    gte_SetRotMatrix2(world[0]);
    gte_SetTransMatrix2(world[0]);
    args.tris = node->unk0->unkC;
    args.prim = db->unk4368.unk8;
    args.ot = &db->unk4098[otIndex];
    args.model = node->unk0;
    db->unk4368.unk8 = func_800A8604(&args);
}

// Draw every background triangle on the draw list, front to back.
// PC: C_005E9E7E
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
// PC: C_005E9F33
void func_800A13AC(void) {
    Unk800E2608* list;
    SVECTOR* left;
    SVECTOR* right;
    u16 trackId;
    u_long* ot;

    trackId = D_800A89DC;
    ot = D_800D1964[0]->unk4368.unk14;
    list = D_800D9948;
// A loop keyword makes gcc duplicate the exit test, so the goto is load-bearing.
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

// Build the world matrix from the camera rotation and the view position.
void func_800A1450(void) {
    MATRIX** view;
    MATRIX** world;
    MATRIX* cam;

    view = &D_800D1728;
    view[0]->t[0] = -D_800A83A8.vx;
    view[0]->t[1] = -D_800A83A8.vy;
    view[0]->t[2] = -D_800A83A8.vz;
    cam = &D_800EE404;
    gte_SetRotMatrix2(cam);
    gte_ldclmv2(&view[0]->m[0][0]);
    gte_rtir();
    world = &D_800D1BD0;
    gte_stclmv2(&world[0]->m[0][0]);
    gte_ldclmv2(&view[0]->m[0][1]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][1]);
    gte_ldclmv2(&view[0]->m[0][2]);
    gte_rtir();
    gte_stclmv2(&world[0]->m[0][2]);
    gte_SetTransMatrix2(cam);
    gte_ldlv0_2(&view[0]->t[0]);
    gte_rtv0();
    gte_stlvnl2(&world[0]->t[0]);
    gte_SetRotMatrix2(world[0]);
    gte_SetTransMatrix2(world[0]);
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A16A4);
#else
// Sample the track at a fractional segment index, giving a point lifted along
// the surface normal and the interpolated banking rotation.
void func_800A16A4(u32 at, s32 lift, VECTOR* pos, SVECTOR* rot) {
    VECTOR left;
    VECTOR right;
    VECTOR mid;
    VECTOR nextMid;
    VECTOR curMid;
    VECTOR across;
    VECTOR along;
    VECTOR normal;
    VECTOR unit;
    VECTOR dLeft;
    VECTOR dRight;
    SVECTOR* rotCur;
    SVECTOR* rotNext;
    SVECTOR* leftCur;
    SVECTOR* leftNext;
    SVECTOR* rightCur;
    SVECTOR* rightNext;
    s32 seg;
    s32 frac;
    s32 dx;
    s32 dy;
    s32 dz;

    seg = at >> 16;
    frac = at & 0xFFFF;
    rotCur = &D_800EE424[seg];
    rotNext = &D_800EE424[seg + 1];
    dx = rotNext->vx - rotCur->vx;
    dy = rotCur->vy - rotNext->vy;
    dz = rotNext->vz - rotCur->vz;
    if (dx > 0x800) {
        dx -= 0x1000;
    }
    if (dy > 0x800) {
        dy -= 0x1000;
    }
    if (dz > 0x800) {
        dz -= 0x1000;
    }
    if (dx < -0x800) {
        dx += 0x1000;
    }
    if (dy < -0x800) {
        dy += 0x1000;
    }
    if (dz < -0x800) {
        dz += 0x1000;
    }
    dx *= frac;
    dy *= frac;
    dz *= frac;

    leftCur = &D_800D1C58[seg];
    leftNext = &D_800D1C58[seg + 1];
    dLeft.vx = (leftNext->vx - leftCur->vx) * frac;
    dLeft.vy = (leftNext->vy - leftCur->vy) * frac;
    dLeft.vz = (leftNext->vz - leftCur->vz) * frac;

    rightCur = &D_800EE194[seg];
    rightNext = &D_800EE194[seg + 1];
    left.vx = leftCur->vx + (dLeft.vx >> 16);
    left.vy = leftCur->vy + (dLeft.vy >> 16);
    left.vz = leftCur->vz + (dLeft.vz >> 16);

    dRight.vx = (rightNext->vx - rightCur->vx) * frac;
    dRight.vy = (rightNext->vy - rightCur->vy) * frac;
    dRight.vz = (rightNext->vz - rightCur->vz) * frac;

    right.vx = rightCur->vx + (dRight.vx >> 16);
    right.vy = rightCur->vy + (dRight.vy >> 16);
    right.vz = rightCur->vz + (dRight.vz >> 16);
    mid.vx = (right.vx + left.vx) >> 1;
    mid.vy = (right.vy + left.vy) >> 1;
    mid.vz = (right.vz + left.vz) >> 1;

    curMid.vx = (rightCur->vx + leftCur->vx) >> 1;
    curMid.vy = (rightCur->vy + leftCur->vy) >> 1;
    curMid.vz = (rightCur->vz + leftCur->vz) >> 1;

    nextMid.vx = (rightNext->vx + leftNext->vx) >> 1;
    along.vx = nextMid.vx - curMid.vx;
    nextMid.vy = (rightNext->vy + leftNext->vy) >> 1;
    along.vy = nextMid.vy - curMid.vy;
    nextMid.vz = (rightNext->vz + leftNext->vz) >> 1;
    along.vz = nextMid.vz - curMid.vz;

    across.vx = right.vx - left.vx;
    across.vy = right.vy - left.vy;
    across.vz = right.vz - left.vz;

    OuterProduct0(&along, &across, &normal);
    VectorNormal(&normal, &unit);

    pos->vx = (s16)mid.vx + ((unit.vx * lift) >> 12);
    pos->vy = (s16)mid.vy + ((unit.vy * lift) >> 12);
    pos->vz = (s16)mid.vz + ((unit.vz * lift) >> 12);

    rot->vx = rotCur->vx + (dx >> 16);
    rot->vy = (dy >> 16) - rotCur->vy;
    rot->vz = rotCur->vz + (dz >> 16);
}
#endif

// PC: C_005F15C7, draw the shoot power gauge
void func_800A1A64(void) {
    Unk800D1964** db;
    POLY_G4* poly;
    s16 power;
    s32 top;

    db = D_800D1964;
    poly = db[0]->unk4368.unkC;
    power = D_800D1C5C;
    top = 0xDC - power;
    setXY4(poly, 0x14, top, 0x1C, top, 0x14, 0xDC, 0x1C, 0xDC);
    setRGB0(poly, -0x80 - power, power, 0);
    setRGB1(poly, -0x80 - power, power, 0);
    setRGB2(poly, 0x80, 0, 0);
    setRGB3(poly, 0x80, 0, 0);
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->unk4098[1], poly);
    poly++;
    db[0]->unk4368.unkC = poly;
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1B64);
#else
// Spin and draw the score model, alternating it with the title every so often.
void func_800A1B64(Unk800D1964* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4) {
    u8* alternate;
    s16* counter;
    s32 unused;

    if (arg1 == 0 || arg1 == 0x5B) {
        return;
    }
    alternate = &D_800E25E8;
    D_800A8A74[0]->unk0 = D_800D1730[arg1];
    D_800EE18C.vx += arg2;
    D_800EE18C.vy += arg3;
    D_800EE18C.vz += arg4;
    if (alternate[0] == 1) {
        RotMatrix(&D_800EE18C, &D_800A8A74[0]->m);
        func_800A1198(arg0, D_800A8A74[0], 0, 0, unused);
        func_800A1CD8(D_800A8CC4, 0xDC, 0xA0, 0, 0x18);
    }
    counter = &D_800A89CC;
    (*counter)--;
    if (*counter < 0x32) {
        if (alternate[0] == 0) {
            alternate[0] = 1;
        } else {
            alternate[0] = 0;
        }
    }
    if (D_800A89CC == 0) {
        D_800A8A88 = 0;
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1CD8);
#else
// PC: C_005F2119, draw a number as four digits from the HUD digit sprite.
void func_800A1CD8(s32 value, u16 x, s16 y, s16 padWithZero, u16 v) {
    POLY_FT4* poly;
    s32 digit;
    s32 power;
    s32 remain;
    s32 i;
    s16 left;
    s32 w;
    u8 leading;

    power = 1000;
    leading = 1;
    remain = value + 1;
    w = 0x10;
    left = x;
    poly = D_800D1964[0]->unk4368.unk14;
    for (i = 0; i < 4; i++) {
        digit = 0;
        while (remain > power) {
            remain -= power;
            digit++;
        }
        if (digit) {
            leading = 0;
        }
        if (value == 0 && power == 1) {
            leading = 0;
        }
        if (padWithZero == 1 || digit || leading == 0) {
            setXY4(poly, left, y, x + w, y, left, y + 0x10, x + w, y + 0x10);
            setRGB0(poly, 0x80, 0x80, 0x80);
            setUVWH(poly, digit * 0x10 + 0x30, v, 0x10, 0x12);
            poly->tpage = D_800A8990[8];
            poly->clut = D_800EE198[8];
            SetSemiTrans(poly, 1);
            addPrim(&D_800D1964[0]->unk4098[1], poly);
            poly++;
        }
        power /= 10;
        w += 0xE;
        left += 0xE;
    }
    D_800D1964[0]->unk4368.unk14 = poly;
}
#endif

// Draw one sprite from the HUD sprite table.
void func_800A1F18(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans) {
    Unk800D1964** db;
    POLY_FT4* poly;

    db = D_800D1964;
    poly = db[0]->unk4368.unk14;
    setXYWH(poly, x, y, w, h);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUVWH(poly, u, v, uw, vh);
    poly->tpage = D_800A8990[spriteId];
    poly->clut = D_800EE198[spriteId];
    SetSemiTrans(poly, semiTrans);
    addPrim(&db[0]->unk4098[1], poly);
    poly++;
    db[0]->unk4368.unk14 = poly;
}

// Queue two blank textured quads, one at each end of the background OT.
void func_800A2058(void) {
    Unk800D1964** db;
    POLY_FT4* poly;

    db = D_800D1964;
    poly = db[0]->unk4368.unk14;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = D_800A8990[5];
    poly->clut = D_800EE198[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->unk70[0xFFF], poly);
    poly++;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = D_800A8990[5];
    poly->clut = D_800EE198[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->unk70[2], poly);
    poly++;
    db[0]->unk4368.unk14 = poly;
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2214);
#else
// Point every matrix and vector at scratchpad, then build the world.
void func_800A2214(void) {
    s32* state;
    s32 i;

    state = &D_800A8A84;
    state[0] = 0;
    D_80110BB8 = (void*)0x1F800000;
    D_800D16D4 = (void*)0x1F800000;
    D_800D16DC = 0;
    D_800D1BD0 = (MATRIX*)0x1F800010;
    D_800D1728 = (MATRIX*)0x1F800030;
    D_800D1A38 = (void*)0x1F800050;
    D_800D1A3C = (void*)0x1F800058;
    D_800A891C = (void*)0x1F800060;
    D_800A8920 = (void*)0x1F800064;
    D_800D1728->m[0][0] = 0x1000;
    D_800D1728->m[0][1] = 0;
    D_800D1728->m[0][2] = 0;
    D_800D1728->m[1][0] = 0;
    D_800D1728->m[1][1] = 0x1000;
    D_800D1728->m[1][2] = 0;
    D_800D1728->m[2][0] = 0;
    D_800D1728->m[2][1] = 0;
    D_800D1728->m[2][2] = 0x1000;
    func_800A7C88();
    func_800A2420();
    state[0] = 0x99;
    func_800A334C();
    func_800A8010();
    func_800A7AF8();
    func_800A2BE0();
    func_800A3AAC();
    func_800A70D4();
    for (i = 0; i < 0x64; i++) {
        D_800D1730[i] = func_800A7B48(i);
    }
    D_800A897C = 0x2710;
    D_800A89D0 = 0x28AA;
    D_800A89D4 = 0x37DC;
    D_800D16E0 = 0;
    D_800D1C54 = 0;
    D_800D16D8 = 0;
    D_800E25F4 = 0;
    D_800E2600 = 0;
    D_800A8A88 = 0;
    D_800E25E8 = 0;
    D_800EE18C.vx = 0;
    D_800EE18C.vy = 0;
    D_800EE18C.vz = 0;
    D_800A8928 = 0;
    D_800A8330 = 0x7F;
    D_800A8334 = 0x7F;
    D_800A8338 = 0;
    D_800A833C = 0;
    D_800A89CC = 0;
}
#endif

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

// Upload the nine loaded TIMs and build the sprite tpage/clut tables.
void func_800A2518(void) {
    TIM_IMAGE timimg;
    u_long** tims;
    s32 i;
    u_long* addr;

    // i is created before tims so the two take the registers the target uses.
    i = 0;
    tims = (u_long**)&D_800A89E4;
    for (; i < 9; i++) {
        addr = *tims++;
        func_800A27F0(addr);
        OpenTIM(addr);
        ReadTIM(&timimg);
    }
    D_800AB894 = GetTPage(0, 2, 0x280, 0);
    D_800A8A68 = GetClut(0, 0x1E0);
    D_800A8990[0] = GetTPage(0, 1, 0x280, 0);
    D_800EE198[0] = GetClut(0, 0x1E0);
    D_800A8990[1] = GetTPage(1, 1, 0x2C0, 0);
    D_800EE198[1] = GetClut(0, 0x1E1);
    D_800A8990[2] = GetTPage(1, 1, 0x2D0, 0);
    D_800EE198[2] = GetClut(0, 0x1E1);
    D_800A8990[3] = GetTPage(1, 1, 0x2E0, 0);
    D_800EE198[3] = GetClut(0, 0x1E1);
    D_800A8990[4] = GetTPage(0, 1, 0x280, 0x100);
    D_800EE198[4] = GetClut(0, 0x1FF);
    D_800A8990[5] = GetTPage(0, 1, 0x280, 0x100);
    D_800EE198[5] = GetClut(0, 0x1FE);
    D_800A8990[6] = GetTPage(0, 1, 0x300, 0);
    D_800EE198[6] = GetClut(0x10, 0x1E0);
    D_800A8990[7] = GetTPage(0, 1, 0x240, 0);
    D_800EE198[7] = GetClut(0x40, 0x1E0);
    D_800A8990[8] = GetTPage(0, 1, 0x240, 0x18);
    D_800EE198[8] = GetClut(0x30, 0x1E0);
    D_800A8990[9] = GetTPage(0, 1, 0x240, 0x50);
    D_800EE198[9] = GetClut(0x50, 0x1E0);
    D_800A8990[10] = GetTPage(0, 1, 0x240, 0x30);
    D_800EE198[10] = GetClut(0x20, 0x1E0);
    D_800A8990[11] = GetTPage(0, 1, 0x240, 0);
    D_800EE198[11] = GetClut(0x60, 0x1E0);
}

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

// Alternate the two laser channels on each shot.
void func_800A29AC(s16 arg0) {
    u8* pChannel;
    s32 channel;

    pChannel = &D_800A8928;
    channel = (*pChannel + 1) & 1;
    *pChannel = channel;
    if (channel == 0) {
        D_8009A000[0] = 0xB0;
        D_8009A004 = 0;
        SystemAkaoExecute();
        D_8009A000[0] = 0x28;
        D_8009A004 = 0x40;
        D_8009A008 = arg0;
        SystemAkaoExecute();
    }
    if (*pChannel == 1) {
        D_8009A000[0] = 0xB1;
        D_8009A004 = 0;
        SystemAkaoExecute();
        D_8009A000[0] = 0x29;
        D_8009A004 = 0x40;
        D_8009A008 = arg0;
        SystemAkaoExecute();
    }
}

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

// Advance the camera along its path and rebuild the view matrices.
void func_800A2C50(s32 arg0) {
    VECTOR pos;
    SVECTOR rot;
    SVECTOR camRot;
    s32* pathPos;
    s32* speed;
    s32* limit;
    s32 step;

    pathPos = &D_800D1C54;
    func_800A16A4(pathPos[0], -0x64, &pos, &rot);
    speed = &D_800A897C;
    pathPos[0] += speed[0];
    D_800A83B8.vx = pos.vx;
    D_800A83B8.vy = pos.vy;
    D_800A83B8.vz = pos.vz;
    if (rot.vx < 0) {
        rot.vx += 0x1000;
    }
    D_800A83A0.vz = -rot.vz;
    step = rsin(rot.vx) / 15;
    if (step > 0) {
        if (speed[0] > 0xA7F8) {
            speed[0] -= step;
        }
    }
    if (step < 0) {
        limit = &D_800A897C;
        if (limit[0] <= 0x1D4BF) {
            limit[0] -= step;
        }
    }
    D_800A83A8.vx = pos.vx;
    D_800A83A8.vy = pos.vy;
    D_800A83A8.vz = pos.vz;
    camRot.vx = -rot.vx;
    camRot.vy = -rot.vy;
    camRot.vz = 0;
    RotMatrix(&camRot, &D_800EE404);
    RotMatrix(&D_800A83A0, &D_800A8380);
    CompMatrix(&D_800A8380, &D_800EE404, &D_800EE404);
}

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

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A2E38);
#else
// Read the pad and drive the cursor, the camera tweaks and the pause toggle.
void func_800A2E38(void) {
    u32 pad;
    s32* dir;
    u16* cursorX;
    u16* cursorY;
    u8* shoot;
    u16* power;
    u8* repeat;
    u8* scroll;
    s32* fogFar;
    s32* fogNear;
    s32* speed;
    s32* held;
    u8* paused;
    u8 next;
    s32 count;

    pad = InputReadPadsRaw();
    if (D_800D16DC == 0) {
        dir = &D_800A8A6C;
        *dir = 0;
        D_800A8A7C = 0;
        if (pad & 0x8000) {
            *dir = 4;
        }
        if (pad & 0x2000) {
            *dir = 6;
        }
        if (pad & 0x1000) {
            *dir = 8;
            if (pad & 0x8000) {
                *dir = 7;
            }
            if (pad & 0x2000) {
                *dir = 9;
            }
        }
        if (pad & 0x4000) {
            dir = &D_800A8A6C;
            *dir = 2;
            if (pad & 0x8000) {
                *dir = 1;
            }
            if (pad & 0x2000) {
                *dir = 3;
            }
        }
        if (D_800D1960 == 1) {
            if (pad & 0x4000) {
                cursorY = &D_800E25F0;
                *cursorY += 5;
            }
            if (pad & 0x1000) {
                cursorY = &D_800E25F0;
                *cursorY -= 5;
            }
            if (pad & 0x8000) {
                cursorX = &D_800E25EC;
                *cursorX -= 5;
            }
            if (pad & 0x2000) {
                cursorX = &D_800E25EC;
                *cursorX += 5;
            }
            shoot = &D_800D1C4C;
            *shoot = 0;
            if (pad & 0x20) {
                power = &D_800D1C5C;
                func_800A2AA0(*power & 0xFF);
                if ((s16)*power >= 9) {
                    (*power)--;
                }
                repeat = &D_800D1C7C;
                count = *repeat;
                if (count == 0) {
                    scroll = &D_800D1720;
                    next = *scroll + 3;
                    *repeat = 1;
                    *shoot = 1;
                    *scroll = next % 15;
                } else {
                    *repeat = count - 1;
                }
            } else {
                func_800A2AA0(0);
                power = &D_800D1C5C;
                if ((s16)*power < 0x80) {
                    (*power)++;
                }
            }
            cursorX = &D_800E25EC;
            if ((s16)*cursorX > 0x140) {
                *cursorX = 0x140;
            }
            if ((s16)*cursorX < 0) {
                *cursorX = 0;
            }
            cursorY = &D_800E25F0;
            if ((s16)*cursorY > 0xF0) {
                *cursorY = 0xF0;
            }
            if ((s16)*cursorY < 0) {
                *cursorY = 0;
            }
        }
        if (D_800D1960 == 0) {
            if (pad & 0x4000) {
                fogFar = &D_800A89D4;
                *fogFar -= 10;
            }
            if (pad & 0x1000) {
                fogFar = &D_800A89D4;
                *fogFar += 10;
            }
            if (pad & 0x8000) {
                fogNear = &D_800A89D0;
                *fogNear -= 10;
            }
            if (pad & 0x2000) {
                fogNear = &D_800A89D0;
                *fogNear += 10;
            }
            if (pad & 0x40) {
                D_800A83D8.vz = D_800A83D8.vz - 100;
            }
            if (pad & 0x10) {
                D_800A83D8.vz = D_800A83D8.vz + 100;
            }
            if (pad & 0x80) {
                D_800A83D8.vx -= 100;
            }
            if (pad & 0x20) {
                D_800A83D8.vx += 100;
            }
            if (pad & 0x8) {
                D_800A83D8.vy = D_800A83D8.vy - 100;
            }
            if (pad & 0x2) {
                D_800A83D8.vy = D_800A83D8.vy + 100;
            }
            if (pad & 0x4) {
                speed = &D_800A897C;
                *speed += 0x400;
            }
            if (pad & 0x1) {
                speed = &D_800A897C;
                if (*speed >= 0x400) {
                    *speed -= 0x400;
                }
            }
            if (pad & 0x800) {
                D_800A897C = 0;
            }
        }
    }
    if (pad & 0x800) {
        held = &D_800A8A80;
        *held = *held + 1;
    } else {
        D_800A8A80 = 0;
    }
    if (D_800A8A80 == 1) {
        paused = &D_800D16DC;
        if (*paused == 1) {
            *paused = 0;
        } else {
            *paused = 1;
        }
        func_800A29AC(0x3B);
    }
}
#endif

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

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3414);
#else
// Step the track streams forward, spawning whatever each segment lists.
void func_800A3414(s32 advance) {
    u16** tri;
    u16** quad;
    u32* pos;
    s32* segment;
    u32 prev;
    u32 next;
    u32 steps;
    u32 i;
    u16 id;
    s32 unused[4];

    pos = &D_800D172C;
    prev = pos[0];
    next = prev + advance;
    D_800A8CC8 = prev;
    pos[0] = next;
    steps = (next >> 18) - (prev >> 18);
    segment = &D_800D16E0;
    segment[0] = segment[0] + steps;
    i = 0;
    if (steps + D_800E25F8 != 0) {
        tri = &D_800D1C60;
        quad = &D_800EE428;
        do {
            for (;;) {
                id = *tri[0]++;
                if (id == 0xFFFF) {
                    break;
                }
                func_800A385C(id);
            }
            for (;;) {
                id = *quad[0]++;
                if (id == 0xFFFF) {
                    break;
                }
                func_800A38D4(id);
            }
            i++;
        } while (i < steps + D_800E25F8);
    }
    i = 0;
    if (steps != 0) {
        tri = &D_800D196C;
        quad = &D_800EE188;
        do {
            for (;;) {
                id = *tri[0]++;
                if (id == 0xFFFF) {
                    break;
                }
                func_800A3980(id);
            }
            for (;;) {
                id = *quad[0]++;
                if (id == 0xFFFF) {
                    break;
                }
                func_800A3A20(id);
            }
            i++;
        } while (i < steps);
    }
    if (D_800E25F8 == 1) {
        D_800E25F8 = 0;
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A35DC);
#else
// PC: C_005EDC59, step the track position and append what the new segments list.
void func_800A35DC(s32 advance) {
    u32* pos;
    s32* segment;
    u32 prev;
    u32 next;
    u32 i;
    u16** tri;
    u16** track;
    u32 id;

    pos = &D_800D172C;
    D_800A8CC8 = pos[0];
    pos[0] = D_800A8CC8 + advance;
    prev = D_800A8CC8 >> 18;
    next = pos[0] >> 18;
    D_800A894C = next - prev;
    segment = &D_800D16E0;
    segment[0] = segment[0] + D_800A894C;
    for (i = 0; i < D_800A894C + D_800E25F8; i++) {
        tri = &D_800D1C60;
        while (1) {
            id = *tri[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A385C(id);
        }
    }
    for (i = 0; i < D_800A894C; i++) {
        track = &D_800D196C;
        while (1) {
            id = *track[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A3980(id);
        }
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A372C);
#else
// PC: C_005EDD82, unlink the track and background elements just passed.
void func_800A372C(s32 advance) {
    u32 i;
    u16** tri;
    u16** track;
    u32 id;
    u8* first;

    for (i = 0; i < D_800A894C + D_800E25F8; i++) {
        tri = &D_800EE428;
        while (1) {
            id = *tri[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A38D4(id);
        }
    }
    for (i = 0; i < D_800A894C; i++) {
        track = &D_800EE188;
        while (1) {
            id = *track[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A3A20(id);
        }
    }
    first = &D_800E25F8;
    if (*first == 1) {
        *first = 0;
    }
}
#endif

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
void func_800A3B58(u8 pathIndex, u8 mode) {
    s32* lengths;
    s32* offsets;
    s32 offset;

    if (mode == 0) {
        lengths = D_800D1C08;
        offsets = D_800D1C04;
        D_800A8984 = lengths[pathIndex];
        offset = offsets[pathIndex];
        D_800A8954 = (SVECTOR*)(D_800D1C00 + offset);
    }
    if (mode == 1) {
        lengths = D_800D1BEC;
        offsets = D_800D1BE8;
        D_800A8984 = lengths[pathIndex];
        offset = offsets[pathIndex];
        D_800A8954 = (SVECTOR*)(D_800D1BE4 + offset);
    }
}
#endif

// Sample a path at a 16.16 position, mirroring y and z when flag is zero.
void func_800A3C04(u32 pos, SVECTOR* path, VECTOR* out, u8 flag) {
    SVECTOR seg[2];
    VECTOR delta;
    s32 idx;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 frac;

    idx = pos >> 16;
    frac = pos & 0xFFFF;
    // seg is dead and the scale/shift are split on purpose: both are needed
    // for the stack frame and the codegen to match.
    seg[0].vx = path[idx].vx;
    seg[0].vy = path[idx].vy;
    seg[0].vz = path[idx].vz;
    seg[1].vx = path[idx + 1].vx;
    seg[1].vy = path[idx + 1].vy;
    seg[1].vz = path[idx + 1].vz;
    dx = seg[1].vx - seg[0].vx;
    dy = seg[1].vy - seg[0].vy;
    dz = seg[1].vz - seg[0].vz;
    delta.vx = dx;
    delta.vy = dy;
    delta.vz = dz;
    delta.vx = dx * frac;
    delta.vy = dy * frac;
    delta.vz = dz * frac;
    delta.vx = delta.vx >> 16;
    delta.vy = delta.vy >> 16;
    delta.vz = delta.vz >> 16;
    if (flag == 0) {
        out->vx = path[idx].vx + delta.vx;
        out->vy = -path[idx].vy - delta.vy;
        out->vz = -path[idx].vz - delta.vz;
    } else {
        out->vx = path[idx].vx + delta.vx;
        out->vy = path[idx].vy + delta.vy;
        out->vz = path[idx].vz + delta.vz;
    }
}

// Draw the aiming cursor sprite.
void func_800A3D50(Unk800D1964* arg0) {
    POLY_FT4* poly;
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;

    poly = arg0->unk4368.unk14;
    left = D_800E25EC - 0x10;
    top = D_800E25F0 - 0x10;
    right = D_800E25EC + 0x10;
    bottom = D_800E25F0 + 0x10;
    setXY4(poly, left, top, right, top, left, bottom, right, bottom);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0x40, 0, 0, 0x40, 0x40, 0x40);
    poly->tpage = D_800A8990[0];
    poly->clut = D_800EE198[0];
    SetSemiTrans(poly, 0);
    addPrim(&arg0->unk70[1], poly);
    poly++;
    arg0->unk4368.unk14 = poly;
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3E58);
#else
// Draw the two laser beams, from each gun muzzle to the aiming cursor.
void func_800A3E58(void) {
    Unk800D1964** db;
    POLY_FT4* poly;
    u8* scroll;
    s16 power;
    s32 spread;

    if (D_800D1C4C == 1) {
        db = D_800D1964;
        scroll = &D_800D1720;
        power = D_800D1C5C;
        spread = power >> 3;
        poly = db[0]->unk4368.unk14;
        setXY4(poly, D_800A895C + spread, D_800A8964, D_800E25EC, D_800E25F0, D_800A895C - spread, D_800A8964,
               D_800E25EC, D_800E25F0);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = D_800A8990[1];
        poly->clut = D_800EE198[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->unk70[1], poly);
        poly++;
        setXY4(poly, D_800A8970 + spread, D_800A8978, D_800E25EC, D_800E25F0, D_800A8970 - spread, D_800A8978,
               D_800E25EC, D_800E25F0);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = D_800A8990[1];
        poly->clut = D_800EE198[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->unk70[1], poly);
        poly++;
        db[0]->unk4368.unk14 = poly;
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A40F4);
#else
// PC: C_005EB3A6, take an object from the pool and give it a node and its
// model's six bounding box face centres.
s16 func_800A40F4(Unk800A4390* src, s16 parentIndex) {
    s16* count;
    Unk800A4390* obj;
    Unk800A89D8* info;
    Unk800EE1D4* parent;
    s16 index;
    s32 modelId;
    s32 midX;
    s32 midY;
    s32 midZ;
    s16 maxX;
    s16 minX;
    s16 minY;
    s16 maxY;
    s16 minZ;
    s16 maxZ;

    count = (s16*)&D_800EE42C;
    if (*count < 0x63) {
        *count = *count + 1;
        index = func_800A4400();
        D_800D1DC0[index] = *src;
        obj = &D_800D1DC0[index];
        modelId = src->unk28.unk8;
        obj->unkDA = 1;
        obj->unkD8 = index;
        if (parentIndex == 0) {
            parent = &D_800D16E4;
        } else {
            parent = D_800D1DC0[parentIndex].unkD4;
        }
        obj->unkD4 = func_800A80F8(modelId, 0, 0, 1, parent, src->unk0.vx, src->unk0.vy, src->unk0.vz, src->unk18.vx,
                                   src->unk18.vy, src->unk18.vz);
        info = &D_800A89D8[(s16)modelId];
        obj = &D_800D1DC0[index];
        maxX = info->unkC.vx;
        minX = info->unk4.vx;
        minY = info->unk4.vy;
        maxY = info->unkC.vy;
        minZ = info->unk4.vz;
        maxZ = info->unkC.vz;
        midX = (maxX + minX) >> 1;
        midY = (maxY + minY) >> 1;
        obj->unkDC[0].vz = maxZ;
        obj->unkDC[1].vz = minZ;
        midZ = (maxZ + minZ) >> 1;
        obj->unkDC[0].vx = midX;
        obj->unkDC[0].vy = midY;
        obj->unkDC[1].vx = midX;
        obj->unkDC[1].vy = midY;
        obj->unkDC[2].vx = midX;
        obj->unkDC[2].vy = maxY;
        obj->unkDC[2].vz = midZ;
        obj->unkDC[3].vx = midX;
        obj->unkDC[3].vy = minY;
        obj->unkDC[3].vz = midZ;
        obj->unkDC[4].vx = maxX;
        obj->unkDC[4].vy = midY;
        obj->unkDC[4].vz = midZ;
        obj->unkDC[5].vx = minX;
        obj->unkDC[5].vy = midY;
        obj->unkDC[5].vz = midZ;
    }
    return index;
}
#endif

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

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A4458);
#else
// Spawn the objects scheduled for every track segment reached this frame.
void func_800A4458(void) {
    Unk800D1C0C* spawns;
    Unk800D1C0C* spawn;
    s32* streamIndex;
    u8* counts;
    u8* count;
    s32 segment;
    s32 i;
    s32 j;

    for (segment = D_800A898C; segment < D_800D16E0; segment++) {
        streamIndex = &D_800A89E0;
        counts = D_800D1C10;
        count = counts + segment;
        for (i = 0; i < *count; i++) {
            spawns = D_800D1C0C;
            for (j = 0; j < 0x14; j++) {
                D_800D1C84.unk28.unk50[j] = spawns[*streamIndex].unk10[j];
            }
            spawn = &spawns[*streamIndex];
            D_800D1C84.unk28.unk18 = spawn->unk8;
            D_800D1C84.unk28.unk1C = spawn->unkC;
            func_800A45C0(0, 0, 0, spawn->unk0, spawn->unk4);
            *streamIndex = *streamIndex + 1;
        }
    }
    D_800A898C = D_800D16E0;
}
#endif

// PC: C_005EB507, create an object at a position
void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    setVector(&D_800D1C84.unk0, arg0, arg1, arg2);
    D_800D1C84.unk28.unk0 = arg3;
    D_800D1C84.unk28.unk10 = 1;
    D_800D1C84.unk28.unk8 = arg4;
    D_800D1C84.unk28.unk4 = 0;
    func_800A40F4(&D_800D1C84, 0);
}

// PC: C_005EB566, create an object at a position, clearing f_50[0xC]
void func_800A4650(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    setVector(&D_800D1C84.unk0, arg0, arg1, arg2);
    D_800D1C84.unk28.unk0 = arg3;
    D_800D1C84.unk28.unk10 = 1;
    D_800D1C84.unk28.unk8 = arg4;
    D_800D1C84.unk28.unk4 = 0;
    D_800D1C84.unk28.unk50[0xC] = 0;
    func_800A40F4(&D_800D1C84, 0);
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A46E8);
#else
// Step every live object through its behaviour, then queue its model.
void func_800A46E8(Unk800D1964* db) {
    VECTOR next;
    VECTOR unusedA; // the two unused vectors reproduce the target stack frame
    VECTOR unusedB;
    VECTOR pos;
    SVECTOR rot;
    Unk800A4390* obj;
    Unk800D1CAC* st;
    POLY_G4* fade;
    POLY_FT4* flash;
    s32 modelId;
    s32 shade;
    s32 count;
    s32 i;
    s32 j;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 x;
    s32 y;
    s32 z;
    s32 step;
    s32* score;
    s32* frame;
    SVECTOR* path;
    s32 pathLen;
    s32 sound;
    s32 release;
    u8 order;
    u8 drawMode;
    u16 otIndex;

    func_800A4458();
    func_800A3D50(db);
    func_800A3E58();
    for (i = 0; i < 100; i++) {
        otIndex = 0;
        obj = &D_800D1DC0[i];
        st = &obj->unk28;
        if (obj->unkDA == 0) {
            continue;
        }
        drawMode = 0;
        release = 0;
        switch (obj->unk28.unk0) {
        case 100:
            if (st->unk10 == 1) {
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->unk4 = 0;
                st->unk50[10] = 1;
                st->unk28 = 0;
                st->unk2C = obj->unk0.vx;
                st->unk30 = obj->unk0.vy;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk50[0] = 0;
                st->unk34 = obj->unk0.vz;
            } else {
                st->unk14++;
                st->unk28++;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            step = st->unk28;
            func_800A16A4(D_800D1C54 + 0x2FFFD, -100, &pos, &rot);
            obj->unk0.vx = st->unk2C + ((step * (pos.vx - st->unk2C)) >> 7);
            obj->unk0.vy = st->unk30 + ((step * (pos.vy - st->unk30)) >> 7);
            obj->unk0.vz = st->unk34 + ((step * (pos.vz - st->unk34)) >> 7);
            func_800A16A4(D_800D1C54 + 0x3FFFC, -100, &pos, &rot);
            dx = obj->unk0.vx - pos.vx;
            dy = obj->unk0.vy - pos.vy;
            dz = obj->unk0.vz - pos.vz;
            SquareRoot0(dx * dx + dy * dy + dz * dz);
            if (st->unk14 >= 0x81) {
                score = &D_800D16D8;
                if (*score < 6) {
                    *score = 0;
                } else {
                    *score -= 5;
                }
                st->unkC = 0;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 17:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->unk4 = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk2C = (obj->unkC8 - 2) << 16;
            } else {
                st->unk14++;
            }
            frame = &D_800D16E0;
            if (st->unk50[3] < *frame) {
                st->unk28 += st->unk1C;
            }
            if (st->unk50[2] < *frame) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            if (st->unk28 < st->unk2C) {
                func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
                obj->unk18.vx = 0;
                obj->unk18.vy = 0;
                obj->unk18.vz = 0;
            }
            break;
        case 0:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->unk4 = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk2C = (obj->unkC8 - 1) << 16;
            } else {
                st->unk14++;
            }
            st->unk28 += st->unk1C;
            if (st->unk50[1] == 1) {
                st->unk28 %= st->unk2C;
            }
            if (st->unk28 > st->unk2C) {
                st->unkC = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
            obj->unk18.vx += st->unk50[3];
            obj->unk18.vy += st->unk50[4];
            obj->unk18.vz += st->unk50[5];
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 1:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->unk4 = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk2C = (obj->unkC8 - 1) << 16;
            } else {
                st->unk14++;
            }
            st->unk28 += st->unk1C;
            if (st->unk50[1] == 1) {
                st->unk28 %= st->unk2C;
            }
            if (st->unk28 > st->unk2C) {
                st->unkC = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
            func_800A3C04((st->unk28 + 0x10000) % ((obj->unkC8 - 1) << 16), obj->unkCC, &next, 0);
            dx = next.vx - obj->unk0.vx;
            dy = next.vy - obj->unk0.vy;
            dz = next.vz - obj->unk0.vz;
            obj->unk18.vx = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
            obj->unk18.vy = -ratan2(dz, dx) - 0x400;
            obj->unk18.vz = 0;
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 10:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                path = (SVECTOR*)(D_800D1BE4 + D_800D1BE8[0]);
                pathLen = D_800D1BEC[0];
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->unk4 = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk2C = (obj->unkC8 - 1) << 16;
            } else {
                st->unk14++;
            }
            st->unk28 += st->unk1C;
            if (st->unk50[1] == 1) {
                st->unk28 %= st->unk2C;
            }
            if (st->unk28 > st->unk2C) {
                st->unkC = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            func_800A16A4(D_800D1C54 + 0x3FFFC, 10, &obj->unk0, &obj->unk18);
            drawMode = 1;
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 4:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            frame = &D_800D16E0;
            if (st->unk50[2] < *frame) {
                st->unkC = 0;
            }
            if (st->unk50[3] < *frame) {
                st->unk2C += 4;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            obj->unk0.vy += st->unk2C;
            if (obj->unk0.vy > 0) {
                st->unkC = 0;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 5:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                st->unk28 = 0;
                st->unk2C = (obj->unkC8 - 1) << 16;
                obj->unk18.vx = st->unk50[3];
                obj->unk18.vy = st->unk50[4];
                D_800A8984 = pathLen;
                D_800A8954 = path;
                obj->unk18.vz = st->unk50[5];
            }
            st->unk28 += st->unk1C;
            if (st->unk50[1] == 1) {
                st->unk28 %= st->unk2C;
            }
            if (st->unk28 > st->unk2C) {
                st->unkC = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            obj->unk18.vx += st->unk50[6];
            obj->unk18.vy += st->unk50[7];
            obj->unk18.vz += st->unk50[8];
            if (st->unk50[10] == 5) {
                obj->unkD4->unk0 = D_800D189C[st->unk50[14]];
            }
            if (st->unk50[14] == 1) {
                st->unk50[14] = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
            if (st->unk4 == 0) {
                break;
            }
            if (st->unk50[10] != 5 || D_800A897C < 0x4015) {
                func_800A6B08(obj);
            }
            if (st->unk50[10] == 5) {
                st->unk50[14] = 1;
            }
            break;
        case 2:
            if (st->unk10 == 1) {
                sound = st->unk50[17];
                if (sound != 0) {
                    func_800A29AC(sound);
                }
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                step = st->unk50[3];
                st->unk2C = 0;
                st->unk30 = 0;
                st->unk28 = (rand() % step) * 2 - step - 1;
                st->unk34 = (obj->unkC8 - 1) << 16;
                obj->unk18.vx = 0;
                obj->unk18.vy = 0;
                obj->unk18.vz = 0;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            st->unk30 += st->unk1C;
            if (st->unk34 < st->unk30) {
                st->unkC = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unk28 < st->unk2C) {
                st->unk2C -= 5;
            }
            if (st->unk28 > st->unk2C) {
                st->unk2C += 5;
            }
            obj->unk18.vx = st->unk2C;
            obj->unk18.vy += st->unk50[4];
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            func_800A3C04(st->unk30, obj->unkCC, &obj->unk0, 0);
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 230:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 7:
        case 13:
            if (st->unk10 == 1) {
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                obj->unk18.vx = st->unk50[3];
                D_800A8984 = pathLen;
                D_800A8954 = path;
                obj->unk18.vz = 0;
                obj->unk18.vy = st->unk50[4];
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
                st->unk28 = 0;
            }
            frame = &D_800D16E0;
            if (st->unk50[2] < *frame) {
                st->unkC = 0;
            }
            if (st->unk50[5] < *frame) {
                count = st->unk28;
                st->unk28 = count + 1;
                if (count < st->unk50[7]) {
                    obj->unk18.vx += st->unk50[6];
                }
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 11:
            func_800A29AC(0x8E);
            for (j = 0; j < st->unk50[3]; j++) {
                D_800D1C84.unk0.vy = -0x2710;
                D_800D1C84.unk0.vz = 0x20CB;
                D_800D1C84.unk28.unk0 = 0xC;
                D_800D1C84.unk0.vx = 0x3446;
                D_800D1C84.unk28.unk10 = 1;
                D_800D1C84.unk28.unk8 = 0x2A;
                D_800D1C84.unk28.unk4 = 0;
                D_800D1C84.unk28.unk50[12] = 0;
                func_800A40F4(&D_800D1C84, 0);
            }
            if (obj->unkD8 != -1) {
                D_800EE42C--;
                func_800A8204(obj->unkD4);
                func_800A442C(obj->unkD8);
                obj->unkD8 = -1;
                obj->unkDA = 0;
            }
            // falls through into the debris behaviour below
        case 12:
            if (st->unk10 == 1) {
                st->unk4 = 0;
                st->unkC = 100;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = rand() % 60 - 30;
                st->unk2C = -rand() % 200;
                st->unk30 = rand() % 60 - 30;
            } else {
                st->unk14++;
            }
            st->unk2C++;
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk2C;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x190;
            obj->unk18.vz += 0xC8;
            obj->unk0.vz += st->unk30;
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 8:
            if (st->unk10 == 1) {
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            obj->unk0.vy -= st->unk50[3];
            st->unk50[3] -= st->unk50[4];
            if (st->unk50[3] < 0) {
                func_800A29AC(0x98);
                for (j = 0; j < 20; j++) {
                    x = obj->unk0.vx;
                    y = obj->unk0.vy;
                    z = obj->unk0.vz;
                    modelId = rand() % 3 + 0x44;
                    D_800D1C84.unk28.unk0 = 9;
                    D_800D1C84.unk28.unk10 = 1;
                    D_800D1C84.unk28.unk4 = 0;
                    D_800D1C84.unk28.unk50[12] = 0;
                    D_800D1C84.unk0.vx = (s16)x;
                    D_800D1C84.unk0.vy = (s16)y;
                    D_800D1C84.unk0.vz = (s16)z;
                    D_800D1C84.unk28.unk8 = (s16)modelId;
                    func_800A40F4(&D_800D1C84, 0);
                }
                st->unkC = 0;
            }
            break;
        case 9:
            if (st->unk10 == 1) {
                st->unk4 = 0;
                st->unkC = 100;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = rand() % 60 - 30;
                st->unk2C = rand() % 60 - 30;
                st->unk30 = rand() % 60 - 30;
            } else {
                st->unk14++;
            }
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk2C;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x190;
            obj->unk18.vz += 0xC8;
            obj->unk0.vz += st->unk30;
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 14:
            if (st->unk10 == 1) {
                func_800A29AC(0xA);
                modelId = st->unk18 & 0xFF;
                pathLen = D_800D1C08[modelId];
                path = (SVECTOR*)(D_800D1C00 + D_800D1C04[modelId]);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk4 = 0;
                st->unk14 = 0;
                st->unk28 = -0x46;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            st->unk28++;
            st->unk14++;
            if (st->unk14 == 5) {
                for (j = 0; j < st->unk50[3]; j++) {
                    y = obj->unk0.vy + 0x1F4;
                    x = obj->unk0.vx + rand() % 100 - 0x32;
                    z = obj->unk0.vz + rand() % 100 - 0x32;
                    D_800D1C84.unk0.vx = (s16)x;
                    D_800D1C84.unk0.vy = (s16)y;
                    D_800D1C84.unk28.unk0 = 0xF;
                    D_800D1C84.unk28.unk8 = 0x2A;
                    D_800D1C84.unk28.unk10 = 1;
                    D_800D1C84.unk28.unk4 = 0;
                    D_800D1C84.unk28.unk50[12] = 0;
                    D_800D1C84.unk0.vz = (s16)z;
                    func_800A40F4(&D_800D1C84, 0);
                }
                D_800D1C84.unk28.unk0 = 0x10;
                D_800D1C84.unk28.unk10 = 1;
                D_800D1C84.unk28.unk8 = 0x29;
                D_800D1C84.unk28.unk4 = 0;
                D_800D1C84.unk28.unk50[12] = 0;
                D_800D1C84.unk0.vx = (s16)obj->unk0.vx;
                D_800D1C84.unk0.vy = (s16)obj->unk0.vy;
                D_800D1C84.unk0.vz = (s16)obj->unk0.vz;
                func_800A40F4(&D_800D1C84, 0);
            }
            if (st->unk28 < 0) {
                obj->unk18.vy += 0x14;
            }
            if (st->unk28 == 0x50) {
                st->unkC = 0;
            }
            obj->unk0.vy += st->unk28;
            if (st->unk50[2] < D_800D16E0) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                release = 1;
                break;
            }
            if (st->unk4 != 0) {
                func_800A6B08(obj);
            }
            break;
        case 15:
            if (st->unk10 == 1) {
                st->unk4 = 0;
                st->unkC = 200;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = rand() % 80 - 40;
                st->unk2C = rand() % 80 - 40;
                st->unk30 = -(rand() % 40 + 40);
            } else {
                st->unk14++;
            }
            st->unk30++;
            obj->unk18.vx += 0x1E0;
            obj->unk18.vz += 0x262;
            obj->unk18.vy += 0x28;
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk30;
            obj->unk0.vz += st->unk2C;
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 16:
            if (st->unk10 == 1) {
                st->unkC = 200;
                st->unk4 = 0;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = -0x3C;
            } else {
                st->unk14++;
            }
            if (st->unk14 >= 0x15) {
                obj->unk0.vy += st->unk28;
                st->unk28++;
            }
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 3:
            obj->unk0.vx = D_800A83B8.vx;
            obj->unk0.vy = D_800A83B8.vy - 0x9C4;
            otIndex = 0x3E8;
            obj->unk18.vx = 0;
            obj->unk18.vy = 0;
            obj->unk18.vz = 0;
            obj->unk0.vz = D_800A83B8.vz;
            break;
        case 201:
            if (st->unk10 == 1) {
                st->unkC = 0x14;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk4 = 0;
            } else {
                st->unk14++;
            }
            D_800D1C84.unk28.unk0 = 0xCA;
            D_800D1C84.unk28.unk10 = 1;
            D_800D1C84.unk28.unk8 = 0x2A;
            D_800D1C84.unk28.unk4 = 0;
            D_800D1C84.unk28.unk50[12] = 0;
            D_800D1C84.unk0.vx = (s16)obj->unk0.vx;
            D_800D1C84.unk0.vy = (s16)obj->unk0.vy;
            D_800D1C84.unk0.vz = (s16)obj->unk0.vz;
            func_800A40F4(&D_800D1C84, 0);
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 202:
            if (st->unk10 == 1) {
                st->unk4 = 0;
                st->unkC = 0x32;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = rand() % 20 - 10;
                st->unk2C = rand() % 40 - 20;
                st->unk30 = rand() % 20 - 10;
            } else {
                st->unk14++;
            }
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk2C;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x64;
            obj->unk18.vz += 0x14;
            obj->unk0.vz += st->unk30;
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 203:
            if (st->unk10 == 1) {
                st->unk4 = 0;
                st->unkC = 0x32;
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = rand() % 200 - 100;
                st->unk2C = rand() % 200 - 100;
                st->unk30 = rand() % 200 - 100;
            } else {
                st->unk14++;
            }
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk2C;
            obj->unk0.vz += st->unk30;
            obj->unk18.vy += 0x12C;
            st->unkC--;
            if (st->unkC == 0) {
                release = 1;
            }
            break;
        case 255:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
            } else {
                st->unk14++;
            }
            if (st->unk50[2] < D_800A897C) {
                D_800A897C -= st->unk50[0];
            }
            if (D_800A897C < 0) {
                D_800A897C = 0;
                D_800D1C84.unk0.vx = 0;
                D_800D1C84.unk0.vy = 0;
                D_800D1C84.unk0.vz = 0;
                D_800D1C84.unk28.unk0 = 0xFD;
                D_800D1C84.unk28.unk10 = 1;
                D_800D1C84.unk28.unk8 = 0x1D;
                D_800D1C84.unk28.unk4 = 0;
                D_800D1C84.unk28.unk50[12] = 0;
                func_800A40F4(&D_800D1C84, 0);
            }
            if (st->unk50[1] < st->unk14) {
                release = 1;
            }
            break;
        case 254:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
                st->unk28 = 0;
                st->unk2C = VSync(-1);
                st->unk30 = 0;
            } else {
                st->unk14++;
            }
            if (st->unk28 == 0) {
                D_800A897C = 0;
            }
            if (st->unk28 == 1) {
                D_800A897C += st->unk50[1];
                st->unk30++;
            }
            if (st->unk50[0] < VSync(-1) - st->unk2C) {
                st->unk28 = 1;
            }
            if (st->unk50[2] < st->unk30) {
                release = 1;
            }
            break;
        case 252:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
                D_800D1C84.unk0.vx = 0;
                D_800D1C84.unk0.vy = 0;
                D_800D1C84.unk0.vz = 0;
                D_800D1C84.unk28.unk0 = 3;
                D_800A897C = 0;
                D_800E25F4 = 1;
                D_800D1C84.unk28.unk10 = 1;
                D_800D1C84.unk28.unk8 = 0x3B;
                D_800D1C84.unk28.unk4 = 0;
                D_800D1C84.unk28.unk50[12] = 0;
                func_800A40F4(&D_800D1C84, 0);
            } else {
                st->unk14++;
            }
            shade = ~(st->unk14 * 2);
            fade = db->unk4368.unkC;
            setXY4(fade, 0, 0, 0x140, 0, 0, 0xF0, 0x140, 0xF0);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->unk4098[0], fade);
            fade++;
            db->unk4368.unkC = fade;
            flash = db->unk4368.unk14;
            setRGB0(flash, 0, 0, 0);
            setXY4(flash, 0, 0, 0, 0, 0, 0, 0, 0);
            flash->tpage = D_800AB894;
            flash->clut = D_800A8A68;
            SetSemiTrans(flash, 0);
            addPrim(&db->unk4098[1], flash);
            flash++;
            db->unk4368.unk14 = flash;
            if (st->unk14 >= 0x7E) {
                if (obj->unkD8 != -1) {
                    D_800EE42C--;
                    func_800A8204(obj->unkD4);
                    func_800A442C(obj->unkD8);
                    obj->unkD8 = -1;
                    obj->unkDA = 0;
                }
                D_800A897C = 0x4000;
            }
            break;
        case 253:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
                D_800A897C = 0;
                func_800A2938();
            } else {
                st->unk14++;
            }
            shade = st->unk14 * 2;
            fade = db->unk4368.unkC;
            setXY4(fade, 0, 0, 0x140, 0, 0, 0xF0, 0x140, 0xF0);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->unk4098[0], fade);
            fade++;
            db->unk4368.unkC = fade;
            flash = db->unk4368.unk14;
            setRGB0(flash, 0, 0, 0);
            setXY4(flash, 0, 0, 0, 0, 0, 0, 0, 0);
            flash->tpage = D_800AB894;
            flash->clut = D_800A8A68;
            SetSemiTrans(flash, 0);
            addPrim(&db->unk4098[1], flash);
            flash++;
            db->unk4368.unk14 = flash;
            if (st->unk14 >= 0x80) {
                if (obj->unkD8 != -1) {
                    D_800EE42C--;
                    func_800A8204(obj->unkD4);
                    func_800A442C(obj->unkD8);
                    obj->unkD8 = -1;
                    obj->unkDA = 0;
                }
                D_800A897C = 0x4000;
                D_800E25F4 = 0;
                D_800E2600 = 1;
            }
            break;
        case 250:
            if (D_800D16D8 < st->unk50[0]) {
                D_800D1C84.unk28.unk50[0] = 0x12C;
                D_800D1C84.unk28.unk50[1] = 0x190;
                D_800D1C84.unk28.unk50[2] = 0;
                D_800D1C84.unk28.unk0 = 0xFF;
                D_800D1C84.unk28.unk10 = 1;
                D_800D1C84.unk28.unk8 = 0x1E;
                D_800D1C84.unk28.unk4 = 0;
                D_800D1C84.unk28.unk50[12] = 0;
                D_800D1C84.unk0.vx = (s16)obj->unk0.vx;
                D_800D1C84.unk0.vy = (s16)obj->unk0.vy;
                D_800D1C84.unk0.vz = (s16)obj->unk0.vz;
                func_800A40F4(&D_800D1C84, 0);
            }
            release = 1;
            break;
        default:
            break;
        }
        if (release && obj->unkD8 != -1) {
            D_800EE42C--;
            func_800A8204(obj->unkD4);
            func_800A442C(obj->unkD8);
            obj->unkD8 = -1;
            obj->unkDA = 0;
        }
        obj->unkD4->m.t[0] = obj->unk0.vx;
        obj->unkD4->m.t[1] = obj->unk0.vy;
        obj->unkD4->m.t[2] = obj->unk0.vz;
        order = D_800A0008;
        if (order == 0) {
            RotMatrixYXZ(&obj->unk18, &obj->unkD4->m);
        }
        if (order == 1) {
            RotMatrixZYX(&obj->unk18, &obj->unkD4->m);
        }
        if (order == 2) {
            RotMatrix(&obj->unk18, &obj->unkD4->m);
        }
        if (drawMode == 0) {
            func_800A0874(db, obj->unkD4, otIndex, 0, obj);
        }
        if (drawMode == 1) {
            func_800A0D78(db, obj->unkD4, otIndex, 0, obj);
        }
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6B08);
#else
// Spawn an object at this one's position once its timer runs out.
void func_800A6B08(Unk800A4390* arg0) {
    u8 amount;
    s32 x;
    s32 y;
    s32 z;

    amount = D_800D1C5C >> 5;
    if (amount == 0) {
        amount = 1;
    }
    arg0->unk28.unk50[0xD] -= amount;
    if (arg0->unk28.unk50[0xD] < 0) {
        func_800A6BD8(arg0);
        return;
    }
    x = arg0->unk0.vx;
    y = arg0->unk0.vy;
    z = arg0->unk0.vz;
    D_800D1C84.unk28.unk0 = 0xCA;
    D_800D1C84.unk28.unk10 = 1;
    D_800D1C84.unk28.unk8 = 0x3F;
    D_800D1C84.unk28.unk4 = 0;
    D_800D1C84.unk28.unk50[0xC] = 0;
    setVector(&D_800D1C84.unk0, (s16)x, (s16)y, (s16)z);
    func_800A40F4(&D_800D1C84, 0);
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A6BD8);
#else
// Award the score for a hit object and scatter its debris.
void func_800A6BD8(Unk800A4390* obj) {
    s32* score;
    s32* frame;
    SVECTOR* path;
    s32 pathLen;
    s32 sound;
    s32 i;
    s32 x;
    s32 y;
    s32 z;
    s16 modelId;

    if (obj->unk28.unk50[10] == 1) {
        score = &D_800D16D8;
        *score += obj->unk28.unk50[0];
        func_800A29AC(obj->unk28.unk50[18]);
        obj->unk28.unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            modelId = rand() % 3 + 0x3F;
            D_800D1C84.unk28.unk0 = 0xCA;
            D_800D1C84.unk28.unk10 = 1;
            D_800D1C84.unk28.unk4 = 0;
            D_800D1C84.unk28.unk50[12] = 0;
            D_800D1C84.unk0.vx = (s16)x;
            D_800D1C84.unk0.vy = (s16)y;
            D_800D1C84.unk0.vz = (s16)z;
            D_800D1C84.unk28.unk8 = modelId;
            func_800A40F4(&D_800D1C84, 0);
        }
        D_800A89CC = 100;
        D_800E25E8 = 1;
        D_800EE18C.vx = 0;
        D_800EE18C.vy = 0;
        D_800EE18C.vz = 0;
        D_800A8CC4 = obj->unk28.unk50[0];
        D_800A8A88 = obj->unkD4->unk28;
    }
    if (obj->unk28.unk50[10] == 2) {
        score = &D_800D16D8;
        *score += obj->unk28.unk50[0];
        func_800A29AC(obj->unk28.unk50[18]);
        obj->unk28.unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            modelId = rand() % 3 + 0x3C;
            D_800D1C84.unk28.unk0 = 0xCB;
            D_800D1C84.unk28.unk10 = 1;
            D_800D1C84.unk28.unk4 = 0;
            D_800D1C84.unk28.unk50[12] = 0;
            D_800D1C84.unk0.vx = (s16)x;
            D_800D1C84.unk0.vy = (s16)y;
            D_800D1C84.unk0.vz = (s16)z;
            D_800D1C84.unk28.unk8 = modelId;
            func_800A40F4(&D_800D1C84, 0);
        }
        D_800A89CC = 100;
        D_800E25E8 = 1;
        D_800EE18C.vx = 0;
        D_800EE18C.vy = 0;
        D_800EE18C.vz = 0;
        D_800A8CC4 = obj->unk28.unk50[0];
        D_800A8A88 = obj->unkD4->unk28;
    }
    if (obj->unk28.unk50[10] == 3) {
        score = &D_800D16D8;
        *score += obj->unk28.unk50[0];
        obj->unk18.vx += obj->unk28.unk50[11];
    }
    if (obj->unk28.unk50[10] == 4) {
        score = &D_800D16D8;
        *score += obj->unk28.unk50[0];
        func_800A29AC(obj->unk28.unk50[18]);
        obj->unk28.unkC = 0;
        D_800A89CC = 100;
        D_800E25E8 = 1;
        D_800EE18C.vx = 0;
        D_800EE18C.vy = 0;
        D_800EE18C.vz = 0;
        D_800A8CC4 = obj->unk28.unk50[0];
        D_800A8A88 = obj->unkD4->unk28;
    }
    if (obj->unk28.unk50[10] == 5) {
        score = &D_800D16D8;
        *score += obj->unk28.unk50[0];
        func_800A29AC(obj->unk28.unk50[18]);
        obj->unk28.unkC = 0;
        for (i = 0; i < 100; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            modelId = rand() % 3 + 0x3F;
            D_800D1C84.unk28.unk0 = 0xCB;
            D_800D1C84.unk28.unk10 = 1;
            D_800D1C84.unk28.unk4 = 0;
            D_800D1C84.unk28.unk50[12] = 0;
            D_800D1C84.unk0.vx = (s16)x;
            D_800D1C84.unk0.vy = (s16)y;
            D_800D1C84.unk0.vz = (s16)z;
            D_800D1C84.unk28.unk8 = modelId;
            func_800A40F4(&D_800D1C84, 0);
        }
        D_800A89CC = 100;
        D_800E25E8 = 1;
        D_800EE18C.vx = 0;
        D_800EE18C.vy = 0;
        D_800EE18C.vz = 0;
        D_800A8CC4 = obj->unk28.unk50[0];
        D_800A8A88 = obj->unkD4->unk28;
    }
    score = &D_800D16D8;
    if (*score > 0x270F) {
        *score = 0x270F;
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A70D4);
#else
// Build the left and right frustum planes from the four corner rays.
void func_800A70D4(void) {
    VECTOR tl;
    VECTOR bl;
    VECTOR tr;
    VECTOR br;
    VECTOR blCorner;
    VECTOR brCorner;
    VECTOR tlCorner;
    VECTOR trCorner;
    s32 lx;
    s32 ly;
    s32 lz;
    s32 rx;
    s32 ry;
    s32 rz;

    blCorner = D_800A0410;
    brCorner = D_800A0420;
    tlCorner = D_800A0430;
    trCorner = D_800A0440;

    tl.vx = tlCorner.vx >> 2;
    tl.vy = tlCorner.vy >> 2;
    tl.vz = tlCorner.vz >> 2;
    bl.vx = blCorner.vx >> 2;
    bl.vy = blCorner.vy >> 2;
    bl.vz = blCorner.vz >> 2;
    tr.vx = trCorner.vx >> 2;
    tr.vy = trCorner.vy >> 2;
    tr.vz = trCorner.vz >> 2;
    br.vx = brCorner.vx >> 2;
    br.vy = brCorner.vy >> 2;
    br.vz = brCorner.vz >> 2;
    OuterProduct0(&tl, &bl, (VECTOR*)&D_800A892C);
    OuterProduct0(&tr, &br, (VECTOR*)&D_800A893C);

    lx = D_800A892C;
    ly = D_800A8930;
    lz = D_800A8934;
    rx = D_800A893C;
    ry = D_800A8940;
    rz = D_800A8944;
    D_800A8A5C = -(lx * (tlCorner.vx >> 2)) - (ly * (tlCorner.vy >> 2)) - (lz * (tlCorner.vz >> 2));
    D_800A8A64 = -(rx * (trCorner.vx >> 2)) - (ry * (trCorner.vy >> 2)) - (rz * (trCorner.vz >> 2));
    D_800A8950 = (lx * (trCorner.vx >> 2)) + (ly * (trCorner.vy >> 2)) + (lz * (trCorner.vz >> 2)) + D_800A8A5C;
    D_800A8968 = (rx * (tlCorner.vx >> 2)) + (ry * (tlCorner.vy >> 2)) + (rz * (tlCorner.vz >> 2)) + D_800A8A64;
    D_800AB890 = SquareRoot0((lx * lx) + (ly * ly) + (lz * lz));
    D_800D0550 = SquareRoot0((D_800A893C * D_800A893C) + (D_800A8940 * D_800A8940) + (D_800A8944 * D_800A8944));
}
#endif

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
    ClearOTagR(D_800AB898[0].unk70, LEN(D_800AB898[0].unk70));
    ClearOTagR(D_800AB898[1].unk70, LEN(D_800AB898[1].unk70));
    ClearOTagR(D_800AB898[0].unk4098, LEN(D_800AB898[0].unk4098));
    ClearOTagR(D_800AB898[1].unk4098, LEN(D_800AB898[1].unk4098));
    *D_800D1964 = &D_800AB898[0];
}

void func_800A7E1C(void) {
    ClearOTagR(D_800D1964[0]->unk70, LEN(D_800D1964[0]->unk70));
    ClearOTagR(D_800D1964[0]->unk4098, LEN(D_800D1964[0]->unk4098));
    func_800A7FAC(&D_800D1964[0]->unk4368);
}

// Tag every primitive in the nine pools with its type and length.
void func_800A7E70(Unk800A7FAC* arg0) {
    s32 i;

    for (i = 0; i < LEN(arg0->unk24); i++) {
        SetPolyF3(&arg0->unk24[i]);
    }
    for (i = 0; i < LEN(arg0->unk38); i++) {
        SetPolyF4(&arg0->unk38[i]);
    }
    for (i = 0; i < LEN(arg0->unk50); i++) {
        SetPolyG3(&arg0->unk50[i]);
    }
    for (i = 0; i < LEN(arg0->unkAF50); i++) {
        SetPolyG4(&arg0->unkAF50[i]);
    }
    for (i = 0; i < LEN(arg0->unkB388); i++) {
        SetPolyFT3(&arg0->unkB388[i]);
    }
    for (i = 0; i < LEN(arg0->unkB3A8); i++) {
        SetPolyFT4(&arg0->unkB3A8[i]);
    }
    for (i = 0; i < LEN(arg0->unkE288); i++) {
        SetPolyGT3(&arg0->unkE288[i]);
    }
    for (i = 0; i < LEN(arg0->unkE2B0); i++) {
        SetPolyGT4(&arg0->unkE2B0[i]);
    }
    for (i = 0; i < LEN(arg0->unkE2E4); i++) {
        SetLineF2(&arg0->unkE2E4[i]);
    }
}

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
                           s16 arg8, s16 arg9, s16 arg10) {
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
