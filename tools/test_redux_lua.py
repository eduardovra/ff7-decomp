import sys
from pathlib import Path
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parent))

import redux_lua as rl


def test_chunks_fit_the_query_cap_when_encoded() -> None:
    # Lua that divides instead of shifting is full of '%', which triples
    # under percent-encoding. Splitting on raw length overshot the cap
    # and Redux answered 400.
    source = "x = a % b % c % d\n" * 40
    for piece in rl._chunks(source):
        assert len(quote(piece, safe="")) <= rl.MAX_QUERY


def test_chunks_reassemble_to_the_original() -> None:
    source = "".join(chr(c) for c in range(32, 127)) * 5
    assert "".join(rl._chunks(source)) == source


def test_chunks_of_an_empty_source_are_empty() -> None:
    assert list(rl._chunks("")) == []
