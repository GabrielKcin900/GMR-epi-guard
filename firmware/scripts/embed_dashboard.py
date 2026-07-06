#!/usr/bin/env python3
"""Regenera src/dashboard_embed.c a partir de dashboard/index.html e app.js."""

from __future__ import annotations

import pathlib


def c_array(name: str, data: bytes) -> str:
    lines = [f"const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("    " + ", ".join(str(b) for b in chunk) + ",")
    lines.append("};")
    lines.append(f"const size_t {name}_len = sizeof({name});")
    return "\n".join(lines)


def main() -> None:
    root = pathlib.Path(__file__).resolve().parents[1]
    html = (root / "dashboard/index.html").read_bytes()
    js = (root / "dashboard/app.js").read_bytes()
    out = root / "firmware/src/dashboard_embed.c"
    content = (
        "/*\n * SPDX-License-Identifier: Apache-2.0\n"
        " * Gerado por firmware/scripts/embed_dashboard.py\n */\n\n"
        '#include "dashboard_embed.h"\n\n'
        "#include <stddef.h>\n#include <stdint.h>\n\n"
        + c_array("epi_dashboard_index_html", html)
        + "\n\n"
        + c_array("epi_dashboard_app_js", js)
        + "\n"
    )
    out.write_text(content, newline="\n")
    print(f"Wrote {out} ({len(html)} + {len(js)} bytes)")


if __name__ == "__main__":
    main()
