"""Render every jet model in g_JetModelTable, from the native build under gdb.

Runs ./build-pc/ff7_pc -jet up to the first JetObjectsUpdate, reads each
model's triangles and draws them flat-shaded from a three-quarter view.
Writes contact sheets of 20 models, labelled with id and triangle count,
into build/jet_models/.

    python3 tools/jet_models.py

See docs/jet-probe.md.
"""

from __future__ import annotations

import math
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "build" / "jet_models"
BINARY = REPO_ROOT / "build-pc" / "ff7_pc"
MODEL_COUNT = 100
PER_SHEET = 20
COLUMNS = 5
TILE = 220
YAW = math.radians(35)
PITCH = math.radians(20)

Point = tuple[float, float, float]
Triangle = tuple[list[Point], tuple[int, int, int]]


def launch() -> int:
    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen")
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
    return subprocess.run(command, cwd=REPO_ROOT, env=env).returncode


def rotate(x: int, y: int, z: int) -> Point:
    # PSX +y points down; flip it so up is up in the picture.
    y = -y
    yawed_x = x * math.cos(YAW) + z * math.sin(YAW)
    yawed_z = -x * math.sin(YAW) + z * math.cos(YAW)
    pitched_y = y * math.cos(PITCH) - yawed_z * math.sin(PITCH)
    pitched_z = y * math.sin(PITCH) + yawed_z * math.cos(PITCH)
    return yawed_x, pitched_y, pitched_z


def depth(triangle: Triangle) -> float:
    points, _ = triangle
    return sum(z for _, _, z in points)


def read_triangles(model_id: int) -> list[Triangle]:
    model = gdb.parse_and_eval(f"g_JetModelTable[{model_id}]")
    if int(model) == 0:
        return []
    triangles = []
    for i in range(int(model["triCount"])):
        tri = model["tris"][i]
        points = []
        for v in (tri["v0"], tri["v1"], tri["v2"]):
            points.append(rotate(int(v["vx"]), int(v["vy"]), int(v["vz"])))
        colours = (tri["c0"], tri["c1"], tri["c2"])
        average = tuple(
            sum(int(c[channel]) for c in colours) // 3 for channel in "rgb"
        )
        triangles.append((points, average))
    return triangles


def draw_tile(model_id: int, triangles: list[Triangle]) -> Image.Image:
    tile = Image.new("RGB", (TILE, TILE), (40, 40, 48))
    draw = ImageDraw.Draw(tile)
    if triangles:
        xs = [p[0] for points, _ in triangles for p in points]
        ys = [p[1] for points, _ in triangles for p in points]
        span = max(max(xs) - min(xs), max(ys) - min(ys), 1)
        scale = (TILE - 24) / span
        centre_x = (max(xs) + min(xs)) / 2
        centre_y = (max(ys) + min(ys)) / 2
        # Painter's algorithm: farthest first.
        for points, colour in sorted(triangles, key=depth, reverse=True):
            polygon = []
            for x, y, _ in points:
                screen_x = TILE / 2 + (x - centre_x) * scale
                screen_y = TILE / 2 - (y - centre_y) * scale
                polygon.append((screen_x, screen_y))
            draw.polygon(polygon, fill=colour, outline=(0, 0, 0))
    draw.text((4, 4), f"{model_id}  tris={len(triangles)}", fill=(255, 255, 0))
    return tile


def render_all() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for first in range(0, MODEL_COUNT, PER_SHEET):
        rows = math.ceil(PER_SHEET / COLUMNS)
        sheet = Image.new("RGB", (COLUMNS * TILE, rows * TILE))
        for n, model_id in enumerate(range(first, first + PER_SHEET)):
            tile = draw_tile(model_id, read_triangles(model_id))
            sheet.paste(tile, ((n % COLUMNS) * TILE, (n // COLUMNS) * TILE))
        path = OUT_DIR / f"models_{first:02d}.png"
        sheet.save(path)
        print(path)
    gdb.execute("kill")


try:
    import gdb
except ImportError:
    gdb = None

if gdb is None:
    sys.exit(launch())
else:
    from PIL import Image, ImageDraw

    render_all()
