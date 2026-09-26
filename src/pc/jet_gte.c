// Native versions of src/mini/jet/jet_gte.s, driving PSY-Z's GTE emulator.
//
// The assembly writes primitives at their PS1 offsets and links them into the
// ordering table with 24-bit addresses. PSY-Z's primitives carry a 64-bit tag,
// so these go through the struct fields and addPrim instead.

#include "../mini/jet/jet_private.h"
#include <psyz/gte.h>

#define GTE_VXY0 0
#define GTE_VZ0 1
#define GTE_RGBC 6
#define GTE_OTZ 7
#define GTE_IR0 8
#define GTE_SXY0 12
#define GTE_SXY1 13
#define GTE_SXY2 14
#define GTE_RGB2 22
#define GTE_MAC0 24

static void LoadVertex(int slot, SVECTOR* v) {
    Psyz_GteDataWrite(GTE_VXY0 + slot * 2, *(unsigned int*)&v->vx);
    Psyz_GteDataWrite(GTE_VZ0 + slot * 2, *(unsigned int*)&v->vz);
}

static void StoreXy(short* xy, int reg) { *(unsigned int*)xy = Psyz_GteDataRead(reg); }

static unsigned int DepthCue(CVECTOR* color, unsigned int ir0) {
    Psyz_GteDataWrite(GTE_IR0, ir0);
    Psyz_GteDataWrite(GTE_RGBC, *(unsigned int*)color);
    Psyz_GteDpcs();
    return Psyz_GteDataRead(GTE_RGB2);
}

// Projects, culls and depth-cues one triangle into prim; returns the next free
// primitive, which is prim itself when the triangle was rejected.
static POLY_G3* DrawTri(JetTriangle* tri, POLY_G3* prim, OT_TYPE* ot, int otzShift, int otzMax) {
    unsigned int ir0v0;
    unsigned int ir0v1;
    unsigned int rgb;
    int otz;

    LoadVertex(0, &tri->v0);
    Psyz_GteRtps();
    ir0v0 = Psyz_GteDataRead(GTE_IR0);
    LoadVertex(0, &tri->v1);
    Psyz_GteRtps();
    ir0v1 = Psyz_GteDataRead(GTE_IR0);
    LoadVertex(0, &tri->v2);
    Psyz_GteRtps();
    Psyz_GteNclip();
    if ((int)Psyz_GteDataRead(GTE_MAC0) < 0) {
        return prim;
    }
    Psyz_GteAvsz3();
    otz = (int)Psyz_GteDataRead(GTE_OTZ) >> otzShift;
    if (otz <= 0 || otz > otzMax) {
        return prim;
    }
    StoreXy(&prim->x0, GTE_SXY0);
    StoreXy(&prim->x1, GTE_SXY1);
    StoreXy(&prim->x2, GTE_SXY2);
    rgb = DepthCue(&tri->c2, Psyz_GteDataRead(GTE_IR0));
    *(unsigned int*)&prim->r2 = rgb;
    rgb = DepthCue(&tri->c1, ir0v1);
    *(unsigned int*)&prim->r1 = rgb;
    // c0's code byte passes through DPCS, so this also writes the GPU command.
    rgb = DepthCue(&tri->c0, ir0v0);
    *(unsigned int*)&prim->r0 = rgb;
    addPrim(&ot[otz], prim);
    return prim + 1;
}

static void* DrawModelTris(JetModelDrawArgs* args, int otzShift) {
    JetTriangle* tri = args->tris;
    POLY_G3* prim = args->prim;
    s16 count = args->model->triCount;

    do {
        prim = DrawTri(tri, prim, args->ot, otzShift, 0x1000);
        tri++;
    } while (--count != 0);
    return prim;
}

void* JetDrawModelTris(JetModelDrawArgs* args) { return DrawModelTris(args, 0); }

void* JetDrawModelTrisUI(JetModelDrawArgs* args) { return DrawModelTris(args, 4); }

POLY_G3* JetDrawTriangle(JetTriangle* arg0, POLY_G3* arg1, OT_TYPE* arg2, JetTriangle* arg3) {
    return DrawTri(arg0, arg1, arg2, 0, 0xFA0);
}

static void Project3(SVECTOR* points, u_long* screen, int count) {
    LoadVertex(0, &points[0]);
    LoadVertex(1, &points[1]);
    LoadVertex(2, &points[2]);
    Psyz_GteRtpt();
    screen[0] = Psyz_GteDataRead(GTE_SXY0);
    screen[1] = Psyz_GteDataRead(GTE_SXY1);
    if (count > 2) {
        screen[2] = Psyz_GteDataRead(GTE_SXY2);
    }
}

void JetProject3Points(SVECTOR* points, u_long* screen) { Project3(points, screen, 3); }

// The second batch keeps only two of its three projections.
void JetProject6Points(SVECTOR* points, u_long* screen) {
    Project3(&points[0], &screen[0], 3);
    Project3(&points[3], &screen[3], 2);
}

// One track segment: left[0], right[1] and left[8] project first, right[9]
// completes the quad, which is drawn flat-lit with a fixed 32x32 texture.
POLY_FT4* JetDrawTrackQuad(SVECTOR* arg0, POLY_FT4* arg1, OT_TYPE* arg2, SVECTOR* arg3) {
    POLY_FT4* prim = arg1;
    int otz;

    LoadVertex(0, &arg0[0]);
    LoadVertex(1, &arg3[1]);
    LoadVertex(2, &arg0[8]);
    Psyz_GteRtpt();
    Psyz_GteAvsz3();
    otz = (int)Psyz_GteDataRead(GTE_OTZ);
    if (otz <= 0 || otz > 0xFA0) {
        return prim;
    }
    StoreXy(&prim->x0, GTE_SXY0);
    StoreXy(&prim->x1, GTE_SXY1);
    StoreXy(&prim->x2, GTE_SXY2);
    LoadVertex(0, &arg3[9]);
    Psyz_GteRtps();
    StoreXy(&prim->x3, GTE_SXY2);
    Psyz_GteDataWrite(GTE_RGBC, 0x808080);
    Psyz_GteDpcs();
    *(unsigned int*)&prim->r0 = Psyz_GteDataRead(GTE_RGB2);
    prim->code = 0x2C;
    prim->u0 = 0x00;
    prim->v0 = 0x00;
    prim->clut = 0x7801;
    prim->u1 = 0x1F;
    prim->v1 = 0x00;
    prim->tpage = 0x2C;
    prim->u2 = 0x00;
    prim->v2 = 0x1F;
    prim->u3 = 0x1F;
    prim->v3 = 0x1F;
    addPrim(&arg2[otz], prim);
    return prim + 1;
}
