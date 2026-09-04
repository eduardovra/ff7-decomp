#!/usr/bin/env python3

import glob
import os
import re
import sys
import yaml

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
    install_annotator(args)


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
