import pytest

from tests.infrastructure.scenarios import SdfScenario
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.table_spec import assert_table_matches
from tests.infrastructure.table_spec import build_table
from tests.tables.sample_tables import SAMPLE_TABLE_SPEC


@pytest.mark.parametrize("do_compact", [False, True])
def test_sdf_compact_full_structure_matches_source(
        sdf_scenario: SdfScenario, do_compact: bool,
) -> None:
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        build_table(connection, SAMPLE_TABLE_SPEC, sdf_scenario.version)

    if do_compact:
        sdf_scenario.compact()

    db = sdf_scenario.open_database()
    with open_connection(sdf_scenario.path, sdf_scenario.password, sdf_scenario.version) as connection:
        assert_table_matches(connection, db, SAMPLE_TABLE_SPEC)
