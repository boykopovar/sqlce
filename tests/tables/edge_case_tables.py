import datetime
import decimal
import uuid

from tests.infrastructure.table_spec import ColumnSpec
from tests.infrastructure.table_spec import TableSpec

HUGE_TEXT_CHAR_COUNT = 50000
HUGE_BINARY_BYTE_COUNT = 4096 * 20

_HUGE_TEXT_UNIT = "abcdefghij0123456789"
_HUGE_TEXT_VALUE = (_HUGE_TEXT_UNIT * (HUGE_TEXT_CHAR_COUNT // len(_HUGE_TEXT_UNIT)))[:HUGE_TEXT_CHAR_COUNT]
_HUGE_BINARY_VALUE = bytes(range(256)) * (HUGE_BINARY_BYTE_COUNT // 256)

INTEGER_EXTREMES_TABLE_SPEC = TableSpec(
    name="IntegerExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="TinyMin", sql_type="tinyint"),
        ColumnSpec(name="TinyMax", sql_type="tinyint"),
        ColumnSpec(name="SmallMin", sql_type="smallint"),
        ColumnSpec(name="SmallMax", sql_type="smallint"),
        ColumnSpec(name="IntMin", sql_type="int"),
        ColumnSpec(name="IntMax", sql_type="int"),
        ColumnSpec(name="BigMin", sql_type="bigint"),
        ColumnSpec(name="BigMax", sql_type="bigint"),
    ),
    rows=(
        (1, 0, 255, -32768, 32767, -2147483648, 2147483647, -9223372036854775808, 9223372036854775807),
    ),
)

NUMERIC_EXTREMES_TABLE_SPEC = TableSpec(
    name="NumericExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="HugePositive", sql_type="numeric(38, 0)"),
        ColumnSpec(name="HugeNegative", sql_type="numeric(38, 0)"),
        ColumnSpec(name="TinyFraction", sql_type="numeric(38, 37)"),
        ColumnSpec(name="NegativeTinyFraction", sql_type="numeric(38, 37)"),
        ColumnSpec(name="ZeroExact", sql_type="numeric(38, 0)"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="LargePositive", sql_type="float"),
        ColumnSpec(name="LargeNegative", sql_type="float"),
        ColumnSpec(name="TinyPositive", sql_type="float"),
        ColumnSpec(name="TinyNegative", sql_type="float"),
    ),
    rows=(
        (1, 1.7976931348623157e+308, -1.7976931348623157e+308, 2.2250738585072014e-308, -2.2250738585072014e-308),
    ),
)

DATETIME_EXTREMES_TABLE_SPEC = TableSpec(
    name="DateTimeExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="EarliestSupported", sql_type="datetime"),
        ColumnSpec(name="LatestSupported", sql_type="datetime"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="AllZeros", sql_type="uniqueidentifier"),
        ColumnSpec(name="AllOnes", sql_type="uniqueidentifier"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Content", sql_type="ntext"),
    ),
    rows=(
        (1, _HUGE_TEXT_VALUE),
    ),
)

HUGE_BINARY_TABLE_SPEC = TableSpec(
    name="HugeBinary",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Content", sql_type="image"),
    ),
    rows=(
        (1, _HUGE_BINARY_VALUE),
    ),
)

MANY_ROWS_COUNT = 2000

MANY_ROWS_TABLE_SPEC = TableSpec(
    name="ManyRows",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Payload", sql_type="nvarchar(50)"),
    ),
    rows=tuple((row_index, f"row-{row_index}") for row_index in range(MANY_ROWS_COUNT)),
)

NULLABLE_EXTREMES_TABLE_SPEC = TableSpec(
    name="NullableExtremes",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="NullableInt", sql_type="int"),
        ColumnSpec(name="NullableText", sql_type="nvarchar(50)"),
        ColumnSpec(name="NullableBinary", sql_type="varbinary(50)"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="A", sql_type="nvarchar(50)"),
        ColumnSpec(name="B", sql_type="int"),
        ColumnSpec(name="C", sql_type="bit"),
        ColumnSpec(name="D", sql_type="datetime"),
        ColumnSpec(name="E", sql_type="money"),
        ColumnSpec(name="F", sql_type="float"),
        ColumnSpec(name="G", sql_type="numeric(10, 2)"),
        ColumnSpec(name="H", sql_type="uniqueidentifier"),
        ColumnSpec(name="I", sql_type="ntext"),
        ColumnSpec(name="J", sql_type="image"),
        ColumnSpec(name="K", sql_type="smallint"),
        ColumnSpec(name="L", sql_type="tinyint"),
        ColumnSpec(name="M", sql_type="bigint"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="B1", sql_type="bit"),
        ColumnSpec(name="B2", sql_type="bit"),
        ColumnSpec(name="B3", sql_type="bit"),
        ColumnSpec(name="B4", sql_type="bit"),
        ColumnSpec(name="B5", sql_type="bit"),
        ColumnSpec(name="B6", sql_type="bit"),
        ColumnSpec(name="B7", sql_type="bit"),
        ColumnSpec(name="B8", sql_type="bit"),
        ColumnSpec(name="B9", sql_type="bit"),
        ColumnSpec(name="B10", sql_type="bit"),
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
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Col1", sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})"),
        ColumnSpec(name="Col2", sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})"),
        ColumnSpec(name="Col3", sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})"),
        ColumnSpec(name="Col4", sql_type=f"nvarchar({_WIDE_ROW_COL_CHAR_COUNT})"),
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
            ColumnSpec(name="Id", sql_type="int"),
            ColumnSpec(name="Val", sql_type="nvarchar(50)"),
        ),
        rows=((1, "A row"),),
    ),
    TableSpec(
        name="MultiTableB",
        columns=(
            ColumnSpec(name="Id", sql_type="int"),
            ColumnSpec(name="Val", sql_type="nvarchar(50)"),
        ),
        rows=((1, "B row"),),
    ),
    TableSpec(
        name="MultiTableC",
        columns=(
            ColumnSpec(name="Id", sql_type="int"),
            ColumnSpec(name="Val", sql_type="nvarchar(50)"),
        ),
        rows=((1, "C row"),),
    ),
)
