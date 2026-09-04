"""Annotate split assembly with what each register holds.

gcc 2.6.3 emits no DWARF variable locations (the .debug_info in our
objects is produced by GNU as, not cc1), so a register/variable map
cannot be read out of the build. This infers one instead, by walking
the instructions and tracking the last definition of every register.

Values are tracked as a linear form `base * coef + const`, which folds
gcc's shift/add/sub strength-reduction chains back into the multiply
they came from -- usually an array stride.

    tools/regtrack_poc.py func_801B066C
    tools/regtrack_poc.py asm/us/magic/nonmatchings/thunder/foo.s
"""

import argparse
import os
import re
import sys
from dataclasses import dataclass
from typing import Optional

ASM_ROOT = "asm"

# /* 66C 801B066C 1580023C */  lui  $v0, %hi(D_8015169C)
INSN_RE = re.compile(
    r"^\s*/\*\s*(?P<off>[0-9A-Fa-f]+)\s+(?P<vram>[0-9A-Fa-f]+)"
    r"(?:\s+[0-9A-Fa-f]{8})?\s*\*/\s+(?P<op>[a-z0-9.]+)\s*(?P<args>.*?)$"
)
LABEL_RE = re.compile(r"^\s*(?P<label>\.L[0-9A-Fa-f]+|glabel\s+\S+):?\s*$")
MEM_RE = re.compile(r"^(?P<disp>.*)\((?P<base>\$?\w+)\)$")
HILO_RE = re.compile(r"^%(?P<part>hi|lo)\((?P<sym>[^)]+)\)$")

CALLER_SAVED = (
    ["v0", "v1", "at"]
    + [f"a{i}" for i in range(4)]
    + [f"t{i}" for i in range(10)]
)

LOAD_WIDTH = {
    "lb": "s8",
    "lbu": "u8",
    "lh": "s16",
    "lhu": "u16",
    "lw": "s32",
}
STORE_WIDTH = {"sb": "s8", "sh": "s16", "sw": "s32"}
BITWISE = {"xori": "^", "andi": "&", "ori": "|"}
COMPARE = {"slti": "<", "sltiu": "<", "slt": "<", "sltu": "<"}


@dataclass
class Val:
    """What a register holds: an expression, plus a linear form when known.

    kind is one of hi (upper half of a symbol address), addr (a symbol
    address), idx (a subscripted symbol address) or val (anything else).
    """

    text: str
    kind: str = "val"
    sym: str = ""
    base: str = ""
    coef: int = 0
    const: int = 0
    cname: str = ""  # matching C identifier, when one was found
    ctype: str = ""  # its struct type, for resolving field offsets

    @property
    def linear(self) -> bool:
        return bool(self.base)


def parse_int(text: str) -> Optional[int]:
    try:
        return int(text, 0)
    except ValueError:
        return None


def reg_name(token: str) -> str:
    return token.lstrip("$")


def render(base: str, coef: int, const: int) -> str:
    """Render a linear form the way it would read in C."""
    if coef == 0:
        return hex(const)
    if coef == 1:
        body = base
    else:
        body = f"{base} * {hex(coef)}"
    if const == 0:
        return body
    if const < 0:
        return f"{body} - {hex(-const)}"
    return f"{body} + {hex(const)}"


def linear_of(state: dict, token: str) -> Optional[Val]:
    """Linear form of an operand, treating unknown registers as roots."""
    name = reg_name(token)
    if name == "zero":
        return Val(text="0", base="", coef=0, const=0)
    held = state.get(name)
    if held is None:
        return Val(text=name, base=name, coef=1, const=0)
    if held.linear:
        return held
    if held.kind == "hi":
        return None
    # An opaque value (a call result, a load) is still a valid root to
    # scale from -- that is how gcc indexes by a returned slot number.
    return Val(text=held.text, base=held.text, coef=1)


def get(state: dict, token: str) -> Val:
    name = reg_name(token)
    if name == "zero":
        return Val(text="0", coef=0, const=0)
    return state.get(name, Val(text=name, base=name, coef=1))


def show(state: dict, token: str) -> str:
    number = parse_int(token)
    if number is not None:
        return hex(number)
    return get(state, token).text


def make_linear(base: str, coef: int, const: int) -> Val:
    return Val(
        text=render(base, coef, const),
        base=base,
        coef=coef,
        const=const,
    )


def deref(val: Val, disp: int, info: Optional["CInfo"] = None) -> str:
    """Render a load/store target given the base register's value."""
    if val.cname and val.kind in ("addr", "idx"):
        field = info.field(val.ctype, disp) if info else ""
        if field:
            return f"{val.cname}->{field}"
        return f"{val.cname}->{hex(disp)}"
    if val.kind in ("addr", "idx"):
        target = val.text.lstrip("&")
        if disp == 0:
            return target
        return f"{target}.{hex(disp)}"
    if disp == 0:
        return f"*({val.text})"
    return f"*({val.text} + {hex(disp)})"


def split_mem(arg: str) -> tuple:
    """Split `disp(base)` into its displacement and base register."""
    match = MEM_RE.match(arg.strip())
    if not match:
        return arg.strip(), ""
    return match.group("disp").strip(), reg_name(match.group("base"))


def fold_address(state: dict, base: str, disp: str) -> Optional[Val]:
    """Combine a %lo against a register already holding the %hi."""
    hilo = HILO_RE.match(disp)
    if not hilo or hilo.group("part") != "lo":
        return None
    held = state.get(base)
    if not held or held.kind != "hi" or held.sym != hilo.group("sym"):
        return None
    return Val(text=f"&{held.sym}", kind="addr", sym=held.sym)


def do_load(state: dict, op: str, args: list, info=None) -> str:
    dest = reg_name(args[0])
    disp, base = split_mem(args[1])
    width = LOAD_WIDTH[op]
    folded = fold_address(state, base, disp)
    if folded:
        state[dest] = Val(text=folded.sym, base=folded.sym, coef=1)
        return f"{dest} = {folded.sym}    ({width})"
    held = get(state, base)
    text = deref(val=held, disp=parse_int(disp) or 0, info=info)
    state[dest] = Val(text=text, base=text, coef=1)
    return f"{dest} = {text}    ({width})"


def do_store(state: dict, op: str, args: list, info=None) -> str:
    src = show(state, args[0])
    disp, base = split_mem(args[1])
    width = STORE_WIDTH[op]
    folded = fold_address(state, base, disp)
    if folded:
        return f"{folded.sym} = {src}    ({width})"
    held = get(state, base)
    text = deref(val=held, disp=parse_int(disp) or 0, info=info)
    return f"{text} = {src}    ({width})"


def combine(state: dict, args: list, sign: int) -> Optional[str]:
    """addu/subu of two linear forms sharing a base folds into one."""
    dest = reg_name(args[0])
    left = linear_of(state, args[1])
    right = linear_of(state, args[2])
    if left is None or right is None:
        return None
    if left.coef and right.coef and left.base != right.base:
        return None
    base = left.base or right.base
    coef = left.coef + sign * right.coef
    const = left.const + sign * right.const
    if not base:
        state[dest] = Val(text=hex(const), coef=0, const=const)
        return f"{dest} = {hex(const)}"
    state[dest] = make_linear(base, coef, const)
    return f"{dest} = {state[dest].text}"


def do_addu(state: dict, args: list) -> str:
    dest = reg_name(args[0])
    left = get(state, args[1])
    right = get(state, args[2])
    # base + index*stride is a subscript; render it as one.
    for addr, scaled in ((left, right), (right, left)):
        if addr.kind != "addr" or not scaled.linear or scaled.coef < 2:
            continue
        text = f"&{addr.sym}[{scaled.base}]"
        state[dest] = Val(text=text, kind="idx", sym=addr.sym)
        return f"{dest} = {text}    (stride {hex(scaled.coef)})"
    # a register plus $zero is a move, however it is spelled.
    if reg_name(args[2]) == "zero":
        state[dest] = left
        return f"{dest} = {left.text}"
    if reg_name(args[1]) == "zero":
        state[dest] = right
        return f"{dest} = {right.text}"
    folded = combine(state, args, sign=1)
    if folded:
        return folded
    text = f"{left.text} + {right.text}"
    state[dest] = Val(text=text)
    return f"{dest} = {text}"


def do_shift(state: dict, op: str, args: list) -> str:
    dest = reg_name(args[0])
    amount = parse_int(args[2]) or 0
    operand = linear_of(state, args[1])
    if op == "sll" and operand is not None and operand.base:
        state[dest] = make_linear(
            operand.base,
            operand.coef << amount,
            operand.const << amount,
        )
        return f"{dest} = {state[dest].text}"
    symbol = ">>" if op in ("srl", "sra") else "<<"
    text = f"{show(state, args[1])} {symbol} {amount}"
    state[dest] = Val(text=text)
    return f"{dest} = {text}"


def step(state: dict, op: str, args: list, info=None) -> str:
    """Update the register map for one instruction, return a note."""
    if op == "lui" and len(args) == 2:
        dest = reg_name(args[0])
        hilo = HILO_RE.match(args[1])
        if hilo:
            sym = hilo.group("sym")
            state[dest] = Val(text=f"&{sym}(hi)", kind="hi", sym=sym)
            return f"{dest} = HI({sym})"
        state[dest] = Val(text=f"{args[1]} << 16")
        return f"{dest} = {args[1]} << 16"

    if op == "addiu" and len(args) == 3:
        dest = reg_name(args[0])
        base = reg_name(args[1])
        folded = fold_address(state, base, args[2])
        if folded:
            state[dest] = folded
            return f"{dest} = {folded.text}"
        amount = parse_int(args[2])
        operand = linear_of(state, args[1])
        if amount is not None and operand is not None:
            state[dest] = make_linear(
                operand.base,
                operand.coef,
                operand.const + amount,
            )
            return f"{dest} = {state[dest].text}"
        text = f"{show(state, args[1])} + {args[2]}"
        state[dest] = Val(text=text)
        return f"{dest} = {text}"

    if op in ("addu", "add") and len(args) == 3:
        return do_addu(state, args)
    if op in ("subu", "sub") and len(args) == 3:
        folded = combine(state, args, sign=-1)
        if folded:
            return folded
        dest = reg_name(args[0])
        text = f"{show(state, args[1])} - {show(state, args[2])}"
        state[dest] = Val(text=text)
        return f"{dest} = {text}"

    if op in ("negu", "neg") and len(args) == 2:
        dest = reg_name(args[0])
        before = show(state, args[1])
        operand = linear_of(state, args[1])
        if operand is not None and operand.base:
            state[dest] = make_linear(
                operand.base, -operand.coef, -operand.const
            )
            return f"{dest} = -({before})"
        state[dest] = Val(text=f"-({before})")
        return f"{dest} = -({before})"

    if op in LOAD_WIDTH and len(args) == 2:
        return do_load(state, op, args, info)
    if op in STORE_WIDTH and len(args) == 2:
        return do_store(state, op, args, info)
    if op in ("sll", "srl", "sra") and len(args) == 3:
        return do_shift(state, op, args)

    if op == "move" and len(args) == 2:
        dest = reg_name(args[0])
        state[dest] = get(state, args[1])
        return f"{dest} = {state[dest].text}"

    if op in BITWISE and len(args) == 3:
        dest = reg_name(args[0])
        if reg_name(args[1]) == "zero" and op == "ori":
            amount = parse_int(args[2]) or 0
            state[dest] = Val(text=hex(amount), coef=0, const=amount)
            return f"{dest} = {hex(amount)}"
        text = f"{show(state, args[1])} {BITWISE[op]} {args[2]}"
        state[dest] = Val(text=text)
        return f"{dest} = {text}"

    if op in COMPARE and len(args) == 3:
        dest = reg_name(args[0])
        text = f"{show(state, args[1])} {COMPARE[op]} {show(state, args[2])}"
        state[dest] = Val(text=text)
        return f"{dest} = {text}"

    if op in ("jal", "jalr"):
        target = args[0] if args else "?"
        for reg in CALLER_SAVED:
            state.pop(reg, None)
        state["v0"] = Val(text=f"{target}(...)")
        return f"call {target}    (v0 = result, a0-a3/t* clobbered)"

    if op in ("beqz", "bnez", "blez", "bgtz", "bltz") and len(args) == 2:
        cond = show(state, args[0])
        shape = {
            "beqz": f"!({cond})",
            "bnez": f"({cond})",
            "blez": f"({cond}) <= 0",
            "bgtz": f"({cond}) > 0",
            "bltz": f"({cond}) < 0",
        }
        return f"if {shape[op]} -> {args[1]}"

    if op in ("beq", "bne") and len(args) == 3:
        symbol = "==" if op == "beq" else "!="
        left = show(state, args[0])
        right = show(state, args[1])
        return f"if ({left} {symbol} {right}) -> {args[2]}"

    if op == "jr":
        return "return"
    return ""


SRC_ROOT = "src"
PARAM_REGS = ["a0", "a1", "a2", "a3"]
DECL_RE = re.compile(
    r"^\s*([A-Za-z_]\w*)[\w\s\*]*?[\s\*](\w+)\s*=\s*([^;]+);"
)
STRUCT_RE = re.compile(
    r"typedef\s+struct\s*\{(.*?)\}\s*(\w+)\s*;", re.DOTALL
)
FIELD_RE = re.compile(
    r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*[\w\s\*]*?[\s\*](\w+)\s*"
    r"(?:\[[^\]]*\])?\s*;"
)


@dataclass
class CInfo:
    params: list
    locals: dict  # normalized initialiser -> (identifier, type)
    structs: dict  # type name -> {offset: field name}

    def field(self, ctype: str, offset: int) -> str:
        return self.structs.get(ctype, {}).get(offset, "")


def squash(text: str) -> str:
    return re.sub(r"\s+", "", text)


def read_c(name: str) -> Optional[CInfo]:
    """Find `name`'s definition in src/ and pull out its identifiers."""
    pattern = re.compile(rf"^[\w \*]*\b{re.escape(name)}\s*\(([^)]*)\)\s*\{{",
                         re.MULTILINE)
    for root, _dirs, files in os.walk(SRC_ROOT):
        for fname in files:
            if not fname.endswith(".c"):
                continue
            path = os.path.join(root, fname)
            text = open(path, encoding="utf-8", errors="replace").read()
            match = pattern.search(text)
            if not match:
                continue
            params = []
            for piece in match.group(1).split(","):
                piece = piece.strip()
                if not piece or piece == "void":
                    continue
                ident = re.findall(r"\w+", piece)
                if ident:
                    params.append(ident[-1])
            body, depth = [], 0
            for line in text[match.end() - 1:].splitlines():
                body.append(line)
                depth += line.count("{") - line.count("}")
                if depth == 0 and len(body) > 1:
                    break
            locals_ = {}
            for line in body:
                decl = DECL_RE.match(line)
                if decl:
                    key = squash(decl.group(3))
                    locals_[key] = (decl.group(2), decl.group(1))
            structs = {}
            for block, tname in STRUCT_RE.findall(text):
                fields = {}
                for offset, fname in FIELD_RE.findall(block):
                    fields[int(offset, 16)] = fname
                structs[tname] = fields
            return CInfo(params=params, locals=locals_, structs=structs)
    return None


def name_registers(state: dict, info: CInfo) -> None:
    """Attach C identifiers to whatever register currently holds them."""
    for val in state.values():
        if val.cname:
            continue
        found = info.locals.get(squash(val.text))
        if found:
            val.cname, val.ctype = found
            val.text = val.cname


@dataclass
class Line:
    addr: str = ""
    op: str = ""
    args: str = ""
    note: str = ""
    label: str = ""


def parse(path: str) -> list:
    lines = []
    for raw in open(path, encoding="utf-8", errors="replace"):
        label = LABEL_RE.match(raw)
        if label:
            lines.append(Line(label=label.group("label")))
            continue
        match = INSN_RE.match(raw)
        if not match:
            continue
        lines.append(
            Line(
                addr=match.group("vram"),
                op=match.group("op"),
                args=match.group("args").strip(),
            )
        )
    return lines


def annotate(lines: list, info: Optional[CInfo] = None) -> None:
    state: dict = {}
    if info:
        # o32: the first four arguments arrive in a0-a3, by name.
        for reg, ident in zip(PARAM_REGS, info.params):
            state[reg] = Val(text=ident, base=ident, coef=1, cname=ident)
    for line in lines:
        if line.label:
            # A branch target merges paths we did not track; start clean.
            if line.label.startswith(".L"):
                state.clear()
            continue
        if line.args:
            args = [a.strip() for a in line.args.split(",")]
        else:
            args = []
        try:
            line.note = step(state, line.op, args, info)
        except (IndexError, ValueError, KeyError):
            line.note = ""
            continue
        if info:
            name_registers(state, info)


def find_asm(name: str) -> Optional[str]:
    if os.path.isfile(name):
        return name
    for root, _dirs, files in os.walk(ASM_ROOT):
        if f"{name}.s" in files:
            return os.path.join(root, f"{name}.s")
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", help="function name or path to a .s file")
    parser.add_argument(
        "--no-source",
        action="store_true",
        help="do not look for the function's C definition",
    )
    opts = parser.parse_args()

    path = find_asm(opts.target)
    if not path:
        print(f"no assembly found for {opts.target}", file=sys.stderr)
        return 1
    info = None
    if not opts.no_source:
        info = read_c(os.path.basename(opts.target).removesuffix(".s"))
    lines = parse(path)
    if not lines:
        print(f"no instructions parsed from {path}", file=sys.stderr)
        return 1
    annotate(lines, info)

    bodies = {}
    for line in lines:
        if not line.label:
            bodies[id(line)] = f"  {line.addr}  {line.op:<8} {line.args}"
    width = max((len(v) for v in bodies.values()), default=40) + 1

    print(f"# {path}")
    if info:
        joined = ", ".join(info.params) or "none"
        print(f"# C parameters: {joined}")
    for line in lines:
        if line.label:
            if line.label.startswith(".L"):
                print(f"{line.label}:  <- branch target, tracking reset")
            else:
                print(f"{line.label}:")
            continue
        body = bodies[id(line)]
        if line.note:
            print(f"{body:<{width}} ; {line.note}")
        else:
            print(body)
    return 0


if __name__ == "__main__":
    sys.exit(main())
