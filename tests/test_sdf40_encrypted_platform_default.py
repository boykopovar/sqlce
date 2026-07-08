from pathlib import Path

import pytest

from sqlce import InvalidPasswordError
from sqlce import SdfDatabase
from tests.sample_tables import SAMPLE_TABLE_SPEC
from tests.sdf_factory import create_platform_default_encrypted_database
from tests.sdf_factory import open_connection
from tests.table_spec import assert_table_matches
from tests.table_spec import build_table

PASSWORD = "P1atform!Default"


def test_sdf40_encrypted_platform_default_database_created_file_exists(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PASSWORD)

    assert path.exists()
    assert path.stat().st_size > 0


def test_sdf40_encrypted_platform_default_database_opens_with_password(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PASSWORD)

    db = SdfDatabase(str(path), PASSWORD)

    assert db.list_tables() == []


def test_sdf40_encrypted_platform_default_database_rejects_wrong_password(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PASSWORD)

    with pytest.raises(InvalidPasswordError):
        SdfDatabase(str(path), "wrong-password")


def test_sdf40_encrypted_platform_default_database_full_structure_matches_source(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PASSWORD)

    connection = open_connection(path, PASSWORD)
    try:
        build_table(connection, SAMPLE_TABLE_SPEC)
    finally:
        connection.Close()

    db = SdfDatabase(str(path), PASSWORD)

    assert_table_matches(db, SAMPLE_TABLE_SPEC)
