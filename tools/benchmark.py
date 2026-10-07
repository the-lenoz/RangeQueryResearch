#!/usr/bin/env python3
"""Запускает воспроизводимые бенчмарки rangequery и записывает таблицу результатов."""

from __future__ import annotations

import argparse
import csv
import os
import platform
import subprocess
import time
from pathlib import Path

from generate_payload import build_query_workload


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--data-sizes",
        "--sizes",
        dest="data_sizes",
        type=int,
        nargs="+",
        default=[1_000, 3_000, 10_000],
        help="Размеры множеств; --sizes сохранён как псевдоним.",
    )
    parser.add_argument(
        "--query-count",
        type=int,
        default=10_000,
        help="Число запросов после заполнения; по умолчанию 10 000.",
    )
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--localities", choices=("uniform", "hotspot"), nargs="+", default=["uniform"])
    parser.add_argument("--query-kind", choices=("mixed", "rank", "select", "range"), default="mixed")
    args = parser.parse_args()

    if args.repetitions < 1 or any(size < 1 for size in args.data_sizes):
        parser.error("Размеры множеств и число повторов должны быть положительными")
    if args.query_count is not None and args.query_count < 1:
        parser.error("--query-count должен быть положительным")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, object]] = []
    metadata = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "compiler": os.environ.get("CXX", "c++"),
        "seed": args.seed,
        "query_kind": args.query_kind,
    }

    for data_size in args.data_sizes:
        query_count = args.query_count
        for locality in args.localities:
            workload = build_query_workload(
                data_size=data_size,
                query_count=query_count,
                seed=args.seed + data_size,
                locality=locality,
                query_kind=args.query_kind,
            )
            for structure in ("vector", "set", "avl"):
                for repetition in range(args.repetitions):
                    started = time.perf_counter_ns()
                    completed = subprocess.run(
                        [str(args.binary), "--structure", structure],
                        input=workload.payload,
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    elapsed_ns = time.perf_counter_ns() - started
                    if completed.returncode != 0:
                        raise RuntimeError(f"{structure} завершился с ошибкой: {completed.stderr.strip()}")
                    rows.append(
                        {
                            **metadata,
                            "data_size": workload.data_size,
                            "query_count": workload.query_count,
                            "command_count": workload.data_size + workload.query_count,
                            "locality": locality,
                            "structure": structure,
                            "repetition": repetition,
                            "elapsed_ns": elapsed_ns,
                            "ns_per_query": elapsed_ns / workload.query_count,
                        }
                    )

    with args.output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
