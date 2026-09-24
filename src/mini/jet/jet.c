//! PSYQ=3.3 FORCE_MEM=true

#include "jet_private.h"
#include <libc.h>

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

extern u8 D_800A8928;
extern s32 D_800A8A84;
extern void* D_800A891C;
extern void* D_800A8920;
extern s32 D_800A894C;
extern s16 D_800A8960;
extern s16 D_800A896C;
extern s16 D_800A8974;
extern s16 D_800A8980;
extern SVECTOR* D_800A8988;
extern s32 D_800A89D0; // fog near
extern s32 D_800A89D4; // fog far
extern u16 g_JetTrackListHead;
extern s32 D_800A8958;
extern u_long D_800A89E4; // loaded TIM address table
extern u16 g_JetTriangleListHead;
extern s32 D_800A8A6C; // pad direction code, 1..9 keypad layout
extern s32 D_800A8A7C;
extern s32 D_800A8A80; // frames R1 has been held
extern JetNode* D_800A8A74[1];
extern s32* D_800A8CC0;
extern u32 D_800A8CC8;
extern u8 D_800D16DC;
extern void* D_800D16D4;
extern s32 D_800D1724;
extern u32 D_800D172C;
extern u8 D_800D1960;
extern u16* D_800D196C;
extern void* D_800D1A38;
extern void* D_800D1A3C;
extern MATRIX* g_JetViewMatrix;
extern MATRIX* g_JetWorldMatrix;
extern u_long D_800D1BD4;
extern void* D_800D1BDC;
extern void* D_800D1BE0;
extern void* D_800D1BF0;
extern void* D_800D1BF4;
extern void* D_800D1BF8;
extern u16 D_800D1C50;
extern SVECTOR* g_JetTrackLeft;
extern u16* D_800D1C60;
extern u16 g_JetTriangleListCount;
extern u16 D_800D1C80;
extern u16 D_800D9930;
extern DR_MODE D_800D9934;
extern JetListLink g_JetTrackLinks[9000];
extern u8 D_800E25F8;
extern s32 D_800E25FC;
extern s32* D_800E2604;
extern JetListLink g_JetTriangleLinks[12000];
extern u16* D_800EE188;
extern SVECTOR* g_JetTrackRight;
extern MATRIX g_JetCameraRot;
extern SVECTOR* g_JetTrackRot;
extern u16* D_800EE428;
extern void* D_80110BB8;
void* JetDrawModelTris(Unk800A8604* arg0);
void JetProject3Points(SVECTOR* arg0, u_long* arg1);
void JetProject6Points(SVECTOR* arg0, u_long* arg1);
void* JetDrawModelTrisUI(Unk800A8604* arg0);
POLY_G3* JetDrawTriangle(JetTriangle* arg0, POLY_G3* arg1, OT_TYPE* arg2, JetTriangle* arg3);
POLY_FT4* JetDrawTrackQuad(SVECTOR* arg0, POLY_FT4* arg1, OT_TYPE* arg2, SVECTOR* arg3);

static void JetDrawEnergyGauge();
static void JetDrawNumber(s32 value, s32 x, s32 y, s16 padWithZero, u16 v);
static void JetDrawScorePopup(JetBuffer* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4);
static void JetDrawSprite(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans);
static void JetDrawTrack(void);
static void JetDrawTriangleList(void);
static void JetSetWorldMatrix();
static void func_800A2058();
static void func_800A2214();
static void func_800A2518(void);
static void func_800A27F0(u_long* addr);
static void func_800A2860(void);
static void func_800A2B78(void);
static void func_800A2BE0(void);
static void func_800A2C50(void);
static void func_800A2DE4(s32 arg0, s32 arg1);
static void func_800A2E38();
static void func_800A334C(void);
static void func_800A35DC(s32 arg0);
static void func_800A372C(s32 arg0);
static void func_800A385C(u16 arg0);
static void func_800A38D4(u16 arg0);
static void func_800A3980(u16 arg0);
static void func_800A3A20(u16 arg0);
static void func_800A2420(void);

static const RECT D_800A0000 = {0, 0, 320, 200};

// .data in ROM order, the sector and size pairs feeding func_800A2420's loads.
static s32 D_800A8310 = 0x9D8;
static u32 D_800A8314 = 0x28;
static s32 D_800A8318 = 0x9D9;
static u32 D_800A831C = 0x4DE8;
static s32 D_800A8320 = 0x9E3;
static u32 D_800A8324 = 0x44;
static s32 D_800A8328 = 0x9E4;
static u32 D_800A832C = 0xA7958;
static s32 D_800A8330 = 0x7F;
static s32 D_800A8334 = 0x7F;
static s32 D_800A8338 = 0;
static s32 D_800A833C = 0;
static MATRIX D_800A8340 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
static MATRIX D_800A8360 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
static MATRIX D_800A8380 = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
static SVECTOR D_800A83A0 = {0, 0, 0, 0}; // world rotation
static VECTOR g_JetCameraPos = {0, 0, 0, 0};
VECTOR D_800A83B8 = {0, 0, 0, 0};
static VECTOR D_800A83C8 = {0, 0, 0, 0};
static VECTOR D_800A83D8 = {0, 0, 0, 0};
static s32 D_800A83E8[2] = {0, 0};

u16 MINI_Jet(void) {
    volatile s32 dummy;
    JetBuffer* next;
    JetBuffer* current;
    s32* speed;
    volatile s32* frame;
    SVECTOR** path;

    func_800A2214();
    SetDrawMode(&D_800D9934, 0, 1, GetTPage(1, 1, 768, 0) & 0xFFFF, NULL);
    g_JetTrackRot = D_800D1BF0;
    func_800A2DE4(0, 0);
    path = &D_800A8988;
    g_JetTrackLeft = *path;
    func_800A2DE4(1, 0);
    g_JetTrackRight = *path;
    func_800A2860();
    SetFogNearFar(D_800A89D0, D_800A89D4, 256);
    D_800A8A74[0] = JetNodeAlloc(30, 0, 0, 1, &g_JetRootNode, 1200, 50, 3000, 0, 1000, 0);
    for (;;) {
        speed = &D_800A897C;
        if ((g_JetTrackSegment * 4) > (D_800D1724 - 0x10) || D_800E2600 == 1) {
            break;
        }
        func_800A2E38();
        if (D_800D16DC == 0) {
            func_800A2C50();
            func_800A35DC(*speed);
            JetSetWorldMatrix();
            JetDrawTrack();
            JetDrawTriangleList();
            JetDrawScorePopup(g_JetBufferPtr[0], g_JetPopupModelId, 5, 40, 0);
            func_800A372C(*speed);
            func_800A46E8(g_JetBufferPtr[0]);
            JetDrawNumber(g_JetScore, 244, 200, 0, 0);
            JetDrawSprite(7, 204, 200, 39, 17, 0, 0, 0x27, 0x11, 0);
            JetDrawSprite(11, 18, 86, 12, 140, 0, 0x70, 0xC, 0x8C, 0);
            JetDrawEnergyGauge();
            if (*speed < 0x4000) {
                D_800A8338 = 0;
            } else {
                D_800A8338 = 0x7F;
            }
        } else {
            JetDrawSprite(9, 202, 192, 96, 32, 0, 0x50, 0x60, 0x20, 0);
            D_800A8338 = 0;
            D_800A833C = 0;
        }
        func_800A2B78();
        func_800A2058();
        JetDrawSprite(10, 200, 192, 111, 31, 0, 0x30, 0x70, 0x20, 0);
        DrawSync(0);
        VSync(0);
        ResetGraph(1);
        PutDrawEnv(&g_JetBufferPtr[0]->draw);
        PutDispEnv(&g_JetBufferPtr[0]->disp);
        ClearImage(&g_JetBufferPtr[0]->draw.clip, 0, 0, 0);
        if (D_800E25F4 != 0) {
            DrawOTag(&g_JetBufferPtr[0]->ot[0xFFF]);
            DrawOTag(&g_JetBufferPtr[0]->ot2[0xB3]);
        }
        next = g_JetBuffers;
        current = g_JetBufferPtr[0];
        frame = &D_800E25FC;
        *frame = 0;
        if (current == next) {
            next++;
        }
        g_JetBufferPtr[0] = next;
        ClearOTagR(next->ot, 0x1000);
        ClearOTagR(g_JetBufferPtr[0]->ot2, 0xB4);
        JetPrimCursorsReset(&g_JetBufferPtr[0]->prims);
    }
    g_AkaoCmd.opcode = 0xB8;
    g_AkaoCmd.params[0] = 0;
    AkaoExec();
    return g_JetScore;
}

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

    world = &g_JetWorldMatrix;
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
    wm->t[0] -= g_JetCameraPos.vx;
    wm->t[1] -= g_JetCameraPos.vy;
    wm->t[2] -= g_JetCameraPos.vz;
    cam = &g_JetCameraRot;
    gte_SetRotMatrix(cam);
    gte_ldclmv(&world[0]->m[0][0]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][0]);
    gte_ldclmv(&world[0]->m[0][1]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][1]);
    gte_ldclmv(&world[0]->m[0][2]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][2]);
    gte_SetTransMatrix(cam);
    gte_ldlv0(&world[0]->t[0]);
    gte_rt();
    gte_stlvl(&world[0]->t[0]);
    gte_SetRotMatrix(world[0]);
    gte_SetTransMatrix(world[0]);
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = JetDrawModelTris(&args);
    JetProject6Points(obj->unkDC, obj->unk11C);
    // Both reads have to stay: -fforce-mem is what keeps the second one a
    // word load instead of letting gcc narrow it to lhu.
    ys[0] = obj->unk11C[0] >> 16;
    minY = ys[0];
    maxY = minY;
    xs[0] = obj->unk11C[0];
    minX = xs[0];
    maxX = minX;
    for (i = 1; i < LEN(obj->unk11C); i++) {
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
    if (JetVectorInsidePlanes((VECTOR*)g_JetWorldMatrix->t)) {
        obj->unk28.hit = 0;
        if (g_JetCursorX < maxX && minX < g_JetCursorX && g_JetCursorY < maxY && minY < g_JetCursorY &&
            g_JetFiring == 1) {
            obj->unk28.hit = g_JetFiring;
        }
    }
}

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
    s32 index;

    world = &g_JetWorldMatrix;
    m = world[0];
    index = 0;
    m->m[0][0] = node->m.m[index][index];
    m->m[index][1] = node->m.m[index][1];
    m->m[index][2] = node->m.m[index][2];
    m->m[1][index] = node->m.m[1][index];
    m->m[1][1] = node->m.m[1][1];
    m->m[1][2] = node->m.m[1][2];
    m->m[2][index] = node->m.m[2][index];
    m->m[2][1] = node->m.m[2][1];
    m->m[2][2] = node->m.m[2][2];
    m->t[index] = node->m.t[index];
    m->t[1] = node->m.t[1];
    m->t[2] = node->m.t[2];
    if (node->parent != &g_JetRootNode) {
        CompMatrix(&node->parent->m, m, m);
    }
    wm = world[index];
    wm->t[index] -= g_JetCameraPos.vx;
    wm->t[1] -= g_JetCameraPos.vy;
    wm->t[2] -= g_JetCameraPos.vz;
    cam = &g_JetCameraRot;
    gte_SetRotMatrix(cam);
    gte_ldclmv(&world[0]->m[0][0]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][0]);
    gte_ldclmv(&world[0]->m[0][1]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][1]);
    gte_ldclmv(&world[0]->m[0][2]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][2]);
    gte_SetTransMatrix(cam);
    gte_ldlv0(&world[0]->t[0]);
    gte_rt();
    gte_stlvl(&world[0]->t[0]);
    gte_SetRotMatrix(world[0]);
    gte_SetTransMatrix(world[0]);
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot2[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = JetDrawModelTris(&args);
    shadow = &g_JetModelTable[79];
    JetProject3Points(&shadow[index]->tris[0].v0, screen);
    D_800A8964 = screen[1] >> 16;
    D_800A895C = screen[1];
    D_800A896C = screen[2] >> 16;
    D_800A8960 = screen[2];
    JetProject3Points(&shadow[index]->tris[1].v0, screen);
    xy1 = screen[1];
    xy2 = screen[2];
    D_800A8978 = xy1 >> 16;
    D_800A8970 = xy1;
    D_800A8980 = xy2 >> 16;
    D_800A8974 = xy2;
}

// Load a node's matrix into the GTE and draw its model's triangles.
static void JetDrawNodeUI(JetBuffer* db, JetNode* node, s16 otIndex, s32 arg3, s32 arg4) {
    Unk800A8604 args;
    MATRIX** world;
    MATRIX* m;

    world = &g_JetWorldMatrix;
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
    gte_SetRotMatrix(world[0]);
    gte_SetTransMatrix(world[0]);
    args.tris = node->model->tris;
    args.prim = db->prims.g3Cursor;
    args.ot = &db->ot2[otIndex];
    args.model = node->model;
    db->prims.g3Cursor = JetDrawModelTrisUI(&args);
}

// Draw every background triangle on the draw list, front to back.
static void JetDrawTriangleList(void) {
    JetListLink* list;
    JetTriangle* tris;
    u16 triId;
    POLY_G3* prim;

    prim = g_JetBufferPtr[0]->prims.g3Cursor;
    tris = g_JetTrianglesBase;
    if (g_JetTriangleListCount) {
        triId = g_JetTriangleListHead;
        list = g_JetTriangleLinks;
        do {
            prim = JetDrawTriangle(&tris[triId], prim, g_JetBufferPtr[0]->ot, &tris[triId]);
            triId = list[triId].next;
        } while (triId != 0xFFFF);
    }
    g_JetBufferPtr[0]->prims.g3Cursor = prim;
}

// Draw every track element on the draw list, front to back.
static void JetDrawTrack(void) {
    JetListLink* list;
    SVECTOR* left;
    SVECTOR* right;
    u16 trackId;
    POLY_FT4* prim;

    trackId = g_JetTrackListHead;
    prim = g_JetBufferPtr[0]->prims.ft4Cursor;
    list = g_JetTrackLinks;
// A loop keyword hoists the 0xFFFF test into a register; the goto keeps it inline.
loop:
    left = g_JetTrackLeft;
    right = g_JetTrackRight;
    prim = JetDrawTrackQuad(&left[trackId], prim, g_JetBufferPtr[0]->ot, &right[trackId]);
    trackId = list[trackId].next;
    if (trackId != 0xFFFF) {
        goto loop;
    }
    g_JetBufferPtr[0]->prims.ft4Cursor = prim;
}

// Build the world matrix from the camera rotation and the view position.
static void JetSetWorldMatrix(void) {
    MATRIX** view;
    MATRIX** world;
    MATRIX* cam;

    view = &g_JetViewMatrix;
    view[0]->t[0] = -g_JetCameraPos.vx;
    view[0]->t[1] = -g_JetCameraPos.vy;
    view[0]->t[2] = -g_JetCameraPos.vz;
    cam = &g_JetCameraRot;
    gte_SetRotMatrix(cam);
    gte_ldclmv(&view[0]->m[0][0]);
    gte_rtir();
    world = &g_JetWorldMatrix;
    gte_stclmv(&world[0]->m[0][0]);
    gte_ldclmv(&view[0]->m[0][1]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][1]);
    gte_ldclmv(&view[0]->m[0][2]);
    gte_rtir();
    gte_stclmv(&world[0]->m[0][2]);
    gte_SetTransMatrix(cam);
    gte_ldlv0(&view[0]->t[0]);
    gte_rt();
    gte_stlvl(&world[0]->t[0]);
    gte_SetRotMatrix(world[0]);
    gte_SetTransMatrix(world[0]);
}

// Sample the track at a fractional segment index, giving a point lifted along
// the surface normal and the interpolated banking rotation.
void JetTrackSample(u32 at, s32 lift, VECTOR* pos, SVECTOR* rot) {
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
    rotCur = &g_JetTrackRot[seg];
    rotNext = &g_JetTrackRot[seg + 1];
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

    leftCur = &g_JetTrackLeft[seg];
    leftNext = &g_JetTrackLeft[seg + 1];
    dLeft.vx = (leftNext->vx - leftCur->vx) * frac;
    dLeft.vy = (leftNext->vy - leftCur->vy) * frac;
    dLeft.vz = (leftNext->vz - leftCur->vz) * frac;

    rightCur = &g_JetTrackRight[seg];
    rightNext = &g_JetTrackRight[seg + 1];
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

static void JetDrawEnergyGauge(void) {
    JetBuffer** db;
    POLY_G4* poly;
    s16 power;
    s32 top;

    db = g_JetBufferPtr;
    poly = db[0]->prims.g4Cursor;
    power = g_JetShotPower;
    top = 220 - power;
    setXY4(poly, 20, top, 28, top, 20, 220, 28, 220);
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
static void JetDrawScorePopup(JetBuffer* arg0, s16 arg1, s32 arg2, s32 arg3, s32 arg4) {
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
    g_JetPopupRot.vx += arg2;
    g_JetPopupRot.vy += arg3;
    g_JetPopupRot.vz += arg4;
    if (alternate[0] == 1) {
        RotMatrix(&g_JetPopupRot, &D_800A8A74[index]->m);
        JetDrawNodeUI(arg0, D_800A8A74[index], 0, 0, unused);
        JetDrawNumber(g_JetPopupPoints, 220, 160, 0, 0x18);
    }
    counter = &g_JetPopupTimer;
    (*counter)--;
    if (*counter < 50) {
        if (alternate[0] == 0) {
            alternate[0] = 1;
        } else {
            alternate[0] = 0;
        }
    }
    if (g_JetPopupTimer == 0) {
        g_JetPopupModelId = 0;
    }
}

static void JetDrawNumber(s32 value, s32 x, s32 y, s16 padWithZero, u16 v) {
    POLY_FT4* poly;
    JetBuffer* db;
    s32 digit;
    s32 power;
    s32 remain;
    s32 i;
    u16 startX;
    s32 w;
    s32 left;
    u8 leading;

    startX = x;
    power = 1000;
    leading = 1;
    remain = value + 1;
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
            left = x + i * 14;
            w = i * 14 + 16;
            setXY4(poly, left, y, startX + w, y, left, y + 16, startX + w, y + 16);
            setRGB0(poly, 0x80, 0x80, 0x80);
            setUVWH(poly, digit * 0x10 + 0x30, v, 0x10, 0x12);
            poly->tpage = g_JetSpriteTPage[8];
            poly->clut = g_JetSpriteClut[8];
            SetSemiTrans(poly, 1);
            db = g_JetBufferPtr[0];
            addPrim(&db->ot2[1], poly);
            poly++;
        }
        power /= 10;
    }
    g_JetBufferPtr[0]->prims.ft4Cursor = poly;
}

// Draw one sprite from the HUD sprite table.
static void JetDrawSprite(s16 spriteId, s16 x, s16 y, s16 w, s16 h, u8 u, u8 v, u8 uw, u8 vh, u8 semiTrans) {
    JetBuffer** db;
    POLY_FT4* poly;

    db = g_JetBufferPtr;
    poly = db[0]->prims.ft4Cursor;
    setXYWH(poly, x, y, w, h);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUVWH(poly, u, v, uw, vh);
    poly->tpage = g_JetSpriteTPage[spriteId];
    poly->clut = g_JetSpriteClut[spriteId];
    SetSemiTrans(poly, semiTrans);
    addPrim(&db[0]->ot2[1], poly);
    poly++;
    db[0]->prims.ft4Cursor = poly;
}

// Queue two blank textured quads, one at each end of the background OT.
static void func_800A2058(void) {
    JetBuffer** db;
    POLY_FT4* poly;

    db = g_JetBufferPtr;
    poly = db[0]->prims.ft4Cursor;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = g_JetSpriteTPage[5];
    poly->clut = g_JetSpriteClut[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->ot[0xFFF], poly);
    poly++;
    setXY4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0, 0, 0, 0, 0, 0);
    poly->tpage = g_JetSpriteTPage[5];
    poly->clut = g_JetSpriteClut[5];
    SetSemiTrans(poly, 0);
    addPrim(&db[0]->ot[2], poly);
    poly++;
    db[0]->prims.ft4Cursor = poly;
}

// Point every matrix and vector at scratchpad, then build the world.
static void func_800A2214(void) {
    volatile s32* state;
    s32 i;

    state = &D_800A8A84;
    *state = 0;
    D_80110BB8 = (void*)0x1F800000;
    D_800D16D4 = (void*)0x1F800000;
    D_800D16DC = 0;
    g_JetWorldMatrix = (MATRIX*)0x1F800010;
    g_JetViewMatrix = (MATRIX*)0x1F800030;
    D_800D1A38 = (void*)0x1F800050;
    D_800D1A3C = (void*)0x1F800058;
    D_800A891C = (void*)0x1F800060;
    D_800A8920 = (void*)0x1F800064;
    g_JetViewMatrix->m[0][0] = 0x1000;
    g_JetViewMatrix->m[0][1] = 0;
    g_JetViewMatrix->m[0][2] = 0;
    g_JetViewMatrix->m[1][0] = 0;
    g_JetViewMatrix->m[1][1] = 0x1000;
    g_JetViewMatrix->m[1][2] = 0;
    g_JetViewMatrix->m[2][0] = 0;
    g_JetViewMatrix->m[2][1] = 0;
    g_JetViewMatrix->m[2][2] = 0x1000;
    JetBuffersInit();
    func_800A2420();
    *state = 0x99;
    func_800A334C();
    JetNodesInit();
    JetModelsReset();
    func_800A2BE0();
    func_800A3AAC();
    func_800A70D4();
    for (i = 0; i < LEN(g_JetModelTable); i++) {
        g_JetModelTable[i] = JetModelBuild(i);
    }
    D_800A897C = 0x2710;
    D_800A89D0 = 0x28AA;
    D_800A89D4 = 0x37DC;
    g_JetTrackSegment = 0;
    D_800D1C54 = 0;
    g_JetScore = 0;
    D_800E25F4 = 0;
    D_800E2600 = 0;
    g_JetPopupModelId = 0;
    D_800E25E8 = 0;
    g_JetPopupRot.vx = 0;
    g_JetPopupRot.vy = 0;
    g_JetPopupRot.vz = 0;
    D_800A8928 = 0;
    D_800A8330 = 0x7F;
    D_800A8334 = 0x7F;
    D_800A8338 = 0;
    D_800A833C = 0;
    g_JetPopupTimer = 0;
}

static void func_800A2420(void) {
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
static void func_800A2518(void) {
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
    g_JetSpriteTPage[0] = GetTPage(0, 1, 0x280, 0);
    g_JetSpriteClut[0] = GetClut(0, 0x1E0);
    g_JetSpriteTPage[1] = GetTPage(1, 1, 0x2C0, 0);
    g_JetSpriteClut[1] = GetClut(0, 0x1E1);
    g_JetSpriteTPage[2] = GetTPage(1, 1, 0x2D0, 0);
    g_JetSpriteClut[2] = GetClut(0, 0x1E1);
    g_JetSpriteTPage[3] = GetTPage(1, 1, 0x2E0, 0);
    g_JetSpriteClut[3] = GetClut(0, 0x1E1);
    g_JetSpriteTPage[4] = GetTPage(0, 1, 0x280, 0x100);
    g_JetSpriteClut[4] = GetClut(0, 0x1FF);
    g_JetSpriteTPage[5] = GetTPage(0, 1, 0x280, 0x100);
    g_JetSpriteClut[5] = GetClut(0, 0x1FE);
    g_JetSpriteTPage[6] = GetTPage(0, 1, 0x300, 0);
    g_JetSpriteClut[6] = GetClut(0x10, 0x1E0);
    g_JetSpriteTPage[7] = GetTPage(0, 1, 0x240, 0);
    g_JetSpriteClut[7] = GetClut(0x40, 0x1E0);
    g_JetSpriteTPage[8] = GetTPage(0, 1, 0x240, 0x18);
    g_JetSpriteClut[8] = GetClut(0x30, 0x1E0);
    g_JetSpriteTPage[9] = GetTPage(0, 1, 0x240, 0x50);
    g_JetSpriteClut[9] = GetClut(0x50, 0x1E0);
    g_JetSpriteTPage[10] = GetTPage(0, 1, 0x240, 0x30);
    g_JetSpriteClut[10] = GetClut(0x20, 0x1E0);
    g_JetSpriteTPage[11] = GetTPage(0, 1, 0x240, 0);
    g_JetSpriteClut[11] = GetClut(0x60, 0x1E0);
}

static void func_800A27F0(u_long* addr) {
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

static void func_800A2860(void) {
    g_AkaoCmd.opcode = 0x10;
    g_AkaoCmd.params[0] = D_800D1BD4;
    AkaoExec();
    g_AkaoCmd.opcode = 0xC0;
    g_AkaoCmd.params[0] = 0x7F;
    AkaoExec();
    g_AkaoCmd.opcode = 0xB8;
    g_AkaoCmd.params[0] = 0x7F;
    AkaoExec();
    g_AkaoCmd.opcode = 0xBC;
    g_AkaoCmd.params[0] = 0;
    AkaoExec();
    g_AkaoCmd.opcode = 0xA2;
    g_AkaoCmd.params[0] = 0;
    AkaoExec();
    g_AkaoCmd.opcode = 0x2A;
    g_AkaoCmd.params[0] = 0x40;
    g_AkaoCmd.params[1] = 0x177;
    AkaoExec();
}

void func_800A2938(void) {
    g_AkaoCmd.opcode = 0xC1;
    g_AkaoCmd.params[0] = 0xF0;
    g_AkaoCmd.params[1] = 0;
    AkaoExec();
    g_AkaoCmd.opcode = 0xB9;
    g_AkaoCmd.params[0] = 0xF0;
    g_AkaoCmd.params[1] = 0;
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
        g_AkaoCmd.opcode = 0xB0;
        g_AkaoCmd.params[0] = 0;
        AkaoExec();
        g_AkaoCmd.opcode = 0x28;
        g_AkaoCmd.params[0] = 0x40;
        g_AkaoCmd.params[1] = arg0;
        AkaoExec();
    }
    if (*pChannel == 1) {
        g_AkaoCmd.opcode = 0xB1;
        g_AkaoCmd.params[0] = 0;
        AkaoExec();
        g_AkaoCmd.opcode = 0x29;
        g_AkaoCmd.params[0] = 0x40;
        g_AkaoCmd.params[1] = arg0;
        AkaoExec();
    }
}

static void func_800A2AA0(s32 arg0) {
    s32* lastParam;
    s32 param;

    lastParam = &D_800A8958;
    if (*lastParam == 0) {
        if (arg0 & 0xFF) {
            g_AkaoCmd.opcode = 0x2B;
            g_AkaoCmd.params[0] = 0x40;
            g_AkaoCmd.params[1] = 0x22B;
            AkaoExec();
        } else {
            g_AkaoCmd.opcode = 0x2B;
            g_AkaoCmd.params[0] = 0x40;
            g_AkaoCmd.params[1] = 0;
            AkaoExec();
            D_800A8958 = 0;
            return;
        }
    }
    param = arg0 & 0xFF;
    if (param) {
        D_800A833C = param;
        g_AkaoCmd.opcode = 0xB3;
        g_AkaoCmd.params[0] = param;
        AkaoExec();
        *lastParam = param;
    } else {
        g_AkaoCmd.opcode = 0x2B;
        g_AkaoCmd.params[0] = 0x40;
        g_AkaoCmd.params[1] = 0;
        AkaoExec();
        D_800A8958 = 0;
    }
}

static void func_800A2B78(void) {
    g_AkaoCmd.opcode = 0xA2;
    g_AkaoCmd.params[0] = D_800A8338;
    AkaoExec();
    g_AkaoCmd.opcode = 0xA3;
    g_AkaoCmd.params[0] = D_800A833C;
    AkaoExec();
}

static void func_800A2BE0(void) {
    D_800A83C8.vy = -0x1B76;
    D_800A83C8.vx = 0;
    D_800A83C8.vz = 0xC8;
    D_800E2604 = D_800D1BEC;
    D_800A8CC0 = D_800D1BE8;
    func_800A2DE4(0, 3);
    D_800D1960 = 1;
}

// Advance the camera along its path and rebuild the view matrices.
static void func_800A2C50(void) {
    VECTOR pos;
    SVECTOR rot;
    SVECTOR camRot;
    s32* pathPos;
    s32* speed;
    s32* limit;
    s32 step;

    pathPos = &D_800D1C54;
    JetTrackSample(pathPos[0], -0x64, &pos, &rot);
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
    g_JetCameraPos.vx = pos.vx;
    g_JetCameraPos.vy = pos.vy;
    g_JetCameraPos.vz = pos.vz;
    camRot.vx = -rot.vx;
    camRot.vy = -rot.vy;
    camRot.vz = 0;
    RotMatrix(&camRot, &g_JetCameraRot);
    RotMatrix(&D_800A83A0, &D_800A8380);
    CompMatrix(&D_800A8380, &g_JetCameraRot, &g_JetCameraRot);
}

static void func_800A2DE4(s32 arg0, s32 arg1) {
    s32 offset;
    u8* base;

    offset = D_800A8CC0[arg0 & 0xFF];
    base = D_800D1BE4;
    D_800A8988 = (SVECTOR*)(base + offset);
    D_800D1724 = *D_800E2604;
}

static void func_800A2E30(void) {}

// Read the pad and drive the cursor, the camera tweaks and the pause toggle.
static void func_800A2E38(void) {
    u32 pad;
    s32* dir;
    s16* cursorX;
    s16* cursorY;
    u8* shoot;
    s16* power;
    s16* powerRegen;
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
                cursorY = &g_JetCursorY;
                *cursorY += 5;
            }
            if (pad & 0x1000) {
                cursorY = &g_JetCursorY;
                *cursorY -= 5;
            }
            if (pad & 0x8000) {
                cursorX = &g_JetCursorX;
                *cursorX -= 5;
            }
            if (pad & 0x2000) {
                cursorX = &g_JetCursorX;
                *cursorX += 5;
            }
            shoot = &g_JetFiring;
            *shoot = 0;
            if (pad & 0x20) {
                power = &g_JetShotPower;
                func_800A2AA0(*power & 0xFF);
                if (*power >= 9) {
                    (*power)--;
                }
                repeat = &D_800D1C7C;
                count = *repeat;
                if (count == 0) {
                    scroll = &g_JetBeamScroll;
                    next = *scroll + 3;
                    *repeat = 1;
                    *shoot = 1;
                    *scroll = next % 15;
                } else {
                    *repeat = count - 1;
                }
            } else {
                func_800A2AA0(0);
                powerRegen = &g_JetShotPower;
                if (*powerRegen < 128) {
                    (*powerRegen)++;
                }
            }
            cursorX = &g_JetCursorX;
            if (*cursorX > 320) {
                *cursorX = 320;
            }
            if (*cursorX < 0) {
                *cursorX = 0;
            }
            cursorY = &g_JetCursorY;
            if (*cursorY > 240) {
                *cursorY = 240;
            }
            if (*cursorY < 0) {
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
static void func_800A334C(void) {
    JetListLink* list;
    s32 i;

    D_800D172C = 0xFFFE;
    D_800A8CC8 = 0;
    D_800D1C60 = D_800D1BF4;
    D_800EE428 = D_800D1BF8;
    D_800D196C = D_800D1BDC;
    D_800EE188 = D_800D1BE0;
    list = g_JetTriangleLinks;
    for (i = 0; i < LEN(g_JetTriangleLinks); i++) {
        list[i].prev = 0xFFFF;
        list[i].next = 0xFFFF;
    }
    g_JetTriangleListCount = 0;
    list = g_JetTrackLinks;
    for (i = 0; i < LEN(g_JetTrackLinks); i++) {
        list[i].prev = 0xFFFF;
        list[i].next = 0xFFFF;
    }
    D_800D1C50 = 0;
    D_800E25F8 = 1;
}

// Step the track streams forward, spawning whatever each segment lists.
static void func_800A3414(s32 advance) {
    u32* pos;
    s32* segment;
    u32 prev;
    u32 next;
    u32 steps;
    u32 i;
    u32 id;
    u8* first;

    pos = &D_800D172C;
    D_800A8CC8 = pos[0];
    pos[0] = D_800A8CC8 + advance;
    prev = D_800A8CC8 >> 18;
    next = pos[0] >> 18;
    steps = next - prev;
    segment = &g_JetTrackSegment;
    segment[0] = segment[0] + steps;
    for (i = 0; i < steps + D_800E25F8; i++) {
        u16** tri;
        u16** quad;
        u32 end;

        tri = &D_800D1C60;
        end = 0xFFFF;
        quad = &D_800EE428;
        while (1) {
            id = *tri[0]++;
            if (id == end) {
                break;
            }
            func_800A385C(id);
        }
        while (1) {
            id = *quad[0]++;
            if (id == end) {
                break;
            }
            func_800A38D4(id);
        }
    }
    for (i = 0; i < steps; i++) {
        u16** tri;
        u16** quad;
        u32 end;

        tri = &D_800D196C;
        end = 0xFFFF;
        quad = &D_800EE188;
        while (1) {
            id = *tri[0]++;
            if (id == end) {
                break;
            }
            func_800A3980(id);
        }
        while (1) {
            id = *quad[0]++;
            if (id == end) {
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

static void func_800A35DC(s32 advance) {
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
    segment = &g_JetTrackSegment;
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

static void func_800A372C(s32 advance) {
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

static void func_800A385C(u16 arg0) {
    JetListLink* node;
    u16* pCount;
    u16* pTail;
    u16 newCount;
    u16 count;
    u16 tail;

    node = &g_JetTriangleLinks[arg0];
    pCount = &g_JetTriangleListCount;
    count = *pCount;
    if (count == 0) {
        g_JetTriangleListHead = arg0;
        D_800D9930 = arg0;
        *pCount = 1;
    } else {
        pTail = &D_800D9930;
        tail = *pTail;
        newCount = count + 1;
        node->prev = tail;
        (g_JetTriangleLinks + tail)->next = arg0;
        *pTail = arg0;
        *pCount = newCount;
    }
}

static void func_800A38D4(u16 arg0) {
    JetListLink* list;
    JetListLink* node;
    u16* pCount;
    u16 prev;
    u16 next;

    node = &g_JetTriangleLinks[arg0];
    prev = node->prev;
    next = node->next;
    list = g_JetTriangleLinks;
    if (prev != 0xFFFF) {
        list[prev].next = next;
    } else {
        g_JetTriangleListHead = next;
    }
    if (next != 0xFFFF) {
        g_JetTriangleLinks[next].prev = prev;
    } else {
        D_800D9930 = prev;
    }
    {
        JetListLink* links;

        links = g_JetTriangleLinks;
        links[arg0].prev = 0xFFFF;
        links[arg0].next = 0xFFFF;
    }
    pCount = &g_JetTriangleListCount;
    *pCount = *pCount - 1;
}

static void func_800A3980(u16 arg0) {
    u16* pCount;
    u16 count;

    pCount = &D_800D1C50;
    count = *pCount;
    if (count == 0) {
        JetListLink* list;
        JetListLink* node;

        list = g_JetTrackLinks;
        node = &list[arg0];
        node->prev = 0xFFFF;
        node->next = 0xFFFF;
        g_JetTrackListHead = arg0;
        D_800D1C80 = arg0;
        *pCount = 1;
    } else {
        JetListLink* list;
        JetListLink* node;
        u16* pTail;
        u16 newCount;
        u16 tail;

        newCount = count + 1;
        list = g_JetTrackLinks;
        node = &list[arg0];
        pTail = &D_800D1C80;
        tail = *pTail;
        node->prev = tail;
        node->next = 0xFFFF;
        list[tail].next = arg0;
        *pTail = arg0;
        *pCount = newCount;
    }
}

static void func_800A3A20(u16 arg0) {
    JetListLink* list;
    JetListLink* node;
    u16* pCount;
    u16 prev;
    u16 next;

    node = &g_JetTrackLinks[arg0];
    prev = node->prev;
    next = node->next;
    list = g_JetTrackLinks;
    if (prev != 0xFFFF) {
        list[prev].next = next;
    } else {
        g_JetTrackListHead = next;
    }
    if (next != 0xFFFF) {
        g_JetTrackLinks[next].prev = prev;
    } else {
        D_800D1C80 = prev;
    }
    pCount = &D_800D1C50;
    *pCount = *pCount - 1;
}
