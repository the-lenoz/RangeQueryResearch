#!/usr/bin/env python3
"""Exercise each CLI structure and command type."""

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

    for option in ("-h", "--help"):
        result = subprocess.run(
            [str(args.binary), option], input="invalid stdin", text=True,
            capture_output=True, check=False, timeout=10,
        )
        if result.returncode != 0 or result.stderr:
            raise AssertionError(f"help failed: {result!r}")
        for fragment in ("[OPTIONS]", "--structure", "vector", "set", "avl", "treap"):
            if fragment not in result.stdout:
                raise AssertionError(f"help is missing {fragment!r}: {result.stdout!r}")

    for arguments, fragment in (
        (["--structure", "unknown"], "--structure"),
        (["--structure"], "--structure"),
        (["--unknown"], "--unknown"),
        (["--structure", "avl", "--structure", "treap"], "--structure"),
    ):
        result = subprocess.run(
            [str(args.binary), *arguments], input="invalid stdin", text=True,
            capture_output=True, check=False, timeout=10,
        )
        if result.returncode != 2 or result.stdout or fragment not in result.stderr:
            raise AssertionError(f"invalid CLI {arguments!r}: {result!r}")

    for arguments in ([], ["--structure=avl"]):
        result = subprocess.run(
            [str(args.binary), *arguments], input=SUCCESSFUL_COMMANDS, text=True,
            capture_output=True, check=False, timeout=10,
        )
        expect_result(f"CLI {arguments!r}", result, 0, SUCCESSFUL_OUTPUT, "")


if __name__ == "__main__":
    main()
