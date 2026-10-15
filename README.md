# RangeQueryResearch

Исследовательский проект на C++23 для сравнения реализаций динамического множества с поддержкой порядковой статистики.

## Сборка

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Доступны пресеты `debug` для разработки, `release` для бенчмарков и
`sanitizers` для проверки AddressSanitizer и UndefinedBehaviorSanitizer. Все они используют Ninja и
создают изолированные каталоги `build/<preset>`.

```sh
cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

Проект требует CMake 3.28 или новее и компилятор с поддержкой сканирования зависимостей C++ modules. Текущая конфигурация проверена с GCC 16 и CMake 4.4.

## Структура проекта

- `src/` — модули C++, CLI `rangequery` и изолированный runner `rangequery_benchmark`;
- `tests/` — поведенческие тесты и тесты контрактов на GoogleTest;
- `tools/` — детерминированный генератор payload и запускатель бенчмарков;
- `notebooks/benchmark_results.ipynb` — воспроизводимый анализ результатов бенчмарков;
- `AGENTS.md` — актуальные контракты и правила размещения проекта.

Исполняемый файл поддерживает `vector`, `set`, `avl` и `treap`. Все реализации предоставляют одинаковый структурный API и выбираются во время выполнения через `std::variant`; виртуальное наследование не используется. `treap` использует приватный генератор приоритетов с фиксированным seed для воспроизводимости экспериментов.

## Бенчмарки

Для асимптотических графиков используется размер множества `data_size`, а не длина потока команд. Отдельный C++ runner заранее читает и разбирает весь workload. В query-сценариях он также строит множество до запуска таймера и измеряет только `q`, `m` и `n`; в insert-сценариях измеряются все `k`.

Основная метрика — wall time операций структуры по `std::chrono::steady_clock`. Process CPU time записывается как дополнительная диагностика. Parsing, setup query-сценария, запуск и завершение процесса, обмен через stdin/stdout, проверка результата и форматирование JSON в измерение не входят. Аллокации внутри самой операции структуры входят.

```sh
python3 tools/benchmark.py \
  --runner build/release/src/rangequery_benchmark \
  --output benchmark-results/scaling.csv \
  --data-sizes 1000 10000 100000 \
  --query-count 1000 \
  --query-kind rank \
  --density sparse \
  --build-config Release
```

Для проверки локальности обращений используются одинаковые множества среднего размера и большое количество запросов; `hotspot` концентрирует их вокруг центральных 5 % ключей.

```sh
python3 tools/benchmark.py \
  --runner build/release/src/rangequery_benchmark \
  --output benchmark-results/locality.csv \
  --data-sizes 1000 3000 10000 \
  --query-count 100000 \
  --query-kind rank \
  --localities uniform hotspot \
  --build-config Release
```

CSV schema version 2 отдельно хранит `setup_operation_count`, `measured_operation_count` и полное `command_count`. Основная величина для графиков — `wall_ns_per_operation`; `cpu_ns_per_operation` предназначена для диагностики. Поля `measurement_scope=structure_operations`, `wall_clock=steady_clock` и `cpu_clock` фиксируют методику измерения.

После каждого измерения runner вне таймера повторяет запросы либо проверяет итоговое состояние после вставок и возвращает стабильный FNV-1a checksum. Python независимо вычисляет ожидаемое значение и не записывает CSV, если checksum или число измеренных операций не совпали. Эта sanity-проверка защищает результаты эксперимента, но не заменяет полные контрактные тесты.

Сценарий `regimes` сравнивает структуры не по заранее выбранной «лучшей» операции, а по диапазонам размера и плотности. Внутри каждого режима все четыре структуры получают один и тот же входной поток.

| Режим | Размеры | Ключи | Нагрузка |
| --- | --- | --- | --- |
| `small_dense_rank` | 1 000–10 000 | плотные, из непрерывного диапазона | 10 000 запросов ранга |
| `medium_dense_rank` | 10 000–100 000 | плотные | 1 000 запросов ранга |
| `medium_sparse_rank` | 10 000–100 000 | разреженные, по широкому диапазону `int` | 1 000 запросов ранга |
| `medium_sparse_mixed` | 10 000–100 000 | разреженные | поровну `rank`, `select` и запросы интервала |
| `large_sparse_random_growth` | 10 000–100 000 | разреженные | вставки в случайном порядке |
| `large_sparse_ascending_growth` | 10 000–100 000 | разреженные | вставки в возрастающем порядке |
| `large_sparse_descending_growth` | 10 000–100 000 | разреженные | вставки в убывающем порядке |

Пара плотных и разреженных режимов проверяет, влияет ли расположение ключей в их числовом диапазоне. Три режима роста отделяют стоимость структуры от порядка поступления данных: возрастающий поток благоприятен для отсортированного вектора, убывающий требует сдвигать элементы, а случайный ближе к реальному потоку. Плотность и порядок сохраняются в CSV, поэтому результаты нельзя случайно смешать.

```sh
python3 tools/benchmark.py \
  --runner build/release/src/rangequery_benchmark \
  --output benchmark-results/regimes.csv \
  --scenario regimes \
  --build-config Release
```
