import pytest

from sqlce import InvalidPasswordError
from tests.tables.sample_tables import SAMPLE_TABLE_SPEC
from tests.infrastructure.scenarios import SdfScenario
from tests.infrastructure.table_spec import assert_table_matches
from tests.infrastructure.table_spec import build_table


def test_sdf_scenario_database_created_file_exists(sdf_scenario: SdfScenario) -> None:
    assert sdf_scenario.path.exists()
    assert sdf_scenario.path.stat().st_size > 0


def test_sdf_scenario_database_opens_with_correct_password(sdf_scenario: SdfScenario) -> None:
    db = sdf_scenario.open_database()

    assert db.list_tables() == []


def test_sdf_scenario_database_rejects_wrong_password(sdf_scenario: SdfScenario) -> None:
    from sqlce import SqlceDatabase

    if sdf_scenario.password is None:
        return

    with pytest.raises(InvalidPasswordError):
        SqlceDatabase(str(sdf_scenario.path), "wrong-password")


def test_sdf_scenario_full_structure_matches_source(sdf_scenario: SdfScenario) -> None:
    connection = sdf_scenario.open_connection()
    try:
        build_table(connection, SAMPLE_TABLE_SPEC, sdf_scenario.version)
        db = sdf_scenario.open_database()
        assert_table_matches(connection, db, SAMPLE_TABLE_SPEC)
    finally:
        connection.Close()
