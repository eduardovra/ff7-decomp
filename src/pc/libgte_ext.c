// PSY-Q libgte functions PSY-Z does not provide yet, which jet needs.

#include <libgte.h>
#include <math.h>

// Rz * Ry * Rx, following PSY-Z's RotMatrixYXZ, which composes Ry * Rx * Rz.
MATRIX* RotMatrixZYX(SVECTOR* r, MATRIX* m) {
    int sx = rsin(r->vx), cx = rcos(r->vx);
    int sy = rsin(r->vy), cy = rcos(r->vy);
    int sz = rsin(r->vz), cz = rcos(r->vz);
    int sysx = (sy * sx) >> 12;
    int sycx = (sy * cx) >> 12;

    m->m[0][0] = (cz * cy) >> 12;
    m->m[0][1] = ((cz * sysx) >> 12) - ((sz * cx) >> 12);
    m->m[0][2] = ((cz * sycx) >> 12) + ((sz * sx) >> 12);
    m->m[1][0] = (sz * cy) >> 12;
    m->m[1][1] = ((sz * sysx) >> 12) + ((cz * cx) >> 12);
    m->m[1][2] = ((sz * sycx) >> 12) - ((cz * sx) >> 12);
    m->m[2][0] = -sy;
    m->m[2][1] = (cy * sx) >> 12;
    m->m[2][2] = (cy * cx) >> 12;
    return m;
}

// PSY-Q's CompMatrix(m0, m1, m2) stores m0 * m1 in m2, rotation and
// translation. PSY-Z declares it with other argument types, hence the casts.
MATRIX* CompMatrix(SVECTOR* rot, VECTOR* trans, MATRIX* m) {
    MATRIX* m0 = (MATRIX*)rot;
    MATRIX* m1 = (MATRIX*)trans;
    MATRIX r;
    int i, j;

    for (i = 0; i < 3; i++) {
        long long t = 0;

        for (j = 0; j < 3; j++) {
            r.m[i][j] = (m0->m[i][0] * m1->m[0][j] + m0->m[i][1] * m1->m[1][j] + m0->m[i][2] * m1->m[2][j]) >> 12;
            t += (long long)m0->m[i][j] * m1->t[j];
        }
        r.t[i] = (int)(t >> 12) + m0->t[i];
    }
    *m = r;
    return m;
}

VECTOR* OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* v2) {
    int x = v0->vy * v1->vz - v0->vz * v1->vy;
    int y = v0->vz * v1->vx - v0->vx * v1->vz;
    int z = v0->vx * v1->vy - v0->vy * v1->vx;

    v2->vx = x;
    v2->vy = y;
    v2->vz = z;
    return v2;
}

// PSY-Z has VectorNormal as an empty stub, so the link wraps it with this.
void __wrap_VectorNormal(VECTOR* v0, VECTOR* v1) {
    double x = v0->vx, y = v0->vy, z = v0->vz;
    double length = sqrt(x * x + y * y + z * z);

    if (length == 0) {
        v1->vx = v1->vy = v1->vz = 0;
        return;
    }
    v1->vx = (long)(x * 4096 / length);
    v1->vy = (long)(y * 4096 / length);
    v1->vz = (long)(z * 4096 / length);
}
