#!/usr/bin/env python3
"""Decode a battle model out of an overlay and draw it as an SVG.

The format is the one documented in docs/how-a-spell-is-drawn.md section 11:
a vertex table preceded by its size in bytes, then four primitive lists.
Only the POLY_G3 list is drawn; brizad is the only caller so far and its
other three lists are empty.
"""

import argparse
import math
import struct
from typing import BinaryIO, NamedTuple

VERTEX_SIZE = 8
LIST_STRIDES = {"FT3": 0x10, "FT4": 0x14, "G3": 0x14, "G4": 0x18}


class Triangle(NamedTuple):
    indices: tuple[int, int, int]
    colours: tuple[int, int, int]  # 0xBBGGRR as stored, code byte stripped


class Model(NamedTuple):
    vertices: list[tuple[int, int, int]]
    triangles: list[Triangle]


def read_model(data: bytes, offset: int) -> Model:
    table_size = struct.unpack_from("<i", data, offset)[0]
    vertices = [
        struct.unpack_from("<4h", data, offset + 4 + i * VERTEX_SIZE)[:3]
        for i in range(table_size // VERTEX_SIZE)
    ]

    pos = offset + 4 + table_size
    triangles: list[Triangle] = []
    for name, stride in LIST_STRIDES.items():
        count = struct.unpack_from("<I", data, pos)[0] & 0xFFFF
        pos += 4
        if name == "G3":
            for i in range(count):
                record = pos + i * stride
                offsets = struct.unpack_from("<3h", data, record)
                words = struct.unpack_from("<3I", data, record + 8)
                triangles.append(
                    Triangle(
                        indices=tuple(o // VERTEX_SIZE for o in offsets),
                        colours=tuple(w & 0xFFFFFF for w in words),
                    )
                )
        pos += count * stride

    return Model(vertices=vertices, triangles=triangles)


def components(model: Model) -> list[list[int]]:
    """Vertex indices of each connected piece, in first-seen order."""
    neighbours: dict[int, set[int]] = {}
    for triangle in model.triangles:
        a, b, c = triangle.indices
        for x, y in ((a, b), (b, c), (c, a)):
            neighbours.setdefault(x, set()).add(y)
            neighbours.setdefault(y, set()).add(x)

    seen: set[int] = set()
    pieces = []
    for start in sorted(neighbours):
        if start in seen:
            continue
        stack, piece = [start], []
        seen.add(start)
        while stack:
            node = stack.pop()
            piece.append(node)
            for other in neighbours[node]:
                if other not in seen:
                    seen.add(other)
                    stack.append(other)
        pieces.append(sorted(piece))
    return pieces


def rotate(
    point: tuple[int, int, int],
    yaw: float,
    pitch: float,
) -> tuple[float, float, float]:
    x, y, z = point
    cy, sy = math.cos(yaw), math.sin(yaw)
    x, z = x * cy + z * sy, -x * sy + z * cy
    cp, sp = math.cos(pitch), math.sin(pitch)
    y, z = y * cp - z * sp, y * sp + z * cp
    return x, y, z


def to_rgb(colour: int) -> str:
    # Stored little-endian as r,g,b with the GPU command byte on top, so the
    # word reads 0x00BBGGRR.
    return "#%02X%02X%02X" % (
        colour & 0xFF,
        (colour >> 8) & 0xFF,
        (colour >> 16) & 0xFF,
    )


def blend(colours: tuple[int, int, int]) -> str:
    channels = []
    for shift in (0, 8, 16):
        total = sum((c >> shift) & 0xFF for c in colours)
        channels.append(total // 3)
    return "#%02X%02X%02X" % tuple(channels)


def project(
    model: Model,
    yaw: float,
    pitch: float,
    scale: float,
    origin: tuple[float, float],
) -> list[tuple[float, float, float]]:
    points = []
    for vertex in model.vertices:
        x, y, z = rotate(point=vertex, yaw=yaw, pitch=pitch)
        # PSX +Y points down the screen, so negate for SVG.
        points.append((origin[0] + x * scale, origin[1] - y * scale, z))
    return points


def draw_faces(
    model: Model,
    points: list[tuple[float, float, float]],
    stroke: str,
) -> list[str]:
    depths = []
    for index, triangle in enumerate(model.triangles):
        depth = sum(points[i][2] for i in triangle.indices) / 3
        depths.append((depth, index))
    depths.sort(reverse=True)

    out = []
    for _, index in depths:
        triangle = model.triangles[index]
        corners = " ".join(
            "%.1f,%.1f" % (points[i][0], points[i][1])
            for i in triangle.indices
        )
        out.append(
            f'<polygon points="{corners}" fill="{blend(triangle.colours)}" '
            f'stroke="{stroke}" stroke-width="0.6" stroke-linejoin="round"/>'
        )
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary")
    parser.add_argument("output")
    parser.add_argument("--offset", type=lambda s: int(s, 0), default=0x3F0)
    args = parser.parse_args()

    data = open(args.binary, "rb").read()
    model = read_model(data=data, offset=args.offset)

    width, height = 760, 380
    stroke = "#6b7a8f"
    body = []

    # Left panel: the whole model, three-quarter view.
    points = project(
        model=model,
        yaw=math.radians(28),
        pitch=math.radians(18),
        scale=0.29,
        origin=(190, 190),
    )
    body += draw_faces(model=model, points=points, stroke=stroke)

    # Right panel: one spike, with its vertex indices. The pieces are
    # vertex-disjoint, so pick the first and keep every triangle in it.
    piece = set(components(model)[0])
    spike = Model(
        vertices=model.vertices,
        triangles=[t for t in model.triangles if piece >= set(t.indices)],
    )
    centre = (570.0, 215.0)
    points = project(
        model=spike,
        yaw=math.radians(28),
        pitch=math.radians(34),
        scale=0.30,
        origin=centre,
    )
    body += draw_faces(model=spike, points=points, stroke=stroke)
    drawn = sorted(piece)
    mid_x = sum(points[i][0] for i in drawn) / len(drawn)
    mid_y = sum(points[i][1] for i in drawn) / len(drawn)
    for index in drawn:
        x, y, _ = points[index]
        # Push each label radially outwards so the pentagon ring stays legible.
        dx, dy = x - mid_x, y - mid_y
        length = math.hypot(dx, dy)
        if length < 1.0:
            dx, dy, length = 0.0, 1.0, 1.0
        lx = x + dx / length * 14
        ly = y + dy / length * 14
        body.append(
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="2.6" fill="#e8402a"/>'
        )
        body.append(
            f'<text x="{lx:.1f}" y="{ly + 4:.1f}" font-size="11" '
            f'text-anchor="middle" font-family="monospace" '
            f'fill="#e8402a">{index}</text>'
        )

    labels = [
        (190, 358, "the whole model: 84 verts, 120 POLY_G3"),
        (570, 358, "one of 12 spikes: 7 verts, 10 tris"),
    ]
    for x, y, text in labels:
        body.append(
            f'<text x="{x}" y="{y}" font-size="12" text-anchor="middle" '
            f'font-family="monospace" fill="{stroke}">{text}</text>'
        )

    svg = (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="0 0 {width} {height}">\n'
        + "\n".join(body)
        + "\n</svg>\n"
    )
    open(args.output, "w").write(svg)


if __name__ == "__main__":
    main()
