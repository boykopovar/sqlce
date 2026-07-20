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
    type_name: str
    declared_size: int
    precision: Optional[int] = None
    scale: Optional[int] = None
    check_declared_size: bool = True


@dataclass(frozen=True)
class TableSpec:
    name: str
    columns: Tuple[ColumnSpec, ...]
    rows: Tuple[Tuple[Any, ...], ...] = field(default_factory=tuple)

    def create_table_sql(self) -> str:
        column_definitions = ", ".join(f"{column.name} {column.sql_type}" for column in self.columns)
        return f"CREATE TABLE {self.name} ({column_definitions})"

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
    numeric_type = f"numeric({column.precision}, {column.scale})" if column.precision is not None else "numeric"
    return f"CAST({sign}{digits_text} AS {numeric_type})"


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


def build_table(connection, spec: TableSpec, version: str = "4.0") -> None:
    from tests.utils.sdf_factory import encode_parameterized_operation
    from tests.utils.sdf_factory import encode_sql_operation
    from tests.utils.sdf_factory import execute_batch

    operations = [encode_sql_operation(spec.create_table_sql())]

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

    execute_batch(connection, operations, version)


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
        if expected_column.check_declared_size:
            assert actual_column.declared_size == expected_column.declared_size
        assert actual_column.precision == expected_column.precision
        assert actual_column.scale == expected_column.scale

    actual_rows = db.read_table(spec.name)
    expected_rows = spec.expected_rows_as_dicts()

    assert len(actual_rows) == len(expected_rows)

    for actual_row, expected_row in zip(actual_rows, expected_rows):
        assert actual_row == expected_row
