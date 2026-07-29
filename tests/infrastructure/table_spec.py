import decimal
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


@dataclass(frozen=True)
class IndexSpec:
    name: str
    columns: Tuple[str, ...]
    unique: bool = False

    def create_index_sql(self, table_name: str) -> str:
        column_list = ", ".join(self.columns)
        unique_keyword = "UNIQUE " if self.unique else ""
        return f"CREATE {unique_keyword}INDEX {self.name} ON {table_name} ({column_list})"


@dataclass(frozen=True)
class TableSpec:
    name: str
    columns: Tuple[ColumnSpec, ...]
    rows: Tuple[Tuple[Any, ...], ...] = field(default_factory=tuple)
    primary_key_columns: Tuple[str, ...] = field(default_factory=tuple)
    indexes: Tuple[IndexSpec, ...] = field(default_factory=tuple)

    def create_table_sql(self) -> str:
        column_definitions = ", ".join(
            f"{column.name} {column.sql_type} NOT NULL"
            if column.name in self.primary_key_columns
            else f"{column.name} {column.sql_type}"
            for column in self.columns
        )
        return f"CREATE TABLE {self.name} ({column_definitions})"

    def primary_key_sql(self) -> Optional[str]:
        if not self.primary_key_columns:
            return None
        key_columns = ", ".join(self.primary_key_columns)
        return f"ALTER TABLE {self.name} ADD CONSTRAINT PK_{self.name} PRIMARY KEY ({key_columns})"

    def index_sql_statements(self) -> List[str]:
        return [index.create_index_sql(self.name) for index in self.indexes]

    def insert_row_parameters(self, row: Tuple[Any, ...]) -> "InsertPlan":
        parameter_columns: List[ColumnSpec] = []
        parameter_names: List[str] = []
        parameter_values: List[Any] = []
        literal_assignments: List[Tuple[str, str]] = []

        for column, value in zip(self.columns, row):
            if isinstance(value, decimal.Decimal):
                literal_assignments.append((column.name, _decimal_cast_literal(value, column)))
            else:
                parameter_name = f"@{column.name}"
                parameter_columns.append(column)
                parameter_names.append(parameter_name)
                parameter_values.append(value)

        return InsertPlan(
            table_name=self.name,
            parameter_columns=tuple(parameter_columns),
            parameter_names=tuple(parameter_names),
            parameter_values=tuple(parameter_values),
            literal_assignments=tuple(literal_assignments),
        )

    def expected_rows_as_dicts(self) -> List[Dict[str, Any]]:
        return [dict(zip((column.name for column in self.columns), row)) for row in self.rows]


@dataclass(frozen=True)
class InsertPlan:
    table_name: str
    parameter_columns: Tuple[ColumnSpec, ...]
    parameter_names: Tuple[str, ...]
    parameter_values: Tuple[Any, ...]
    literal_assignments: Tuple[Tuple[str, str], ...]

    def has_parameterized_part(self) -> bool:
        return len(self.parameter_columns) > 0

    def parameterized_insert_sql(self) -> str:
        column_names = ", ".join(column.name for column in self.parameter_columns)
        placeholders = ", ".join(self.parameter_names)
        return f"INSERT INTO {self.table_name} ({column_names}) VALUES ({placeholders})"

    def literal_update_sql_statements(self, key_column: str, key_value: Any) -> List[str]:
        if not self.literal_assignments:
            return []
        key_literal = _literal_key_value(key_value)
        statements = []
        for column_name, literal in self.literal_assignments:
            statements.append(
                f"UPDATE {self.table_name} SET {column_name} = {literal} "
                f"WHERE {key_column} = {key_literal}"
            )
        return statements

    def literal_only_insert_sql(self) -> str:
        column_names = ", ".join(name for name, _ in self.literal_assignments)
        values = ", ".join(literal for _, literal in self.literal_assignments)
        return f"INSERT INTO {self.table_name} ({column_names}) VALUES ({values})"


def _decimal_cast_literal(value: decimal.Decimal, column: ColumnSpec) -> str:
    precision, scale = _numeric_precision_and_scale(column.sql_type)
    digits, exponent = _decimal_digits_and_exponent(value)
    sign = "-" if value.is_signed() else ""
    if exponent >= 0:
        digits_text = digits + ("0" * exponent)
    else:
        split_at = len(digits) + exponent
        if split_at <= 0:
            digits_text = "0." + ("0" * -split_at) + digits
        else:
            digits_text = digits[:split_at] + "." + digits[split_at:]
    numeric_type = f"numeric({precision}, {scale})" if precision is not None else "numeric"
    return f"CAST({sign}{digits_text} AS {numeric_type})"


def _numeric_precision_and_scale(sql_type: str) -> Tuple[Optional[int], Optional[int]]:
    base_type, _, arguments = sql_type.partition("(")
    if base_type.strip().lower() not in ("numeric", "decimal") or not arguments:
        return None, None
    parts = arguments.rstrip(")").split(",")
    precision = int(parts[0].strip())
    scale = int(parts[1].strip()) if len(parts) > 1 else 0
    return precision, scale


def _decimal_digits_and_exponent(value: decimal.Decimal) -> Tuple[str, int]:
    sign, digits, exponent = value.as_tuple()
    digits_text = "".join(str(digit) for digit in digits)
    return digits_text, int(exponent)


def _literal_key_value(value: Any) -> str:
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, int):
        return str(value)
    raise TypeError(f"unsupported key value type for literal update: {type(value)!r}")


def _row_insert_operations(spec: "TableSpec"):
    from tests.infrastructure.sdf_factory import encode_parameterized_operation
    from tests.infrastructure.sdf_factory import encode_sql_operation

    operations = []
    key_column = spec.columns[0].name
    for row in spec.rows:
        plan = spec.insert_row_parameters(row)
        if plan.has_parameterized_part():
            operations.append(
                encode_parameterized_operation(
                    plan.parameterized_insert_sql(),
                    plan.parameter_columns,
                    plan.parameter_names,
                    plan.parameter_values,
                )
            )
            for statement in plan.literal_update_sql_statements(key_column, row[0]):
                operations.append(encode_sql_operation(statement))
        else:
            operations.append(encode_sql_operation(plan.literal_only_insert_sql()))
    return operations


def _schema_operations(spec: "TableSpec"):
    from tests.infrastructure.sdf_factory import encode_sql_operation

    operations = [encode_sql_operation(spec.create_table_sql())]

    primary_key_sql = spec.primary_key_sql()
    if primary_key_sql is not None:
        operations.append(encode_sql_operation(primary_key_sql))

    for index_sql in spec.index_sql_statements():
        operations.append(encode_sql_operation(index_sql))

    return operations


def build_table(connection, spec: TableSpec, version: str = "4.0") -> None:
    from tests.infrastructure.sdf_factory import execute_batch

    operations = _schema_operations(spec)
    operations.extend(_row_insert_operations(spec))

    execute_batch(connection, operations, version)


_DECOY_COLUMN_SQL_TYPE = "int"


def _decoy_column_name(slot: int) -> str:
    return f"ZzDecoyCol{slot}"


def build_table_via_column_history(connection, spec: TableSpec, version: str = "4.0") -> None:
    from tests.infrastructure.sdf_factory import encode_sql_operation
    from tests.infrastructure.sdf_factory import execute_batch

    real_columns = spec.columns
    operations = []
    decoy_slot = 0

    def add_decoy() -> str:
        nonlocal decoy_slot
        decoy_slot += 1
        name = _decoy_column_name(decoy_slot)
        operations.append(
            encode_sql_operation(f"ALTER TABLE {spec.name} ADD {name} {_DECOY_COLUMN_SQL_TYPE}")
        )
        return name

    def drop_column(name: str) -> None:
        operations.append(encode_sql_operation(f"ALTER TABLE {spec.name} DROP COLUMN {name}"))

    def _column_definition(column: ColumnSpec) -> str:
        if column.name in spec.primary_key_columns:
            return f"{column.name} {column.sql_type} NOT NULL"
        return f"{column.name} {column.sql_type}"

    first_column = real_columns[0]
    create_columns_sql = ", ".join(
        [
            _column_definition(first_column),
            f"{_decoy_column_name(1)} {_DECOY_COLUMN_SQL_TYPE}",
            f"{_decoy_column_name(2)} {_DECOY_COLUMN_SQL_TYPE}",
        ]
    )
    operations.append(encode_sql_operation(f"CREATE TABLE {spec.name} ({create_columns_sql})"))
    decoy_slot = 2

    decoy_a = _decoy_column_name(1)
    decoy_b = _decoy_column_name(2)
    drop_column(decoy_b)

    surviving_decoys = [decoy_a]

    for column in real_columns[1:]:
        operations.append(
            encode_sql_operation(f"ALTER TABLE {spec.name} ADD {_column_definition(column)}")
        )
        fresh_decoy = add_decoy()
        surviving_decoys.append(fresh_decoy)

        drop_column(fresh_decoy)
        surviving_decoys.remove(fresh_decoy)

        if len(surviving_decoys) > 1:
            oldest_surviving = surviving_decoys.pop(0)
            drop_column(oldest_surviving)

    for remaining_decoy in surviving_decoys:
        drop_column(remaining_decoy)

    primary_key_sql = spec.primary_key_sql()
    if primary_key_sql is not None:
        operations.append(encode_sql_operation(primary_key_sql))

    for index_sql in spec.index_sql_statements():
        operations.append(encode_sql_operation(index_sql))

    operations.extend(_row_insert_operations(spec))

    execute_batch(connection, operations, version)


@dataclass(frozen=True)
class RuntimeColumnSchema:
    ordinal: int
    name: str
    type_name: str
    declared_size: Optional[int]
    precision: Optional[int]
    scale: Optional[int]


def _column_summary(columns) -> List[Tuple[Any, ...]]:
    return [
        (column.ordinal, column.name, column.type_name, column.declared_size, column.precision, column.scale)
        for column in columns
    ]


def assert_schemas_equivalent(native_columns, runtime_columns: List[RuntimeColumnSchema]) -> None:
    native_summary = _column_summary(native_columns)
    runtime_summary = _column_summary(runtime_columns)

    assert len(native_columns) == len(runtime_columns), (
        f"column count mismatch\n"
        f"native columns ({len(native_columns)}):  {native_summary}\n"
        f"runtime columns ({len(runtime_columns)}): {runtime_summary}"
    )

    native_ordinals = [column.ordinal for column in native_columns]
    runtime_ordinals = [column.ordinal for column in runtime_columns]
    assert native_ordinals == sorted(native_ordinals)
    assert runtime_ordinals == sorted(runtime_ordinals)

    native_by_ordinal_rank = sorted(zip(native_ordinals, native_columns), key=lambda pair: pair[0])
    runtime_by_ordinal_rank = sorted(zip(runtime_ordinals, runtime_columns), key=lambda pair: pair[0])

    try:
        for (_, native_column), (_, runtime_column) in zip(native_by_ordinal_rank, runtime_by_ordinal_rank):
            assert native_column.name == runtime_column.name
            assert native_column.type_name == runtime_column.type_name
            assert native_column.precision == runtime_column.precision
            assert native_column.scale == runtime_column.scale
            if native_column.declared_size is not None and runtime_column.declared_size is not None:
                assert native_column.declared_size == runtime_column.declared_size
    except AssertionError as error:
        raise AssertionError(
            f"schema mismatch: {error}\n"
            f"native columns:  {native_summary}\n"
            f"runtime columns: {runtime_summary}"
        ) from error


def runtime_columns_for(connection, table_name: str) -> List[RuntimeColumnSchema]:
    from tests.infrastructure.sdf_factory import table_schema_via_runtime

    raw_columns = table_schema_via_runtime(connection, table_name)
    return [RuntimeColumnSchema(**column) for column in raw_columns]


def assert_table_matches_runtime(connection, native_db, table_name: str) -> None:
    native_columns = native_db.table_schema(table_name)
    runtime_columns = runtime_columns_for(connection, table_name)
    assert_schemas_equivalent(native_columns, runtime_columns)


def assert_table_matches(connection, native_db, spec: TableSpec) -> None:
    tables = native_db.list_tables()
    assert spec.name in tables

    assert_table_matches_runtime(connection, native_db, spec.name)

    actual_rows = native_db.read_table(spec.name)
    expected_rows = spec.expected_rows_as_dicts()

    assert len(actual_rows) == len(expected_rows)

    for actual_row, expected_row in zip(actual_rows, expected_rows):
        assert actual_row == expected_row
