import pytest

from tests.tables.sample_tables import SAMPLE_TABLE_SPEC
from tests.infrastructure.scenarios import SdfScenario
from tests.infrastructure.table_spec import assert_table_matches
from tests.infrastructure.table_spec import build_table


@pytest.mark.parametrize("do_compact", [False, True])
def test_sdf_compact_full_structure_matches_source(sdf_scenario: SdfScenario, do_compact: bool) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, SAMPLE_TABLE_SPEC, sdf_scenario.version)
    finally:
        connection.Close()

    if do_compact:
        sdf_scenario.compact()

    db = sdf_scenario.open_database()

    assert_table_matches(db, SAMPLE_TABLE_SPEC)
