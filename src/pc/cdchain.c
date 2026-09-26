// Native stand-in for the CD chain's handler table, D_8004A634.
//
// SystemCdromReadChain dispatches the pending operation through this table,
// once per call, until the handler sets the operation back to idle. On the PS1
// most handlers are still assembly and run the read asynchronously; here each
// one completes its operation synchronously through PSY-Z's libcd.

#include <game.h>
#include <libcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CD_SECTOR_SIZE 0x800

#define CDOP_IDLE 0
#define CDOP_READ 3
#define CDOP_READ_LZS 11

// The chain's state, still bss in assembly on the PS1, defined at its real types.
s32 D_80071A60;          // pending operation
CdlLOC D_80071A68;       // first sector
size_t D_80071A6C;       // sector count
u_long* D_80071A80;      // destination
void (*D_80071A84)();    // completion callback
size_t g_PcCdChainBytes; // byte length of the pending read

void func_80034420(void);
void SystemLzsDecompress(u8* src, u8* dst);

// PSY-Z has no CdRead; it serves ReadN sector by sector through CdReady and
// CdGetSector, the header first and then the 2048 data bytes.
// The PS1 transfers whole sectors, overrunning small destinations into
// whatever follows them; a native link puts different data there, so the last
// sector is cut to the requested length.
static void ReadSectors(void* dst, size_t count, size_t bytes) {
    u_long header[3];
    u8 sector[CD_SECTOR_SIZE];
    u8* out = dst;
    size_t chunk;

    CdControlB(CdlSetloc, (u_char*)&D_80071A68, NULL);
    CdControlB(CdlReadN, NULL, NULL);
    while (count--) {
        while (CdReady(1, NULL) != CdlDataReady) {
        }
        CdGetSector(header, 3);
        CdGetSector(sector, CD_SECTOR_SIZE / 4);
        chunk = bytes < CD_SECTOR_SIZE ? bytes : CD_SECTOR_SIZE;
        memcpy(out, sector, chunk);
        out += chunk;
        bytes -= chunk;
    }
    CdControlB(CdlPause, NULL, NULL);
}

static void Finish(void) {
    D_80071A60 = CDOP_IDLE;
    if (D_80071A84) {
        D_80071A84();
    }
}

static void CdChainRead(void) {
    ReadSectors(D_80071A80, D_80071A6C, g_PcCdChainBytes);
    Finish();
}

// The PS1 decompresses while the sectors stream in; this reads the whole file
// first, which leaves the same bytes at the destination.
static void CdChainReadLzs(void) {
    u8* packed = malloc(D_80071A6C * CD_SECTOR_SIZE);

    ReadSectors(packed, D_80071A6C, D_80071A6C * CD_SECTOR_SIZE);
    SystemLzsDecompress(packed, (u8*)D_80071A80);
    free(packed);
    Finish();
}

static void CdChainUnsupported(void) {
    printf("cdchain: operation %d not supported natively, dropped\n", (int)D_80071A60);
    D_80071A60 = CDOP_IDLE;
}

void (*D_8004A634[21])(void) = {
    func_80034420,      CdChainUnsupported, CdChainUnsupported, CdChainRead,        CdChainUnsupported,
    CdChainUnsupported, CdChainUnsupported, func_80034420,      CdChainUnsupported, CdChainUnsupported,
    CdChainUnsupported, CdChainReadLzs,     CdChainUnsupported, CdChainUnsupported, CdChainUnsupported,
    CdChainUnsupported, CdChainUnsupported, CdChainUnsupported, CdChainUnsupported, CdChainUnsupported,
    CdChainUnsupported,
};
