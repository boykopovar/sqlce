from pathlib import Path

import pytest

from sqlce import SdfDatabase
from tests.sample_tables import SAMPLE_TABLE_SPEC
from tests.sdf_factory import SDF_VERSION_35
from tests.sdf_factory import SDF_VERSION_40
from tests.sdf_factory import create_plain_database
from tests.sdf_factory import open_connection
from tests.table_spec import assert_table_matches
from tests.table_spec import build_table

SDF_VERSIONS = pytest.mark.parametrize("version", [SDF_VERSION_35, SDF_VERSION_40])


@SDF_VERSIONS
def test_sdf_plain_database_created_file_exists(sdf_dir: Path, version: str) -> None:
    path = create_plain_database(sdf_dir, version=version)

    assert path.exists()
    assert path.stat().st_size > 0


@SDF_VERSIONS
def test_sdf_plain_database_opens_without_password(sdf_dir: Path, version: str) -> None:
    path = create_plain_database(sdf_dir, version=version)

    db = SdfDatabase(str(path))

    assert db.list_tables() == []


@SDF_VERSIONS
def test_sdf_plain_database_full_structure_matches_source(sdf_dir: Path, version: str) -> None:
    path = create_plain_database(sdf_dir, version=version)

    connection = open_connection(path, version=version)
    try:
        build_table(connection, SAMPLE_TABLE_SPEC, version=version)
    finally:
        connection.Close()

    db = SdfDatabase(str(path))

    assert_table_matches(db, SAMPLE_TABLE_SPEC)
