#!/usr/bin/env python3
"""Run reproducible rangequery benchmarks and write the results table."""

from __future__ import annotations

import argparse
import csv
import json
import platform
import random
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

from generate_payload import (
    DynamicWorkload,
    InsertWorkload,
    QueryWorkload,
    build_dynamic_workload,
    build_insert_workload,
    build_query_workload,
)

STRUCTURES = ("vector", "set", "avl", "treap")


@dataclass(frozen=True)
class BenchmarkCase:
    """One measured workload and its metadata."""

    structure: str
    profile: str
    operation_kind: str
    data_size: int
    payload: str
    mode: str
    setup_operation_count: int
    measured_operation_count: int
    command_count: int
    query_count: int
    locality: str
    density: str
    insertion_order: str
    workload_seed: int
    initial_data_size: int
    final_data_size: int
    measured_insert_count: int
    successful_insert_count: int
    rank_query_count: int
    select_query_count: int
    range_query_count: int
    expected_checksum: str


@dataclass(frozen=True)
class DataRegime:
    """Size range and density for comparing structures."""

    name: str
    data_sizes: tuple[int, ...]
    density: str
    workload: str
    query_count: int
    insertion_order: str = "random"


DATA_REGIMES = (
    DataRegime("small_dense_rank", (1_000, 3_000, 10_000), "dense", "rank", 10_000),
    DataRegime("medium_dense_rank", (10_000, 30_000, 100_000), "dense", "rank", 1_000),
    DataRegime("medium_sparse_rank", (10_000, 30_000, 100_000), "sparse", "rank", 1_000),
    DataRegime("medium_sparse_mixed", (10_000, 30_000, 100_000), "sparse", "mixed", 1_000),
    DataRegime("large_sparse_random_growth", (10_000, 30_000, 100_000), "sparse", "insert", 0),
    DataRegime(
        "large_sparse_ascending_growth",
        (10_000, 30_000, 100_000),
        "sparse",
        "insert",
        0,
        "ascending",
    ),
    DataRegime(
        "large_sparse_descending_growth",
        (10_000, 30_000, 100_000),
        "sparse",
        "insert",
        0,
        "descending",
    ),
)

DYNAMIC_INSERT_PERCENTAGES = (1, 10, 50, 90)
DYNAMIC_MEASURED_OPERATION_COUNTS = {1_000: 10_000, 10_000: 10_000, 100_000: 1_000}


def query_case(
    structure: str,
    profile: str,
    operation_kind: str,
    data_size: int,
    query_count: int,
    seed: int,
    locality: str,
    density: str,
) -> BenchmarkCase:
    workload: QueryWorkload = build_query_workload(
        data_size=data_size,
        query_count=query_count,
        seed=seed,
        locality=locality,
        query_kind=operation_kind,
        density=density,
    )
    return BenchmarkCase(
        structure=structure,
        profile=profile,
        operation_kind=operation_kind,
        data_size=workload.data_size,
        payload=workload.payload,
        mode="queries",
        setup_operation_count=workload.data_size,
        measured_operation_count=workload.query_count,
        command_count=workload.data_size + workload.query_count,
        query_count=workload.query_count,
        locality=locality,
        density=density,
        insertion_order="not_applicable",
        workload_seed=seed,
        initial_data_size=workload.data_size,
        final_data_size=workload.data_size,
        measured_insert_count=0,
        successful_insert_count=0,
        rank_query_count=workload.rank_query_count,
        select_query_count=workload.select_query_count,
        range_query_count=workload.range_query_count,
        expected_checksum=workload.expected_checksum,
    )


def insert_case(
    structure: str,
    profile: str,
    data_size: int,
    seed: int,
    density: str,
    insertion_order: str,
) -> BenchmarkCase:
    workload: InsertWorkload = build_insert_workload(data_size, seed, density, insertion_order)
    return BenchmarkCase(
        structure=structure,
        profile=profile,
        operation_kind="insert",
        data_size=workload.data_size,
        payload=workload.payload,
        mode="insert",
        setup_operation_count=0,
        measured_operation_count=workload.command_count,
        command_count=workload.command_count,
        query_count=0,
        locality="not_applicable",
        density=density,
        insertion_order=insertion_order,
        workload_seed=seed,
        initial_data_size=0,
        final_data_size=workload.data_size,
        measured_insert_count=workload.command_count,
        successful_insert_count=workload.data_size,
        rank_query_count=0,
        select_query_count=0,
        range_query_count=0,
        expected_checksum=workload.expected_checksum,
    )


def dynamic_cases(
    initial_data_size: int,
    measured_operation_count: int,
    insert_percentage: int,
    seed: int,
) -> list[BenchmarkCase]:
    """One dynamic workload shared by all structures without regeneration."""
    workload: DynamicWorkload = build_dynamic_workload(
        initial_data_size=initial_data_size,
        measured_operation_count=measured_operation_count,
        insert_percentage=insert_percentage,
        seed=seed,
    )
    common = {
        "profile": f"dynamic_{insert_percentage:02d}pct_insert",
        "operation_kind": "dynamic_mixed",
        "data_size": initial_data_size,
        "payload": workload.payload,
        "mode": "dynamic",
        "setup_operation_count": initial_data_size,
        "measured_operation_count": workload.measured_operation_count,
        "command_count": initial_data_size + workload.measured_operation_count,
        "query_count": workload.query_count,
        "locality": "uniform",
        "density": "sparse",
        "insertion_order": "random_interleaved",
        "workload_seed": seed,
        "initial_data_size": workload.initial_data_size,
        "final_data_size": workload.final_data_size,
        "measured_insert_count": workload.insert_count,
        "successful_insert_count": workload.successful_insert_count,
        "rank_query_count": workload.rank_query_count,
        "select_query_count": workload.select_query_count,
        "range_query_count": workload.range_query_count,
        "expected_checksum": workload.expected_checksum,
    }
    return [BenchmarkCase(structure=structure, **common) for structure in STRUCTURES]


def comparison_cases(
    data_size: int,
    query_count: int,
    seed: int,
    localities: list[str],
    query_kind: str,
    density: str,
) -> list[BenchmarkCase]:
    return [
        query_case(
            structure=structure,
            profile="comparison",
            operation_kind=query_kind,
            data_size=data_size,
            query_count=query_count,
            seed=seed,
            locality=locality,
            density=density,
        )
        for locality in localities
        for structure in STRUCTURES
    ]


def regime_cases(regime: DataRegime, data_size: int, seed: int) -> list[BenchmarkCase]:
    """Use the same workload for every structure in the selected regime."""
    if regime.workload == "insert":
        return [
            insert_case(structure, regime.name, data_size, seed, regime.density, regime.insertion_order)
            for structure in STRUCTURES
        ]
    return [
        query_case(
            structure=structure,
            profile=regime.name,
            operation_kind=regime.workload,
            data_size=data_size,
            query_count=regime.query_count,
            seed=seed,
            locality="uniform",
            density=regime.density,
        )
        for structure in STRUCTURES
    ]


def parse_runner_output(output: str) -> dict[str, int | str]:
    """Validate the single JSON record returned by the C++ benchmark runner."""
    try:
        result = json.loads(output)
    except json.JSONDecodeError as error:
        raise RuntimeError(f"benchmark runner returned invalid JSON: {error}") from error
    if not isinstance(result, dict):
        raise RuntimeError("benchmark runner must return a JSON object")

    required_integers = ("wall_elapsed_ns", "cpu_elapsed_ns", "measured_operation_count")
    for field in required_integers:
        value = result.get(field)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise RuntimeError(f"benchmark runner returned an invalid field {field}")
    if result["measured_operation_count"] == 0:
        raise RuntimeError("benchmark runner measured no operations")

    checksum = result.get("validation_checksum")
    if not isinstance(checksum, str) or re.fullmatch(r"[0-9a-f]{16}", checksum) is None:
        raise RuntimeError("benchmark runner returned an invalid validation_checksum")
    cpu_clock = result.get("cpu_clock")
    if not isinstance(cpu_clock, str) or not cpu_clock:
        raise RuntimeError("benchmark runner returned an invalid cpu_clock")
    for field in ("benchmark_version", "compiler", "compiler_version", "build_config"):
        if not isinstance(result.get(field), str) or not result[field]:
            raise RuntimeError(f"benchmark runner returned an invalid field {field}")
    for field in ("benchmark_engine", "wall_clock"):
        if result.get(field) != "google_benchmark":
            raise RuntimeError(f"benchmark runner returned an invalid field {field}")
    iterations = result.get("benchmark_iterations")
    if isinstance(iterations, bool) or not isinstance(iterations, int) or iterations != 1:
        raise RuntimeError("benchmark runner must measure exactly one workload pass")
    return result


def execute_case(
    runner: Path,
    case: BenchmarkCase,
    repetition: int,
    execution_order: int,
    metadata: dict[str, object],
) -> dict[str, object]:
    arguments = [
        str(runner),
        "--structure",
        case.structure,
        "--mode",
        case.mode,
        "--benchmark_min_warmup_time=0",
    ]
    if case.mode == "dynamic":
        arguments.extend(("--setup-count", str(case.setup_operation_count)))
    completed = subprocess.run(
        arguments,
        input=case.payload,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{case.structure} failed: {completed.stderr.strip()}")
    result = parse_runner_output(completed.stdout)
    if result["measured_operation_count"] != case.measured_operation_count:
        raise RuntimeError(
            f"{case.structure}: expected {case.measured_operation_count} measured operations, "
            f"runner reported {result['measured_operation_count']}"
        )
    if result["validation_checksum"] != case.expected_checksum:
        raise RuntimeError(
            f"{case.structure}: sanity checksum mismatch: expected {case.expected_checksum}, "
            f"got {result['validation_checksum']}"
        )

    wall_elapsed_ns = int(result["wall_elapsed_ns"])
    cpu_elapsed_ns = int(result["cpu_elapsed_ns"])

    return {
        **metadata,
        "seed": case.workload_seed,
        "benchmark_schema_version": 4,
        "benchmark_engine": result["benchmark_engine"],
        "benchmark_version": result["benchmark_version"],
        "benchmark_iterations": result["benchmark_iterations"],
        "compiler": result["compiler"],
        "compiler_version": result["compiler_version"],
        "build_config": result["build_config"],
        "scenario": case.profile,
        "workload_mode": case.mode,
        "operation_kind": case.operation_kind,
        "data_size": case.data_size,
        "initial_data_size": case.initial_data_size,
        "final_data_size": case.final_data_size,
        "setup_operation_count": case.setup_operation_count,
        "measured_operation_count": case.measured_operation_count,
        "measured_insert_count": case.measured_insert_count,
        "successful_insert_count": case.successful_insert_count,
        "query_count": case.query_count,
        "rank_query_count": case.rank_query_count,
        "select_query_count": case.select_query_count,
        "range_query_count": case.range_query_count,
        "insert_fraction": case.measured_insert_count / case.measured_operation_count,
        "command_count": case.command_count,
        "locality": case.locality,
        "density": case.density,
        "insertion_order": case.insertion_order,
        "structure": case.structure,
        "workload_seed": case.workload_seed,
        "repetition": repetition,
        "execution_order": execution_order,
        "wall_elapsed_ns": wall_elapsed_ns,
        "cpu_elapsed_ns": cpu_elapsed_ns,
        "wall_ns_per_operation": wall_elapsed_ns / case.measured_operation_count,
        "cpu_ns_per_operation": cpu_elapsed_ns / case.measured_operation_count,
        "validation_checksum": result["validation_checksum"],
        "measurement_scope": "structure_operations",
        "wall_clock": result["wall_clock"],
        "cpu_clock": result["cpu_clock"],
    }


def execute_repetition(
    runner: Path,
    cases: list[BenchmarkCase],
    repetition: int,
    metadata: dict[str, object],
) -> list[dict[str, object]]:
    """Run cases in a reproducibly shuffled structure order."""
    if not cases:
        return []
    ordered_cases = list(cases)
    first = cases[0]
    shuffle_seed = (
        first.workload_seed * 1_000_003
        + first.data_size * 101
        + first.measured_insert_count * 17
        + repetition
    )
    random.Random(shuffle_seed).shuffle(ordered_cases)
    return [
        execute_case(runner, case, repetition, execution_order, metadata)
        for execution_order, case in enumerate(ordered_cases, start=1)
    ]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--runner",
        "--binary",
        dest="runner",
        type=Path,
        required=True,
        help="Path to rangequery_benchmark; --binary is a compatibility alias.",
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--scenario",
        choices=("comparison", "regimes", "dynamic"),
        default="comparison",
        help=(
            "comparison runs the selected shared workload; regimes uses predefined data regimes; "
            "dynamic runs a matrix of interleaved insertions and queries."
        ),
    )
    parser.add_argument(
        "--data-sizes",
        "--sizes",
        dest="data_sizes",
        type=int,
        nargs="+",
        default=[1_000, 10_000, 100_000],
        help="Set sizes; --sizes is a compatibility alias.",
    )
    parser.add_argument("--query-count", type=int, default=10_000)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument(
        "--seeds",
        type=int,
        nargs="+",
        help="Independent workload seeds; defaults to a single --seed.",
    )
    parser.add_argument(
        "--insert-percentages",
        type=int,
        nargs="+",
        default=list(DYNAMIC_INSERT_PERCENTAGES),
        help="Exact insertion percentages for the dynamic scenario.",
    )
    parser.add_argument(
        "--build-config",
        default="unspecified",
        help="Optional build label; the runner reports the actual build_config.",
    )
    parser.add_argument("--localities", choices=("uniform", "hotspot"), nargs="+", default=["uniform"])
    parser.add_argument("--query-kind", choices=("mixed", "rank", "select", "range"), default="mixed")
    parser.add_argument("--density", choices=("dense", "sparse"), default="sparse")
    args = parser.parse_args()

    if args.repetitions < 1 or args.query_count < 1 or any(size < 1 for size in args.data_sizes):
        parser.error("Set sizes, query count, and repetition count must be positive")
    if any(percentage <= 0 or percentage >= 100 for percentage in args.insert_percentages):
        parser.error("Insertion percentages must be between 1 and 99")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    metadata = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "build_label": args.build_config,
    }
    rows: list[dict[str, object]] = []
    workload_seeds = args.seeds if args.seeds is not None else [args.seed]

    if args.scenario == "comparison":
        for seed in workload_seeds:
            for data_size in args.data_sizes:
                cases = comparison_cases(
                    data_size, args.query_count, seed, args.localities, args.query_kind, args.density
                )
                for repetition in range(args.repetitions):
                    rows.extend(execute_repetition(args.runner, cases, repetition, metadata))
    elif args.scenario == "regimes":
        for seed in workload_seeds:
            for regime in DATA_REGIMES:
                for data_size in regime.data_sizes:
                    cases = regime_cases(regime, data_size, seed)
                    for repetition in range(args.repetitions):
                        rows.extend(execute_repetition(args.runner, cases, repetition, metadata))
    else:
        for seed in workload_seeds:
            for data_size, measured_operation_count in DYNAMIC_MEASURED_OPERATION_COUNTS.items():
                for insert_percentage in args.insert_percentages:
                    cases = dynamic_cases(
                        data_size, measured_operation_count, insert_percentage, seed
                    )
                    for repetition in range(args.repetitions):
                        rows.extend(execute_repetition(args.runner, cases, repetition, metadata))

    with args.output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
