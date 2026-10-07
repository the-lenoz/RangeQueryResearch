#!/usr/bin/env python3
"""Генерирует детерминированные потоки команд для CLI rangequery."""

from __future__ import annotations

import argparse
import random
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class QueryWorkload:
    """Поток с точным размером множества и заданным числом запросов."""

    payload: str
    data_size: int
    query_count: int


def _make_values(rng: random.Random, data_size: int) -> list[int]:
    return rng.sample(range(-1_000_000_000, 1_000_000_001), data_size)


def _hot_index(rng: random.Random, data_size: int) -> int:
    """Выбирает один из 5 % ключей вокруг центра множества."""
    radius = max(1, data_size // 40)
    center = data_size // 2
    return min(data_size - 1, max(0, center + rng.randint(-radius, radius)))


def build_query_workload(
    data_size: int,
    query_count: int,
    seed: int,
    locality: str = "uniform",
    query_kind: str = "mixed",
) -> QueryWorkload:
    """Создаёт множество точного размера, затем выполняет только запросы к нему.

    ``uniform`` выбирает обращения по всему множеству. ``hotspot`` концентрирует
    обращения вокруг центральных 5 % ключей, что моделирует локальность доступа.
    """
    if data_size < 1 or query_count < 1:
        raise ValueError("data_size и query_count должны быть положительными")

    rng = random.Random(seed)
    values = _make_values(rng, data_size)
    sorted_values = sorted(values)
    commands = [f"k {value}" for value in values]
    query_kinds = ("rank", "select", "range")

    for _ in range(query_count):
        kind = rng.choice(query_kinds) if query_kind == "mixed" else query_kind
        index = rng.randrange(data_size) if locality == "uniform" else _hot_index(rng, data_size)
        value = sorted_values[index]

        if kind == "select":
            commands.append(f"m {index + 1}")
        elif kind == "rank":
            commands.append(f"n {value}")
        else:
            width = max(1, data_size // 100)
            left_index = max(0, index - width)
            right_index = min(data_size - 1, index + width)
            commands.append(f"q {sorted_values[left_index]} {sorted_values[right_index]}")

    return QueryWorkload("\n".join(commands) + "\n", data_size, query_count)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--data-size", type=int, required=True)
    parser.add_argument("--query-count", type=int, required=True)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--locality", choices=("uniform", "hotspot"), default="uniform")
    parser.add_argument("--query-kind", choices=("mixed", "rank", "select", "range"), default="mixed")
    args = parser.parse_args()

    try:
        workload = build_query_workload(
            args.data_size, args.query_count, args.seed, args.locality, args.query_kind
        )
    except ValueError as error:
        parser.error(str(error))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(workload.payload, encoding="utf-8")


if __name__ == "__main__":
    main()
