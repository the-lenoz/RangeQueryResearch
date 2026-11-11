#!/usr/bin/env python3
"""Short AAA scenarios for the command-line application."""

from __future__ import annotations

import argparse
import subprocess
import sys
import unittest
from pathlib import Path


def execute(arguments: list[str], payload: str = "") -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        arguments, input=payload, text=True, capture_output=True, check=False, timeout=10
    )


class CommandScenarios:
    structure: str

    def test_command_stream(self) -> None:
        arguments = [str(BINARY), "--structure", self.structure]
        payload = "k 9\nk -4\nk 0\nk 0\nn 0\nm 2\nq -4 9\n"

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, "1\n0\n2\n", ""))


class VectorCommands(CommandScenarios, unittest.TestCase):
    structure = "vector"


class SetCommands(CommandScenarios, unittest.TestCase):
    structure = "set"


class AvlCommands(CommandScenarios, unittest.TestCase):
    structure = "avl"


class TreapCommands(CommandScenarios, unittest.TestCase):
    structure = "treap"


class ApplicationCli(unittest.TestCase):
    def test_empty_input(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = ''

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, '', ""))

    def test_invalid_insert(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = 'k value\n'

        result = execute(arguments, payload)

        self.assertEqual(
            (result.returncode, result.stdout, result.stderr),
            (2, "", 'invalid k command: expected an integer\n'),
        )

    def test_incomplete_range(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = 'q 1\n'

        result = execute(arguments, payload)

        self.assertEqual(
            (result.returncode, result.stdout, result.stderr),
            (2, "", 'invalid q command: expected two integers\n'),
        )

    def test_zero_select(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = 'm 0\n'

        result = execute(arguments, payload)

        self.assertEqual(
            (result.returncode, result.stdout, result.stderr),
            (2, "", 'invalid m command: expected a positive index\n'),
        )

    def test_past_end_select(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = 'k 2\nm 2\n'

        result = execute(arguments, payload)

        self.assertEqual(
            (result.returncode, result.stdout, result.stderr),
            (2, "", 'm index is outside the set\n'),
        )

    def test_unknown_command(self) -> None:
        arguments = [str(BINARY), "--structure", "vector"]
        payload = 'x\n'

        result = execute(arguments, payload)

        self.assertEqual(
            (result.returncode, result.stdout, result.stderr),
            (2, "", 'unknown command: x\n'),
        )

    def test_long_help(self) -> None:
        arguments = [str(BINARY), '--help']
        payload = "invalid stdin"

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stderr), (0, ""))
        self.assertIn("[OPTIONS]", result.stdout)
        self.assertIn("--structure", result.stdout)

    def test_unknown_structure(self) -> None:
        arguments = [str(BINARY), '--structure', 'unknown']
        payload = "invalid stdin"

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout), (2, ""))
        self.assertIn('--structure', result.stderr)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    args, unittest_args = parser.parse_known_args()
    global BINARY
    BINARY = args.binary
    unittest.main(argv=[sys.argv[0], *unittest_args])


if __name__ == "__main__":
    main()
