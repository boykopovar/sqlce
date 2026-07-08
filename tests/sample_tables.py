from tests.table_spec import ColumnSpec
from tests.table_spec import TableSpec

SAMPLE_TABLE_SPEC = TableSpec(
    name="Sample",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Name", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ColumnSpec(name="Score", sql_type="float", type_name="float/real", declared_size=8),
    ),
    rows=(
        (1, "foo", 1.5),
        (2, "bar", 2.25),
    ),
)
