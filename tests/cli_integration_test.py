#!/usr/bin/env python3
"""Проверяет CLI для каждой реализации и каждого вида команды."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


SUCCESSFUL_COMMANDS = """\
k 0
k -2
k 2
k -2
k -2147483648
k 2147483647
q -2147483648 2147483647
q -2 2
q 2 2
q 2 -2
m 1
m 3
m 5
n -2147483648
n 0
n 2147483647
"""

SUCCESSFUL_OUTPUT = """\
4
2
0
0
-2147483648
0
2147483647
0
2
4
"""


def execute(binary: Path, structure: str, payload: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(binary), "--structure", structure],
        input=payload,
        text=True,
        capture_output=True,
        check=False,
    )


def expect_result(
    label: str,
    result: subprocess.CompletedProcess[str],
    return_code: int,
    stdout: str,
    stderr: str,
) -> None:
    if (result.returncode, result.stdout, result.stderr) != (return_code, stdout, stderr):
        raise AssertionError(
            f"{label}: expected {(return_code, stdout, stderr)!r}, "
            f"got {(result.returncode, result.stdout, result.stderr)!r}"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()

    for structure in ("vector", "set", "avl", "treap"):
        expect_result(
            f"{structure}: valid commands",
            execute(args.binary, structure, SUCCESSFUL_COMMANDS),
            0,
            SUCCESSFUL_OUTPUT,
            "",
        )
        for payload, message in (
            ("k value\n", "invalid k command: expected an integer\n"),
            ("q 1\n", "invalid q command: expected two integers\n"),
            ("m 0\n", "invalid m command: expected a positive index\n"),
            ("m 1\n", "m index is outside the set\n"),
            ("n value\n", "invalid n command: expected an integer\n"),
            ("x\n", "unknown command: x\n"),
        ):
            expect_result(
                f"{structure}: {payload.strip()}",
                execute(args.binary, structure, payload),
                2,
                "",
                message,
            )

    help_result = subprocess.run(
        [str(args.binary), "--help"], text=True, capture_output=True, check=False
    )
    expect_result(
        "help", help_result, 0, "Usage: rangequery --structure <vector|set|avl|treap>\n", ""
    )

    invalid_structure = subprocess.run(
        [str(args.binary), "--structure", "unknown"], text=True, capture_output=True, check=False
    )
    expect_result(
        "invalid structure",
        invalid_structure,
        2,
        "",
        "unknown structure; expected vector, set, avl, or treap\n",
    )

    missing_structure = subprocess.run(
        [str(args.binary), "--structure"], text=True, capture_output=True, check=False
    )
    expect_result(
        "missing structure",
        missing_structure,
        2,
        "",
        "--structure requires vector, set, avl, or treap\n",
    )


if __name__ == "__main__":
    main()
