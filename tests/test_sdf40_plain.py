from pathlib import Path

from sqlce import SdfDatabase
from tests.sample_tables import SAMPLE_TABLE_SPEC
from tests.sdf_factory import create_plain_database
from tests.sdf_factory import open_connection
from tests.table_spec import assert_table_matches
from tests.table_spec import build_table


def test_sdf40_plain_database_created_file_exists(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    assert path.exists()
    assert path.stat().st_size > 0


def test_sdf40_plain_database_opens_without_password(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    db = SdfDatabase(str(path))

    assert db.list_tables() == []


def test_sdf40_plain_database_full_structure_matches_source(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    connection = open_connection(path)
    try:
        build_table(connection, SAMPLE_TABLE_SPEC)
    finally:
        connection.Close()

    db = SdfDatabase(str(path))

    assert_table_matches(db, SAMPLE_TABLE_SPEC)
