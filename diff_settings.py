#!/usr/bin/env python3

import bisect
import glob
import os
import re
import sys
import yaml
from typing import Callable

sys.path.insert(0, "tools")
try:
    import regtrack_poc
except ImportError:  # the --annotate helper is optional
    regtrack_poc = None


def add_custom_arguments(parser):
    # Peek at argv: if the value after --overlay is a function name (not a real
    # overlay), register it as a flag so argparse doesn't consume the function name.
    # this allows aliases with --overlay burned into it to always work:
    # differ='.venv/bin/python3 tools/asm-differ/diff.py -mows --overlay'
    # allowed: differ main func_8002B958
    # allowed: differ func_8002B958

    overlay_as_flag = False
    if "--overlay" in sys.argv:
        idx = sys.argv.index("--overlay")
        next_val = sys.argv[idx + 1] if idx + 1 < len(sys.argv) else None
        build_path = load_config("us")["build_path"]
        if next_val is None or not os.path.isfile(f"{build_path}/{next_val}.map"):
            overlay_as_flag = True
    if overlay_as_flag:
        parser.add_argument(
            "--overlay",
            action="store_const",
            const=None,
            default=None,
            dest="overlay",
        )
    else:
        parser.add_argument(
            "--overlay",
            default=None,
            dest="overlay",
        )

    parser.add_argument(
        "--raw-relocs",
        action="store_true",
        dest="raw_relocs",
        help="keep objdump's relocation symbols instead of canonicalising them",
    )

    parser.add_argument(
        "--annotate",
        action="store_true",
        dest="annotate",
        help="annotate each instruction with the C variable a register holds",
    )


def load_config(version: str) -> dict:
    with open(f"config/{version}.yaml") as f:
        return yaml.safe_load(f)


def estimate_overlay_from_func_name(func_name: str, build_path: str) -> str | None:
    # match symbol definitions but not imports (imports have "= 0x..." after the name)
    matches = []
    pattern = re.compile(
        r"^\s+0x[0-9a-f]+\s+" + re.escape(func_name) + r"\s*$", re.MULTILINE
    )
    for map_path in glob.glob(f"{build_path}/*.map"):
        with open(map_path) as f:
            if pattern.search(f.read()):
                ovl = map_path.rsplit("/", 1)[-1].removesuffix(".map")
                matches.append(ovl)
    if len(matches) == 1:
        return matches[0]
    return None


def apply(config, args):
    cfg = load_config("us")
    build_path = cfg["build_path"]
    overlays = {ovl["name"]: ovl for ovl in cfg["overlays"]}

    overlay = args.overlay
    if overlay is None:
        func_name = next((a for a in sys.argv[1:] if not a.startswith("-")), None)
        if func_name:
            overlay = estimate_overlay_from_func_name(func_name, build_path)
        if overlay is None:
            overlay = "main"
    if overlay not in overlays:
        raise KeyError(
            f"unknown overlay '{overlay}'\nknown: {', '.join(sorted(overlays))}"
        )
    ovl_cfg = overlays[overlay]
    config["baseimg"] = ovl_cfg["disk_path"]
    config["myimg"] = f"{build_path}/{overlay}.elf"
    config["mapfile"] = f"{build_path}/{overlay}.map"
    config["source_directories"] = [f"{cfg['src_path']}/{ovl_cfg['base_path']}"]
    config["build_dir"] = build_path
    config["expected_dir"] = "expected"
    config["objdump_executable"] = "mipsel-linux-gnu-objdump"
    config["arch"] = "mipsel"
    config["makeflags"] = []
    install_reloc_normalizer(args, ovl_cfg.get("symbol_addrs_path", []))
    install_annotator(args)


# gcc relocates against the symbol it can see plus an addend, while splat's
# generated asm names whatever symbol sits at the final address. Both encode
# the same instruction, so canonicalise each operand to one spelling.

_RELOC_OPERAND = re.compile(
    r"%(hi|lo)\(([A-Za-z_.$][\w.$]*)((?:[+-]0x[0-9a-fA-F]+)?)\)"
)
_SYMBOL_DEF = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;(.*)$")
_SYMBOL_SIZE = re.compile(r"size:\s*0x([0-9A-Fa-f]+)")
_ADDRESS_IN_NAME = re.compile(r"(?:D|func|jtbl|jpt)_([0-9A-Fa-f]{8})$")

_symbol_paths: "list[str]" = []
_symbol_table: "SymbolTable | None" = None


class SymbolTable:
    """One overlay's symbols, plus the ranges of the sized ones.

    Scoped to a single overlay on purpose: they share load addresses, so a
    table built from every config/*.txt spells fire's functions with
    mabaria's names.
    """

    def __init__(self, paths: "list[str]") -> None:
        self.by_name: dict[str, int] = {}
        self.by_address: dict[int, str] = {}
        self.ranges: list[tuple[int, int, str]] = []
        for path in paths:
            self._read(path, self._add_symbol)
        # Only symbols.*.txt annotates sizes, and an overlay's list rarely
        # includes the one that owns the symbol it imports.
        for path in sorted(glob.glob("config/*.txt")):
            self._read(path, self._add_size)
        self.ranges.sort()
        self._range_starts = [start for start, _, _ in self.ranges]

    def _read(
        self,
        path: str,
        handler: "Callable[[re.Match[str]], None]",
    ) -> None:
        if not os.path.isfile(path):
            return
        with open(path) as f:
            for line in f:
                match = _SYMBOL_DEF.match(line)
                if match:
                    handler(match)

    def _add_symbol(self, match: "re.Match[str]") -> None:
        name, address_text, _ = match.groups()
        address = int(address_text, 16)
        self.by_name.setdefault(name, address)
        # A generated name carries no information the address lacks, so let a
        # hand-written name for the same address win.
        known = self.by_address.get(address)
        if known is None or _ADDRESS_IN_NAME.match(known):
            self.by_address[address] = name

    def _add_size(self, match: "re.Match[str]") -> None:
        name, address_text, tail = match.groups()
        size_match = _SYMBOL_SIZE.search(tail)
        if not size_match:
            return
        address = int(address_text, 16)
        # Adopt the size only for a symbol this overlay already agrees on,
        # so a colliding address in another overlay cannot leak in.
        if self.by_name.get(name) != address:
            return
        size = int(size_match.group(1), 16)
        self.ranges.append((address, address + size, name))

    def resolve(self, name: str) -> "int | None":
        address = self.by_name.get(name)
        if address is not None:
            return address
        match = _ADDRESS_IN_NAME.match(name)
        if match:
            return int(match.group(1), 16)
        return None

    def containing(self, address: int) -> "tuple[int, str] | None":
        index = bisect.bisect_right(self._range_starts, address) - 1
        if index < 0:
            return None
        start, end, name = self.ranges[index]
        if address >= end:
            return None
        return start, name

    def spell(self, address: int, keep_offset: bool) -> "str | None":
        name = self.by_address.get(address)
        if name is not None:
            return name
        found = self.containing(address)
        if found is None:
            return None
        start, name = found
        if not keep_offset:
            return name
        return f"{name}+{hex(address - start)}"


def _symbols() -> SymbolTable:
    global _symbol_table
    if _symbol_table is None:
        _symbol_table = SymbolTable(_symbol_paths)
    return _symbol_table


def _canonical_operand(match: "re.Match[str]") -> str:
    kind, name, addend = match.groups()
    symbols = _symbols()
    base = symbols.resolve(name)
    if base is None:
        return match.group(0)
    address = base
    if addend:
        address += int(addend, 16)
    # objdump loses the order of paired relocations, so asm-differ can only
    # recover an addend for %lo. Compare %hi by its symbol alone.
    spelling = symbols.spell(address, keep_offset=kind == "lo")
    if spelling is None:
        return match.group(0)
    return f"%{kind}({spelling})"


def _canonicalize_relocs(text: str) -> str:
    return _RELOC_OPERAND.sub(_canonical_operand, text)


def _rewrite_line(line) -> None:
    fields = ("original", "normalized_original", "scorable_line", "symbol")
    for name in fields:
        value = getattr(line, name, None)
        if value:
            setattr(line, name, _canonicalize_relocs(value))


def install_reloc_normalizer(args, symbol_paths: "list[str]") -> None:
    """Make both sides spell a relocated address the same way.

    asm-differ compares objdump text, so `D_801518F6` and `D_801518E4+0x12`
    read as a difference even though they assemble to identical bytes.
    """
    if getattr(args, "raw_relocs", False):
        return

    module = sys.modules.get("__main__")
    if module is None or not hasattr(module, "process"):
        return

    global _symbol_paths, _symbol_table
    _symbol_paths = symbol_paths
    _symbol_table = None

    original_process = module.process

    def process_with_canonical_relocs(dump: str, config) -> list:
        lines = original_process(dump, config)
        try:
            for line in lines:
                _rewrite_line(line)
        except Exception:
            pass
        return lines

    module.process = process_with_canonical_relocs


def _find_c_info() -> "regtrack_poc.CInfo | None":
    """Locate the C definition of whichever positional names a function."""
    for arg in sys.argv[1:]:
        if arg.startswith("-"):
            continue
        info = regtrack_poc.read_c(arg)
        if info:
            return info
    return None


def _annotate(
    lines: list,
    info: "regtrack_poc.CInfo | None",
    dim: str = "",
    reset: str = "",
) -> None:
    """Fill Line.comment with what each register holds at that point."""
    state = {}
    if info:
        # o32: the first four arguments arrive in a0-a3, by name.
        for reg, ident in zip(regtrack_poc.PARAM_REGS, info.params):
            state[reg] = regtrack_poc.Val(
                text=ident, base=ident, coef=1, cname=ident
            )
    targets = {x.branch_target for x in lines if x.branch_target is not None}
    # asm-differ renders "addr:" then four spaces, then the instruction
    # tab-expanded on its own. The address is not padded, so its length
    # varies; measure the whole thing to align the annotations.
    widths = {}
    for line in lines:
        if line.line_num is None:
            prefix = ""
        else:
            prefix = f"{line.line_num:x}:"
        body = line.original.expandtabs(8)
        widths[id(line)] = len(prefix) + 4 + len(body)
    column = max(widths.values(), default=0)
    for line in lines:
        if line.line_num in targets:
            # A branch target merges paths we did not track; start clean.
            state.clear()
        parts = line.original.split("\t", 1)
        if len(parts) > 1 and parts[1].strip():
            args = [a.strip() for a in parts[1].split(",")]
        else:
            args = []
        try:
            note = regtrack_poc.step(state, line.mnemonic, args, info)
        except Exception:
            note = ""
        if info:
            regtrack_poc.name_registers(state, info)
        if note:
            pad = " " * max(0, column - widths[id(line)])
            line.comment = f"{pad}{dim}; {note}{reset}"


def install_annotator(args) -> None:
    """Wrap asm-differ's process() so every parsed line carries a note.

    asm-differ is a submodule, so this hooks the running module rather
    than patching it. Any failure leaves the diff untouched.
    """
    if not getattr(args, "annotate", False) or regtrack_poc is None:
        return
    # Line.comment only renders under --source; turn it on for the user.
    args.show_source = True

    module = sys.modules.get("__main__")
    if module is None or not hasattr(module, "process"):
        return

    original_process = module.process
    info = _find_c_info()

    # Match the dim styling asm-differ gives source lines (SOURCE_OTHER),
    # but only when it is actually emitting colour.
    dim = reset = ""
    if getattr(args, "format", "color") == "color":
        style = getattr(module, "Style", None)
        dim = getattr(style, "DIM", "\x1b[2m")
        reset = getattr(style, "RESET_ALL", "\x1b[0m")

    def process_with_notes(dump: str, config) -> list:
        lines = original_process(dump, config)
        try:
            _annotate(lines, info, dim, reset)
        except Exception:
            pass
        return lines

    module.process = process_with_notes
