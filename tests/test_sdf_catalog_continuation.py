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

    for name in actual_names:
        assert name is not None
        assert name.strip() != ""

    for spec in WIDE_CATALOG_TABLE_SPECS:
        assert_table_matches(db, spec)
