from pathlib import Path

import pytest

from sqlce import InvalidPasswordError
from sqlce import SdfDatabase
from tests.sdf_factory import create_engine_default_encrypted_database
from tests.sdf_factory import execute_non_query
from tests.sdf_factory import open_connection

PASSWORD = "Eng1ne-Default"


def test_sdf40_encrypted_engine_default_database_created_file_exists(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, PASSWORD)

    assert path.exists()
    assert path.stat().st_size > 0


def test_sdf40_encrypted_engine_default_database_opens_with_password(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, PASSWORD)

    db = SdfDatabase(str(path), PASSWORD)

    assert db.list_tables() == []


def test_sdf40_encrypted_engine_default_database_rejects_wrong_password(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, PASSWORD)

    with pytest.raises(InvalidPasswordError):
        SdfDatabase(str(path), "wrong-password")


def test_sdf40_encrypted_engine_default_database_reads_table_rows(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, PASSWORD)

    connection = open_connection(path, PASSWORD)
    try:
        execute_non_query(connection, "CREATE TABLE Sample (Id int, Name nvarchar(50))")
        execute_non_query(connection, "INSERT INTO Sample (Id, Name) VALUES (1, N'foo')")
    finally:
        connection.Close()

    db = SdfDatabase(str(path), PASSWORD)
    rows = db.read_table("Sample")

    assert len(rows) == 1
    assert rows[0]["Id"] == 1
    assert rows[0]["Name"] == "foo"


def test_sdf40_encrypted_engine_default_database_table_schema(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, PASSWORD)

    connection = open_connection(path, PASSWORD)
    try:
        execute_non_query(connection, "CREATE TABLE Sample (Id int, Name nvarchar(50))")
    finally:
        connection.Close()

    db = SdfDatabase(str(path), PASSWORD)
    schema = db.table_schema("Sample")

    names = [column.name for column in schema]

    assert "Id" in names
    assert "Name" in names
