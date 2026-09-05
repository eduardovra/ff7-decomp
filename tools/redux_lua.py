"""Run Lua in PCSX-Redux over its web API.

The Lua endpoint dispatches to a pre-registered handler and takes its
input from the query string, which Redux caps at roughly 256 bytes of
URL -- far too small for a useful script. `tools/redux_probe.lua`
therefore exposes an append buffer, and this module chunks into it.

Redux closes the connection after every response, so there is no
keep-alive to exploit here.
"""

from __future__ import annotations

import sys
from urllib.parse import quote
from urllib.request import urlopen

CHUNK_SIZE = 120
DEFAULT_HOST = "localhost:8080"


def _call(host: str, query: str) -> str:
    url = f"http://{host}/api/v1/lua/eval?{query}"
    with urlopen(url, timeout=30) as response:
        return response.read().decode(errors="replace")


def eval_lua(
    source: str,
    host: str = DEFAULT_HOST,
) -> str:
    """Run a Lua chunk and return whatever string it produced."""
    if len(source) <= CHUNK_SIZE:
        return _call(host, "code=" + quote(source, safe=""))
    _call(host, "reset=1")
    for start in range(0, len(source), CHUNK_SIZE):
        piece = source[start:start + CHUNK_SIZE]
        result = _call(host, "append=" + quote(piece, safe=""))
        if result.startswith("error"):
            return result
    return _call(host, "run=1")


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    host = DEFAULT_HOST
    if args and args[0].startswith("--host="):
        host = args.pop(0).split("=", 1)[1]
    if args:
        source = " ".join(args)
    else:
        source = sys.stdin.read()
    print(eval_lua(source=source, host=host))
    return 0


if __name__ == "__main__":
    sys.exit(main())
