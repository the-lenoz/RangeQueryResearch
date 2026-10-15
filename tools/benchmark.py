#!/usr/bin/env python3
"""Запускает воспроизводимые бенчмарки rangequery и записывает таблицу результатов."""

from __future__ import annotations

import argparse
import csv
import json
import os
import platform
import re
import subprocess
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
    mode: str
    setup_operation_count: int
    measured_operation_count: int
    command_count: int
    query_count: int
    locality: str
    density: str
    insertion_order: str
    expected_checksum: str


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
        mode="queries",
        setup_operation_count=workload.data_size,
        measured_operation_count=workload.query_count,
        command_count=workload.data_size + workload.query_count,
        query_count=workload.query_count,
        locality=locality,
        density=density,
        insertion_order="not_applicable",
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
        expected_checksum=workload.expected_checksum,
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


def parse_runner_output(output: str) -> dict[str, int | str]:
    """Проверяет единственную JSON-запись, возвращённую C++ benchmark runner."""
    try:
        result = json.loads(output)
    except json.JSONDecodeError as error:
        raise RuntimeError(f"benchmark runner вернул некорректный JSON: {error}") from error
    if not isinstance(result, dict):
        raise RuntimeError("benchmark runner должен вернуть JSON-объект")

    required_integers = ("wall_elapsed_ns", "cpu_elapsed_ns", "measured_operation_count")
    for field in required_integers:
        value = result.get(field)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise RuntimeError(f"benchmark runner вернул некорректное поле {field}")
    if result["measured_operation_count"] == 0:
        raise RuntimeError("benchmark runner не измерил ни одной операции")

    checksum = result.get("validation_checksum")
    if not isinstance(checksum, str) or re.fullmatch(r"[0-9a-f]{16}", checksum) is None:
        raise RuntimeError("benchmark runner вернул некорректный validation_checksum")
    cpu_clock = result.get("cpu_clock")
    if not isinstance(cpu_clock, str) or not cpu_clock:
        raise RuntimeError("benchmark runner вернул некорректный cpu_clock")
    return result


def execute_case(runner: Path, case: BenchmarkCase, repetition: int, metadata: dict[str, object]) -> dict[str, object]:
    completed = subprocess.run(
        [str(runner), "--structure", case.structure, "--mode", case.mode],
        input=case.payload,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{case.structure} завершился с ошибкой: {completed.stderr.strip()}")
    result = parse_runner_output(completed.stdout)
    if result["measured_operation_count"] != case.measured_operation_count:
        raise RuntimeError(
            f"{case.structure}: ожидалось {case.measured_operation_count} измеренных операций, "
            f"runner сообщил {result['measured_operation_count']}"
        )
    if result["validation_checksum"] != case.expected_checksum:
        raise RuntimeError(
            f"{case.structure}: sanity checksum не совпал: ожидался {case.expected_checksum}, "
            f"получен {result['validation_checksum']}"
        )

    wall_elapsed_ns = int(result["wall_elapsed_ns"])
    cpu_elapsed_ns = int(result["cpu_elapsed_ns"])

    return {
        **metadata,
        "benchmark_schema_version": 2,
        "scenario": case.profile,
        "operation_kind": case.operation_kind,
        "data_size": case.data_size,
        "setup_operation_count": case.setup_operation_count,
        "measured_operation_count": case.measured_operation_count,
        "query_count": case.query_count,
        "command_count": case.command_count,
        "locality": case.locality,
        "density": case.density,
        "insertion_order": case.insertion_order,
        "structure": case.structure,
        "repetition": repetition,
        "wall_elapsed_ns": wall_elapsed_ns,
        "cpu_elapsed_ns": cpu_elapsed_ns,
        "wall_ns_per_operation": wall_elapsed_ns / case.measured_operation_count,
        "cpu_ns_per_operation": cpu_elapsed_ns / case.measured_operation_count,
        "validation_checksum": result["validation_checksum"],
        "measurement_scope": "structure_operations",
        "wall_clock": "steady_clock",
        "cpu_clock": result["cpu_clock"],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--runner",
        "--binary",
        dest="runner",
        type=Path,
        required=True,
        help="Путь к rangequery_benchmark; --binary сохранён как совместимый псевдоним.",
    )
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
    parser.add_argument(
        "--build-config",
        default="unspecified",
        help="Имя конфигурации сборки, записываемое в CSV.",
    )
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
        "compiler_version": subprocess.run(
            [os.environ.get("CXX", "c++"), "--version"],
            text=True,
            capture_output=True,
            check=True,
        ).stdout.splitlines()[0],
        "build_config": args.build_config,
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
                    rows.append(execute_case(args.runner, case, repetition, metadata))
    else:
        for regime in DATA_REGIMES:
            for data_size in regime.data_sizes:
                for case in regime_cases(regime, data_size, args.seed + data_size):
                    for repetition in range(args.repetitions):
                        rows.append(execute_case(args.runner, case, repetition, metadata))

    with args.output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
