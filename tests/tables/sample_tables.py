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
