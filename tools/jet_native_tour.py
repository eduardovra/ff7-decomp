"""Capture jet objects of chosen types in the native build, under gdb.

Runs ./build-pc/ff7_pc -jet with no input for one whole ride and saves the
displayed frame whenever an object of a target type reaches one of the given
ages, with its projected box drawn in magenta. Needs no emulator and no save
state; a full ride takes seconds. Writes PNGs into build/jet_native_tour/.

    python3 tools/jet_native_tour.py 0 2 4 14
    python3 tools/jet_native_tour.py 14 --ages 3,10,20 --per-type 4

See docs/jet-probe.md.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "build" / "jet_native_tour"
BINARY = REPO_ROOT / "build-pc" / "ff7_pc"
CONFIG_ENV = "JET_NATIVE_TOUR"
POOL_SIZE = 100
# The game projects only the first five box points; the sixth stays stale.
BOX_POINTS = 5


def launch(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("types", nargs="+", type=int)
    parser.add_argument("--ages", default="3,15,40,90")
    parser.add_argument("--per-type", type=int, default=2)
    parser.add_argument("--frames", type=int, default=13000)
    args = parser.parse_args(argv)
    config = {
        "types": args.types,
        "ages": [int(age) for age in args.ages.split(",")],
        "per_type": args.per_type,
        "frames": args.frames,
    }
    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen")
    env[CONFIG_ENV] = json.dumps(config)
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
    result = subprocess.run(command, cwd=REPO_ROOT, env=env)
    return result.returncode


def eval_int(expr: str) -> int:
    return int(gdb.parse_and_eval(expr))


def field_offset(field: str) -> int:
    return eval_int(f"(long)&((JetObject*)0)->{field}")


def to_s16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def read_objects(offsets: dict[str, int], size: int) -> dict[int, dict]:
    base = eval_int("(long)&g_JetObjects")
    inferior = gdb.selected_inferior()
    raw = inferior.read_memory(base, size * POOL_SIZE).tobytes()
    objects = {}
    for slot in range(POOL_SIZE):
        obj = raw[slot * size : (slot + 1) * size]
        if struct.unpack_from("<h", obj, offsets["active"])[0] == 0:
            continue
        points = []
        for k in range(BOX_POINTS):
            packed = struct.unpack_from("<Q", obj, offsets["unk11C"] + 8 * k)[0]
            x = to_s16(packed & 0xFFFF)
            y = to_s16((packed >> 16) & 0xFFFF)
            points.append((x, y))
        objects[slot] = {
            "node": struct.unpack_from("<Q", obj, offsets["node"])[0],
            "type": struct.unpack_from("<i", obj, offsets["type"])[0],
            "model": struct.unpack_from("<i", obj, offsets["model"])[0],
            "points": points,
        }
    return objects


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


def save_capture(frame: Image.Image, obj: dict, name: str) -> None:
    image = frame.copy()
    xs = [x for x, _ in obj["points"]]
    ys = [y for _, y in obj["points"]]
    box = [min(xs) - 2, min(ys) - 2, max(xs) + 2, max(ys) + 2]
    ImageDraw.Draw(image).rectangle(box, outline=(255, 0, 255))
    image = image.resize((image.width * 2, image.height * 2), Image.NEAREST)
    image.save(OUT_DIR / name)
    print(f"saved {name} box {box}")


def tour(config: dict) -> None:
    fields = ("active", "node", "unk11C")
    offsets = {name: field_offset(name) for name in fields}
    offsets["type"] = field_offset("state.type")
    offsets["model"] = field_offset("state.modelId")
    size = eval_int("sizeof(JetObject)")
    targets = set(config["types"])
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    born: dict[tuple, int] = {}
    counts: dict[int, int] = {}
    ordinal: dict[tuple, int] = {}
    for frame_no in range(config["frames"]):
        try:
            gdb.execute("continue", to_string=True)
        except gdb.error:
            break
        if not gdb.selected_inferior().pid:
            break
        objects = read_objects(offsets=offsets, size=size)
        live_keys = {
            (slot, obj["node"], obj["type"]) for slot, obj in objects.items()
        }
        born = {key: frame for key, frame in born.items() if key in live_keys}
        for key in live_keys:
            if key not in born:
                born[key] = frame_no
                if key[2] in targets:
                    counts[key[2]] = counts.get(key[2], 0) + 1
                    ordinal[key] = counts[key[2]]
        wanted = []
        for key, first_frame in born.items():
            age = frame_no - first_frame
            if ordinal.get(key, 0) > config["per_type"]:
                continue
            if key[2] in targets and age in config["ages"]:
                wanted.append((key, age))
        if not wanted:
            continue
        frame = grab_frame()
        segment = eval_int("g_JetTrackSegment")
        for (slot, _, obj_type), age in wanted:
            obj = objects[slot]
            name = (
                f"t{obj_type:03d}_m{obj['model']:03d}_s{slot:02d}"
                f"_f{frame_no:05d}_age{age:03d}_seg{segment:04d}.png"
            )
            save_capture(frame=frame, obj=obj, name=name)
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

    tour(json.loads(os.environ[CONFIG_ENV]))
