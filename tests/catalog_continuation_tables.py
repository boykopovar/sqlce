from tests.utils.table_spec import ColumnSpec
from tests.utils.table_spec import IndexSpec
from tests.utils.table_spec import TableSpec

WIDE_CATALOG_TABLE_COUNT = 200
COLUMNS_PER_TABLE = 6

_NAME_ALPHABET = "AbcdefghijKlmnopqrstUvwxyzAbcdefghijKlmnopqrstUvwxyzAbcdefghijKlmnopqrstUvwxyz"

_MAX_IDENTIFIER_LENGTH = 100


def _padded_name(prefix: str, table_index: int, unit_index: int, target_length: int) -> str:
    base = f"{prefix}{table_index:03d}_{unit_index:02d}_"
    remaining = max(target_length - len(base), 1)
    filler = (_NAME_ALPHABET * ((remaining // len(_NAME_ALPHABET)) + 1))[:remaining]
    name = base + filler
    return name[:_MAX_IDENTIFIER_LENGTH]


def _table_name_length(table_index: int) -> int:
    return 5 + (table_index % 96)


def _column_name_length(table_index: int, column_index: int) -> int:
    return 5 + ((table_index * 7 + column_index * 13) % 96)


def _index_name_length(table_index: int) -> int:
    return 5 + ((table_index * 11) % 96)


def _build_wide_table_spec(table_index: int) -> TableSpec:
    table_name = _padded_name("WCT", table_index, 0, _table_name_length(table_index))

    columns = [ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4)]
    key_column_names = ["Id"]

    for column_index in range(COLUMNS_PER_TABLE):
        column_name = _padded_name(
            "Col", table_index, column_index, _column_name_length(table_index, column_index)
        )
        columns.append(
            ColumnSpec(
                name=column_name,
                sql_type="nvarchar(80)",
                type_name="nvarchar/nchar",
                declared_size=160,
            )
        )

    row_values = [table_index]
    for column_index in range(COLUMNS_PER_TABLE):
        row_values.append(f"t{table_index}_c{column_index}_value")

    index_name = _padded_name("IX", table_index, 0, _index_name_length(table_index))
    index_target_column = columns[1].name

    return TableSpec(
        name=table_name,
        columns=tuple(columns),
        rows=(tuple(row_values),),
        primary_key_columns=tuple(key_column_names),
        indexes=(IndexSpec(name=index_name, columns=(index_target_column,)),),
    )


WIDE_CATALOG_TABLE_SPECS = tuple(
    _build_wide_table_spec(table_index) for table_index in range(WIDE_CATALOG_TABLE_COUNT)
)
