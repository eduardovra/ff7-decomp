#ifndef JET_PRIVATE_H
#define JET_PRIVATE_H

#include "types.h"
#include <game.h>
#include <inline_o.h>
#include <libetc.h>

// Nine write cursors, each reset to the start of its own buffer below.
typedef struct {
    /* 0x0000 */ POLY_F3* f3Cursor;
    /* 0x0004 */ POLY_F4* f4Cursor;
    /* 0x0008 */ POLY_G3* g3Cursor;
    /* 0x000C */ POLY_G4* g4Cursor;
    /* 0x0010 */ POLY_FT3* ft3Cursor;
    /* 0x0014 */ POLY_FT4* ft4Cursor;
    /* 0x0018 */ POLY_GT3* gt3Cursor;
    /* 0x001C */ POLY_GT4* gt4Cursor;
    /* 0x0020 */ LINE_F2* lineCursor;
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
    /* 0x0070 */ OT_TYPE ot[0x1000];
    /* 0x4070 */ u_long unk4070[10];
    /* 0x4098 */ OT_TYPE ot2[0xB4];
    /* 0x4368 */ JetPrimBuffer prims;
} JetBuffer; // size: 0x1265C

typedef struct {
    /* 0x00 */ s16 triCount;
    /* 0x02 */ s16 quadCount;
    /* 0x04 */ SVECTOR unk4;
    /* 0x0C */ SVECTOR unkC;
} JetModelInfo; // size: 0x14

typedef struct {
    /* 0x00 */ SVECTOR v0;
    /* 0x08 */ SVECTOR v1;
    /* 0x10 */ SVECTOR v2;
    /* 0x18 */ CVECTOR c0;
    /* 0x1C */ CVECTOR c1;
    /* 0x20 */ CVECTOR c2;
} JetTriangle; // size: 0x24

typedef struct {
    /* 0x00 */ s32 unk0[10];
} JetQuad; // size: 0x28

typedef struct {
    /* 0x00 */ s16 polyCount;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 triCount;
    /* 0x06 */ s16 quadCount;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 : 16;
    /* 0x0C */ JetTriangle* tris;
    /* 0x10 */ JetQuad* quads;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s32 : 32;
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

// One scheduled object spawn, read from xbin stream 0xE.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 : 16;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 : 16;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10[0x14];
} Unk800D1C0C; // size: 0x60

// Argument block for the GTE renderers in jet_gte.s.
typedef struct {
    /* 0x0 */ JetTriangle* tris;
    /* 0x4 */ POLY_G3* prim;
    /* 0x8 */ OT_TYPE* ot;
    /* 0xC */ JetModel* model;
} Unk800A8604; // size: 0x10

// A doubly-linked draw-list entry, parallel to the array it orders. Both
// links are indices into that array, with 0xFFFF for the ends.
typedef struct {
    /* 0x0 */ u16 prev;
    /* 0x2 */ u16 next;
} JetListLink; // size: 0x4

// The behaviour state an object's type handler drives.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 hit;
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
    /* 0xD0 */ s32 : 32;
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

extern u8 D_800A8928;
extern s32 D_800A8A84;
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
extern u16 g_JetSpriteTPage[];
extern s32 D_800A89D0; // fog near
extern s32 D_800A89D4; // fog far
extern JetModelInfo* g_JetModelInfo;
extern u16 g_JetTrackListHead;
extern s32 D_800A89E0;
extern s32 D_800A8958;
extern u_long D_800A89E4; // loaded TIM address table
extern u16 D_800A8A68;    // background clut
extern s32 g_JetLeftPlaneDistance;
extern u16 g_JetTriangleListHead;
extern s32 g_JetRightPlaneDistance;
extern s32 D_800A8A6C; // pad direction code, 1..9 keypad layout
extern s32 g_JetTriangleCursor;
extern s32 D_800A8A7C;
extern s32 D_800A8A80; // frames R1 has been held
extern s16 g_JetPopupTimer;
extern s16 g_JetPopupPoints;
extern JetNode* D_800A8A74[1];
extern s16 g_JetPopupModelId;
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
extern s32 g_JetScore;
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
extern MATRIX* g_JetViewMatrix;
extern MATRIX* g_JetWorldMatrix;
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
extern u8 g_JetFiring;
extern u16 D_800D1C50;
extern s32 D_800D1C54;
extern SVECTOR* g_JetTrackLeft;
extern u16 g_JetShotPower;
extern u16* D_800D1C60;
extern u16 g_JetTriangleListCount;
extern u8 D_800D1C7C;
extern u16 D_800D1C80;
extern Unk800A4390 D_800D1C84;
extern Unk800A4390 D_800D1DC0[0x64];
extern u16 D_800D9930;
extern DR_MODE D_800D9934;
extern u16 D_800D9940;
extern s16 g_JetNextFreeNode;
extern JetListLink g_JetTrackLinks[];
extern u16 g_JetCursorX;
extern u16 g_JetCursorY;
extern u8 D_800E25E8;
extern u8 D_800E25F4;
extern u8 D_800E25F8;
extern s32 D_800E25FC;
extern u8 D_800E2600;
extern s32* D_800E2604;
extern JetListLink g_JetTriangleLinks;
extern u16* D_800EE188;
extern SVECTOR g_JetPopupRot;
extern u16 g_JetSpriteClut[];
extern SVECTOR* g_JetTrackRight;
extern JetNode g_JetNodeListTails[10];
extern MATRIX g_JetCameraRot;
extern SVECTOR* g_JetTrackRot;
extern u16* D_800EE428;
extern s16 D_800EE42C;
extern void* D_80110BB8;

void JetPrimsInit(JetPrimBuffer* prims);
void JetPrimCursorsReset(JetPrimBuffer* prims);
void JetNodeInit(JetNode* node, s16 index);
void JetNodeIndexFree(s16 index);
void JetNodeUnlink(JetNode* node);
s16 JetNodeIndexAlloc(void);
void JetNodeLink(JetNode* node, JetNode* parent);
void func_800A2518(void);
void func_800A27F0(u_long* addr);
void JetNodeFree(JetNode* node);
void func_800A442C(s16 arg0);
s16 func_800A4400(void);
void func_800A2DE4(s32 arg0, s32 arg1);
void func_800A29AC(s16 arg0);
s16 func_800A40F4(Unk800A4390* arg0, s16 arg1);
void* JetDrawModelTris(Unk800A8604* arg0);
void JetProject3Points(SVECTOR* arg0, u_long* arg1);
void JetProject6Points(SVECTOR* arg0, u_long* arg1);
void func_800A0874(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, Unk800A4390* obj);
void* JetDrawModelTrisUI(Unk800A8604* arg0);
POLY_G3* JetDrawTriangle(JetTriangle* arg0, POLY_G3* arg1, OT_TYPE* arg2, JetTriangle* arg3);
POLY_FT4* JetDrawTrackQuad(SVECTOR* arg0, POLY_FT4* arg1, OT_TYPE* arg2, SVECTOR* arg3);
JetModel* JetModelAlloc(void);
JetTriangle* JetTrianglesAlloc(s32 count);
JetQuad* JetQuadsAlloc(s32 count);
void func_800A6BD8(Unk800A4390* arg0);
s32 JetVectorInsidePlanes(VECTOR* arg0);
void func_800A6B08(Unk800A4390* arg0);
void JetDrawTriangleList(void);
void JetDrawTrack(void);
void JetSetWorldMatrix();
void JetDrawEnergyGauge();
void JetDrawScorePopup(JetBuffer* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_800A2420(void);
void func_800A2BE0(void);
void func_800A334C(void);
void func_800A3AAC(void);
void func_800A70D4(void);
void JetModelsReset(void);
JetModel* JetModelBuild(s32 infoIndex);
void JetBuffersInit(void);
void JetNodesInit(void);
void JetTrackSample(u32 at, s32 lift, VECTOR* pos, SVECTOR* rot);
void JetDrawNumber(s32 value, s32 x, s32 y, s16 padWithZero, u16 v);
void JetDrawSprite(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans);
void func_800A2058();
void func_800A2214();
void func_800A2860(void);
void func_800A2B78(void);
void func_800A2C50(void);
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
    s16 modelId, s32 arg1, s32 arg2, s32 arg3, JetNode* parent, s32 x, s32 y, s32 z, s16 rotX, s16 rotY, s16 rotZ);

#endif
