#!/usr/bin/env python3
"""Generate deterministic command streams for the rangequery CLI."""

from __future__ import annotations

import argparse
import bisect
import random
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class QueryWorkload:
    """Workload with an exact set size and query count."""

    payload: str
    data_size: int
    query_count: int
    rank_query_count: int
    select_query_count: int
    range_query_count: int
    expected_checksum: str


@dataclass(frozen=True)
class InsertWorkload:
    """Insertion workload with an exact final set size."""

    payload: str
    data_size: int
    command_count: int
    expected_checksum: str


@dataclass(frozen=True)
class DynamicWorkload:
    """Workload with setup insertions and mixed measured operations."""

    payload: str
    initial_data_size: int
    final_data_size: int
    measured_operation_count: int
    insert_count: int
    successful_insert_count: int
    query_count: int
    rank_query_count: int
    select_query_count: int
    range_query_count: int
    expected_checksum: str


FNV_OFFSET = 14_695_981_039_346_656_037
FNV_PRIME = 1_099_511_628_211
UINT64_MASK = (1 << 64) - 1


def checksum_record(checksum: int, code: str, has_value: bool, value: int) -> int:
    """Add a typed result to the stable FNV-1a checksum."""
    encoded = bytes((ord(code), int(has_value))) + (value & UINT64_MASK).to_bytes(8, "little")
    for byte in encoded:
        checksum ^= byte
        checksum = (checksum * FNV_PRIME) & UINT64_MASK
    return checksum


def checksum_text(checksum: int) -> str:
    return f"{checksum:016x}"


def state_validation_checksum(checksum: int, values: list[int]) -> int:
    """Add the set size and inexpensive state probes to the checksum."""
    sorted_values = sorted(set(values))
    checksum = checksum_record(checksum, "s", True, len(sorted_values))
    indices = sorted(
        {
            1,
            1 + len(sorted_values) // 4,
            1 + len(sorted_values) // 2,
            1 + (3 * len(sorted_values)) // 4,
            len(sorted_values),
        }
    )
    for index in indices:
        checksum = checksum_record(checksum, "m", True, sorted_values[index - 1])
    probes = [
        -(1 << 31),
        0,
        (1 << 31) - 1,
        sorted_values[0],
        sorted_values[len(sorted_values) // 2],
        sorted_values[-1],
    ]
    for probe in probes:
        checksum = checksum_record(checksum, "n", True, bisect.bisect_left(sorted_values, probe))
    return checksum


def insert_validation_checksum(values: list[int]) -> str:
    """Compute the expected probes of the final set."""
    return checksum_text(state_validation_checksum(FNV_OFFSET, values))


def _make_values(rng: random.Random, data_size: int, density: str) -> list[int]:
    if density == "dense":
        values = list(range(-data_size // 2, -data_size // 2 + data_size))
        rng.shuffle(values)
        return values
    if density == "sparse":
        return rng.sample(range(-1_000_000_000, 1_000_000_001), data_size)
    raise ValueError("density must be dense or sparse")


def _hot_index(rng: random.Random, data_size: int) -> int:
    """Choose a key among the central 5% of the set."""
    radius = max(1, data_size // 40)
    center = data_size // 2
    return min(data_size - 1, max(0, center + rng.randint(-radius, radius)))


def build_query_workload(
    data_size: int,
    query_count: int,
    seed: int,
    locality: str = "uniform",
    query_kind: str = "mixed",
    density: str = "sparse",
) -> QueryWorkload:
    """Build a set of exact size, then issue queries only.

    ``uniform`` accesses the full set. ``hotspot`` concentrates accesses
    around the central 5% of keys to model locality.
    """
    if data_size < 1 or query_count < 1:
        raise ValueError("data_size and query_count must be positive")

    rng = random.Random(seed)
    values = _make_values(rng, data_size, density)
    sorted_values = sorted(values)
    commands = [f"k {value}" for value in values]
    query_kinds = ("rank", "select", "range")
    query_kind_counts = {kind: 0 for kind in query_kinds}
    checksum = FNV_OFFSET

    for _ in range(query_count):
        kind = rng.choice(query_kinds) if query_kind == "mixed" else query_kind
        query_kind_counts[kind] += 1
        index = rng.randrange(data_size) if locality == "uniform" else _hot_index(rng, data_size)
        value = sorted_values[index]

        if kind == "select":
            commands.append(f"m {index + 1}")
            checksum = checksum_record(checksum, "m", True, value)
        elif kind == "rank":
            commands.append(f"n {value}")
            checksum = checksum_record(checksum, "n", True, index)
        else:
            width = max(1, data_size // 100)
            left_index = max(0, index - width)
            right_index = min(data_size - 1, index + width)
            commands.append(f"q {sorted_values[left_index]} {sorted_values[right_index]}")
            checksum = checksum_record(checksum, "q", True, right_index - left_index)

    return QueryWorkload(
        "\n".join(commands) + "\n",
        data_size,
        query_count,
        query_kind_counts["rank"],
        query_kind_counts["select"],
        query_kind_counts["range"],
        checksum_text(checksum),
    )


def build_insert_workload(
    data_size: int,
    seed: int,
    density: str = "sparse",
    insertion_order: str = "random",
) -> InsertWorkload:
    """Generate unique insertions in random, ascending, or descending order."""
    if data_size < 1:
        raise ValueError("data_size must be positive")

    values = _make_values(random.Random(seed), data_size, density)
    if insertion_order == "ascending":
        values.sort()
    elif insertion_order == "descending":
        values.sort(reverse=True)
    elif insertion_order != "random":
        raise ValueError("insertion_order must be random, ascending, or descending")

    return InsertWorkload(
        "\n".join(f"k {value}" for value in values) + "\n",
        data_size,
        len(values),
        insert_validation_checksum(values),
    )


def build_dynamic_workload(
    initial_data_size: int,
    measured_operation_count: int,
    insert_percentage: int,
    seed: int,
) -> DynamicWorkload:
    """Grow a set with an exact successful insertion fraction and balanced query types."""
    if initial_data_size < 1 or measured_operation_count < 1:
        raise ValueError("Set size and measured operation count must be positive")
    if insert_percentage <= 0 or insert_percentage >= 100:
        raise ValueError("Insertion percentage must be an integer from 1 to 99")
    if measured_operation_count * insert_percentage % 100 != 0:
        raise ValueError("Operation count cannot represent the requested insertion percentage exactly")

    insert_count = measured_operation_count * insert_percentage // 100
    query_count = measured_operation_count - insert_count
    rng = random.Random(seed)
    all_values = _make_values(rng, initial_data_size + insert_count, "sparse")
    initial_values = all_values[:initial_data_size]
    pending_insertions = iter(all_values[initial_data_size:])
    sorted_values = sorted(initial_values)
    commands = [f"k {value}" for value in initial_values]

    query_kinds = ["rank", "select", "range"]
    measured_kinds = ["insert"] * insert_count
    measured_kinds.extend(query_kinds[index % len(query_kinds)] for index in range(query_count))
    rng.shuffle(measured_kinds)

    counts = {kind: 0 for kind in query_kinds}
    checksum = FNV_OFFSET
    for kind in measured_kinds:
        if kind == "insert":
            value = next(pending_insertions)
            commands.append(f"k {value}")
            bisect.insort(sorted_values, value)
            continue

        counts[kind] += 1
        index = rng.randrange(len(sorted_values))
        value = sorted_values[index]
        if kind == "select":
            commands.append(f"m {index + 1}")
            checksum = checksum_record(checksum, "m", True, value)
        elif kind == "rank":
            commands.append(f"n {value}")
            checksum = checksum_record(checksum, "n", True, index)
        else:
            width = max(1, len(sorted_values) // 100)
            left_index = max(0, index - width)
            right_index = min(len(sorted_values) - 1, index + width)
            commands.append(f"q {sorted_values[left_index]} {sorted_values[right_index]}")
            checksum = checksum_record(checksum, "q", True, right_index - left_index)

    checksum = state_validation_checksum(checksum, sorted_values)
    return DynamicWorkload(
        payload="\n".join(commands) + "\n",
        initial_data_size=initial_data_size,
        final_data_size=len(sorted_values),
        measured_operation_count=measured_operation_count,
        insert_count=insert_count,
        successful_insert_count=insert_count,
        query_count=query_count,
        rank_query_count=counts["rank"],
        select_query_count=counts["select"],
        range_query_count=counts["range"],
        expected_checksum=checksum_text(checksum),
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--data-size", type=int, required=True)
    parser.add_argument("--query-count", type=int, required=True)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--locality", choices=("uniform", "hotspot"), default="uniform")
    parser.add_argument("--query-kind", choices=("mixed", "rank", "select", "range"), default="mixed")
    parser.add_argument("--density", choices=("dense", "sparse"), default="sparse")
    args = parser.parse_args()

    try:
        workload = build_query_workload(
            args.data_size, args.query_count, args.seed, args.locality, args.query_kind, args.density
        )
    except ValueError as error:
        parser.error(str(error))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(workload.payload, encoding="utf-8")


if __name__ == "__main__":
    main()
