from typing import List

import pytest

from sqlce import LazyLob
from sqlce import TableRowIterator
from tests.conftest import SdfScenario
from tests.table_spec import ColumnSpec
from tests.table_spec import TableSpec
from tests.table_spec import build_table

LAZY_MANY_ROWS_COUNT = 500

LAZY_MANY_ROWS_TABLE_SPEC = TableSpec(
    name="LazyManyRows",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Payload", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
    ),
    rows=tuple((row_index, f"row-{row_index}") for row_index in range(LAZY_MANY_ROWS_COUNT)),
)

LAZY_EMPTY_TABLE_SPEC = TableSpec(
    name="LazyEmptyTable",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Payload", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
    ),
    rows=(),
)

_LOB_TEXT_UNIT = "the quick brown fox jumps over the lazy dog "
_LOB_TEXT_A = (_LOB_TEXT_UNIT * (30000 // len(_LOB_TEXT_UNIT) + 1))[:30000]
_LOB_TEXT_B = (_LOB_TEXT_UNIT[::-1] * (45000 // len(_LOB_TEXT_UNIT) + 1))[:45000]
_LOB_TEXT_C = "short inline text"

_LOB_BINARY_A = bytes((i * 7) % 256 for i in range(4096 * 6))
_LOB_BINARY_B = bytes((i * 13 + 3) % 256 for i in range(4096 * 9))
_LOB_BINARY_C = b"\x01\x02\x03short"

LAZY_MULTI_LOB_TABLE_SPEC = TableSpec(
    name="LazyMultiLob",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Content", sql_type="ntext", type_name="ntext", declared_size=0, check_declared_size=False),
        ColumnSpec(name="Payload", sql_type="image", type_name="image", declared_size=0, check_declared_size=False),
    ),
    rows=(
        (1, _LOB_TEXT_A, _LOB_BINARY_A),
        (2, _LOB_TEXT_B, _LOB_BINARY_B),
        (3, _LOB_TEXT_C, _LOB_BINARY_C),
    ),
)

LAZY_NULL_LOB_TABLE_SPEC = TableSpec(
    name="LazyNullLob",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Content", sql_type="ntext", type_name="ntext", declared_size=0, check_declared_size=False),
        ColumnSpec(name="Payload", sql_type="image", type_name="image", declared_size=0, check_declared_size=False),
    ),
    rows=(
        (1, None, None),
        (2, "", b""),
        (3, "not null", b"\x00\x01\x02"),
    ),
)


def _build(sdf_scenario: SdfScenario, spec: TableSpec):
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, spec, sdf_scenario.version)
    finally:
        connection.Close()
    return sdf_scenario.open_database()


def test_iterate_table_returns_a_real_iterator_not_a_list(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MANY_ROWS_TABLE_SPEC)

    iterator = db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name)

    assert isinstance(iterator, TableRowIterator)
    assert not isinstance(iterator, list)
    assert iter(iterator) is iterator
    assert hasattr(iterator, "__next__")


def test_iterate_table_matches_read_table_content_and_order(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MANY_ROWS_TABLE_SPEC)

    eager_rows = db.read_table(LAZY_MANY_ROWS_TABLE_SPEC.name)
    lazy_rows = list(db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name))

    assert len(lazy_rows) == len(eager_rows) == LAZY_MANY_ROWS_COUNT
    assert lazy_rows == eager_rows

    for index, row in enumerate(lazy_rows):
        assert row["Id"] == index
        assert row["Payload"] == f"row-{index}"


def test_iterate_table_can_be_partially_consumed_without_reading_the_whole_table(
        sdf_scenario: SdfScenario,
) -> None:
    db = _build(sdf_scenario, LAZY_MANY_ROWS_TABLE_SPEC)

    iterator = db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name)

    first_three = [next(iterator) for _ in range(3)]

    assert [row["Id"] for row in first_three] == [0, 1, 2]
    assert [row["Payload"] for row in first_three] == ["row-0", "row-1", "row-2"]

    fourth = next(iterator)
    assert fourth["Id"] == 3
    assert fourth["Payload"] == "row-3"


def test_iterate_table_two_independent_iterators_do_not_interfere(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MANY_ROWS_TABLE_SPEC)

    iterator_a = db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name)
    iterator_b = db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name)

    first_from_a = next(iterator_a)
    first_from_b = next(iterator_b)
    second_from_a = next(iterator_a)

    assert first_from_a["Id"] == 0
    assert first_from_b["Id"] == 0
    assert second_from_a["Id"] == 1


def test_iterate_table_raises_stop_iteration_after_last_row(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_NULL_LOB_TABLE_SPEC)

    iterator = db.iterate_table(LAZY_NULL_LOB_TABLE_SPEC.name)
    rows = list(iterator)

    assert len(rows) == len(LAZY_NULL_LOB_TABLE_SPEC.rows)
    with pytest.raises(StopIteration):
        next(iterator)


def test_iterate_table_on_empty_table_stops_immediately(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_EMPTY_TABLE_SPEC)

    iterator = db.iterate_table(LAZY_EMPTY_TABLE_SPEC.name)

    assert list(iterator) == []
    with pytest.raises(StopIteration):
        next(db.iterate_table(LAZY_EMPTY_TABLE_SPEC.name))


def test_iterate_table_survives_database_reference_going_out_of_scope(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MANY_ROWS_TABLE_SPEC)

    def _make_iterator() -> TableRowIterator:
        local_db = db
        return local_db.iterate_table(LAZY_MANY_ROWS_TABLE_SPEC.name)

    iterator = _make_iterator()

    rows = list(iterator)
    assert len(rows) == LAZY_MANY_ROWS_COUNT
    assert rows[0]["Id"] == 0
    assert rows[-1]["Id"] == LAZY_MANY_ROWS_COUNT - 1


def test_iterate_table_huge_lob_values_match_source_via_lazy_lob(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MULTI_LOB_TABLE_SPEC)

    rows = list(db.iterate_table(LAZY_MULTI_LOB_TABLE_SPEC.name))
    assert len(rows) == 3

    expected_text = {1: _LOB_TEXT_A, 2: _LOB_TEXT_B, 3: _LOB_TEXT_C}
    expected_binary = {1: _LOB_BINARY_A, 2: _LOB_BINARY_B, 3: _LOB_BINARY_C}

    for row in rows:
        content = row["Content"]
        payload = row["Payload"]

        assert isinstance(content, LazyLob)
        assert isinstance(payload, LazyLob)

        assert content.read() == expected_text[row["Id"]]
        assert payload.read() == expected_binary[row["Id"]]


def test_iterate_table_reading_lobs_out_of_row_order_stays_consistent(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MULTI_LOB_TABLE_SPEC)

    rows = list(db.iterate_table(LAZY_MULTI_LOB_TABLE_SPEC.name))
    by_id = {row["Id"]: row for row in rows}

    expected_text = {1: _LOB_TEXT_A, 2: _LOB_TEXT_B, 3: _LOB_TEXT_C}
    expected_binary = {1: _LOB_BINARY_A, 2: _LOB_BINARY_B, 3: _LOB_BINARY_C}

    reversed_ids = sorted(by_id.keys(), reverse=True)
    for row_id in reversed_ids:
        row = by_id[row_id]
        assert row["Payload"].read() == expected_binary[row_id]

    for row_id in reversed_ids:
        row = by_id[row_id]
        assert row["Content"].read() == expected_text[row_id]


def test_iterate_table_lazy_lob_read_is_idempotent(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MULTI_LOB_TABLE_SPEC)

    rows = list(db.iterate_table(LAZY_MULTI_LOB_TABLE_SPEC.name))
    first_row = next(row for row in rows if row["Id"] == 1)

    first_read = first_row["Content"].read()
    second_read = first_row["Content"].read()
    third_read = first_row["Payload"].read()
    fourth_read = first_row["Payload"].read()

    assert first_read == second_read == _LOB_TEXT_A
    assert third_read == fourth_read == _LOB_BINARY_A


def test_iterate_table_lazy_lob_read_chunks_matches_read(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MULTI_LOB_TABLE_SPEC)

    rows = list(db.iterate_table(LAZY_MULTI_LOB_TABLE_SPEC.name))
    row = next(r for r in rows if r["Id"] == 2)

    text_chunks: List[bytes] = row["Content"].read_chunks()
    binary_chunks: List[bytes] = row["Payload"].read_chunks()

    assert all(isinstance(chunk, bytes) for chunk in text_chunks)
    assert all(isinstance(chunk, bytes) for chunk in binary_chunks)

    assert len(text_chunks) >= 1
    assert len(binary_chunks) >= 1

    reassembled_binary = b"".join(binary_chunks)
    assert reassembled_binary == _LOB_BINARY_B
    assert reassembled_binary == row["Payload"].read()

    reassembled_text_bytes = b"".join(text_chunks)
    assert len(reassembled_text_bytes) > 0
    assert row["Content"].read() == _LOB_TEXT_B


def test_iterate_table_null_lob_values_are_none_not_lazy_lob(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_NULL_LOB_TABLE_SPEC)

    rows = list(db.iterate_table(LAZY_NULL_LOB_TABLE_SPEC.name))
    by_id = {row["Id"]: row for row in rows}

    assert by_id[1]["Content"] is None
    assert by_id[1]["Payload"] is None

    assert isinstance(by_id[2]["Content"], LazyLob)
    assert by_id[2]["Content"].read() == ""
    assert isinstance(by_id[2]["Payload"], LazyLob)
    assert by_id[2]["Payload"].read() == b""

    assert by_id[3]["Content"].read() == "not null"
    assert by_id[3]["Payload"].read() == b"\x00\x01\x02"


def test_iterate_table_matches_read_table_for_lob_columns(sdf_scenario: SdfScenario) -> None:
    db = _build(sdf_scenario, LAZY_MULTI_LOB_TABLE_SPEC)

    lazy_rows = list(db.iterate_table(LAZY_MULTI_LOB_TABLE_SPEC.name))
    eager_rows = db.read_table(LAZY_MULTI_LOB_TABLE_SPEC.name)

    assert len(lazy_rows) == len(eager_rows)

    for lazy_row, eager_row in zip(lazy_rows, eager_rows):
        assert lazy_row["Id"] == eager_row["Id"]
        assert lazy_row["Content"].read() == eager_row["Content"].read()
        assert lazy_row["Payload"].read() == eager_row["Payload"].read()
