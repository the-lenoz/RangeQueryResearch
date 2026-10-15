#!/usr/bin/env python3
"""Integration and unit checks for the isolated benchmark runner."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import benchmark  # noqa: E402
from generate_payload import (  # noqa: E402
    FNV_OFFSET,
    build_query_workload,
    checksum_record,
    checksum_text,
    insert_validation_checksum,
)


QUERY_PAYLOAD = """\
k -2147483648
k -2
k 0
k 2
k 2147483647
k -2
n -2147483648
n 0
n 2147483647
m 1
m 3
m 5
m 6
q -2147483648 2147483647
q -2 2
q 2 2
"""
QUERY_CHECKSUM = "996afd1f324d0d25"
INSERT_PAYLOAD = """\
k -2147483648
k -2
k 0
k 2
k 2147483647
k -2
"""
INSERT_CHECKSUM = "214f856356040575"


class BenchmarkChecks(unittest.TestCase):
    runner: Path

    @classmethod
    def setUpClass(cls) -> None:
        cls.runner = RUNNER

    def execute(self, structure: str, mode: str, payload: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(self.runner), "--structure", structure, "--mode", mode],
            input=payload,
            text=True,
            capture_output=True,
            check=False,
        )

    def assert_success(
        self, structure: str, mode: str, payload: str, count: int, checksum: str
    ) -> None:
        completed = self.execute(structure, mode, payload)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertEqual(completed.stderr, "")
        result = benchmark.parse_runner_output(completed.stdout)
        self.assertEqual(result["measured_operation_count"], count)
        self.assertEqual(result["validation_checksum"], checksum)
        self.assertGreaterEqual(result["wall_elapsed_ns"], 0)
        self.assertGreaterEqual(result["cpu_elapsed_ns"], 0)
        self.assertIn(result["cpu_clock"], ("process", "std_clock"))

    def test_query_runner_for_every_structure(self) -> None:
        for structure in benchmark.STRUCTURES:
            with self.subTest(structure=structure):
                self.assert_success(structure, "queries", QUERY_PAYLOAD, 10, QUERY_CHECKSUM)

    def test_insert_runner_for_every_structure(self) -> None:
        for structure in benchmark.STRUCTURES:
            with self.subTest(structure=structure):
                self.assert_success(structure, "insert", INSERT_PAYLOAD, 6, INSERT_CHECKSUM)

    def test_workload_generator_expected_answers(self) -> None:
        for query_kind in ("rank", "select", "range", "mixed"):
            with self.subTest(query_kind=query_kind):
                workload = build_query_workload(
                    data_size=31,
                    query_count=47,
                    seed=123,
                    query_kind=query_kind,
                    density="dense",
                )
                completed = self.execute("avl", "queries", workload.payload)
                self.assertEqual(completed.returncode, 0, completed.stderr)
                result = benchmark.parse_runner_output(completed.stdout)
                self.assertEqual(result["measured_operation_count"], workload.query_count)
                self.assertEqual(result["validation_checksum"], workload.expected_checksum)

    def test_fnv_golden_values(self) -> None:
        checksum = checksum_record(FNV_OFFSET, "n", True, 0)
        self.assertEqual(checksum_text(checksum), "7eeb98f40a411050")
        self.assertEqual(
            insert_validation_checksum([-2147483648, -2, 0, 2, 2147483647, -2]),
            INSERT_CHECKSUM,
        )

    def test_invalid_workloads_and_mode_return_two(self) -> None:
        cases = (
            ("queries", "k 1\nq 0\n"),
            ("queries", "k 1\nn 1\nk 2\n"),
            ("insert", "k 1\nn 1\n"),
        )
        for mode, payload in cases:
            with self.subTest(mode=mode, payload=payload):
                completed = self.execute("vector", mode, payload)
                self.assertEqual(completed.returncode, 2)
                self.assertEqual(completed.stdout, "")
                self.assertNotEqual(completed.stderr, "")

        completed = self.execute("vector", "wrong", "k 1\n")
        self.assertEqual(completed.returncode, 2)
        self.assertEqual(completed.stdout, "")
        self.assertNotEqual(completed.stderr, "")

    def test_runner_json_parser_rejects_missing_and_negative_fields(self) -> None:
        valid = {
            "wall_elapsed_ns": 1,
            "cpu_elapsed_ns": 1,
            "measured_operation_count": 1,
            "validation_checksum": "0123456789abcdef",
            "cpu_clock": "process",
        }
        for mutation in (
            {key: value for key, value in valid.items() if key != "wall_elapsed_ns"},
            {**valid, "cpu_elapsed_ns": -1},
            {**valid, "measured_operation_count": 0},
            {**valid, "validation_checksum": "invalid"},
        ):
            with self.subTest(mutation=mutation):
                with self.assertRaises(RuntimeError):
                    benchmark.parse_runner_output(json.dumps(mutation))

    def test_checksum_mismatch_aborts_before_producing_a_row(self) -> None:
        case = benchmark.query_case("vector", "test", "rank", 5, 3, 1, "uniform", "dense")
        runner_result = {
            "wall_elapsed_ns": 10,
            "cpu_elapsed_ns": 9,
            "measured_operation_count": case.measured_operation_count,
            "validation_checksum": "0000000000000000",
            "cpu_clock": "process",
        }
        completed = SimpleNamespace(
            returncode=0, stdout=json.dumps(runner_result), stderr=""
        )
        with mock.patch.object(benchmark.subprocess, "run", return_value=completed):
            with self.assertRaisesRegex(RuntimeError, "checksum"):
                benchmark.execute_case(Path("runner"), case, 0, {})


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    args, unittest_args = parser.parse_known_args()
    global RUNNER
    RUNNER = args.runner
    unittest.main(argv=[sys.argv[0], *unittest_args])


if __name__ == "__main__":
    main()
