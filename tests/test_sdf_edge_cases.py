from tests.utils.scenarios import SdfScenario
from tests.edge_case_tables import DATETIME_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import FLOAT_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import GUID_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import HUGE_BINARY_TABLE_SPEC
from tests.edge_case_tables import HUGE_TEXT_TABLE_SPEC
from tests.edge_case_tables import INTEGER_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import MANY_BIT_COLUMNS_TABLE_SPEC
from tests.edge_case_tables import MANY_ROWS_TABLE_SPEC
from tests.edge_case_tables import MANY_TABLES_IN_ONE_FILE_SPECS
from tests.edge_case_tables import NULLABLE_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import NUMERIC_EXTREMES_TABLE_SPEC
from tests.edge_case_tables import WIDE_MIXED_TYPES_TABLE_SPEC
from tests.edge_case_tables import WIDE_ROW_PAGE_SPLIT_TABLE_SPEC
from tests.utils.table_spec import assert_table_matches
from tests.utils.table_spec import build_table


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


def test_sdf_edge_case_wide_mixed_types_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, WIDE_MIXED_TYPES_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, WIDE_MIXED_TYPES_TABLE_SPEC)


def test_sdf_edge_case_wide_row_page_split_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, WIDE_ROW_PAGE_SPLIT_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, WIDE_ROW_PAGE_SPLIT_TABLE_SPEC)


def test_sdf_edge_case_many_bit_columns_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, MANY_BIT_COLUMNS_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    assert_table_matches(db, MANY_BIT_COLUMNS_TABLE_SPEC)


def test_sdf_edge_case_many_tables_in_one_file_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        for spec in MANY_TABLES_IN_ONE_FILE_SPECS:
            build_table(connection, spec, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    for spec in MANY_TABLES_IN_ONE_FILE_SPECS:
        assert_table_matches(db, spec)
