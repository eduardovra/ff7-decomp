"""Decode PSX VRAM into a PNG.

PCSX-Redux's /api/v1/screen/still captures the GUI's offscreen texture,
which stays black unless the window is actually rendering. VRAM is the
GPU's own output and does not depend on the GUI, so it is the reliable
source for automated capture.

VRAM is 1024x512 16-bit pixels. The display reads them as XBGR1555, or
as packed RGB888 when the GPU is in 24bpp mode -- which FF7 uses for FMV,
and not for battle. Decoding one as the other yields structured garbage
that reads convincingly as emulator corruption, so pick the right mode
before concluding anything from a capture.
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path
from urllib.request import urlopen

VRAM_WIDTH = 1024
VRAM_HEIGHT = 512


def fetch_vram(host: str) -> bytes:
    with urlopen(f"http://{host}/api/v1/gpu/vram/raw", timeout=30) as f:
        return f.read()


def decode_24bpp(
    vram: bytes,
    x: int,
    y: int,
    width: int,
    height: int,
) -> bytearray:
    """Expand a 24bpp VRAM rectangle, where pixels are packed RGB888."""
    rgb = bytearray(width * height * 3)
    out = 0
    for row in range(height):
        base = (y + row) * VRAM_WIDTH * 2 + x * 3
        for col in range(width):
            pixel = base + col * 3
            rgb[out:out + 3] = vram[pixel:pixel + 3]
            out += 3
    return rgb


def decode_15bpp(
    vram: bytes,
    x: int,
    y: int,
    width: int,
    height: int,
) -> bytearray:
    """Expand a 15bpp VRAM rectangle to packed RGB888 rows."""
    rgb = bytearray(width * height * 3)
    out = 0
    for row in range(height):
        base = ((y + row) * VRAM_WIDTH + x) * 2
        for col in range(width):
            pixel = vram[base + col * 2] | (vram[base + col * 2 + 1] << 8)
            red = (pixel & 0x1F) << 3
            green = ((pixel >> 5) & 0x1F) << 3
            blue = ((pixel >> 10) & 0x1F) << 3
            rgb[out] = red | (red >> 5)
            rgb[out + 1] = green | (green >> 5)
            rgb[out + 2] = blue | (blue >> 5)
            out += 3
    return rgb


def write_png(
    path: Path,
    rgb: bytes,
    width: int,
    height: int,
) -> None:
    """Minimal RGB888 PNG writer, so Pillow is not a dependency."""
    raw = bytearray()
    stride = width * 3
    for row in range(height):
        raw.append(0)
        raw += rgb[row * stride:(row + 1) * stride]

    def chunk(tag: bytes, payload: bytes) -> bytes:
        head = struct.pack(">I", len(payload)) + tag
        return head + payload + struct.pack(">I", zlib.crc32(tag + payload))

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", header)
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 6))
    png += chunk(b"IEND", b"")
    path.write_bytes(png)


def decode_screenshot(
    raw: bytes,
    width: int,
    height: int,
) -> bytearray:
    """Expand a packed XBGR1555 framebuffer dump to RGB888 rows."""
    rgb = bytearray(width * height * 3)
    out = 0
    for index in range(width * height):
        pixel = raw[index * 2] | (raw[index * 2 + 1] << 8)
        red = (pixel & 0x1F) << 3
        green = ((pixel >> 5) & 0x1F) << 3
        blue = ((pixel >> 10) & 0x1F) << 3
        rgb[out] = red | (red >> 5)
        rgb[out + 1] = green | (green >> 5)
        rgb[out + 2] = blue | (blue >> 5)
        out += 3
    return rgb


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("out", type=Path)
    parser.add_argument("--host", default="localhost:8080")
    parser.add_argument("--from-file", type=Path)
    parser.add_argument("-x", type=int, default=0)
    parser.add_argument("-y", type=int, default=0)
    parser.add_argument("--width", type=int, default=VRAM_WIDTH)
    parser.add_argument("--height", type=int, default=VRAM_HEIGHT)
    parser.add_argument(
        "--bpp",
        type=int,
        choices=(15, 24),
        default=15,
        help="15 for battle and field, 24 for FMV",
    )
    args = parser.parse_args(argv)

    if args.from_file is not None:
        vram = args.from_file.read_bytes()
    else:
        vram = fetch_vram(args.host)
    if args.screenshot:
        rgb = decode_screenshot(
            raw=vram,
            width=args.width,
            height=args.height,
        )
        write_png(
            path=args.out,
            rgb=bytes(rgb),
            width=args.width,
            height=args.height,
        )
        print(f"{args.out} {args.width}x{args.height}", file=sys.stderr)
        return 0
    if args.bpp == 24:
        decode = decode_24bpp
    else:
        decode = decode_15bpp
    rgb = decode(
        vram=vram,
        x=args.x,
        y=args.y,
        width=args.width,
        height=args.height,
    )
    write_png(
        path=args.out,
        rgb=bytes(rgb),
        width=args.width,
        height=args.height,
    )
    print(f"{args.out} {args.width}x{args.height}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
