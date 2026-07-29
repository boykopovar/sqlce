from tests.infrastructure.scenarios import SdfScenario
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.table_spec import assert_table_matches
from tests.infrastructure.table_spec import build_table
from tests.tables.edge_case_tables import DATETIME_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import FLOAT_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import GUID_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import HUGE_BINARY_TABLE_SPEC
from tests.tables.edge_case_tables import HUGE_TEXT_TABLE_SPEC
from tests.tables.edge_case_tables import INTEGER_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import MANY_BIT_COLUMNS_TABLE_SPEC
from tests.tables.edge_case_tables import MANY_ROWS_TABLE_SPEC
from tests.tables.edge_case_tables import MANY_TABLES_IN_ONE_FILE_SPECS
from tests.tables.edge_case_tables import NULLABLE_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import NUMERIC_EXTREMES_TABLE_SPEC
from tests.tables.edge_case_tables import WIDE_MIXED_TYPES_TABLE_SPEC
from tests.tables.edge_case_tables import WIDE_ROW_PAGE_SPLIT_TABLE_SPEC


def _build_and_check(sdf_scenario: SdfScenario, spec) -> None:
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        build_table(connection, spec, sdf_scenario.version)

    db = sdf_scenario.open_database()
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        assert_table_matches(connection, db, spec)


def test_sdf_edge_case_integer_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, INTEGER_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_numeric_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, NUMERIC_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_float_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, FLOAT_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_datetime_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, DATETIME_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_guid_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, GUID_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_huge_text_beyond_single_page_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, HUGE_TEXT_TABLE_SPEC)


def test_sdf_edge_case_huge_binary_beyond_single_page_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, HUGE_BINARY_TABLE_SPEC)


def test_sdf_edge_case_many_rows_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, MANY_ROWS_TABLE_SPEC)


def test_sdf_edge_case_nullable_extremes_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, NULLABLE_EXTREMES_TABLE_SPEC)


def test_sdf_edge_case_wide_mixed_types_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, WIDE_MIXED_TYPES_TABLE_SPEC)


def test_sdf_edge_case_wide_row_page_split_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, WIDE_ROW_PAGE_SPLIT_TABLE_SPEC)


def test_sdf_edge_case_many_bit_columns_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    _build_and_check(sdf_scenario, MANY_BIT_COLUMNS_TABLE_SPEC)


def test_sdf_edge_case_many_tables_in_one_file_full_structure_matches_source(
        sdf_scenario: SdfScenario,
) -> None:
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        for spec in MANY_TABLES_IN_ONE_FILE_SPECS:
            build_table(connection, spec, sdf_scenario.version)

    db = sdf_scenario.open_database()
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        for spec in MANY_TABLES_IN_ONE_FILE_SPECS:
            assert_table_matches(connection, db, spec)
