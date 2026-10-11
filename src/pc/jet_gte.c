#include "../mini/jet/jet_private.h"

static void ProjectAndDepthCue(SVECTOR* vertex, CVECTOR* color, u_char* out) {
    gte_ldv0(vertex);
    gte_rtps();
    gte_ldrgb(color);
    gte_dpcs();
    gte_strgb(out);
}

static POLY_G3* DrawTri(JetTriangle* tri, POLY_G3* prim, OT_TYPE* ot, int otzShift, int otzMax) {
    int clip;
    int otz;

    ProjectAndDepthCue(&tri->v0, &tri->c0, &prim->r0);
    ProjectAndDepthCue(&tri->v1, &tri->c1, &prim->r1);
    ProjectAndDepthCue(&tri->v2, &tri->c2, &prim->r2);
    gte_nclip();
    gte_stopz(&clip);
    if (clip < 0) {
        return prim;
    }
    gte_avsz3();
    gte_stotz(&otz);
    otz >>= otzShift;
    if (otz <= 0 || otz > otzMax) {
        return prim;
    }
    gte_stsxy3(&prim->x0, &prim->x1, &prim->x2);
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

void JetProject3Points(SVECTOR* points, u_long* screen) {
    gte_ldv3(&points[0], &points[1], &points[2]);
    gte_rtpt();
    gte_stsxy3(&screen[0], &screen[1], &screen[2]);
}

void JetProject6Points(SVECTOR* points, u_long* screen) {
    u_long unused;

    JetProject3Points(points, screen);
    gte_ldv3(&points[3], &points[4], &points[5]);
    gte_rtpt();
    gte_stsxy3(&screen[3], &screen[4], &unused);
}

POLY_FT4* JetDrawTrackQuad(SVECTOR* arg0, POLY_FT4* arg1, OT_TYPE* arg2, SVECTOR* arg3) {
    static CVECTOR grey = {0x80, 0x80, 0x80, 0};
    POLY_FT4* prim = arg1;
    int otz;

    gte_ldv3(&arg0[0], &arg3[1], &arg0[8]);
    gte_rtpt();
    gte_avsz3();
    gte_stotz(&otz);
    if (otz <= 0 || otz > 0xFA0) {
        return prim;
    }
    gte_stsxy3(&prim->x0, &prim->x1, &prim->x2);
    gte_ldv0(&arg3[9]);
    gte_rtps();
    gte_stsxy(&prim->x3);
    gte_ldrgb(&grey);
    gte_dpcs();
    gte_strgb(&prim->r0);
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
