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
    build_dynamic_workload,
    build_query_workload,
    checksum_record,
    checksum_text,
    insert_validation_checksum,
    state_validation_checksum,
)


RUNNER_METADATA = {
    "benchmark_engine": "google_benchmark",
    "benchmark_version": "1.9.4",
    "benchmark_iterations": 1,
    "wall_clock": "google_benchmark",
    "compiler": "/test/c++",
    "compiler_version": "test compiler 1.0",
    "build_config": "Release",
}

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
DYNAMIC_PAYLOAD = """\
k -2147483648
k 0
k 2147483647
k -2
n 0
m 2
q -2 2147483647
k 2
n 2147483647
m 5
q 0 2
"""


class BenchmarkChecks(unittest.TestCase):
    runner: Path

    @classmethod
    def setUpClass(cls) -> None:
        cls.runner = RUNNER

    def execute(
        self, structure: str, mode: str, payload: str, setup_count: int | None = None
    ) -> subprocess.CompletedProcess[str]:
        arguments = [str(self.runner), "--structure", structure, "--mode", mode]
        if setup_count is not None:
            arguments.extend(("--setup-count", str(setup_count)))
        return subprocess.run(
            arguments,
            input=payload,
            text=True,
            capture_output=True,
            check=False,
        )

    def assert_success(
        self,
        structure: str,
        mode: str,
        payload: str,
        count: int,
        checksum: str,
        setup_count: int | None = None,
    ) -> None:
        completed = self.execute(structure, mode, payload, setup_count)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertEqual(completed.stderr, "")
        result = benchmark.parse_runner_output(completed.stdout)
        self.assertEqual(result["measured_operation_count"], count)
        self.assertEqual(result["validation_checksum"], checksum)
        self.assertGreaterEqual(result["wall_elapsed_ns"], 0)
        self.assertGreaterEqual(result["cpu_elapsed_ns"], 0)
        self.assertEqual(result["cpu_clock"], "process")
        self.assertEqual(result["benchmark_engine"], "google_benchmark")
        self.assertEqual(result["benchmark_iterations"], 1)
        self.assertEqual(result["benchmark_version"], "1.9.4")

    def test_library_help_and_cli_errors(self) -> None:
        for option in ("-h", "--help"):
            with self.subTest(option=option):
                result = subprocess.run(
                    [str(self.runner), option], input="invalid stdin", text=True,
                    capture_output=True, check=False, timeout=10,
                )
                self.assertEqual(result.returncode, 0)
                self.assertEqual(result.stderr, "")
                for fragment in ("[OPTIONS]", "--structure", "--mode", "--setup-count"):
                    self.assertIn(fragment, result.stdout)

        valid = ["--structure", "avl", "--mode", "dynamic", "--setup-count", "3"]
        invalid = [
            ([], "--structure"),
            (["--structure"], "--structure"),
            (["--structure", "wrong", "--mode", "queries"], "--structure"),
            (["--structure", "avl"], "--mode"),
            (["--structure", "avl", "--mode"], "--mode"),
            (["--structure", "avl", "--mode", "wrong"], "--mode"),
            (["--structure", "avl", "--mode", "dynamic"], "--setup-count"),
            (["--structure", "avl", "--mode", "queries", "--setup-count", "3"], "--setup-count"),
            (["--structure", "avl", "--mode", "insert", "--setup-count", "3"], "--setup-count"),
        ]
        for option, value in (("--structure", "treap"), ("--mode", "dynamic"),
                              ("--setup-count", "3")):
            invalid.append((valid + [option, value], option))
        invalid.append((valid[:-1], "--setup-count"))
        for value in ("0", "-1", "invalid", "1.5", "1x", "18446744073709551616"):
            invalid.append((valid[:-1] + [value], "--setup-count"))
        for arguments, fragment in invalid:
            with self.subTest(arguments=arguments):
                result = subprocess.run(
                    [str(self.runner), *arguments], input="invalid stdin", text=True,
                    capture_output=True, check=False, timeout=10,
                )
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, "")
                self.assertIn(fragment, result.stderr)

    def test_google_benchmark_flags_are_forwarded(self) -> None:
        result = subprocess.run(
            [
                str(self.runner),
                "--structure",
                "treap",
                "--mode",
                "queries",
                "--benchmark_min_warmup_time=0",
            ],
            input=QUERY_PAYLOAD,
            text=True,
            capture_output=True,
            check=False,
            timeout=10,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(
            benchmark.parse_runner_output(result.stdout)["validation_checksum"], QUERY_CHECKSUM
        )

    def test_equals_syntax_preserves_query_checksum(self) -> None:
        result = subprocess.run(
            [str(self.runner), "--structure=treap", "--mode=queries"],
            input=QUERY_PAYLOAD, text=True, capture_output=True, check=False, timeout=10,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        parsed = benchmark.parse_runner_output(result.stdout)
        self.assertEqual(parsed["validation_checksum"], QUERY_CHECKSUM)

    def test_query_runner_for_every_structure(self) -> None:
        for structure in benchmark.STRUCTURES:
            with self.subTest(structure=structure):
                self.assert_success(structure, "queries", QUERY_PAYLOAD, 10, QUERY_CHECKSUM)

    def test_insert_runner_for_every_structure(self) -> None:
        for structure in benchmark.STRUCTURES:
            with self.subTest(structure=structure):
                self.assert_success(structure, "insert", INSERT_PAYLOAD, 6, INSERT_CHECKSUM)

    def test_dynamic_runner_for_every_structure(self) -> None:
        checksum = FNV_OFFSET
        checksum = checksum_record(checksum, "n", True, 2)
        checksum = checksum_record(checksum, "m", True, -2)
        checksum = checksum_record(checksum, "q", True, 2)
        checksum = checksum_record(checksum, "n", True, 4)
        checksum = checksum_record(checksum, "m", True, 2147483647)
        checksum = checksum_record(checksum, "q", True, 1)
        expected = checksum_text(
            state_validation_checksum(checksum, [-2147483648, -2, 0, 2, 2147483647])
        )
        for structure in benchmark.STRUCTURES:
            with self.subTest(structure=structure):
                self.assert_success(
                    structure, "dynamic", DYNAMIC_PAYLOAD, 8, expected, setup_count=3
                )

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

    def test_dynamic_workload_has_exact_mix_and_is_deterministic(self) -> None:
        first = build_dynamic_workload(31, 300, 10, 123)
        second = build_dynamic_workload(31, 300, 10, 123)
        self.assertEqual(first, second)
        self.assertEqual(first.insert_count, 30)
        self.assertEqual(first.successful_insert_count, 30)
        self.assertEqual(first.final_data_size, 61)
        self.assertEqual(first.query_count, 270)
        self.assertEqual(
            first.rank_query_count + first.select_query_count + first.range_query_count,
            first.query_count,
        )
        self.assertLessEqual(
            max(first.rank_query_count, first.select_query_count, first.range_query_count)
            - min(first.rank_query_count, first.select_query_count, first.range_query_count),
            1,
        )
        completed = self.execute("avl", "dynamic", first.payload, first.initial_data_size)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        result = benchmark.parse_runner_output(completed.stdout)
        self.assertEqual(result["measured_operation_count"], first.measured_operation_count)
        self.assertEqual(result["validation_checksum"], first.expected_checksum)

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

    def test_dynamic_mode_rejects_invalid_setup_boundary(self) -> None:
        cases = (
            (None, "k 1\nn 1\n"),
            (2, "k 1\nn 1\nk 2\n"),
            (2, "k 1\nk 2\n"),
        )
        for setup_count, payload in cases:
            with self.subTest(setup_count=setup_count, payload=payload):
                completed = self.execute("vector", "dynamic", payload, setup_count)
                self.assertEqual(completed.returncode, 2)
                self.assertEqual(completed.stdout, "")
                self.assertNotEqual(completed.stderr, "")

        completed = subprocess.run(
            [
                str(self.runner),
                "--structure",
                "vector",
                "--mode",
                "dynamic",
                "--setup-count",
                "invalid",
            ],
            input="k 1\nn 1\n",
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(completed.returncode, 2)

    def test_runner_json_parser_rejects_missing_and_negative_fields(self) -> None:
        valid = {
            **RUNNER_METADATA,
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
            {**valid, "benchmark_iterations": 2},
            {**valid, "benchmark_iterations": True},
            {**valid, "benchmark_iterations": 1.0},
            {**valid, "benchmark_engine": "unknown"},
            {**valid, "wall_clock": "steady_clock"},
            {**valid, "benchmark_version": ""},
            {**valid, "compiler_version": None},
        ):
            with self.subTest(mutation=mutation):
                with self.assertRaises(RuntimeError):
                    benchmark.parse_runner_output(json.dumps(mutation))

    def test_checksum_mismatch_aborts_before_producing_a_row(self) -> None:
        case = benchmark.query_case("vector", "test", "rank", 5, 3, 1, "uniform", "dense")
        runner_result = {
            **RUNNER_METADATA,
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
                benchmark.execute_case(Path("runner"), case, 0, 1, {})

    def test_dynamic_cases_share_payload_and_emit_schema_four_metadata(self) -> None:
        cases = benchmark.dynamic_cases(100, 1_000, 10, 44)
        self.assertEqual({case.payload for case in cases}, {cases[0].payload})
        self.assertEqual({case.expected_checksum for case in cases}, {cases[0].expected_checksum})
        case = cases[0]
        runner_result = {
            **RUNNER_METADATA,
            "wall_elapsed_ns": 1_000,
            "cpu_elapsed_ns": 900,
            "measured_operation_count": case.measured_operation_count,
            "validation_checksum": case.expected_checksum,
            "cpu_clock": "process",
        }
        completed = SimpleNamespace(
            returncode=0, stdout=json.dumps(runner_result), stderr=""
        )
        with mock.patch.object(benchmark.subprocess, "run", return_value=completed) as run:
            row = benchmark.execute_case(
                Path("runner"), case, 2, 3, {"build_config": "wrong", "compiler": "wrong"}
            )

        self.assertEqual(row["benchmark_schema_version"], 4)
        self.assertEqual(row["benchmark_engine"], "google_benchmark")
        self.assertEqual(row["benchmark_version"], "1.9.4")
        self.assertEqual(row["benchmark_iterations"], 1)
        self.assertEqual(row["compiler"], RUNNER_METADATA["compiler"])
        self.assertEqual(row["build_config"], RUNNER_METADATA["build_config"])
        self.assertEqual(row["wall_clock"], "google_benchmark")
        self.assertEqual(row["seed"], 44)
        self.assertEqual(row["workload_seed"], 44)
        self.assertEqual(row["execution_order"], 3)
        self.assertEqual(row["measured_insert_count"], 100)
        self.assertEqual(row["successful_insert_count"], 100)
        self.assertEqual(row["insert_fraction"], 0.1)
        self.assertIn("--setup-count", run.call_args.args[0])
        self.assertIn("--benchmark_min_warmup_time=0", run.call_args.args[0])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    args, unittest_args = parser.parse_known_args()
    global RUNNER
    RUNNER = args.runner
    unittest.main(argv=[sys.argv[0], *unittest_args])


if __name__ == "__main__":
    main()
