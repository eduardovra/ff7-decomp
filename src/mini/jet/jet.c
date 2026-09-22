//! PSYQ=3.3 FORCE_MEM=true

#include "types.h"
#include <game.h>
#include <libetc.h>
#include <libc.h>

// Nine write cursors, each reset to the start of its own buffer below.
typedef struct {
    /* 0x0000 */ void* f3Cursor;
    /* 0x0004 */ void* f4Cursor;
    /* 0x0008 */ void* g3Cursor;
    /* 0x000C */ void* g4Cursor;
    /* 0x0010 */ void* ft3Cursor;
    /* 0x0014 */ void* ft4Cursor;
    /* 0x0018 */ void* gt3Cursor;
    /* 0x001C */ void* gt4Cursor;
    /* 0x0020 */ void* lineCursor;
    /* 0x0024 */ POLY_F3 f3[1];
    /* 0x0038 */ POLY_F4 f4[1];
    /* 0x0050 */ POLY_G3 g3[0x640];
    /* 0xAF50 */ POLY_G4 g4[0x1E];
    /* 0xB388 */ POLY_FT3 ft3[1];
    /* 0xB3A8 */ POLY_FT4 ft4[0x12C];
    /* 0xE288 */ POLY_GT3 gt3[1];
    /* 0xE2B0 */ POLY_GT4 gt4[1];
    /* 0xE2E4 */ LINE_F2 line[1];
} JetPrimBuffer; // size: 0xE2F4

typedef struct {
    /* 0x0000 */ DRAWENV draw;
    /* 0x005C */ DISPENV disp;
    /* 0x0070 */ u_long ot[0x1000];
    /* 0x4070 */ u_long unk4070[10];
    /* 0x4098 */ u_long ot2[0xB4];
    /* 0x4368 */ JetPrimBuffer prims;
} JetBuffer; // size: 0x1265C

typedef struct {
    /* 0x00 */ s16 triCount;
    /* 0x02 */ s16 quadCount;
    /* 0x04 */ SVECTOR unk4;
    /* 0x0C */ SVECTOR unkC;
} JetModelInfo; // size: 0x14

typedef struct {
    /* 0x00 */ s16 polyCount;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 triCount;
    /* 0x06 */ s16 quadCount;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ char padA[2];
    /* 0x0C */ s32* tris;
    /* 0x10 */ s32* quads;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ char pad1C[4];
} JetModel; // size: 0x20

// Doubly linked list node, chained by JetNodesInit with a 0x38 stride.
typedef struct JetNode {
    /* 0x00 */ JetModel* model;
    /* 0x04 */ MATRIX m;
    /* 0x24 */ struct JetNode* parent;
    /* 0x28 */ s16 modelId;
    /* 0x2A */ s16 index;
    /* 0x2C */ u16 depth;
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ struct JetNode* prev;
    /* 0x34 */ struct JetNode* next;
} JetNode; // size: 0x38

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ char pad4[0x24];
} JetQuad; // size: 0x28

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ char pad4[0x20];
} JetTriangle; // size: 0x24

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
    /* 0xC */ JetModel* model;
} Unk800A8604; // size: 0x10

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
} Unk800E2608; // size: 0x4

// The behaviour state an object's type handler drives.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ char pad20[8];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ char pad38[0x18];
    /* 0x50 */ s32 unk50[0x14];
} Unk800D1CAC; // size: 0xA0

typedef struct {
    /* 0x00 */ VECTOR unk0;
    /* 0x10 */ char pad10[8];
    /* 0x18 */ SVECTOR unk18; // spawn rotation
    /* 0x20 */ char pad20[8];
    /* 0x28 */ Unk800D1CAC unk28;
    /* 0xC8 */ s32 unkC8;      // the object path's length
    /* 0xCC */ SVECTOR* unkCC; // the object path itself
    /* 0xD0 */ char padD0[4];
    /* 0xD4 */ JetNode* unkD4;
    /* 0xD8 */ s16 unkD8;
    /* 0xDA */ s16 unkDA;
    /* 0xDC */ SVECTOR unkDC[6]; // the model bounding box's six face centres
    /* 0x10C */ char pad10C[0x10];
    /* 0x11C */ u_long unk11C[6]; // the same six points projected to the screen
    /* 0x134 */ char pad134[8];
} Unk800A4390; // size: 0x13C

extern RECT D_800A0000;
extern u8 D_800A0008;          // the rotation order the object matrices use
extern JetModel* D_800D189C[]; // the models the animated objects switch between
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
MATRIX D_800A8340 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
MATRIX D_800A8360 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
MATRIX D_800A8380 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
SVECTOR D_800A83A0 = {0, 0, 0, 0}; // world rotation
VECTOR D_800A83A8 = {0, 0, 0, 0};
VECTOR D_800A83B8 = {0, 0, 0, 0};
VECTOR D_800A83C8 = {0, 0, 0, 0};
VECTOR D_800A83D8 = {0, 0, 0, 0};
s32 D_800A83E8[2] = {0, 0};

extern u8 D_800A8928;
extern volatile s32 D_800A8A84;
extern void* D_800A891C;
extern void* D_800A8920;
extern s32 g_JetLeftPlaneNormalX;
extern s32 g_JetLeftPlaneNormalY;
extern s32 g_JetLeftPlaneNormalZ;
extern s32 g_JetRightPlaneNormalX;
extern s32 g_JetRightPlaneNormalY;
extern s32 g_JetRightPlaneNormalZ;
extern s32 D_800A894C;
extern s32 D_800A8950;
extern s16 D_800A895C;
extern s16 D_800A8960;
extern s16 D_800A8964;
extern s16 D_800A896C;
extern s16 D_800A8970;
extern s16 D_800A8974;
extern s16 D_800A8978;
extern s16 D_800A8980;
extern SVECTOR* D_800A8954;
extern s32 D_800A897C;
extern s32 D_800A8968;
extern s32 D_800A8984;
extern SVECTOR* D_800A8988;
extern s32 D_800A898C;
extern u16 D_800A8990[]; // sprite tpage table
extern s32 D_800A89D0;   // fog near
extern s32 D_800A89D4;   // fog far
extern JetModelInfo* g_JetModelInfo;
extern u16 D_800A89DC;
extern s32 D_800A89E0;
extern s32 D_800A8958;
extern u_long D_800A89E4; // loaded TIM address table
extern u16 D_800A8A68;    // background clut
extern s32 g_JetLeftPlaneDistance;
extern u16 D_800A8A60;
extern s32 g_JetRightPlaneDistance;
extern s32 D_800A8A6C; // pad direction code, 1..9 keypad layout
extern s32 g_JetTriangleCursor;
extern s32 D_800A8A7C;
extern s32 D_800A8A80; // frames R1 has been held
extern s16 D_800A89CC;
extern s16 D_800A8CC4;
extern JetNode* D_800A8A74[1];
extern s16 D_800A8A88;
extern u32 g_JetModelCount;
extern JetNode g_JetNodeListHeads[10];
extern s32* D_800A8CC0;
extern JetTriangle* g_JetTriangles;
extern u32 D_800A8CC8;
extern JetNode g_JetNodePool[0xC8];
extern u16 D_800AB894; // background tpage
extern s32 g_JetLeftNormalLength;
extern JetBuffer g_JetBuffers[2];
extern s32 g_JetRightNormalLength;
extern JetModel g_JetModelPool[];
extern s32 D_800D16D8;
extern u8 D_800D16DC;
extern void* D_800D16D4;
extern s32 D_800D16E0;
extern JetNode g_JetRootNode;
extern s32 g_JetQuadCursor;
extern u8 D_800D1720; // laser beam texture scroll
extern s32 D_800D1724;
extern u32 D_800D172C;
extern JetModel* g_JetModelTable[];
extern u8 D_800D1960;
extern JetBuffer* g_JetBufferPtr[1];
extern u16* D_800D196C;
extern JetQuad* g_JetQuads;
extern u16 D_800D1970[];
extern s16 g_JetNodeFreeList[0xC8];
extern void* D_800D1A38;
extern void* D_800D1A3C;
extern JetModel* D_800D186C; // shadow triangles
extern MATRIX* D_800D1728;   // view matrix
extern MATRIX* D_800D1BD0;   // world matrix
extern u_long D_800D1BD4;
extern JetModelInfo* g_JetModelInfoBase;
extern void* D_800D1BDC;
extern void* D_800D1BE0;
extern u8* D_800D1BE4;
extern s32* D_800D1BE8;
extern s32* D_800D1BEC;
extern void* D_800D1BF0;
extern void* D_800D1BF4;
extern void* D_800D1BF8;
extern JetTriangle* g_JetTrianglesBase;
extern u8* D_800D1C00;
extern s32* D_800D1C04;
extern s32* D_800D1C08;
extern Unk800D1C0C* D_800D1C0C;
extern u8* D_800D1C10;
extern JetQuad* g_JetQuadsBase;
extern u8 D_800D1C4C;
extern u16 D_800D1C50;
extern s32 D_800D1C54;
extern SVECTOR* D_800D1C58;
extern u16 D_800D1C5C;
extern u16* D_800D1C60;
extern u16 D_800D1C78;
extern u8 D_800D1C7C;
extern u16 D_800D1C80;
extern Unk800A4390 D_800D1C84;
extern Unk800A4390 D_800D1DC0[0x64];
extern u16 D_800D9930;
extern DR_MODE D_800D9934;
extern u16 D_800D9940;
extern s16 g_JetNextFreeNode;
extern Unk800E2608 D_800D9948[];
extern u16 D_800E25EC;
extern u16 D_800E25F0;
extern u8 D_800E25E8;
extern u8 D_800E25F4;
extern u8 D_800E25F8;
extern s32 D_800E25FC;
extern u8 D_800E2600;
extern s32* D_800E2604;
extern Unk800E2608 D_800E2608;
extern u16* D_800EE188;
extern SVECTOR D_800EE18C;
extern u16 D_800EE198[]; // sprite clut table
extern SVECTOR* D_800EE194;
extern JetNode g_JetNodeListTails[10];
extern MATRIX D_800EE404; // camera rotation
extern SVECTOR* D_800EE424;
extern u16* D_800EE428;
extern u16 D_800EE42C;
extern void* D_80110BB8;

void JetPrimsInit(JetPrimBuffer* arg0);
void JetPrimCursorsReset(JetPrimBuffer* arg0);
void JetNodeInit(JetNode* arg0, s16 arg1);
void JetNodeIndexFree(s16);
void JetNodeUnlink(JetNode*);
s16 JetNodeIndexAlloc(void);
void JetNodeLink(JetNode* arg0, JetNode* arg1);
void func_800A2518(void);
void func_800A27F0(u_long* addr);
void JetNodeFree(JetNode* arg0);
void func_800A442C(s16 arg0);
s16 func_800A4400(void);
void func_800A2DE4(s32 arg0, s32 arg1);
s16 func_800A40F4(Unk800A4390* arg0, s16 arg1);
void* func_800A84DC(Unk800A8604* arg0);
void func_800A84A4(s32* arg0, u_long* arg1);
void func_800A83F0(SVECTOR* arg0, u_long* arg1);
void func_800A0874(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, Unk800A4390* obj);
void* func_800A8604(Unk800A8604* arg0);
u_long* func_800A8734(JetTriangle* arg0, u_long* arg1, u_long* arg2, JetTriangle* arg3);
u_long* func_800A882C(SVECTOR* arg0, u_long* arg1, u_long* arg2, SVECTOR* arg3);
JetModel* JetModelAlloc(void);
s32* JetTrianglesAlloc(s32 count);
s32* JetQuadsAlloc(s32 count);
void func_800A6BD8(Unk800A4390* arg0);
s32 JetVectorInsidePlanes(VECTOR* arg0);
void func_800A6B08(Unk800A4390* arg0);
void func_800A12EC(void);
void func_800A13AC(void);
void func_800A1450();
void func_800A1A64();
void func_800A1B64(JetBuffer* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_800A2420(void);
void func_800A2BE0(void);
void func_800A334C(void);
void func_800A3AAC(void);
void func_800A70D4(void);
void JetModelsReset(void);
JetModel* JetModelBuild(s32 arg0);
void JetBuffersInit(void);
void JetNodesInit(void);
void func_800A16A4(u32 at, s32 lift, VECTOR* pos, SVECTOR* rot);
void func_800A1CD8(s32 value, s32 x, s16 y, s16 padWithZero, u16 v);
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
void func_800A46E8(JetBuffer* db);
void func_800A0D78(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, Unk800A4390* obj);
JetNode* JetNodeAlloc(
    s16 arg0, s32 arg1, s32 arg2, s32 arg3, JetNode* arg4, s32 arg5, s32 arg6, s32 arg7, s16 arg8, s16 arg9, s16 arg10);

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", MINI_Jet);
#else
// The dummy local reproduces the target stack frame.
u16 MINI_Jet(void) {
    volatile s32 dummy;
    JetBuffer** db;
    JetBuffer* var_a2;
    s32* speed;
    SVECTOR** path;
    s32 temp_s0;

    func_800A2214();
    SetDrawMode(&D_800D9934, 0, 1, GetTPage(1, 1, 0x300, 0) & 0xFFFF, NULL);
    D_800EE424 = D_800D1BF0;
    func_800A2DE4(0, 0);
    db = g_JetBufferPtr;
    speed = &D_800A897C;
    path = &D_800A8988;
    D_800D1C58 = *path;
    func_800A2DE4(1, 0);
    D_800EE194 = *path;
    temp_s0 = 0x20;
    func_800A2860();
    SetFogNearFar(D_800A89D0, D_800A89D4, 0x100);
    D_800A8A74[0] = JetNodeAlloc(0x1E, 0, 0, 1, &g_JetRootNode, 0x4B0, 0x32, 0xBB8, 0, 0x3E8, 0);
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
        DrawOTag(&db[0]->ot[0xFFF]);
        DrawOTag(&db[0]->ot2[0xB3]);
    }
    var_a2 = g_JetBuffers;
    D_800E25FC = 0;
    if (db[0] == var_a2) {
        var_a2++;
    }
    db[0] = var_a2;
    ClearOTagR(var_a2->ot, 0x1000);
    ClearOTagR(db[0]->ot2, 0xB4);
    JetPrimCursorsReset(&db[0]->prims);
    goto loop;
done:
    *D_8009A000 = 0xB8;
    D_8009A004 = 0;
    AkaoExec();
    return D_800D16D8;
}
#endif

// Draw one object's model, project its bounding box and flag a cursor hit.
void func_800A0874(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, Unk800A4390* obj) {
    Unk800A8604 args;
    s16 xs[6];
    s16 ys[6];
    MATRIX** world;
    MATRIX* m;
    MATRIX* wm;
    MATRIX* cam;
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
    if (node->parent != &g_JetRootNode) {
        CompMatrix(&node->parent->m, m, m);
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
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = func_800A84DC(&args);
    func_800A83F0(obj->unkDC, obj->unk11C);
    ys[0] = obj->unk11C[0] >> 16;
    minY = ys[0];
    maxY = minY;
    xs[0] = obj->unk11C[0];
    minX = xs[0];
    maxX = minX;
    for (i = 1; i < 6; i++) {
        ys[i] = (obj->unk11C[i] & 0xFFFF0000) >> 16;
        xs[i] = obj->unk11C[i];
        if (minX > xs[i]) {
            minX = xs[i];
        }
        if (maxX < xs[i]) {
            maxX = xs[i];
        }
        if (minY > ys[i]) {
            minY = ys[i];
        }
        if (maxY < ys[i]) {
            maxY = ys[i];
        }
    }
    if (JetVectorInsidePlanes((VECTOR*)&D_800D1BD0->t[0])) {
        obj->unk28.unk4 = 0;
        if ((s16)D_800E25EC < maxX && minX < (s16)D_800E25EC && (s16)D_800E25F0 < maxY && minY < (s16)D_800E25F0 &&
            D_800D1C4C == 1) {
            obj->unk28.unk4 = D_800D1C4C;
        }
    }
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A0D78);
#else
// The two locals before screen exist only to reproduce the target stack frame.
void func_800A0D78(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, Unk800A4390* obj) {
    Unk800A8604 args;
    MATRIX unused;
    u_long screen[12];
    MATRIX** world;
    MATRIX* m;
    MATRIX* wm;
    MATRIX* cam;
    JetModel** shadow;
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
    if (node->parent != &g_JetRootNode) {
        CompMatrix(&node->parent->m, m, m);
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
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot2[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = func_800A84DC(&args);
    shadow = &D_800D186C;
    func_800A84A4(shadow[0]->tris, screen);
    D_800A8964 = screen[1] >> 16;
    D_800A895C = screen[1];
    D_800A896C = screen[2] >> 16;
    D_800A8960 = screen[2];
    func_800A84A4(shadow[0]->tris + 9, screen);
    xy1 = screen[1];
    xy2 = screen[2];
    D_800A8978 = xy1 >> 16;
    D_800A8970 = xy1;
    D_800A8980 = xy2 >> 16;
    D_800A8974 = xy2;
}
#endif

// Load a node's matrix into the GTE and draw its model's triangles.
void func_800A1198(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, s32 arg4) {
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
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot2[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = func_800A8604(&args);
}

// Draw every background triangle on the draw list, front to back.
void func_800A12EC(void) {
    Unk800E2608* list;
    JetTriangle* tris;
    u16 triId;
    u_long* ot;

    ot = g_JetBufferPtr[0]->prims.g3Cursor;
    tris = g_JetTrianglesBase;
    if (D_800D1C78) {
        triId = D_800A8A60;
        list = &D_800E2608;
        do {
            ot = func_800A8734(&tris[triId], ot, g_JetBufferPtr[0]->ot, &tris[triId]);
            triId = list[triId].unk2;
        } while (triId != 0xFFFF);
    }
    g_JetBufferPtr[0]->prims.g3Cursor = ot;
}

// Draw every track element on the draw list, front to back.
void func_800A13AC(void) {
    Unk800E2608* list;
    SVECTOR* left;
    SVECTOR* right;
    u16 trackId;
    u_long* ot;

    trackId = D_800A89DC;
    ot = g_JetBufferPtr[0]->prims.ft4Cursor;
    list = D_800D9948;
// A loop keyword makes gcc duplicate the exit test, so the goto is load-bearing.
loop:
    left = D_800D1C58;
    right = D_800EE194;
    ot = func_800A882C(&left[trackId], ot, g_JetBufferPtr[0]->ot, &right[trackId]);
    trackId = list[trackId].unk2;
    if (trackId != 0xFFFF) {
        goto loop;
    }
    g_JetBufferPtr[0]->prims.ft4Cursor = ot;
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
    dx >>= 16;
    dy >>= 16;
    dz >>= 16;

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
    nextMid.vy = (rightNext->vy + leftNext->vy) >> 1;
    nextMid.vz = (rightNext->vz + leftNext->vz) >> 1;
    along.vx = nextMid.vx - curMid.vx;
    along.vy = nextMid.vy - curMid.vy;
    along.vz = nextMid.vz - curMid.vz;

    across.vx = right.vx - left.vx;
    across.vy = right.vy - left.vy;
    across.vz = right.vz - left.vz;

    OuterProduct0(&along, &across, &normal);
    VectorNormal(&normal, &unit);

    pos->vx = (s16)mid.vx + ((unit.vx * lift) >> 12);
    pos->vy = (s16)mid.vy + ((unit.vy * lift) >> 12);
    pos->vz = (s16)mid.vz + ((unit.vz * lift) >> 12);

    rot->vx = rotCur->vx + dx;
    rot->vy = dy - rotCur->vy;
    rot->vz = rotCur->vz + dz;
}

void func_800A1A64(void) {
    JetBuffer** db;
    POLY_G4* poly;
    s16 power;
    s32 top;

    db = g_JetBufferPtr;
    poly = db[0]->prims.g4Cursor;
    power = D_800D1C5C;
    top = 0xDC - power;
    setXY4(poly, 0x14, top, 0x1C, top, 0x14, 0xDC, 0x1C, 0xDC);
    setRGB0(poly, -0x80 - power, power, 0);
    setRGB1(poly, -0x80 - power, power, 0);
    setRGB2(poly, 0x80, 0, 0);
    setRGB3(poly, 0x80, 0, 0);
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->ot2[1], poly);
    poly++;
    db[0]->prims.g4Cursor = poly;
}

// Spin and draw the score model, alternating it with the title every so often.
void func_800A1B64(JetBuffer* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4) {
    JetNode* node;
    u8* alternate;
    s16* counter;
    s32 index;
    s32 unused;

    if (arg1 == 0 || arg1 == 0x5B) {
        return;
    }
    alternate = &D_800E25E8;
    index = 0;
    node = D_800A8A74[index];
    node->model = g_JetModelTable[arg1];
    D_800EE18C.vx += arg2;
    D_800EE18C.vy += arg3;
    D_800EE18C.vz += arg4;
    if (alternate[0] == 1) {
        RotMatrix(&D_800EE18C, &D_800A8A74[index]->m);
        func_800A1198(arg0, D_800A8A74[index], 0, 0, unused);
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

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A1CD8);
#else
void func_800A1CD8(s32 value, s32 x, s16 y, s16 padWithZero, u16 v) {
    POLY_FT4* poly;
    JetBuffer* db;
    s32 digit;
    s32 power;
    s32 remain;
    s32 i;
    u16 startX;
    s32 w;
    u8 leading;

    startX = x;
    power = 1000;
    leading = 1;
    remain = value + 1;
    w = 0x10;
    poly = g_JetBufferPtr[0]->prims.ft4Cursor;
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
            setXY4(poly, x, y, startX + w, y, x, y + 0x10, startX + w, y + 0x10);
            setRGB0(poly, 0x80, 0x80, 0x80);
            setUVWH(poly, digit * 0x10 + 0x30, v, 0x10, 0x12);
            poly->tpage = D_800A8990[8];
            poly->clut = D_800EE198[8];
            SetSemiTrans(poly, 1);
            db = g_JetBufferPtr[0];
            addPrim(&db->ot2[1], poly);
            poly++;
        }
        power /= 10;
        w += 0xE;
        x += 0xE;
    }
    g_JetBufferPtr[0]->prims.ft4Cursor = poly;
}
#endif

// Draw one sprite from the HUD sprite table.
void func_800A1F18(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans) {
    JetBuffer** db;
    POLY_FT4* poly;

    db = g_JetBufferPtr;
    poly = db[0]->prims.ft4Cursor;
    setXYWH(poly, x, y, w, h);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUVWH(poly, u, v, uw, vh);
    poly->tpage = D_800A8990[spriteId];
    poly->clut = D_800EE198[spriteId];
    SetSemiTrans(poly, semiTrans);
    addPrim(&db[0]->ot2[1], poly);
    poly++;
    db[0]->prims.ft4Cursor = poly;
}

// Queue two blank textured quads, one at each end of the background OT.
void func_800A2058(void) {
    JetBuffer** db;
    POLY_FT4* poly;

    db = g_JetBufferPtr;
    poly = db[0]->prims.ft4Cursor;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = D_800A8990[5];
    poly->clut = D_800EE198[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->ot[0xFFF], poly);
    poly++;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = D_800A8990[5];
    poly->clut = D_800EE198[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->ot[2], poly);
    poly++;
    db[0]->prims.ft4Cursor = poly;
}

// Point every matrix and vector at scratchpad, then build the world.
void func_800A2214(void) {
    volatile s32* state;
    s32 i;

    state = &D_800A8A84;
    *state = 0;
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
    JetBuffersInit();
    func_800A2420();
    *state = 0x99;
    func_800A334C();
    JetNodesInit();
    JetModelsReset();
    func_800A2BE0();
    func_800A3AAC();
    func_800A70D4();
    for (i = 0; i < 0x64; i++) {
        g_JetModelTable[i] = JetModelBuild(i);
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
    AkaoExec();
    D_8009A000[0] = 0xC0;
    D_8009A004 = 0x7F;
    AkaoExec();
    D_8009A000[0] = 0xB8;
    D_8009A004 = 0x7F;
    AkaoExec();
    D_8009A000[0] = 0xBC;
    D_8009A004 = 0;
    AkaoExec();
    D_8009A000[0] = 0xA2;
    D_8009A004 = 0;
    AkaoExec();
    D_8009A000[0] = 0x2A;
    D_8009A004 = 0x40;
    D_8009A008 = 0x177;
    AkaoExec();
}

void func_800A2938(void) {
    D_8009A000[0] = 0xC1;
    D_8009A004 = 0xF0;
    D_8009A008 = 0;
    AkaoExec();
    D_8009A000[0] = 0xB9;
    D_8009A004 = 0xF0;
    D_8009A008 = 0;
    AkaoExec();
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
        AkaoExec();
        D_8009A000[0] = 0x28;
        D_8009A004 = 0x40;
        D_8009A008 = arg0;
        AkaoExec();
    }
    if (*pChannel == 1) {
        D_8009A000[0] = 0xB1;
        D_8009A004 = 0;
        AkaoExec();
        D_8009A000[0] = 0x29;
        D_8009A004 = 0x40;
        D_8009A008 = arg0;
        AkaoExec();
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
            AkaoExec();
        } else {
            *D_8009A000 = 0x2B;
            D_8009A004 = 0x40;
            D_8009A008 = 0;
            AkaoExec();
            D_800A8958 = 0;
            return;
        }
    }
    temp_s0 = arg0 & 0xFF;
    if (temp_s0) {
        D_800A833C = temp_s0;
        *D_8009A000 = 0xB3;
        D_8009A004 = temp_s0;
        AkaoExec();
        *temp = temp_s0;
    } else {
        *D_8009A000 = 0x2B;
        D_8009A004 = 0x40;
        D_8009A008 = 0;
        AkaoExec();
        D_800A8958 = 0;
    }
}

void func_800A2B78(void) {
    D_8009A000[0] = 0xA2;
    D_8009A004 = D_800A8338;
    AkaoExec();
    D_8009A000[0] = 0xA3;
    D_8009A004 = D_800A833C;
    AkaoExec();
}

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

void func_800A2DE4(s32 arg0, s32 arg1) {
    s32 elem;
    u8* base;

    elem = D_800A8CC0[arg0 & 0xFF];
    base = D_800D1BE4;
    D_800A8988 = (SVECTOR*)(base + elem);
    D_800D1724 = *D_800E2604;
}

void func_800A2E30(void) {}

// Read the pad and drive the cursor, the camera tweaks and the pause toggle.
void func_800A2E38(void) {
    u32 pad;
    s32* dir;
    u16* cursorX;
    u16* cursorY;
    u8* shoot;
    u16* power;
    u16* powerRegen; // a second pointer keeps this branch's address out of a saved register
    u8* repeat;
    u8* scroll;
    s32* fogFar;
    s32* fogNear;
    s32* speed;
    s32* brake;
    s32* held;
    u8* paused;
    u8 next;
    VECTOR* cam;
    s32 count;

    pad = InputReadPadsRaw(1);
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
                powerRegen = &D_800D1C5C;
                if ((s16)*powerRegen < 0x80) {
                    (*powerRegen)++;
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
                cam = &D_800A83D8;
                cam->vz -= 100;
            }
            if (pad & 0x10) {
                cam = &D_800A83D8;
                cam->vz += 100;
            }
            if (pad & 0x80) {
                D_800A83D8.vx -= 100;
            }
            if (pad & 0x20) {
                D_800A83D8.vx += 100;
            }
            if (pad & 0x8) {
                cam = &D_800A83D8;
                cam->vy -= 100;
            }
            if (pad & 0x2) {
                cam = &D_800A83D8;
                cam->vy += 100;
            }
            if (pad & 0x4) {
                speed = &D_800A897C;
                *speed += 0x400;
            }
            if (pad & 0x1) {
                brake = &D_800A897C;
                if (*brake >= 0x400) {
                    *brake -= 0x400;
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
    u32* pos;
    s32* segment;
    u32 prev;
    u32 next;
    u32 steps;
    u32 i;
    u16** tri;
    u16** quad;
    u32 id;
    u8* first;

    pos = &D_800D172C;
    D_800A8CC8 = pos[0];
    pos[0] = D_800A8CC8 + advance;
    prev = D_800A8CC8 >> 18;
    next = pos[0] >> 18;
    steps = next - prev;
    segment = &D_800D16E0;
    segment[0] = segment[0] + steps;
    for (i = 0; i < steps + D_800E25F8; i++) {
        tri = &D_800D1C60;
        quad = &D_800EE428;
        while (1) {
            id = *tri[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A385C(id);
        }
        while (1) {
            id = *quad[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A38D4(id);
        }
    }
    for (i = 0; i < steps; i++) {
        tri = &D_800D196C;
        quad = &D_800EE188;
        while (1) {
            id = *tri[0]++;
            if (id == 0xFFFF) {
                break;
            }
            func_800A3980(id);
        }
        while (1) {
            id = *quad[0]++;
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

void func_800A35DC(s32 advance) {
    u32* pos;
    s32* segment;
    u32 prev;
    u32 next;
    u32 i;
    u16** tri;
    u16** track;
    u32 id;
    u8* first;

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

void func_800A3AAC(void) {
    Unk800A4390* obj;
    Unk800A4390* pool;
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
void func_800A3D50(JetBuffer* arg0) {
    POLY_FT4* poly;
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;

    poly = arg0->prims.ft4Cursor;
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
    addPrim(&arg0->ot[1], poly);
    poly++;
    arg0->prims.ft4Cursor = poly;
}

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A3E58);
#else
// Draw the two laser beams, from each gun muzzle to the aiming cursor.
void func_800A3E58(void) {
    JetBuffer** db;
    POLY_FT4* poly;
    u8* scroll;
    s16 power;
    s32 spread;

    if (D_800D1C4C == 1) {
        db = g_JetBufferPtr;
        scroll = &D_800D1720;
        power = D_800D1C5C;
        spread = power >> 3;
        poly = db[0]->prims.ft4Cursor;
        setXY4(poly, D_800A895C + spread, D_800A8964, D_800E25EC, D_800E25F0, D_800A895C - spread, D_800A8964,
               D_800E25EC, D_800E25F0);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = D_800A8990[1];
        poly->clut = D_800EE198[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->ot[1], poly);
        poly++;
        setXY4(poly, D_800A8970 + spread, D_800A8978, D_800E25EC, D_800E25F0, D_800A8970 - spread, D_800A8978,
               D_800E25EC, D_800E25F0);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = D_800A8990[1];
        poly->clut = D_800EE198[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->ot[1], poly);
        poly++;
        db[0]->prims.ft4Cursor = poly;
    }
}
#endif

#ifndef NON_MATCHINGS
INCLUDE_ASM("asm/us/mini/jet/nonmatchings/jet", func_800A40F4);
#else
// model's six bounding box face centres.
s16 func_800A40F4(Unk800A4390* src, s16 parentIndex) {
    s16* count;
    Unk800A4390* obj;
    Unk800A4390* pool;
    Unk800A4390* parentObj;
    Unk800A4390* box;
    JetModelInfo* info;
    s16 index;
    s32 rawId;
    s16 modelId;
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
        pool = D_800D1DC0;
        obj = &pool[index];
        rawId = src->unk28.unk8;
        modelId = rawId;
        obj->unkDA = 1;
        obj->unkD8 = index;
        if (parentIndex == 0) {
            obj->unkD4 = JetNodeAlloc(rawId, 0, 0, 1, &g_JetRootNode, src->unk0.vx, src->unk0.vy, src->unk0.vz,
                                      src->unk18.vx, src->unk18.vy, src->unk18.vz);
        } else {
            parentObj = &pool[parentIndex];
            obj->unkD4 = JetNodeAlloc(rawId, 0, 0, 1, parentObj->unkD4, src->unk0.vx, src->unk0.vy, src->unk0.vz,
                                      src->unk18.vx, src->unk18.vy, src->unk18.vz);
        }
        info = &g_JetModelInfo[modelId];
        pool = D_800D1DC0; // reloading the base keeps it out of a saved register
        box = &pool[index];
        minX = info->unk4.vx;
        maxX = info->unkC.vx;
        minY = info->unk4.vy;
        maxY = info->unkC.vy;
        minZ = info->unk4.vz;
        maxZ = info->unkC.vz;
        setVector(&box->unkDC[0], (maxX + minX) >> 1, (maxY + minY) >> 1, maxZ);
        setVector(&box->unkDC[1], (maxX + minX) >> 1, (maxY + minY) >> 1, minZ);
        setVector(&box->unkDC[2], (maxX + minX) >> 1, maxY, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[3], (maxX + minX) >> 1, minY, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[4], maxX, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[5], minX, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
    }
    return index;
}
#endif

void func_800A4390(Unk800A4390* arg0) {
    u16* temp;

    if (arg0->unkD8 != -1) {
        temp = &D_800EE42C;
        *temp -= 1;
        JetNodeFree(arg0->unkD4);
        func_800A442C(arg0->unkD8);
        arg0->unkD8 = -1;
        arg0->unkDA = 0;
    }
}

s16 func_800A4400(void) {
    u16* temp;
    s16 result;

    temp = &D_800D9940;
    result = *temp;
    *temp = D_800D1970[result];

    return result;
}

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

void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    D_800D1C84.unk0.vx = arg0;
    D_800D1C84.unk0.vy = arg1;
    D_800D1C84.unk0.vz = arg2;
    D_800D1C84.unk28.unk0 = arg3;
    D_800D1C84.unk28.unk10 = 1;
    D_800D1C84.unk28.unk8 = arg4;
    D_800D1C84.unk28.unk4 = 0;
    func_800A40F4(&D_800D1C84, 0);
}

inline void func_800A4650(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    D_800D1C84.unk0.vx = arg0;
    D_800D1C84.unk0.vy = arg1;
    D_800D1C84.unk0.vz = arg2;
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
void func_800A46E8(JetBuffer* db) {
    VECTOR next;
    VECTOR unusedA; // the two unused vectors reproduce the target stack frame
    VECTOR unusedB;
    VECTOR pos;
    SVECTOR rot;
    Unk800A4390* obj;
    Unk800A4390* pool;
    Unk800D1CAC* st;
    POLY_G4* fade;
    POLY_FT4* flash;
    s32 rawId;
    s16 modelId;
    s32 shade;
    s32 count;
    s32 i;
    s32 j;
    s32 dx;
    s32 dy;
    s32 dz;
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
                obj->unkD4->model = D_800D189C[st->unk50[14]];
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
                func_800A4650(0x3446, -0x2710, 0x20CB, 0xC, 0x2A);
            }
            if (obj->unkD8 != -1) {
                D_800EE42C--;
                JetNodeFree(obj->unkD4);
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
                    s32 x;
                    s32 y;
                    s32 z;

                    x = obj->unk0.vx;
                    y = obj->unk0.vy;
                    z = obj->unk0.vz;
                    func_800A4650(x, y, z, 9, rand() % 3 + 0x44);
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
                    s32 x;
                    s32 y;
                    s32 z;

                    y = obj->unk0.vy + 0x1F4;
                    x = obj->unk0.vx + rand() % 100 - 0x32;
                    z = obj->unk0.vz + rand() % 100 - 0x32;
                    func_800A4650(x, y, z, 0xF, 0x2A);
                }
                {
                    s32 x;
                    s32 y;
                    s32 z;

                    x = obj->unk0.vx;
                    y = obj->unk0.vy;
                    z = obj->unk0.vz;
                    func_800A4650(x, y, z, 0x10, 0x29);
                }
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
            {
                s32 x;
                s32 y;
                s32 z;

                x = obj->unk0.vx;
                y = obj->unk0.vy;
                z = obj->unk0.vz;
                func_800A4650(x, y, z, 0xCA, 0x2A);
            }
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
                func_800A4650(0, 0, 0, 0xFD, 0x1D);
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
                D_800A897C = 0;
                D_800E25F4 = 1;
                func_800A4650(0, 0, 0, 3, 0x3B);
            } else {
                st->unk14++;
            }
            shade = ~(st->unk14 * 2);
            fade = db->prims.g4Cursor;
            setXY4(fade, 0, 0, 0x140, 0, 0, 0xF0, 0x140, 0xF0);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->ot2[0], fade);
            fade++;
            db->prims.g4Cursor = fade;
            flash = db->prims.ft4Cursor;
            setRGB0(flash, 0, 0, 0);
            setXY4(flash, 0, 0, 0, 0, 0, 0, 0, 0);
            flash->tpage = D_800AB894;
            flash->clut = D_800A8A68;
            SetSemiTrans(flash, 0);
            addPrim(&db->ot2[1], flash);
            flash++;
            db->prims.ft4Cursor = flash;
            if (st->unk14 >= 0x7E) {
                if (obj->unkD8 != -1) {
                    D_800EE42C--;
                    JetNodeFree(obj->unkD4);
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
            fade = db->prims.g4Cursor;
            setXY4(fade, 0, 0, 0x140, 0, 0, 0xF0, 0x140, 0xF0);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->ot2[0], fade);
            fade++;
            db->prims.g4Cursor = fade;
            flash = db->prims.ft4Cursor;
            setRGB0(flash, 0, 0, 0);
            setXY4(flash, 0, 0, 0, 0, 0, 0, 0, 0);
            flash->tpage = D_800AB894;
            flash->clut = D_800A8A68;
            SetSemiTrans(flash, 0);
            addPrim(&db->ot2[1], flash);
            flash++;
            db->prims.ft4Cursor = flash;
            if (st->unk14 >= 0x80) {
                if (obj->unkD8 != -1) {
                    D_800EE42C--;
                    JetNodeFree(obj->unkD4);
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
                {
                    s32 x;
                    s32 y;
                    s32 z;

                    x = obj->unk0.vx;
                    y = obj->unk0.vy;
                    z = obj->unk0.vz;
                    func_800A4650(x, y, z, 0xFF, 0x1E);
                }
            }
            release = 1;
            break;
        default:
            break;
        }
        if (release && obj->unkD8 != -1) {
            D_800EE42C--;
            JetNodeFree(obj->unkD4);
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

void func_800A6B08(Unk800A4390* arg0) {
    Unk800D1CAC* state = &arg0->unk28;
    u8 amount;
    s32 x;
    s32 y;
    s32 z;

    amount = D_800D1C5C >> 5;
    if (amount == 0) {
        amount = 1;
    }
    state->unk50[0xD] -= amount;
    if (state->unk50[0xD] < 0) {
        func_800A6BD8(arg0);
    } else {
        x = arg0->unk0.vx;
        y = arg0->unk0.vy;
        z = arg0->unk0.vz;
        func_800A4650(x, y, z, 0xCA, 0x3F);
    }
}

// Award the score for a hit object and scatter its debris.
void func_800A6BD8(Unk800A4390* obj) {
    Unk800D1CAC* st = &obj->unk28;
    s32* score;
    s32* frame;
    SVECTOR* path;
    s32 pathLen;
    s32 sound;
    s32 i;
    s16 modelId;

    if (st->unk50[10] == 1) {
        s32* score;
        s32 points;
        s32 x;
        s32 y;
        s32 z;

        score = &D_800D16D8;
        *score += st->unk50[0];
        func_800A29AC(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCA, rand() % 3 + 0x3F);
        }
        D_800A8A88 = obj->unkD4->modelId;
        points = st->unk50[0];
        D_800A8CC4 = points;
        D_800A89CC = 100;
        D_800E25E8 = 1;
        setVector(&D_800EE18C, 0, 0, 0);
    }
    if (st->unk50[10] == 2) {
        s32* score;
        s32 points;
        s32 x;
        s32 y;
        s32 z;

        score = &D_800D16D8;
        *score += st->unk50[0];
        func_800A29AC(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCB, rand() % 3 + 0x3C);
        }
        D_800A8A88 = obj->unkD4->modelId;
        points = st->unk50[0];
        D_800A8CC4 = points;
        D_800A89CC = 100;
        D_800E25E8 = 1;
        setVector(&D_800EE18C, 0, 0, 0);
    }
    if (st->unk50[10] == 3) {
        s32* score;
        s32 points;

        score = &D_800D16D8;
        *score += st->unk50[0];
        points = st->unk50[11];
        obj->unk18.vx += points;
    }
    if (st->unk50[10] == 4) {
        s32* score;
        s32 points;
        score = &D_800D16D8;
        *score += st->unk50[0];
        func_800A29AC(st->unk50[18]);
        st->unkC = 0;
        D_800A8A88 = obj->unkD4->modelId;
        points = st->unk50[0];
        D_800A8CC4 = points;
        D_800A89CC = 100;
        D_800E25E8 = 1;
        setVector(&D_800EE18C, 0, 0, 0);
    }
    if (st->unk50[10] == 5) {
        s32* score;
        s32 points;
        s32 x;
        s32 y;
        s32 z;

        score = &D_800D16D8;
        *score += st->unk50[0];
        func_800A29AC(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 100; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCB, rand() % 3 + 0x3F);
        }
        D_800A8A88 = obj->unkD4->modelId;
        points = st->unk50[0];
        D_800A8CC4 = points;
        D_800A89CC = 100;
        D_800E25E8 = 1;
        setVector(&D_800EE18C, 0, 0, 0);
    }
    score = &D_800D16D8;
    if (*score > 0x270F) {
        *score = 0x270F;
    }
}

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
    VECTOR* ln;
    VECTOR* rn;
    s32 lx;
    s32 ly;
    s32 lz;
    s32 rx;
    s32 ry;
    s32 rz;

    ln = (VECTOR*)&g_JetLeftPlaneNormalX;
    rn = (VECTOR*)&g_JetRightPlaneNormalX;
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
    OuterProduct0(&tl, &bl, ln);
    OuterProduct0(&tr, &br, rn);

    lx = ln->vx;
    ly = ln->vy;
    lz = ln->vz;
    rx = rn->vx;
    ry = rn->vy;
    rz = rn->vz;
    g_JetLeftPlaneDistance = -(lx * (tlCorner.vx >> 2)) - (ly * (tlCorner.vy >> 2)) - (lz * (tlCorner.vz >> 2));
    g_JetRightPlaneDistance = -(rx * (trCorner.vx >> 2)) - (ry * (trCorner.vy >> 2)) - (rz * (trCorner.vz >> 2));
    D_800A8950 =
        (lx * (trCorner.vx >> 2)) + (ly * (trCorner.vy >> 2)) + (lz * (trCorner.vz >> 2)) + g_JetLeftPlaneDistance;
    D_800A8968 =
        (rx * (tlCorner.vx >> 2)) + (ry * (tlCorner.vy >> 2)) + (rz * (tlCorner.vz >> 2)) + g_JetRightPlaneDistance;
    g_JetLeftNormalLength = SquareRoot0((lx * lx) + (ly * ly) + (lz * lz));
    g_JetRightNormalLength = SquareRoot0((rn->vx * rn->vx) + (rn->vy * rn->vy) + (rn->vz * rn->vz));
}

s32 JetVectorInsidePlanes(VECTOR* arg0) {
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
    lx = g_JetLeftPlaneNormalX;
    ly = g_JetLeftPlaneNormalY;
    lz = g_JetLeftPlaneNormalZ;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + g_JetLeftPlaneDistance;
    rx = g_JetRightPlaneNormalX;
    ry = g_JetRightPlaneNormalY;
    rz = g_JetRightPlaneNormalZ;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + g_JetRightPlaneDistance;
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

s32 JetSVectorInsidePlanes(SVECTOR* arg0) {
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
    lx = g_JetLeftPlaneNormalX;
    ly = g_JetLeftPlaneNormalY;
    lz = g_JetLeftPlaneNormalZ;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + g_JetLeftPlaneDistance;
    rx = g_JetRightPlaneNormalX;
    ry = g_JetRightPlaneNormalY;
    rz = g_JetRightPlaneNormalZ;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + g_JetRightPlaneDistance;
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

s32 JetLeftPlaneHalfSpace(s32 arg0, s32 arg1, s32 arg2) {
    s32 a;
    s32 b;
    s32 c;

    a = g_JetLeftPlaneNormalX;
    b = g_JetLeftPlaneNormalY;
    c = g_JetLeftPlaneNormalZ;

    return (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + g_JetLeftPlaneDistance;
}

s32 JetRightPlaneHalfSpace(s32 arg0, s32 arg1, s32 arg2) {
    s32 a;
    s32 b;
    s32 c;
    a = g_JetRightPlaneNormalX;
    b = g_JetRightPlaneNormalY;
    c = g_JetRightPlaneNormalZ;

    return (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + g_JetRightPlaneDistance;
}

s32 JetSphereInsidePlanes(VECTOR* arg0, s16 arg1) {
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
    lx = g_JetLeftPlaneNormalX;
    ly = g_JetLeftPlaneNormalY;
    lz = g_JetLeftPlaneNormalZ;
    hsLeft = (lx * (arg0->vx >> 2)) + (ly * (arg0->vy >> 2)) + (lz * (arg0->vz >> 2)) + g_JetLeftPlaneDistance;
    if (D_800A8950 > 0 && hsLeft >= 0) {
        leftOk = 1;
    }
    if (D_800A8950 < 0 && hsLeft <= 0) {
        leftOk = 1;
    }
    if (leftOk == 0) {
        len = g_JetLeftNormalLength;
        planeDistance = ((hsLeft < 0) ? -hsLeft : hsLeft) / len;
        if (planeDistance < arg1) {
            leftOk = 1;
        }
    }
    rx = g_JetRightPlaneNormalX;
    ry = g_JetRightPlaneNormalY;
    rz = g_JetRightPlaneNormalZ;
    hsRight = (rx * (arg0->vx >> 2)) + (ry * (arg0->vy >> 2)) + (rz * (arg0->vz >> 2)) + g_JetRightPlaneDistance;
    if (D_800A8968 > 0 && hsRight >= 0) {
        rightOk = 1;
    }
    if (D_800A8968 < 0 && hsRight <= 0) {
        rightOk = 1;
    }
    if (rightOk == 0) {
        len = g_JetRightNormalLength;
        planeDistance = ((hsRight < 0) ? -hsRight : hsRight) / len;
        if (planeDistance < arg1) {
            rightOk = 1;
        }
    }
    return leftOk & rightOk;
}

s32 JetSphereInsideLeftPlane(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 hs;
    s32 ok;
    s32 len;

    a = g_JetLeftPlaneNormalX;
    b = g_JetLeftPlaneNormalY;
    c = g_JetLeftPlaneNormalZ;
    ok = 0;
    hs = (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + g_JetLeftPlaneDistance;
    if (D_800A8950 > 0 && hs >= 0) {
        ok = 1;
    }
    if (D_800A8950 < 0 && hs <= 0) {
        ok = 1;
    }
    if (ok == 0) {
        len = g_JetLeftNormalLength;
        if (hs < 0) {
            hs = -hs;
        }
        if (hs / len < arg3) {
            ok = 1;
        }
    }
    return ok;
}

s32 JetSphereInsideRightPlane(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 hs;
    s32 ok;
    s32 len;

    a = g_JetRightPlaneNormalX;
    b = g_JetRightPlaneNormalY;
    c = g_JetRightPlaneNormalZ;
    ok = 0;
    hs = (a * (arg0 >> 2)) + (b * (arg1 >> 2)) + (c * (arg2 >> 2)) + g_JetRightPlaneDistance;
    if (D_800A8968 > 0 && hs >= 0) {
        ok = 1;
    }
    if (D_800A8968 < 0 && hs <= 0) {
        ok = 1;
    }
    if (ok == 0) {
        len = g_JetRightNormalLength;
        if (hs < 0) {
            hs = -hs;
        }
        if (hs / len < arg3) {
            ok = 1;
        }
    }
    return ok;
}

void JetModelsReset(void) {
    g_JetTriangleCursor = 0;
    g_JetQuadCursor = 0;
    g_JetModelCount = 0;
    g_JetTriangles = g_JetTrianglesBase;
    g_JetQuads = g_JetQuadsBase;
    g_JetModelInfo = g_JetModelInfoBase;
}

JetModel* JetModelBuild(s32 arg0) {
    JetModel* model;
    s32 numTri;
    s32 numQua;

    model = JetModelAlloc();
    numTri = g_JetModelInfo[arg0].triCount;
    numQua = g_JetModelInfo[arg0].quadCount;
    model->unk16 = g_JetModelInfo[arg0].unk4.vx;
    model->unk14 = g_JetModelInfo[arg0].unkC.vx;
    model->unk1A = g_JetModelInfo[arg0].unk4.vz;
    model->unk18 = g_JetModelInfo[arg0].unkC.vz;
    model->unk2 = 0;
    model->triCount = numTri;
    model->quadCount = numQua;
    model->unk8 = 0;
    model->polyCount = numTri + numQua;
    model->tris = JetTrianglesAlloc(numTri);
    model->quads = JetQuadsAlloc(numQua);
    return model;
}

JetModel* JetModelAlloc(void) {
    u32* counter;
    JetModel* base;
    s32 index;

    counter = &g_JetModelCount;
    index = *counter;
    base = g_JetModelPool;
    *counter = index + 1;
    return &base[index];
}

s32* JetTrianglesAlloc(s32 count) {
    s32* cursor;
    JetTriangle* base;
    s32 index;

    cursor = &g_JetTriangleCursor;
    index = *cursor;
    *cursor = index + count;
    base = g_JetTriangles;
    return &base[index].unk0;
}

s32* JetQuadsAlloc(s32 count) {
    s32* cursor;
    JetQuad* base;
    s32 index;

    cursor = &g_JetQuadCursor;
    index = *cursor;
    *cursor = index + count;
    base = g_JetQuads;
    return &base[index].unk0;
}

// No PC counterpart; the port replaced the PSX double buffer with the DirectX driver
void JetBuffersInit(void) {
    JetPrimBuffer* temp_s1;
    JetBuffer* db;
    u_char* isbg;

    SetDefDrawEnv(&g_JetBuffers[0].draw, 0, 0, 0x140, 0xF0);
    SetDefDispEnv(&g_JetBuffers[0].disp, 0, 0xF0, 0x140, 0xF0);
    SetDefDrawEnv(&g_JetBuffers[1].draw, 0, 0xF0, 0x140, 0xF0);
    SetDefDispEnv(&g_JetBuffers[1].disp, 0, 0, 0x140, 0xF0);
    db = g_JetBuffers;
    g_JetBuffers[0].draw.isbg = 0;
    // Stored off the buffer base register; a direct field store folds to an absolute address.
    isbg = &db[1].draw.isbg;
    *isbg = 0;
    setRGB0(&g_JetBuffers[0].draw, 0, 0, 8);
    setRGB0(&g_JetBuffers[1].draw, 0, 0, 8);
    SetGeomOffset(0xA0, 0xA0);
    SetGeomScreen(0x100);
    SetDispMask(1);
    SetBackColor(0x80, 0x80, 0x80);
    SetFarColor(0, 0, 8);
    temp_s1 = &g_JetBuffers[0].prims;
    JetPrimsInit(temp_s1);
    JetPrimsInit(&g_JetBuffers[1].prims);
    JetPrimCursorsReset(temp_s1);
    JetPrimCursorsReset(&g_JetBuffers[1].prims);
    ClearOTagR(g_JetBuffers[0].ot, LEN(g_JetBuffers[0].ot));
    ClearOTagR(g_JetBuffers[1].ot, LEN(g_JetBuffers[1].ot));
    ClearOTagR(g_JetBuffers[0].ot2, LEN(g_JetBuffers[0].ot2));
    ClearOTagR(g_JetBuffers[1].ot2, LEN(g_JetBuffers[1].ot2));
    *g_JetBufferPtr = &g_JetBuffers[0];
}

void JetBufferReset(void) {
    ClearOTagR(g_JetBufferPtr[0]->ot, LEN(g_JetBufferPtr[0]->ot));
    ClearOTagR(g_JetBufferPtr[0]->ot2, LEN(g_JetBufferPtr[0]->ot2));
    JetPrimCursorsReset(&g_JetBufferPtr[0]->prims);
}

// Tag every primitive in the nine pools with its type and length.
void JetPrimsInit(JetPrimBuffer* arg0) {
    s32 i;

    for (i = 0; i < LEN(arg0->f3); i++) {
        SetPolyF3(&arg0->f3[i]);
    }
    for (i = 0; i < LEN(arg0->f4); i++) {
        SetPolyF4(&arg0->f4[i]);
    }
    for (i = 0; i < LEN(arg0->g3); i++) {
        SetPolyG3(&arg0->g3[i]);
    }
    for (i = 0; i < LEN(arg0->g4); i++) {
        SetPolyG4(&arg0->g4[i]);
    }
    for (i = 0; i < LEN(arg0->ft3); i++) {
        SetPolyFT3(&arg0->ft3[i]);
    }
    for (i = 0; i < LEN(arg0->ft4); i++) {
        SetPolyFT4(&arg0->ft4[i]);
    }
    for (i = 0; i < LEN(arg0->gt3); i++) {
        SetPolyGT3(&arg0->gt3[i]);
    }
    for (i = 0; i < LEN(arg0->gt4); i++) {
        SetPolyGT4(&arg0->gt4[i]);
    }
    for (i = 0; i < LEN(arg0->line); i++) {
        SetLineF2(&arg0->line[i]);
    }
}

void JetPrimCursorsReset(JetPrimBuffer* arg0) {
    arg0->f3Cursor = arg0->f3;
    arg0->f4Cursor = arg0->f4;
    arg0->g3Cursor = arg0->g3;
    arg0->g4Cursor = arg0->g4;
    arg0->ft3Cursor = arg0->ft3;
    arg0->ft4Cursor = arg0->ft4;
    arg0->gt3Cursor = arg0->gt3;
    arg0->gt4Cursor = arg0->gt4;
    arg0->lineCursor = arg0->line;
}

void JetNodesInit(void) {
    JetNode* a;
    JetNode* b;
    s32 i;

    JetNodeInit(&g_JetRootNode, 0);
    g_JetRootNode.depth = 0;
    g_JetNextFreeNode = 0;
    for (i = 0; i < LEN(g_JetNodeFreeList); i++) {
        g_JetNodeFreeList[i] = i + 1;
    }
    for (i = 0; i < LEN(g_JetNodeListHeads); i++) {
        a = &g_JetNodeListHeads[i];
        b = &g_JetNodeListTails[i];
        a->prev = NULL;
        a->next = b;
        b->prev = a;
        b->next = NULL;
    }
}

void JetNodeInit(JetNode* arg0, s16 arg1) {
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
    arg0->parent = &g_JetRootNode;
    arg0->index = arg1;
    arg0->prev = 0;
    arg0->next = 0;
}

JetNode* JetNodeAlloc(s16 arg0, s32 arg1, s32 arg2, s32 arg3, JetNode* arg4, s32 arg5, s32 arg6, s32 arg7, s16 arg8,
                      s16 arg9, s16 arg10) {
    SVECTOR sp10;
    JetNode* temp_s0;
    JetNode* temp_v1;
    s16 temp_v0;

    temp_v0 = JetNodeIndexAlloc();
    temp_v1 = g_JetNodePool;
    temp_s0 = &temp_v1[temp_v0];
    JetNodeLink(temp_s0, arg4);
    temp_s0->model = g_JetModelTable[arg0];
    temp_s0->modelId = arg0;
    temp_s0->index = temp_v0;
    setVector(&sp10, arg8, arg9, arg10);
    RotMatrix(&sp10, &temp_s0->m);
    temp_s0->m.t[0] = arg5;
    temp_s0->m.t[1] = arg6;
    temp_s0->m.t[2] = arg7;
    return temp_s0;
}

void JetNodeFree(JetNode* arg0) {
    JetNodeUnlink(arg0);
    JetNodeIndexFree(arg0->index);
}

s16 JetNodeIndexAlloc(void) {
    s16* head;
    s16 result;

    head = &g_JetNextFreeNode;
    result = *head;
    *head = g_JetNodeFreeList[result];

    return result;
}

void JetNodeIndexFree(s16 arg0) {
    s16* temp;
    s16* temp2;

    temp2 = &g_JetNodeFreeList[arg0];
    temp = &g_JetNextFreeNode;
    *temp2 = *temp;
    *temp = arg0;
}

void JetNodeLink(JetNode* arg0, JetNode* arg1) {
    JetNode* temp_v0_2;
    JetNode* temp_v1;
    s16 temp_v0;

    arg0->parent = arg1;
    temp_v0 = arg1->depth + 1;
    arg0->depth = temp_v0;
    temp_v1 = &g_JetNodeListTails[temp_v0];
    temp_v0_2 = temp_v1->prev;
    arg0->prev = temp_v0_2;
    arg0->next = temp_v0_2->next;
    temp_v1->prev->next = arg0;
    temp_v1->prev = arg0;
}

void JetNodeUnlink(JetNode* arg0) {
    arg0->prev->next = arg0->next;
    arg0->next->prev = arg0->prev;
}
