#!/usr/bin/env python3
"""Short AAA scenarios for the benchmark runner."""

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
    checksum_record,
    checksum_text,
)


def execute(arguments: list[str], payload: str = "") -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        arguments, input=payload, text=True, capture_output=True, check=False, timeout=10
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


DYNAMIC_CHECKSUM = "99c96c6cb06f5ce9"


def runner_record(**changes) -> dict:
    return {
        **RUNNER_METADATA,
        "wall_elapsed_ns": 10,
        "cpu_elapsed_ns": 9,
        "measured_operation_count": 1,
        "validation_checksum": "0123456789abcdef",
        "cpu_clock": "process",
        **changes,
    }


def capture_failure(function, *args):
    """Capture a failure as data so assertions stay in the Assert block."""
    try:
        function(*args)
    except (RuntimeError, ValueError) as error:
        return error
    return None


class RunnerWorkloads(unittest.TestCase):
    def test_queries(self) -> None:
        arguments = [str(RUNNER), "--structure", 'vector', "--mode", "queries"]
        payload = QUERY_PAYLOAD

        completed = execute(arguments, payload)
        result = benchmark.parse_runner_output(completed.stdout)

        self.assertEqual((completed.returncode, completed.stderr), (0, ""))
        self.assertEqual(result["measured_operation_count"], 10)
        self.assertEqual(result["validation_checksum"], QUERY_CHECKSUM)

    def test_insert(self) -> None:
        arguments = [str(RUNNER), "--structure", 'avl', "--mode", "insert"]
        payload = INSERT_PAYLOAD

        completed = execute(arguments, payload)
        result = benchmark.parse_runner_output(completed.stdout)

        self.assertEqual((completed.returncode, completed.stderr), (0, ""))
        self.assertEqual(result["measured_operation_count"], 6)
        self.assertEqual(result["validation_checksum"], INSERT_CHECKSUM)

    def test_dynamic(self) -> None:
        arguments = [
            str(RUNNER),
            "--structure",
            'treap',
            "--mode",
            "dynamic",
            "--setup-count",
            "3",
        ]
        payload = DYNAMIC_PAYLOAD

        completed = execute(arguments, payload)
        result = benchmark.parse_runner_output(completed.stdout)

        self.assertEqual((completed.returncode, completed.stderr), (0, ""))
        self.assertEqual(result["measured_operation_count"], 8)
        self.assertEqual(result["validation_checksum"], DYNAMIC_CHECKSUM)

    def test_benchmark_flags(self) -> None:
        arguments = [
            str(RUNNER),
            '--structure',
            'treap',
            '--mode',
            'queries',
            '--benchmark_min_warmup_time=0',
        ]
        payload = QUERY_PAYLOAD

        completed = execute(arguments, payload)
        result = benchmark.parse_runner_output(completed.stdout)

        self.assertEqual((completed.returncode, completed.stderr), (0, ""))
        self.assertEqual(result["validation_checksum"], QUERY_CHECKSUM)


class InvalidWorkloads(unittest.TestCase):
    def test_insert_after_query(self) -> None:
        arguments = [str(RUNNER), '--structure=vector', '--mode=queries']
        payload = 'k 1\nn 1\nk 2\n'

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout), (2, ""))
        self.assertNotEqual(result.stderr, "")

    def test_query_in_insert(self) -> None:
        arguments = [str(RUNNER), '--structure=vector', '--mode=insert']
        payload = 'k 1\nn 1\n'

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout), (2, ""))
        self.assertNotEqual(result.stderr, "")

    def test_query_inside_setup(self) -> None:
        arguments = [str(RUNNER), '--structure=vector', '--mode=dynamic', '--setup-count=2']
        payload = 'k 1\nn 1\nk 2\n'

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout), (2, ""))
        self.assertNotEqual(result.stderr, "")

    def test_missing_measured_phase(self) -> None:
        arguments = [str(RUNNER), '--structure=vector', '--mode=dynamic', '--setup-count=2']
        payload = 'k 1\nk 2\n'

        result = execute(arguments, payload)

        self.assertEqual((result.returncode, result.stdout), (2, ""))
        self.assertNotEqual(result.stderr, "")


class RunnerJsonParser(unittest.TestCase):
    def test_missing_wall_time(self) -> None:
        record = runner_record()
        del record["wall_elapsed_ns"]
        payload = json.dumps(record)

        error = capture_failure(benchmark.parse_runner_output, payload)

        self.assertIsInstance(error, RuntimeError)
        self.assertIn("wall_elapsed_ns", str(error))

    def test_zero_operations(self) -> None:
        record = runner_record(measured_operation_count=0)
        payload = json.dumps(record)

        error = capture_failure(benchmark.parse_runner_output, payload)

        self.assertIsInstance(error, RuntimeError)

    def test_invalid_checksum(self) -> None:
        record = runner_record(validation_checksum='invalid')
        payload = json.dumps(record)

        error = capture_failure(benchmark.parse_runner_output, payload)

        self.assertIsInstance(error, RuntimeError)


class WorkloadGenerator(unittest.TestCase):
    def test_dynamic_mix(self) -> None:
        initial_size = 3
        operation_count = 10
        insert_percentage = 10
        seed = 123

        result = build_dynamic_workload(initial_size, operation_count, insert_percentage, seed)

        self.assertEqual(result.insert_count, 1)
        self.assertEqual(result.successful_insert_count, 1)
        self.assertEqual(result.final_data_size, 4)
        self.assertEqual(result.query_count, 9)
        self.assertEqual(
            (result.rank_query_count, result.select_query_count, result.range_query_count),
            (3, 3, 3),
        )

    def test_dynamic_determinism(self) -> None:
        expected = build_dynamic_workload(3, 10, 10, 123)

        result = build_dynamic_workload(3, 10, 10, 123)

        self.assertEqual(result, expected)

    def test_fnv_record_golden(self) -> None:
        checksum = FNV_OFFSET

        result = checksum_text(checksum_record(checksum, "n", True, 0))

        self.assertEqual(result, "7eeb98f40a411050")


class BenchmarkExecution(unittest.TestCase):
    def test_checksum_mismatch(self) -> None:
        case = benchmark.query_case("vector", "test", "rank", 3, 3, 1, "uniform", "dense")
        record = runner_record(
            validation_checksum="0000000000000000",
            measured_operation_count=case.measured_operation_count,
        )
        completed = SimpleNamespace(returncode=0, stdout=json.dumps(record), stderr="")

        with mock.patch.object(benchmark.subprocess, "run", return_value=completed):
            error = capture_failure(benchmark.execute_case, Path("runner"), case, 0, 1, {})

        self.assertIsInstance(error, RuntimeError)
        self.assertIn('checksum', str(error))

    def test_operation_count_mismatch(self) -> None:
        case = benchmark.query_case("vector", "test", "rank", 3, 3, 1, "uniform", "dense")
        record = runner_record(
            validation_checksum=case.expected_checksum,
            measured_operation_count=case.measured_operation_count + 1,
        )
        completed = SimpleNamespace(returncode=0, stdout=json.dumps(record), stderr="")

        with mock.patch.object(benchmark.subprocess, "run", return_value=completed):
            error = capture_failure(benchmark.execute_case, Path("runner"), case, 0, 1, {})

        self.assertIsInstance(error, RuntimeError)
        self.assertIn('measured operations', str(error))

    def test_dynamic_cases_share_payload(self) -> None:
        initial_size = 3
        operation_count = 10
        seed = 44

        vector, ordered_set, avl, treap = benchmark.dynamic_cases(
            initial_size,
            operation_count,
            10,
            seed,
        )

        self.assertEqual(
            (vector.structure, ordered_set.structure, avl.structure, treap.structure),
            ("vector", "set", "avl", "treap"),
        )
        self.assertEqual((ordered_set.payload, avl.payload, treap.payload), (vector.payload,) * 3)
        self.assertEqual(
            (ordered_set.expected_checksum, avl.expected_checksum, treap.expected_checksum),
            (vector.expected_checksum,) * 3,
        )

    def test_runner_metadata_overrides_labels(self) -> None:
        case = benchmark.dynamic_cases(3, 10, 10, 44)[0]
        record = runner_record(
            measured_operation_count=case.measured_operation_count,
            validation_checksum=case.expected_checksum,
        )
        completed = SimpleNamespace(returncode=0, stdout=json.dumps(record), stderr="")
        metadata = {"build_config": "wrong", "compiler": "wrong"}

        with mock.patch.object(benchmark.subprocess, "run", return_value=completed):
            row = benchmark.execute_case(Path("runner"), case, 2, 3, metadata)

        self.assertEqual(row["benchmark_schema_version"], 4)
        self.assertEqual(row["compiler"], RUNNER_METADATA["compiler"])
        self.assertEqual(row["build_config"], RUNNER_METADATA["build_config"])
        self.assertEqual(row["benchmark_engine"], "google_benchmark")
        self.assertEqual(row["benchmark_version"], "1.9.4")
        self.assertEqual(row["benchmark_iterations"], 1)
        self.assertEqual(row["wall_clock"], "google_benchmark")

    def test_dynamic_metadata_and_arguments(self) -> None:
        case = benchmark.dynamic_cases(3, 10, 10, 44)[0]
        record = runner_record(
            measured_operation_count=case.measured_operation_count,
            validation_checksum=case.expected_checksum,
        )
        completed = SimpleNamespace(returncode=0, stdout=json.dumps(record), stderr="")
        metadata = {"build_config": "wrong", "compiler": "wrong"}

        with mock.patch.object(benchmark.subprocess, "run", return_value=completed) as run:
            row = benchmark.execute_case(Path("runner"), case, 2, 3, metadata)
        arguments = run.call_args.args[0]

        self.assertEqual((row["seed"], row["workload_seed"]), (44, 44))
        self.assertEqual(row["execution_order"], 3)
        self.assertEqual(row["measured_insert_count"], 1)
        self.assertEqual(row["successful_insert_count"], 1)
        self.assertEqual(row["insert_fraction"], 0.1)
        self.assertIn("--setup-count", arguments)
        self.assertIn("--benchmark_min_warmup_time=0", arguments)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    args, unittest_args = parser.parse_known_args()
    global RUNNER
    RUNNER = args.runner
    unittest.main(argv=[sys.argv[0], *unittest_args])


if __name__ == "__main__":
    main()
