from tests.utils.scenarios import SdfScenario
from tests.utils.table_spec import ColumnSpec
from tests.utils.table_spec import TableSpec
from tests.utils.table_spec import assert_table_matches
from tests.utils.table_spec import build_table_via_column_history

ORDINAL_HISTORY_TABLE_SPEC = TableSpec(
    name="OrdinalHistoryAlpha",
    columns=(
        ColumnSpec(name="ColA", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColB", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColC", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ColumnSpec(name="ColD", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="ColE", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColF", sql_type="int", type_name="int", declared_size=4),
    ),
    rows=(
        (1, 10, "value one", True, 100, 1000),
        (2, 20, "value two", False, 200, 2000),
        (3, 30, "value three", False, 300, 3000),
    ),
)

WIDE_ORDINAL_HISTORY_TABLE_SPEC = TableSpec(
    name="OrdinalHistoryBeta",
    columns=(
        ColumnSpec(name="ColOne", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColTwo", sql_type="nvarchar(50)", type_name="nvarchar/nchar", declared_size=100),
        ColumnSpec(name="ColThree", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColFour", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="ColFive", sql_type="smallint", type_name="smallint", declared_size=2),
        ColumnSpec(name="ColSix", sql_type="bigint", type_name="bigint", declared_size=8),
        ColumnSpec(name="ColSeven", sql_type="float", type_name="float/real", declared_size=8),
    ),
    rows=(
        (1, "foo", 10, True, 100, 1000, 1.5),
        (2, "bar", 20, False, 200, 2000, 2.5),
    ),
)

TWO_COLUMN_ORDINAL_HISTORY_TABLE_SPEC = TableSpec(
    name="OrdinalHistoryGamma",
    columns=(
        ColumnSpec(name="ColKey", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="ColPayload", sql_type="int", type_name="int", declared_size=4),
    ),
    rows=(
        (1, 42),
        (2, 43),
    ),
)


def test_sdf_column_ordinal_survives_add_drop_history(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table_via_column_history(connection, ORDINAL_HISTORY_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, ORDINAL_HISTORY_TABLE_SPEC)


def test_sdf_column_ordinal_survives_add_drop_history_wide_table(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table_via_column_history(connection, WIDE_ORDINAL_HISTORY_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, WIDE_ORDINAL_HISTORY_TABLE_SPEC)


def test_sdf_column_ordinal_survives_add_drop_history_two_columns(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table_via_column_history(connection, TWO_COLUMN_ORDINAL_HISTORY_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, TWO_COLUMN_ORDINAL_HISTORY_TABLE_SPEC)


def test_sdf_column_ordinal_history_ordinals_are_dense_and_ordered(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table_via_column_history(connection, WIDE_ORDINAL_HISTORY_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()
    schema = db.table_schema(WIDE_ORDINAL_HISTORY_TABLE_SPEC.name)

    ordinals = [column.ordinal for column in schema]
    assert len(set(ordinals)) == len(ordinals)
    assert ordinals == sorted(ordinals)

    names_in_ordinal_order = [column.name for _, column in sorted(zip(ordinals, schema), key=lambda pair: pair[0])]
    expected_names = [column.name for column in WIDE_ORDINAL_HISTORY_TABLE_SPEC.columns]
    assert names_in_ordinal_order == expected_names
