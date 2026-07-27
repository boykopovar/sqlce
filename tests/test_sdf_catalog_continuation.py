from tests.catalog_continuation_tables import WIDE_CATALOG_TABLE_SPECS
from tests.utils.scenarios import SdfScenario
from tests.utils.table_spec import assert_table_matches
from tests.utils.table_spec import build_table


def test_sdf_catalog_survives_many_wide_tables_with_indexes(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        for spec in WIDE_CATALOG_TABLE_SPECS:
            build_table(connection, spec, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    expected_names = {spec.name for spec in WIDE_CATALOG_TABLE_SPECS}
    actual_names = set(db.list_tables())

    missing_names = expected_names - actual_names
    unexpected_names = actual_names - expected_names

    assert not missing_names, f"catalog rows lost for tables: {sorted(missing_names)}"
    assert not unexpected_names, f"phantom catalog rows found: {sorted(unexpected_names)}"
    assert actual_names == expected_names


def test_sdf_catalog_survives_many_wide_tables_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        for spec in WIDE_CATALOG_TABLE_SPECS:
            build_table(connection, spec, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    for spec in WIDE_CATALOG_TABLE_SPECS:
        assert_table_matches(db, spec)


def test_sdf_catalog_no_empty_or_blank_table_names_after_many_wide_tables(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        for spec in WIDE_CATALOG_TABLE_SPECS:
            build_table(connection, spec, sdf_scenario.version)
    finally:
        connection.Close()

    db = sdf_scenario.open_database()

    for name in db.list_tables():
        assert name is not None
        assert name.strip() != ""
