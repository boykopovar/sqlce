from tests.utils.table_spec import ColumnSpec
from tests.utils.table_spec import IndexSpec
from tests.utils.table_spec import TableSpec

WIDE_CATALOG_TABLE_COUNT = 24
WIDE_CATALOG_COLUMN_COUNT = 20
COMPOSITE_KEY_COLUMN_COUNT = 4
KEY_COLUMN_CHAR_LENGTH = 40

_LONG_COLUMN_NAME_UNIT = "AbcdefghijKlmnopqrst"


def _long_table_name(table_index: int) -> str:
    return f"WideCatalogTable_{table_index:02d}_LongIdentifierForContinuation"


def _long_column_name(table_index: int, column_index: int) -> str:
    return f"Col_{table_index:02d}_{column_index:02d}_{_LONG_COLUMN_NAME_UNIT}"


def _key_column_name(table_index: int, key_index: int) -> str:
    return f"Key_{table_index:02d}_{key_index:02d}_{_LONG_COLUMN_NAME_UNIT}"


def _index_name(table_index: int) -> str:
    return f"IX_WideCatalog_{table_index:02d}_Secondary"


def _build_wide_table_spec(table_index: int) -> TableSpec:
    columns = [ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4)]

    key_column_names = []
    for key_index in range(COMPOSITE_KEY_COLUMN_COUNT):
        name = _key_column_name(table_index, key_index)
        key_column_names.append(name)
        columns.append(
            ColumnSpec(
                name=name,
                sql_type=f"nvarchar({KEY_COLUMN_CHAR_LENGTH})",
                type_name="nvarchar/nchar",
                declared_size=KEY_COLUMN_CHAR_LENGTH * 2,
            )
        )

    for column_index in range(WIDE_CATALOG_COLUMN_COUNT):
        columns.append(
            ColumnSpec(
                name=_long_column_name(table_index, column_index),
                sql_type="nvarchar(80)",
                type_name="nvarchar/nchar",
                declared_size=160,
            )
        )

    row_values = [table_index]
    for key_index in range(COMPOSITE_KEY_COLUMN_COUNT):
        row_values.append(f"t{table_index}_k{key_index}")
    for column_index in range(WIDE_CATALOG_COLUMN_COUNT):
        row_values.append(f"t{table_index}_c{column_index}_value")

    return TableSpec(
        name=_long_table_name(table_index),
        columns=tuple(columns),
        rows=(tuple(row_values),),
        primary_key_columns=tuple(key_column_names),
        indexes=(IndexSpec(name=_index_name(table_index), columns=tuple(key_column_names[:2])),),
    )


WIDE_CATALOG_TABLE_SPECS = tuple(
    _build_wide_table_spec(table_index) for table_index in range(WIDE_CATALOG_TABLE_COUNT)
)
