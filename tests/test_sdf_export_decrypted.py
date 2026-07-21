from pathlib import Path

import pytest

from sqlce import EncryptionMode
from sqlce import SqlceDatabase
from tests.sample_tables import SAMPLE_TABLE_SPEC
from tests.utils.scenarios import ENGINE_DEFAULT_35
from tests.utils.scenarios import ENGINE_DEFAULT_40
from tests.utils.scenarios import PLATFORM_DEFAULT_35
from tests.utils.scenarios import PLATFORM_DEFAULT_40
from tests.utils.scenarios import build_scenario
from tests.utils.table_spec import ColumnSpec
from tests.utils.table_spec import TableSpec
from tests.utils.table_spec import assert_table_matches
from tests.utils.table_spec import build_table

SECOND_TABLE_SPEC = TableSpec(
    name="Second",
    columns=(
        ColumnSpec(name="Id", sql_type="int", type_name="int", declared_size=4),
        ColumnSpec(name="Flag", sql_type="bit", type_name="bit", declared_size=1),
        ColumnSpec(name="Payload", sql_type="varbinary(20)", type_name="varbinary", declared_size=20),
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
        sdf_dir: Path, scenario_name: str, table_spec: TableSpec,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    connection = scenario.open_connection()
    try:
        build_table(connection, table_spec, scenario.version)
    finally:
        connection.Close()

    encrypted_db = scenario.open_database()
    assert encrypted_db.get_encryption_mode() != EncryptionMode.NONE

    decrypted_bytes = encrypted_db.export_decrypted()
    assert isinstance(decrypted_bytes, bytes)
    assert len(decrypted_bytes) > 0

    decrypted_path = sdf_dir / f"{scenario_name}_decrypted.sdf"
    decrypted_path.write_bytes(decrypted_bytes)

    decrypted_db = SqlceDatabase(str(decrypted_path))
    assert decrypted_db.get_encryption_mode() == EncryptionMode.NONE

    assert_table_matches(decrypted_db, table_spec)

    encrypted_rows = encrypted_db.read_table(table_spec.name)
    decrypted_rows = decrypted_db.read_table(table_spec.name)
    assert encrypted_rows == decrypted_rows
