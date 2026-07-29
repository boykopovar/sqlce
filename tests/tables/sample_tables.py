from datetime import datetime
from uuid import UUID

from tests.infrastructure.table_spec import ColumnSpec
from tests.infrastructure.table_spec import TableSpec

SAMPLE_TABLE_SPEC = TableSpec(
    name="Sample",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Name", sql_type="nvarchar(50)"),
        ColumnSpec(name="Score", sql_type="float"),
    ),
    rows=(
        (1, "foo", 1.5),
        (2, "bar", 2.25),
    ),
)

ALL_COLUMN_TYPES_SPEC = TableSpec(
    name="AllColumnTypes",
    columns=(
        ColumnSpec(name="TinyIntCol", sql_type="tinyint"),
        ColumnSpec(name="SmallIntCol", sql_type="smallint"),
        ColumnSpec(name="IntCol", sql_type="int"),
        ColumnSpec(name="BigIntCol", sql_type="bigint"),
        ColumnSpec(name="NCharCol", sql_type="nchar(10)"),
        ColumnSpec(name="NVarCharCol", sql_type="nvarchar(50)"),
        ColumnSpec(name="NTextCol", sql_type="ntext"),
        ColumnSpec(name="BinaryCol", sql_type="binary(16)"),
        ColumnSpec(name="VarBinaryCol", sql_type="varbinary(100)"),
        ColumnSpec(name="ImageCol", sql_type="image"),
        ColumnSpec(name="DateTimeCol", sql_type="datetime"),
        ColumnSpec(name="UniqueIdentifierCol", sql_type="uniqueidentifier"),
        ColumnSpec(name="BitCol", sql_type="bit"),
        ColumnSpec(name="RealCol", sql_type="real"),
        ColumnSpec(name="FloatCol", sql_type="float"),
        ColumnSpec(name="MoneyCol", sql_type="money"),
        ColumnSpec(name="NumericCol", sql_type="numeric(10, 2)"),
        ColumnSpec(name="RowVersionCol", sql_type="rowversion"),
    ),
    rows=(
        (1, 100, 1000, 100000, "test", "value", "text", b"binary", b"varbinary", b"image", datetime(2023, 1, 1, 12, 0, 0), UUID("12345678-1234-1234-1234-123456789012"), 1, 1.5, 2.5, 100.50, 123.45, None),
    ),
)
