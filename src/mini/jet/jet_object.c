//! PSYQ=3.3 FORCE_MEM=true

#include "jet_private.h"
#include <libc.h>

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

extern SVECTOR* D_800A8954;
extern s32 D_800A8984;
extern s32 g_JetNextSpawnSegment;
extern s32 g_JetSpawnIndex;
extern u16 D_800D1970[100];
extern u8* D_800D1C00;
extern s32* D_800D1C04;
extern s32* D_800D1C08;
extern Unk800D1C0C* g_JetSpawns;
extern u8* g_JetSpawnCounts;
extern Unk800A4390 g_JetSpawnTemplate;
extern Unk800A4390 g_JetObjects[0x64];
extern u16 D_800D9940;
extern s16 g_JetObjectCount;

static s16 func_800A4400(void);
static void func_800A442C(s16 arg0);
static void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 type, s16 arg4);
static void JetObjectDamage(Unk800A4390* arg0);
static void JetObjectAwardPoints(Unk800A4390* arg0);

const u8 D_800A0008 = 0; // the rotation order the object matrices use

void func_800A3AAC(void) {
    Unk800A4390* obj;
    Unk800A4390* pool;
    s32 i;

    obj = g_JetObjects;
    for (i = 0; i < LEN(g_JetObjects); i++) {
        obj[i].unkD8 = -1;
        obj[i].unkDA = 0;
    }
    D_800D9940 = 0;
    for (i = 0; i < LEN(D_800D1970); i++) {
        D_800D1970[i] = i + 1;
    }
    g_JetShotPower = 128;
    g_JetFiring = 0;
    D_800D1C7C = 0;
    g_JetCursorX = 160;
    g_JetCursorY = 120;
    g_JetNextSpawnSegment = 0;
    g_JetSpawnIndex = 0;
    g_JetObjectCount = 0;
}

static void func_800A3B58(u8 pathIndex, u8 mode) {
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
static void func_800A3C04(u32 pos, SVECTOR* path, VECTOR* out, u8 flag) {
    SVECTOR seg[2];
    VECTOR delta;
    s32 idx;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 frac;

    idx = pos >> 16;
    frac = pos & 0xFFFF;
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
static void func_800A3D50(JetBuffer* arg0) {
    POLY_FT4* poly;

    poly = arg0->prims.ft4Cursor;
    setXY4(poly, g_JetCursorX - 16, g_JetCursorY - 16, g_JetCursorX + 16, g_JetCursorY - 16, g_JetCursorX - 16,
           g_JetCursorY + 16, g_JetCursorX + 16, g_JetCursorY + 16);
    setRGB0(poly, 0x80, 0x80, 0x80);
    setUV4(poly, 0, 0, 0x40, 0, 0, 0x40, 0x40, 0x40);
    poly->tpage = g_JetSpriteTPage[0];
    poly->clut = g_JetSpriteClut[0];
    SetSemiTrans(poly, 0);
    addPrim(&arg0->ot[1], poly);
    poly++;
    arg0->prims.ft4Cursor = poly;
}

// Draw the two laser beams, from each gun muzzle to the aiming cursor.
static void func_800A3E58(void) {
    JetBuffer** db;
    POLY_FT4* poly;
    u8* scroll;
    s32 spread;
    s16* cursorX;
    s16* cursorY;

    if (g_JetFiring == 1) {
        cursorX = &g_JetCursorX;
        cursorY = &g_JetCursorY;
        db = g_JetBufferPtr;
        scroll = &g_JetBeamScroll;
        spread = g_JetShotPower >> 3;
        poly = db[0]->prims.ft4Cursor;
        setXY4(poly, D_800A895C + spread, D_800A8964, *cursorX, *cursorY, D_800A895C - spread, D_800A8964, *cursorX,
               *cursorY);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = g_JetSpriteTPage[1];
        poly->clut = g_JetSpriteClut[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->ot[1], poly);
        poly++;
        setXY4(poly, D_800A8970 + spread, D_800A8978, *cursorX, *cursorY, D_800A8970 - spread, D_800A8978, *cursorX,
               *cursorY);
        setRGB0(poly, 0x80, 0x80, 0x80);
        setUV4(poly, 0x20 - *scroll, 0, 0x20 - *scroll, 0x40, 0x10 - *scroll, 0, 0x10 - *scroll, 0x40);
        poly->tpage = g_JetSpriteTPage[1];
        poly->clut = g_JetSpriteClut[1];
        SetSemiTrans(poly, 1);
        addPrim(&db[0]->ot[1], poly);
        poly++;
        db[0]->prims.ft4Cursor = poly;
    }
}

// Allocate an object and its scene node, then initialise its six bounding box face centres.
static s16 func_800A40F4(Unk800A4390* src, s16 parentIndex) {
    s16* count;
    Unk800A4390* obj;
    Unk800A4390* pool;
    Unk800A4390* parentObj;
    Unk800A4390* box;
    Unk800A4390* boxPool;
    s16 index;
    s32 rawId;
    s16 modelId;
    s16 maxX;
    s16 minX;
    s16 minY;
    s16 maxY;
    s16 minZ;
    s16 maxZ;

    count = &g_JetObjectCount;
    if (*count < 0x63) {
        *count = *count + 1;
        index = func_800A4400();
        g_JetObjects[index] = *src;
        pool = g_JetObjects;
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
        minX = g_JetModelInfo[modelId].unk4.vx;
        maxX = g_JetModelInfo[modelId].unkC.vx;
        minY = g_JetModelInfo[modelId].unk4.vy;
        maxY = g_JetModelInfo[modelId].unkC.vy;
        minZ = g_JetModelInfo[modelId].unk4.vz;
        maxZ = g_JetModelInfo[modelId].unkC.vz;
        boxPool = g_JetObjects;
        box = &boxPool[index];
        setVector(&box->unkDC[0], (maxX + minX) >> 1, (maxY + minY) >> 1, maxZ);
        setVector(&box->unkDC[1], (maxX + minX) >> 1, (maxY + minY) >> 1, minZ);
        setVector(&box->unkDC[2], (maxX + minX) >> 1, maxY, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[3], (maxX + minX) >> 1, minY, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[4], maxX, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
        setVector(&box->unkDC[5], minX, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
    }
    return index;
}

static void func_800A4390(Unk800A4390* arg0) {
    s16* count;

    if (arg0->unkD8 != -1) {
        count = &g_JetObjectCount;
        *count -= 1;
        JetNodeFree(arg0->unkD4);
        func_800A442C(arg0->unkD8);
        arg0->unkD8 = -1;
        arg0->unkDA = 0;
    }
}

static s16 func_800A4400(void) {
    u16* head;
    s16 index;

    head = &D_800D9940;
    index = *head;
    *head = D_800D1970[index];

    return index;
}

static void func_800A442C(s16 arg0) {
    u16* head;
    u16* slot;

    slot = &D_800D1970[arg0];
    head = &D_800D9940;
    *slot = *head;
    *head = arg0;
}

// Spawn the objects scheduled for every track segment reached this frame.
static void func_800A4458(void) {
    Unk800D1C0C* spawns;
    u8* counts;
    u8* count;
    s32 segment;
    s32 index;
    s32 i;
    s32 j;

    for (segment = g_JetNextSpawnSegment; segment < g_JetTrackSegment; segment++) {
        counts = g_JetSpawnCounts;
        count = counts + segment;
        for (i = 0; i < *count; i++) {
            spawns = g_JetSpawns;
            for (j = 0; j < LEN(g_JetSpawnTemplate.unk28.unk50); j++) {
                g_JetSpawnTemplate.unk28.unk50[j] = spawns[*(s32*)(u32)&g_JetSpawnIndex].unk10[j];
            }
            index = *(s32*)(u32)&g_JetSpawnIndex;
            g_JetSpawnTemplate.unk28.unk18 = spawns[index].unk8;
            g_JetSpawnTemplate.unk28.unk1C = spawns[index].unkC;
            func_800A45C0(0, 0, 0, spawns[index].unk0, spawns[index].unk4);
            (*(s32*)(u32)&g_JetSpawnIndex)++;
        }
    }
    g_JetNextSpawnSegment = g_JetTrackSegment;
}

static void func_800A45C0(s16 arg0, s16 arg1, s16 arg2, s16 type, s16 arg4) {
    g_JetSpawnTemplate.unk0.vx = arg0;
    g_JetSpawnTemplate.unk0.vy = arg1;
    g_JetSpawnTemplate.unk0.vz = arg2;
    g_JetSpawnTemplate.unk28.type = type;
    g_JetSpawnTemplate.unk28.unk10 = 1;
    g_JetSpawnTemplate.unk28.unk8 = arg4;
    g_JetSpawnTemplate.unk28.hit = 0;
    func_800A40F4(&g_JetSpawnTemplate, 0);
}

inline void func_800A4650(s16 arg0, s16 arg1, s16 arg2, s16 type, s16 arg4) {
    g_JetSpawnTemplate.unk0.vx = arg0;
    g_JetSpawnTemplate.unk0.vy = arg1;
    g_JetSpawnTemplate.unk0.vz = arg2;
    g_JetSpawnTemplate.unk28.type = type;
    g_JetSpawnTemplate.unk28.unk10 = 1;
    g_JetSpawnTemplate.unk28.unk8 = arg4;
    g_JetSpawnTemplate.unk28.hit = 0;
    g_JetSpawnTemplate.unk28.unk50[0xC] = 0;
    func_800A40F4(&g_JetSpawnTemplate, 0);
}

static inline void JetObjectFree(Unk800A4390* obj) {
    if (obj->unkD8 != -1) {
        g_JetObjectCount--;
        JetNodeFree(obj->unkD4);
        func_800A442C(obj->unkD8);
        obj->unkD8 = -1;
        obj->unkDA = 0;
    }
}

// Step every live object through its behaviour, then queue its model.
void func_800A46E8(JetBuffer* db) {
    VECTOR next;
    VECTOR unused[2];
    VECTOR pos;
    SVECTOR rot;
    Unk800A4390* obj;
    Unk800A4390* pool;
    Unk800D1CAC* st;
    POLY_G4* fade;
    POLY_FT4* tpagePrim;
    s16 pathIndex;
    s32 shade;
    s32 count;
    s32 i;
    s32 j;
    s32 dx;
    s32 dy;
    s32 dz;
    s32* score;
    s32* segment;
    s32 sound;
    u8 order;
    u8 drawMode;
    u16 otIndex;

    func_800A4458();
    func_800A3D50(db);
    func_800A3E58();
    for (i = 0; i < LEN(g_JetObjects); i++) {
        otIndex = 0;
        pool = g_JetObjects;
        obj = &pool[i];
        st = &obj->unk28;
        if (obj->unkDA == 0) {
            continue;
        }
        drawMode = 0;
        do {
        } while (0);
        switch (obj->unk28.type) {
        case 100:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->hit = 0;
                st->unk50[10] = 1;
                st->unk28 = 0;
                st->unk2C = obj->unk0.vx;
                st->unk30 = obj->unk0.vy;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk34 = obj->unk0.vz;
                st->unk50[0] = 0;
            } else {
                st->unk14++;
                st->unk28++;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            {
                s32 step;
                s32* pathPos;
                s32 x;
                s32 y;
                s32 z;

                step = st->unk28;
                pathPos = &D_800D1C54;
                JetTrackSample(pathPos[0] + 0x2FFFD, -100, &pos, &rot);
                x = st->unk2C;
                x += (step * (pos.vx - x)) >> 7;
                y = st->unk30;
                y += (step * (pos.vy - y)) >> 7;
                z = st->unk34;
                z += (step * (pos.vz - z)) >> 7;
                obj->unk0.vx = x;
                obj->unk0.vy = y;
                obj->unk0.vz = z;
                JetTrackSample(pathPos[0] + 0x3FFFC, -100, &pos, &rot);
            }
            dx = obj->unk0.vx - pos.vx;
            dy = obj->unk0.vy - pos.vy;
            dz = obj->unk0.vz - pos.vz;
            SquareRoot0(dx * dx + dy * dy + dz * dz);
            if (st->unk14 >= 0x81) {
                score = &g_JetScore;
                if (*score > 5) {
                    *score -= 5;
                } else {
                    *score = 0;
                }
                st->unkC = 0;
            }
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 17:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->hit = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                st->unk2C = (obj->unkC8 - 2) << 16;
            } else {
                st->unk14++;
            }
            segment = &g_JetTrackSegment;
            if (st->unk50[3] < *segment) {
                st->unk28 += st->unk1C;
            }
            if (st->unk50[2] < *segment) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            if (st->hit != 0) {
                JetObjectDamage(obj);
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
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->hit = 0;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
            obj->unk18.vx += st->unk50[3];
            obj->unk18.vy += st->unk50[4];
            obj->unk18.vz += st->unk50[5];
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 1:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->hit = 0;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
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
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 10:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathLen = D_800D1BEC[0];
                offset = D_800D1BE8[0];
                path = (SVECTOR*)(D_800D1BE4 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->unk14 = 0;
                st->hit = 0;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            JetTrackSample(D_800D1C54 + 0x3FFFC, 10, &obj->unk0, &obj->unk18);
            drawMode = 1;
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 4:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            segment = &g_JetTrackSegment;
            if (st->unk50[2] < *segment) {
                st->unkC = 0;
            }
            if (st->unk50[3] < *segment) {
                st->unk2C += 4;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            obj->unk0.vy += st->unk2C;
            if (obj->unk0.vy > 0) {
                st->unkC = 0;
            }
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 5:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            obj->unk18.vx += st->unk50[6];
            obj->unk18.vy += st->unk50[7];
            obj->unk18.vz += st->unk50[8];
            if (st->unk50[10] == 5) {
                obj->unkD4->model = g_JetModelTable[91 + st->unk50[14]];
            }
            if (st->unk50[14] == 1) {
                st->unk50[14] = 0;
            }
            if (st->unkC != 0) {
                func_800A3C04(st->unk28, obj->unkCC, &obj->unk0, 0);
                if (st->hit != 0) {
                    if (st->unk50[10] != 5 || g_JetSpeed < 0x4015) {
                        JetObjectDamage(obj);
                    }
                    if (st->unk50[10] == 5) {
                        st->unk50[14] = 1;
                    }
                }
            } else {
                JetObjectFree(obj);
            }
            break;
        case 2:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                sound = st->unk50[17];
                if (sound != 0) {
                    JetPlaySfx(sound);
                }
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                D_800A8984 = pathLen;
                D_800A8954 = path;
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
                st->unk28 = (rand() % st->unk50[3]) * 2 - st->unk50[3] - 1;
                st->unk2C = 0;
                st->unk30 = 0;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unk2C > st->unk28) {
                st->unk2C -= 5;
            }
            if (st->unk2C < st->unk28) {
                st->unk2C += 5;
            }
            obj->unk18.vx = st->unk2C;
            obj->unk18.vy += st->unk50[4];
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            func_800A3C04(st->unk30, obj->unkCC, &obj->unk0, 0);
            if (st->hit != 0) {
                JetObjectDamage(obj);
            }
            break;
        case 230:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
            }
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC != 0) {
                if (st->hit != 0) {
                    JetObjectDamage(obj);
                }
            } else {
                JetObjectFree(obj);
            }
            break;
        case 7:
        case 13:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
                obj->unk18.vx = st->unk50[3];
                obj->unk18.vy = st->unk50[4];
                obj->unk18.vz = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
                st->unk28 = 0;
            }
            segment = &g_JetTrackSegment;
            if (st->unk50[2] < *segment) {
                st->unkC = 0;
            }
            if (st->unk50[5] < *segment) {
                count = st->unk28;
                st->unk28 = count + 1;
                if (count < st->unk50[7]) {
                    obj->unk18.vx += st->unk50[6];
                }
            }
            if (st->unkC != 0) {
                if (st->hit != 0) {
                    JetObjectDamage(obj);
                }
            } else {
                JetObjectFree(obj);
            }
            break;
        case 11:
            JetPlaySfx(0x8E);
            for (j = 0; j < st->unk50[3]; j++) {
                func_800A4650(0x3446, -0x2710, 0x20CB, 0xC, 0x2A);
            }
            JetObjectFree(obj);
            // falls through into the debris behaviour below
        case 12:
            if (st->unk10 == 1) {
                st->hit = 0;
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
            obj->unk0.vz += st->unk30;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x190;
            obj->unk18.vz += 0xC8;
            st->unkC--;
            if (st->unkC == 0) {
                JetObjectFree(obj);
            }
            break;
        case 8:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
                st->unk28 = 0;
                D_800A8984 = pathLen;
                D_800A8954 = path;
                func_800A3C04(0, obj->unkCC, &obj->unk0, 0);
            }
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC == 0) {
                JetObjectFree(obj);
                break;
            }
            obj->unk0.vy -= st->unk50[3];
            st->unk50[3] -= st->unk50[4];
            if (st->unk50[3] < 0) {
                JetPlaySfx(0x98);
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
                st->hit = 0;
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
            obj->unk0.vz += st->unk30;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x190;
            obj->unk18.vz += 0xC8;
            st->unkC--;
            if (st->unkC == 0) {
                JetObjectFree(obj);
            }
            break;
        case 14:
            if (st->unk10 == 1) {
                SVECTOR* path;
                s32 pathLen;
                s32 offset;

                JetPlaySfx(0xA);
                pathIndex = st->unk18 & 0xFF;
                pathLen = D_800D1C08[pathIndex];
                offset = D_800D1C04[pathIndex];
                path = (SVECTOR*)(D_800D1C00 + offset);
                obj->unkCC = path;
                obj->unkC8 = pathLen;
                st->unk10 = 0;
                st->unkC = 1;
                st->hit = 0;
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

                    x = obj->unk0.vx + rand() % 100 - 0x32;
                    y = obj->unk0.vy + 0x1F4;
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
            if (st->unk50[2] < g_JetTrackSegment) {
                st->unkC = 0;
            }
            if (st->unkC != 0) {
                if (st->hit != 0) {
                    JetObjectDamage(obj);
                }
            } else {
                JetObjectFree(obj);
            }
            break;
        case 15:
            if (st->unk10 == 1) {
                st->hit = 0;
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
            obj->unk18.vy += 0x28;
            obj->unk18.vz += 0x262;
            obj->unk0.vx += st->unk28;
            obj->unk0.vy += st->unk30;
            obj->unk0.vz += st->unk2C;
            st->unkC--;
            if (st->unkC == 0) {
                JetObjectFree(obj);
            }
            break;
        case 16:
            if (st->unk10 == 1) {
                st->unkC = 200;
                st->hit = 0;
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
                JetObjectFree(obj);
            }
            break;
        case 3:
            obj->unk0.vx = D_800A83B8.vx;
            obj->unk0.vy = D_800A83B8.vy - 0x9C4;
            obj->unk0.vz = D_800A83B8.vz;
            otIndex = 0x3E8;
            obj->unk18.vx = 0;
            obj->unk18.vy = 0;
            obj->unk18.vz = 0;
            break;
        case 201:
            if (st->unk10 == 1) {
                st->unkC = 0x14;
                st->unk10 = 0;
                st->unk14 = 0;
                st->hit = 0;
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
                JetObjectFree(obj);
            }
            break;
        case 202:
            if (st->unk10 == 1) {
                st->hit = 0;
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
            obj->unk0.vz += st->unk30;
            obj->unk18.vx += 0xA;
            obj->unk18.vy += 0x64;
            obj->unk18.vz += 0x14;
            st->unkC--;
            if (st->unkC == 0) {
                JetObjectFree(obj);
            }
            break;
        case 203:
            if (st->unk10 == 1) {
                st->hit = 0;
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
            obj->unk18.vx += 0;
            obj->unk18.vy += 0x12C;
            obj->unk18.vz += 0;
            st->unkC--;
            if (st->unkC == 0) {
                JetObjectFree(obj);
            }
            break;
        case 255:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
            } else {
                st->unk14++;
            }
            {
                s32* speed;

                speed = &g_JetSpeed;
                if (*speed > st->unk50[2]) {
                    *speed -= st->unk50[0];
                }
                if (*speed < 0) {
                    *speed = 0;
                    func_800A4650(0, 0, 0, 0xFD, 0x1D);
                }
            }
            if (st->unk14 > st->unk50[1]) {
                JetObjectFree(obj);
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
                g_JetSpeed = 0;
            }
            if (st->unk28 == 1) {
                s32* speed;

                speed = &g_JetSpeed;
                *speed += st->unk50[1];
                st->unk30++;
            }
            if (st->unk50[0] < VSync(-1) - st->unk2C) {
                st->unk28 = 1;
            }
            if (st->unk30 > st->unk50[2]) {
                JetObjectFree(obj);
            }
            break;
        case 252:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
                g_JetSpeed = 0;
                D_800E25F4 = 1;
                func_800A4650(0, 0, 0, 3, 0x3B);
            } else {
                st->unk14++;
            }
            shade = ~(st->unk14 * 2);
            fade = db->prims.g4Cursor;
            setXY4(fade, 0, 0, 320, 0, 0, 240, 320, 240);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->ot2[0], fade);
            fade++;
            db->prims.g4Cursor = fade;
            tpagePrim = db->prims.ft4Cursor;
            setRGB0(tpagePrim, 0, 0, 0);
            setXY4(tpagePrim, 0, 0, 0, 0, 0, 0, 0, 0);
            tpagePrim->tpage = D_800AB894;
            tpagePrim->clut = D_800A8A68;
            SetSemiTrans(tpagePrim, 0);
            addPrim(&db->ot2[1], tpagePrim);
            tpagePrim++;
            db->prims.ft4Cursor = tpagePrim;
            if (st->unk14 >= 0x7E) {
                JetObjectFree(obj);
                g_JetSpeed = 0x4000;
            }
            break;
        case 253:
            if (st->unk10 == 1) {
                st->unk10 = 0;
                st->unk14 = 0;
                g_JetSpeed = 0;
                func_800A2938();
            } else {
                st->unk14++;
            }
            shade = st->unk14 * 2;
            fade = db->prims.g4Cursor;
            setXY4(fade, 0, 0, 320, 0, 0, 240, 320, 240);
            setRGB0(fade, shade, shade, shade);
            setRGB1(fade, shade, shade, shade);
            setRGB2(fade, shade, shade, shade);
            setRGB3(fade, shade, shade, shade);
            SetSemiTrans(fade, 1);
            addPrim(&db->ot2[0], fade);
            fade++;
            db->prims.g4Cursor = fade;
            tpagePrim = db->prims.ft4Cursor;
            setRGB0(tpagePrim, 0, 0, 0);
            setXY4(tpagePrim, 0, 0, 0, 0, 0, 0, 0, 0);
            tpagePrim->tpage = D_800AB894;
            tpagePrim->clut = D_800A8A68;
            SetSemiTrans(tpagePrim, 0);
            addPrim(&db->ot2[1], tpagePrim);
            tpagePrim++;
            db->prims.ft4Cursor = tpagePrim;
            if (st->unk14 >= 0x80) {
                JetObjectFree(obj);
                g_JetSpeed = 0x4000;
                D_800E25F4 = 0;
                g_JetExit = 1;
            }
            break;
        case 250:
            if (g_JetScore < st->unk50[0]) {
                Unk800A4390* spawn;

                spawn = &g_JetSpawnTemplate;
                spawn->unk28.unk50[0] = 0x12C;
                spawn->unk28.unk50[1] = 0x190;
                spawn->unk28.unk50[2] = 0;
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
            JetObjectFree(obj);
            break;
        default:
            break;
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

static void JetObjectDamage(Unk800A4390* arg0) {
    Unk800D1CAC* state = &arg0->unk28;
    u8 amount;
    s32 x;
    s32 y;
    s32 z;

    amount = g_JetShotPower >> 5;
    if (amount == 0) {
        amount = 1;
    }
    state->unk50[0xD] -= amount;
    if (state->unk50[0xD] < 0) {
        JetObjectAwardPoints(arg0);
    } else {
        x = arg0->unk0.vx;
        y = arg0->unk0.vy;
        z = arg0->unk0.vz;
        func_800A4650(x, y, z, 0xCA, 0x3F);
    }
}

// Award the score for a hit object and scatter its debris.
static void JetObjectAwardPoints(Unk800A4390* obj) {
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

        score = &g_JetScore;
        *score += st->unk50[0];
        JetPlaySfx(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCA, rand() % 3 + 0x3F);
        }
        g_JetPopupModelId = obj->unkD4->modelId;
        points = st->unk50[0];
        g_JetPopupPoints = points;
        g_JetPopupTimer = 100;
        D_800E25E8 = 1;
        setVector(&g_JetPopupRot, 0, 0, 0);
    }
    if (st->unk50[10] == 2) {
        s32* score;
        s32 points;
        s32 x;
        s32 y;
        s32 z;

        score = &g_JetScore;
        *score += st->unk50[0];
        JetPlaySfx(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 3; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCB, rand() % 3 + 0x3C);
        }
        g_JetPopupModelId = obj->unkD4->modelId;
        points = st->unk50[0];
        g_JetPopupPoints = points;
        g_JetPopupTimer = 100;
        D_800E25E8 = 1;
        setVector(&g_JetPopupRot, 0, 0, 0);
    }
    if (st->unk50[10] == 3) {
        s32* score;
        s32 points;

        score = &g_JetScore;
        *score += st->unk50[0];
        points = st->unk50[11];
        obj->unk18.vx += points;
    }
    if (st->unk50[10] == 4) {
        s32* score;
        s32 points;
        score = &g_JetScore;
        *score += st->unk50[0];
        JetPlaySfx(st->unk50[18]);
        st->unkC = 0;
        g_JetPopupModelId = obj->unkD4->modelId;
        points = st->unk50[0];
        g_JetPopupPoints = points;
        g_JetPopupTimer = 100;
        D_800E25E8 = 1;
        setVector(&g_JetPopupRot, 0, 0, 0);
    }
    if (st->unk50[10] == 5) {
        s32* score;
        s32 points;
        s32 x;
        s32 y;
        s32 z;

        score = &g_JetScore;
        *score += st->unk50[0];
        JetPlaySfx(st->unk50[18]);
        st->unkC = 0;
        for (i = 0; i < 100; i++) {
            x = obj->unk0.vx;
            y = obj->unk0.vy;
            z = obj->unk0.vz;
            func_800A4650(x, y, z, 0xCB, rand() % 3 + 0x3F);
        }
        g_JetPopupModelId = obj->unkD4->modelId;
        points = st->unk50[0];
        g_JetPopupPoints = points;
        g_JetPopupTimer = 100;
        D_800E25E8 = 1;
        setVector(&g_JetPopupRot, 0, 0, 0);
    }
    score = &g_JetScore;
    if (*score > 9999) {
        *score = 9999;
    }
}
