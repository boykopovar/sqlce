from tests.conftest import SdfScenario
from tests.edge_case_tables import DATETIME_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import FLOAT_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import GUID_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import HUGE_BINARY_TABLE_SPEC
from tests.edge_case_tables import HUGE_TEXT_TABLE_SPEC
from tests.edge_case_tables import INTEGER_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import MANY_ROWS_TABLE_SPEC
from tests.edge_case_tables import NULLABLE_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import NUMERIC_EXTREMES_TABLE_SPEC
from tests.table_spec import assert_table_matches
from tests.table_spec import build_table


def test_sdf_edge_case_integer_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, INTEGER_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, INTEGER_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_numeric_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, NUMERIC_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, NUMERIC_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_float_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, FLOAT_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, FLOAT_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_datetime_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, DATETIME_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, DATETIME_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_guid_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, GUID_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, GUID_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_huge_text_beyond_single_page_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, HUGE_TEXT_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, HUGE_TEXT_TABLE_SPEC)


def test_sdf_edge_case_huge_binary_beyond_single_page_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, HUGE_BINARY_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, HUGE_BINARY_TABLE_SPEC)


def test_sdf_edge_case_many_rows_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, MANY_ROWS_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, MANY_ROWS_TABLE_SPEC)


def test_sdf_edge_case_nullable_extremes_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, NULLABLE_EXTREMES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, NULLABLE_EXTREMES_TABLE_SPEC)
