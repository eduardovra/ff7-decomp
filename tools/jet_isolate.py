"""Film one jet object type alone on an empty screen, in the native build.

Patches the native-only copies of jet.c and jet_object.c under build-pc/strd/
(never src/), rebuilds, and runs one no-input ride under gdb. The patch skips
the scenery, draws only the first live object of the chosen type, and puts the
camera a fixed distance in front of it. Object logic is untouched, so spawning
and slot counts behave as in a normal ride. A frame is saved every --step
frames of the object's life, plus a contact sheet. The patched copies are
deleted and rebuilt from src/ afterwards, even on failure.

    python3 tools/jet_isolate.py 2
    python3 tools/jet_isolate.py 2 --model 90 --follow --dist 5000
    python3 tools/jet_isolate.py 14 --also 15,16 --instances 1 --step 10

Writes PNGs into build/jet_isolate/t<type>[_m<model>]/. See docs/jet-probe.md.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = REPO_ROOT / "build-pc"
BINARY = BUILD_DIR / "ff7_pc"
STRD_JET = BUILD_DIR / "strd" / "src" / "mini" / "jet"
OUT_ROOT = REPO_ROOT / "build" / "jet_isolate"

# Added to jet_object.c. Env vars configure it; unset JET_ISOLATE disables it.
ISOLATE_C = r"""
extern char* getenv(const char*);
static int isoInit, isoType = -1, isoDist, isoFollow, isoModel, isoSkip;
static int isoAlso[8], isoAlsoCount;
static JetNode* isoNode;
static VECTOR isoAnchor;
s32 g_JetIsoSlot = -1;
s32 g_JetIsoFrames;
s32 g_JetIsoCount;

static int IsoParse(const char** sp, int def) {
    const char* s = *sp;
    int sign = 1, v = 0, any = 0;
    if (!s) return def;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s++ - '0'); any = 1; }
    *sp = s;
    return any ? sign * v : def;
}

static int IsoEnv(const char* name, int def) {
    const char* s = getenv(name);
    return IsoParse(&s, def);
}

int JetIsoEnabled(void) {
    if (!isoInit) {
        const char* also = getenv("JET_ISOLATE_ALSO");
        isoInit = 1;
        isoType = IsoEnv("JET_ISOLATE", -1);
        isoDist = IsoEnv("JET_ISOLATE_DIST", 3000);
        isoFollow = IsoEnv("JET_ISOLATE_FOLLOW", 0);
        isoModel = IsoEnv("JET_ISOLATE_MODEL", -1);
        isoSkip = IsoEnv("JET_ISOLATE_SKIP", 0);
        while (also && *also && isoAlsoCount < 8) {
            isoAlso[isoAlsoCount++] = IsoParse(&also, -1);
            if (*also != ',') break;
            also++;
        }
    }
    return isoType >= 0;
}

static int IsoWanted(JetObject* o) {
    return o->state.type == isoType && (isoModel < 0 || o->state.modelId == isoModel);
}

static void IsoPickTarget(void) {
    JetObject* o;
    int i;

    for (i = 0; i < 100; i++) {
        o = &g_JetObjects[i];
        if (o->index == -1 || !IsoWanted(o) || o->state.needsInit == 1 || o->node == isoNode) {
            continue;
        }
        isoNode = o->node;
        if (isoSkip > 0) {
            isoSkip--;
            continue;
        }
        g_JetIsoSlot = i;
        isoAnchor = o->position;
        g_JetIsoFrames = 0;
        g_JetIsoCount++;
        return;
    }
}

void JetIsoCamera(VECTOR* camPos, MATRIX* camRot) {
    JetObject* o;

    if (!JetIsoEnabled()) {
        return;
    }
    if (g_JetIsoSlot >= 0) {
        o = &g_JetObjects[g_JetIsoSlot];
        if (o->index == -1 || o->node != isoNode || !IsoWanted(o)) {
            g_JetIsoSlot = -1;
        } else {
            g_JetIsoFrames++;
        }
    }
    if (g_JetIsoSlot < 0) {
        IsoPickTarget();
    }
    if (g_JetIsoSlot < 0) {
        return;
    }
    o = &g_JetObjects[g_JetIsoSlot];
    if (isoFollow) {
        isoAnchor = o->position;
    }
    camPos->vx = isoAnchor.vx;
    camPos->vy = isoAnchor.vy;
    camPos->vz = isoAnchor.vz - isoDist;
    camRot->m[0][0] = 0x1000; camRot->m[0][1] = 0; camRot->m[0][2] = 0;
    camRot->m[1][0] = 0; camRot->m[1][1] = 0x1000; camRot->m[1][2] = 0;
    camRot->m[2][0] = 0; camRot->m[2][1] = 0; camRot->m[2][2] = 0x1000;
    camRot->t[0] = camRot->t[1] = camRot->t[2] = 0;
}

int JetIsoShouldDraw(JetObject* o) {
    int i;

    if (!JetIsoEnabled()) {
        return 1;
    }
    if (g_JetIsoSlot >= 0 && o == &g_JetObjects[g_JetIsoSlot]) {
        return 1;
    }
    for (i = 0; i < isoAlsoCount; i++) {
        if (o->state.type == isoAlso[i]) {
            return 1;
        }
    }
    return 0;
}
"""

# (file, anchor, replacement); each anchor must occur exactly once.
EDITS = [
    (
        "jet_object.c",
        "void JetObjectsUpdate(JetBuffer* db) {",
        "int JetIsoShouldDraw(JetObject* o);\n"
        "void JetObjectsUpdate(JetBuffer* db) {",
    ),
    (
        "jet_object.c",
        "        if (drawMode == 0) {\n"
        "            JetDrawObjectAndCheckHit(",
        "        if (!JetIsoShouldDraw(obj)) {\n"
        "            continue;\n"
        "        }\n"
        "        if (drawMode == 0) {\n"
        "            JetDrawObjectAndCheckHit(",
    ),
    (
        "jet.c",
        "static void JetDrawTrack(void);",
        "static void JetDrawTrack(void);\n"
        "void JetIsoCamera(VECTOR* camPos, MATRIX* camRot);\n"
        "int JetIsoEnabled(void);",
    ),
    (
        "jet.c",
        "            JetCameraUpdate();\n"
        "            JetTrackListsAdvance(*speed);\n"
        "            JetSetWorldMatrix();\n"
        "            JetDrawTrack();\n"
        "            JetDrawTriangleList();",
        "            JetCameraUpdate();\n"
        "            JetIsoCamera(&g_JetCameraPos, &g_JetCameraRot);\n"
        "            JetTrackListsAdvance(*speed);\n"
        "            JetSetWorldMatrix();\n"
        "            if (!JetIsoEnabled()) {\n"
        "                JetDrawTrack();\n"
        "                JetDrawTriangleList();\n"
        "            }",
    ),
]


def cmake_build() -> None:
    command = ["cmake", "--build", str(BUILD_DIR), "-j8"]
    subprocess.run(command, cwd=REPO_ROOT, check=True, stdout=subprocess.DEVNULL)


def apply_patch() -> None:
    sources = {name: (STRD_JET / name).read_text() for name in ("jet.c", "jet_object.c")}
    for name, anchor, replacement in EDITS:
        count = sources[name].count(anchor)
        if count != 1:
            raise SystemExit(f"{name}: anchor found {count} times, expected 1:\n{anchor}")
        sources[name] = sources[name].replace(anchor, replacement)
    sources["jet_object.c"] += ISOLATE_C
    for name, text in sources.items():
        (STRD_JET / name).write_text(text)


def restore() -> None:
    # The strd copies are generated from src/; deleting them regenerates them.
    for name in ("jet.c", "jet_object.c"):
        (STRD_JET / name).unlink(missing_ok=True)
    cmake_build()


def launch(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("type", type=int)
    parser.add_argument("--model", type=int, default=-1)
    parser.add_argument("--dist", type=int, default=3000, help="fog hides >~8000")
    parser.add_argument("--follow", action="store_true", help="track the object")
    parser.add_argument("--also", default="", help="more types to draw, e.g. 15,16")
    parser.add_argument("--skip", type=int, default=0, help="instances to pass over")
    parser.add_argument("--instances", type=int, default=2)
    parser.add_argument("--step", type=int, default=10)
    parser.add_argument("--max-age", type=int, default=300)
    args = parser.parse_args(argv)

    name = f"t{args.type:03d}" + (f"_m{args.model:03d}" if args.model >= 0 else "")
    out_dir = OUT_ROOT / name
    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen")
    env.update(
        JET_ISOLATE=str(args.type),
        JET_ISOLATE_MODEL=str(args.model),
        JET_ISOLATE_DIST=str(args.dist),
        JET_ISOLATE_FOLLOW=str(int(args.follow)),
        JET_ISOLATE_ALSO=args.also,
        JET_ISOLATE_SKIP=str(args.skip),
        ISO_OUT=str(out_dir),
        ISO_INSTANCES=str(args.instances),
        ISO_STEP=str(args.step),
        ISO_MAX_AGE=str(args.max_age),
    )
    command = [
        "gdb",
        "-q",
        "-batch",
        "-ex", "set pagination off",
        "-ex", "set confirm off",
        "-ex", "break JetObjectsUpdate",
        "-ex", "run",
        "-x", __file__,
        "--args", str(BINARY), "-jet",
    ]  # fmt: skip

    cmake_build()  # sync the strd copies with src/ before patching them
    apply_patch()
    try:
        cmake_build()
        result = subprocess.run(command, cwd=REPO_ROOT, env=env, capture_output=True, text=True)
    finally:
        restore()
    summary = [line for line in result.stdout.splitlines() if line.startswith("jet_isolate:")]
    print("\n".join(summary) or result.stdout[-2000:])
    return result.returncode


def eval_int(expr: str) -> int:
    return int(gdb.parse_and_eval(expr))


def grab_frame() -> Image.Image:
    # Inferior calls work here, at the top level, but hang inside a stop().
    sizes = eval_int("(unsigned long)malloc(8)")
    call = f"Psyz_VideoAllocCapturedFrame((int*){sizes}, (int*){sizes + 4})"
    pixels = eval_int(f"(unsigned long){call}")
    width = eval_int(f"*(int*){sizes}")
    height = eval_int(f"*(int*){sizes + 4}")
    data = gdb.selected_inferior().read_memory(pixels, width * height * 3)
    gdb.parse_and_eval(f"free((void*){sizes})")
    gdb.parse_and_eval(f"free((void*){pixels})")
    return Image.frombytes("RGB", (width, height), data.tobytes())


def save_sheet(frames: list[tuple[str, Image.Image]], path: Path) -> None:
    columns = 6
    width, height = 320, 240
    rows = (len(frames) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * width, rows * (height + 14)), (40, 40, 40))
    draw = ImageDraw.Draw(sheet)
    for i, (label, frame) in enumerate(frames):
        x, y = (i % columns) * width, (i // columns) * (height + 14)
        sheet.paste(frame.resize((width, height)), (x, y))
        draw.text((x + 4, y + height), label, fill=(255, 255, 255))
    sheet.save(path)


def capture() -> None:
    out_dir = Path(os.environ["ISO_OUT"])
    instances = int(os.environ["ISO_INSTANCES"])
    step = int(os.environ["ISO_STEP"])
    max_age = int(os.environ["ISO_MAX_AGE"])
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()
    frames = []
    while True:
        try:
            gdb.execute("continue", to_string=True)
            # the port crashes back in field after the ride ends
            if gdb.selected_frame().name() != "JetObjectsUpdate":
                break
        except gdb.error:
            break
        slot = eval_int("g_JetIsoSlot")
        age = eval_int("g_JetIsoFrames")
        count = eval_int("g_JetIsoCount")
        if count > instances:
            break
        # The displayed frame lags one update behind the object state, and
        # the first one still predates the object.
        if slot < 0 or age < 2 or (age - 1) % step or age > max_age:
            continue
        model = eval_int(f"g_JetObjects[{slot}].state.modelId")
        segment = eval_int("g_JetTrackSegment")
        label = f"i{count}_m{model:03d}_age{age - 1:03d}_seg{segment:04d}"
        frame = grab_frame()
        frame.save(out_dir / f"{label}.png")
        frames.append((label, frame))
    if frames:
        save_sheet(frames=frames, path=out_dir / "sheet.png")
    print(f"jet_isolate: {len(frames)} frames in {out_dir}")
    if gdb.selected_inferior().pid:
        gdb.execute("kill")


try:
    import gdb
except ImportError:
    gdb = None

if gdb is None:
    sys.exit(launch(sys.argv[1:]))
else:
    from PIL import Image, ImageDraw

    capture()
