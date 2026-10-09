#!/usr/bin/env python3
"""Запускает воспроизводимые бенчмарки rangequery и записывает таблицу результатов."""

from __future__ import annotations

import argparse
import csv
import os
import platform
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

from generate_payload import InsertWorkload, QueryWorkload, build_insert_workload, build_query_workload

STRUCTURES = ("vector", "set", "avl", "treap")


@dataclass(frozen=True)
class BenchmarkCase:
    """Один измеряемый входной поток и его метаданные."""

    structure: str
    profile: str
    operation_kind: str
    data_size: int
    payload: str
    operation_count: int
    query_count: int
    locality: str
    density: str
    insertion_order: str


@dataclass(frozen=True)
class DataRegime:
    """Диапазон размера и плотность данных для общего сравнения структур."""

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
        operation_count=workload.data_size + workload.query_count,
        query_count=workload.query_count,
        locality=locality,
        density=density,
        insertion_order="not_applicable",
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
        operation_count=workload.command_count,
        query_count=0,
        locality="not_applicable",
        density=density,
        insertion_order=insertion_order,
    )


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
    """Один и тот же входной поток для всех структур выбранного режима."""
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


def execute_case(binary: Path, case: BenchmarkCase, repetition: int, metadata: dict[str, object]) -> dict[str, object]:
    started = time.perf_counter_ns()
    completed = subprocess.run(
        [str(binary), "--structure", case.structure],
        input=case.payload,
        text=True,
        capture_output=True,
        check=False,
    )
    elapsed_ns = time.perf_counter_ns() - started
    if completed.returncode != 0:
        raise RuntimeError(f"{case.structure} завершился с ошибкой: {completed.stderr.strip()}")

    return {
        **metadata,
        "scenario": case.profile,
        "operation_kind": case.operation_kind,
        "data_size": case.data_size,
        "operation_count": case.operation_count,
        "query_count": case.query_count,
        "command_count": case.operation_count,
        "locality": case.locality,
        "density": case.density,
        "insertion_order": case.insertion_order,
        "structure": case.structure,
        "repetition": repetition,
        "elapsed_ns": elapsed_ns,
        "ns_per_operation": elapsed_ns / case.operation_count,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--scenario",
        choices=("comparison", "regimes"),
        default="comparison",
        help="comparison запускает указанный общий поток; regimes — готовые режимы размера и плотности данных.",
    )
    parser.add_argument(
        "--data-sizes",
        "--sizes",
        dest="data_sizes",
        type=int,
        nargs="+",
        default=[1_000, 10_000, 100_000],
        help="Размеры множеств; --sizes сохранён как псевдоним.",
    )
    parser.add_argument("--query-count", type=int, default=10_000)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--localities", choices=("uniform", "hotspot"), nargs="+", default=["uniform"])
    parser.add_argument("--query-kind", choices=("mixed", "rank", "select", "range"), default="mixed")
    parser.add_argument("--density", choices=("dense", "sparse"), default="sparse")
    args = parser.parse_args()

    if args.repetitions < 1 or args.query_count < 1 or any(size < 1 for size in args.data_sizes):
        parser.error("Размеры множеств, число запросов и число повторов должны быть положительными")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    metadata = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "compiler": os.environ.get("CXX", "c++"),
        "seed": args.seed,
    }
    rows: list[dict[str, object]] = []

    if args.scenario == "comparison":
        for data_size in args.data_sizes:
            cases = comparison_cases(
                data_size, args.query_count, args.seed + data_size, args.localities, args.query_kind, args.density
            )
            for case in cases:
                for repetition in range(args.repetitions):
                    rows.append(execute_case(args.binary, case, repetition, metadata))
    else:
        for regime in DATA_REGIMES:
            for data_size in regime.data_sizes:
                for case in regime_cases(regime, data_size, args.seed + data_size):
                    for repetition in range(args.repetitions):
                        rows.append(execute_case(args.binary, case, repetition, metadata))

    with args.output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
