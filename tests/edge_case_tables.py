import datetime
import decimal
import uuid

from tests.table_spec import ColumnSpec
from tests.table_spec import TableSpec

HUGE_TEXT_CHAR_COUNT = 50000
HUGE_BINARY_BYTE_COUNT = 4096 * 20

_HUGE_TEXT_UNIT = "abcdefghij0123456789"
_HUGE_TEXT_VALUE = (_HUGE_TEXT_UNIT * (HUGE_TEXT_CHAR_COUNT // len(_HUGE_TEXT_UNIT)))[:HUGE_TEXT_CHAR_COUNT]
_HUGE_BINARY_VALUE = bytes(range(256)) * (HUGE_BINARY_BYTE_COUNT // 256)

INTEGER_EXTREMES_TABLE_SPEC = TableSpec(
    name="IntegerExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="TinyMin", sql_type="tinyint", type_name="tinyint", declared_size=1),
        ColumnSpec(name="TinyMax", sql_type="tinyint", type_name="tinyint", declared_size=1),
        ColumnSpec(name="SmallMin", sql_type="smallint", type_name="smallint", declared_size=2),
        ColumnSpec(name="SmallMax", sql_type="smallint", type_name="smallint", declared_size=2),
        ColumnSpec(name="IntMin", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="IntMax", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="BigMin", sql_type="bigint", type_name="bigint", declared_size=8),
        ColumnSpec(name="BigMax", sql_type="bigint", type_name="bigint", declared_size=8),
    ),
    rows=(
        (1, 0, 255, -32768, 32767, -2147483648, 2147483647, -9223372036854775808, 9223372036854775807),
    ),
)

NUMERIC_EXTREMES_TABLE_SPEC = TableSpec(
    name="NumericExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(
            name="HugePositive",
            sql_type="numeric(38, 0)",
            type_name="numeric/decimal",
            declared_size=21,
            precision=38,
            scale=0,
            check_declared_size=False,
        ),
        ColumnSpec(
            name="HugeNegative",
            sql_type="numeric(38, 0)",
            type_name="numeric/decimal",
            declared_size=21,
            precision=38,
            scale=0,
            check_declared_size=False,
        ),
        ColumnSpec(
            name="TinyFraction",
            sql_type="numeric(38, 37)",
            type_name="numeric/decimal",
            declared_size=21,
            precision=38,
            scale=37,
            check_declared_size=False,
        ),
        ColumnSpec(
            name="NegativeTinyFraction",
            sql_type="numeric(38, 37)",
            type_name="numeric/decimal",
            declared_size=21,
            precision=38,
            scale=37,
            check_declared_size=False,
        ),
        ColumnSpec(
            name="ZeroExact",
            sql_type="numeric(38, 0)",
            type_name="numeric/decimal",
            declared_size=21,
            precision=38,
            scale=0,
            check_declared_size=False,
        ),
    ),
    rows=(
        (
            1,
            decimal.Decimal("99999999999999999999999999999999999999"),
            decimal.Decimal("-99999999999999999999999999999999999999"),
            decimal.Decimal("9.9999999999999999999999999999999999999"),
            decimal.Decimal("-9.9999999999999999999999999999999999999"),
            decimal.Decimal("0"),
        ),
    ),
)

FLOAT_EXTREMES_TABLE_SPEC = TableSpec(
    name="FloatExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="LargePositive", sql_type="float", type_name="float/real", declared_size=8),
        ColumnSpec(name="LargeNegative", sql_type="float", type_name="float/real", declared_size=8),
        ColumnSpec(name="TinyPositive", sql_type="float", type_name="float/real", declared_size=8),
        ColumnSpec(name="TinyNegative", sql_type="float", type_name="float/real", declared_size=8),
    ),
    rows=(
        (1, 1.7976931348623157e+308, -1.7976931348623157e+308, 2.2250738585072014e-308, -2.2250738585072014e-308),
    ),
)

DATETIME_EXTREMES_TABLE_SPEC = TableSpec(
    name="DateTimeExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="EarliestSupported", sql_type="datetime", type_name="datetime", declared_size=8),
        ColumnSpec(name="LatestSupported", sql_type="datetime", type_name="datetime", declared_size=8),
    ),
    rows=(
        (
            1,
            datetime.datetime(1753, 1, 1, 0, 0, 0),
            datetime.datetime(9999, 12, 31, 23, 59, 59),
        ),
    ),
)

GUID_EXTREMES_TABLE_SPEC = TableSpec(
    name="GuidExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(
            name="AllZeros",
            sql_type="uniqueidentifier",
            type_name="uniqueidentifier",
            declared_size=16,
        ),
        ColumnSpec(
            name="AllOnes",
            sql_type="uniqueidentifier",
            type_name="uniqueidentifier",
            declared_size=16,
        ),
    ),
    rows=(
        (
            1,
            uuid.UUID(int=0),
            uuid.UUID(int=(2 ** 128) - 1),
        ),
    ),
)

HUGE_TEXT_TABLE_SPEC = TableSpec(
    name="HugeText",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(
            name="Content",
            sql_type="ntext",
            type_name="ntext",
            declared_size=0,
            check_declared_size=False,
        ),
    ),
    rows=(
        (1, _HUGE_TEXT_VALUE),
    ),
)

HUGE_BINARY_TABLE_SPEC = TableSpec(
    name="HugeBinary",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(
            name="Content",
            sql_type="image",
            type_name="image",
            declared_size=0,
            check_declared_size=False,
        ),
    ),
    rows=(
        (1, _HUGE_BINARY_VALUE),
    ),
)

MANY_ROWS_COUNT = 2000

MANY_ROWS_TABLE_SPEC = TableSpec(
    name="ManyRows",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Payload", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
    ),
    rows=tuple((row_index, f"row-{row_index}") for row_index in range(MANY_ROWS_COUNT)),
)

NULLABLE_EXTREMES_TABLE_SPEC = TableSpec(
    name="NullableExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="NullableInt", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="NullableText", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ColumnSpec(name="NullableBinary", sql_type="varbinary(50)", type_name="varbinary", declared_size=50),
    ),
    rows=(
        (1, None, None, None),
        (2, 0, "", b""),
        (3, -1, "not null", b"\x00\x01\x02"),
    ),
)

WIDE_MIXED_TYPES_TABLE_SPEC = TableSpec(
    name="WideMixedTypes",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="A", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ColumnSpec(name="B", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="C", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="D", sql_type="datetime", type_name="datetime", declared_size=8),
        ColumnSpec(name="E", sql_type="money", type_name="money", declared_size=8),
        ColumnSpec(name="F", sql_type="float", type_name="float/real", declared_size=8),
        ColumnSpec(
            name="G",
            sql_type="numeric(10, 2)",
            type_name="numeric/decimal",
            declared_size=6,
            precision=10,
            scale=2,
        ),
        ColumnSpec(name="H", sql_type="uniqueidentifier", type_name="uniqueidentifier", declared_size=16),
        ColumnSpec(name="I", sql_type="ntext", type_name="ntext", declared_size=0, check_declared_size=False),
        ColumnSpec(name="J", sql_type="image", type_name="image", declared_size=0, check_declared_size=False),
        ColumnSpec(name="K", sql_type="smallint", type_name="smallint", declared_size=2),
        ColumnSpec(name="L", sql_type="tinyint", type_name="tinyint", declared_size=1),
        ColumnSpec(name="M", sql_type="bigint", type_name="bigint", declared_size=8),
    ),
    rows=(
        (1, None, None, None, None, None, None, decimal.Decimal("0.00"), None, None, None, None, None, None),
        (
            2,
            "not null",
            42,
            True,
            datetime.datetime(2020, 1, 1, 12, 30, 0),
            decimal.Decimal("100.00"),
            1.5,
            decimal.Decimal("1500.50"),
            uuid.UUID(int=(2 ** 128) - 1),
            "long text value",
            b"\x00\x01\x02\x03",
            32767,
            255,
            9223372036854775807,
        ),
    ),
)

MANY_BIT_COLUMNS_TABLE_SPEC = TableSpec(
    name="ManyBitColumns",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="B1", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B2", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B3", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B4", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B5", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B6", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B7", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B8", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B9", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="B10", sql_type="bit", type_name="bit", declared_size=1),
    ),
    rows=(
        (1, False, False, False, False, False, False, False, False, False, False),
        (2, True, True, True, True, True, True, True, True, True, True),
        (3, True, False, True, False, True, False, True, False, True, False),
        (4, True, False, False, False, False, False, False, False, False, False),
        (5, False, False, False, False, False, False, False, False, True, False),
        (6, False, False, False, False, False, False, False, False, False, True),
    ),
)

_WIDE_ROW_COL_CHAR_COUNT = 1000
_WIDE_ROW_FILL_UNIT = "Привет мир проверка длинной строки "


def _wide_row_fill(char_count: int) -> str:
    repeated = _WIDE_ROW_FILL_UNIT * (char_count // len(_WIDE_ROW_FILL_UNIT) + 1)
    return repeated[:char_count]


WIDE_ROW_PAGE_SPLIT_TABLE_SPEC = TableSpec(
    name="WideRowPageSplit",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(
            name="Col1",
            sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})",
            type_name="nvarchar/nchar",
            declared_size=_WIDE_ROW_COL_CHAR_COUNT * 2,
        ),
        ColumnSpec(
            name="Col2",
            sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})",
            type_name="nvarchar/nchar",
            declared_size=_WIDE_ROW_COL_CHAR_COUNT * 2,
        ),
        ColumnSpec(
            name="Col3",
            sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})",
            type_name="nvarchar/nchar",
            declared_size=_WIDE_ROW_COL_CHAR_COUNT * 2,
        ),
        ColumnSpec(
            name="Col4",
            sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})",
            type_name="nvarchar/nchar",
            declared_size=_WIDE_ROW_COL_CHAR_COUNT * 2,
        ),
    ),
    rows=(
        (
            1,
            _wide_row_fill(_WIDE_ROW_COL_CHAR_COUNT),
            _wide_row_fill(_WIDE_ROW_COL_CHAR_COUNT)[::-1],
            _wide_row_fill(_WIDE_ROW_COL_CHAR_COUNT),
            _wide_row_fill(_WIDE_ROW_COL_CHAR_COUNT)[::-1],
        ),
        (
            2,
            "second row col1",
            "second row col2",
            "second row col3",
            "second row col4",
        ),
    ),
)

MANY_TABLES_IN_ONE_FILE_SPECS = (
    TableSpec(
        name="MultiTableA",
        columns=(
            ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
            ColumnSpec(name="Val", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ),
        rows=((1, "A row"),),
    ),
    TableSpec(
        name="MultiTableB",
        columns=(
            ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
            ColumnSpec(name="Val", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ),
        rows=((1, "B row"),),
    ),
    TableSpec(
        name="MultiTableC",
        columns=(
            ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
            ColumnSpec(name="Val", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ),
        rows=((1, "C row"),),
    ),
)
