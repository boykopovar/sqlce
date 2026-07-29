from pathlib import Path

import pytest

from sqlce import EncryptionMode
from sqlce import SqlceDatabase
from tests.infrastructure.scenarios import ENGINE_DEFAULT_35
from tests.infrastructure.scenarios import ENGINE_DEFAULT_40
from tests.infrastructure.scenarios import PLATFORM_DEFAULT_35
from tests.infrastructure.scenarios import PLATFORM_DEFAULT_40
from tests.infrastructure.scenarios import build_scenario
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.table_spec import ColumnSpec
from tests.infrastructure.table_spec import TableSpec
from tests.infrastructure.table_spec import assert_table_matches
from tests.infrastructure.table_spec import build_table
from tests.tables.sample_tables import SAMPLE_TABLE_SPEC

SECOND_TABLE_SPEC = TableSpec(
    name="Second",
    columns=(
        ColumnSpec(name="Id", sql_type="int"),
        ColumnSpec(name="Flag", sql_type="bit"),
        ColumnSpec(name="Payload", sql_type="varbinary(20)"),
    ),
    rows=(
        (1, True, b"\x01\x02\x03"),
        (2, False, b"\xff\xee\xdd\xcc"),
    ),
)

ENCRYPTED_SCENARIOS_WITH_SPECS = (
    (PLATFORM_DEFAULT_35, SAMPLE_TABLE_SPEC),
    (ENGINE_DEFAULT_35, SECOND_TABLE_SPEC),
    (PLATFORM_DEFAULT_40, SAMPLE_TABLE_SPEC),
    (ENGINE_DEFAULT_40, SECOND_TABLE_SPEC),
)


@pytest.mark.parametrize("scenario_name, table_spec", ENCRYPTED_SCENARIOS_WITH_SPECS)
def test_export_decrypted_matches_source_data(
        sdf_dir: Path,
        scenario_name: str,
        table_spec: TableSpec,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    with open_connection(scenario.path, scenario.password, scenario.version) as connection:
        build_table(connection, table_spec, scenario.version)

    encrypted_db = scenario.open_database()
    assert encrypted_db.get_encryption_mode() != EncryptionMode.NONE

    decrypted_bytes = encrypted_db.export_decrypted()
    assert isinstance(decrypted_bytes, bytes)
    assert len(decrypted_bytes) > 0

    decrypted_path = sdf_dir / f"{scenario_name}_decrypted.sdf"
    decrypted_path.write_bytes(decrypted_bytes)

    decrypted_db = SqlceDatabase(str(decrypted_path))
    assert decrypted_db.get_encryption_mode() == EncryptionMode.NONE

    with open_connection(decrypted_path, None, scenario.version) as decrypted_connection:
        assert_table_matches(decrypted_connection, decrypted_db, table_spec)

    encrypted_rows = encrypted_db.read_table(table_spec.name)
    decrypted_rows = decrypted_db.read_table(table_spec.name)
    assert encrypted_rows == decrypted_rows
