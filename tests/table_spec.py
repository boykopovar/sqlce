from dataclasses import dataclass
from dataclasses import field
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Tuple


@dataclass(frozen=True)
class ColumnSpec:
    name: str
    sql_type: str
    type_name: str
    declared_size: int
    precision: Optional[int] = None
    scale: Optional[int] = None


@dataclass(frozen=True)
class TableSpec:
    name: str
    columns: Tuple[ColumnSpec, ...]
    rows: Tuple[Tuple[Any, ...], ...] = field(default_factory=tuple)

    def create_table_sql(self) -> str:
        column_definitions = ", ".join(f"{column.name} {column.sql_type}" for column in self.columns)
        return f"CREATE TABLE {self.name} ({column_definitions})"

    def insert_row_sql(self, row: Tuple[Any, ...]) -> str:
        column_names = ", ".join(column.name for column in self.columns)
        values = ", ".join(_format_sql_value(value) for value in row)
        return f"INSERT INTO {self.name} ({column_names}) VALUES ({values})"

    def expected_rows_as_dicts(self) -> List[Dict[str, Any]]:
        return [dict(zip((column.name for column in self.columns), row)) for row in self.rows]


def _format_sql_value(value: Any) -> str:
    if value is None:
        return "NULL"
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, str):
        escaped = value.replace("'", "''")
        return f"N'{escaped}'"
    raise TypeError(f"unsupported value type for sql literal: {type(value)!r}")


def build_table(connection, spec: TableSpec) -> None:
    from tests.sdf_factory import execute_non_query

    execute_non_query(connection, spec.create_table_sql())
    for row in spec.rows:
        execute_non_query(connection, spec.insert_row_sql(row))


def assert_table_matches(db, spec: TableSpec) -> None:
    tables = db.list_tables()
    assert spec.name in tables

    schema = db.table_schema(spec.name)
    assert len(schema) == len(spec.columns)

    ordinals = [column.ordinal for column in schema]
    assert ordinals == sorted(ordinals)
    assert len(set(ordinals)) == len(ordinals)

    for actual_column, expected_column in zip(schema, spec.columns):
        assert actual_column.name == expected_column.name
        assert actual_column.type_name == expected_column.type_name
        assert actual_column.declared_size == expected_column.declared_size
        assert actual_column.precision == expected_column.precision
        assert actual_column.scale == expected_column.scale

    actual_rows = db.read_table(spec.name)
    expected_rows = spec.expected_rows_as_dicts()

    assert len(actual_rows) == len(expected_rows)

    for actual_row, expected_row in zip(actual_rows, expected_rows):
        assert actual_row == expected_row
