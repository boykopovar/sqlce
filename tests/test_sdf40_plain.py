from pathlib import Path

import pytest

from sqlce import SdfDatabase
from tests.sdf_factory import create_plain_database
from tests.sdf_factory import execute_non_query
from tests.sdf_factory import open_connection


def test_sdf40_plain_database_created_file_exists(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    assert path.exists()
    assert path.stat().st_size > 0


def test_sdf40_plain_database_opens_without_password(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    db = SdfDatabase(str(path))

    assert db.list_tables() == []


def test_sdf40_plain_database_lists_created_table(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    connection = open_connection(path)
    try:
        execute_non_query(connection, "CREATE TABLE Sample (Id int, Name nvarchar(50))")
    finally:
        connection.Close()

    db = SdfDatabase(str(path))
    tables = db.list_tables()

    assert "Sample" in tables


def test_sdf40_plain_database_reads_table_rows(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    connection = open_connection(path)
    try:
        execute_non_query(connection, "CREATE TABLE Sample (Id int, Name nvarchar(50))")
        execute_non_query(connection, "INSERT INTO Sample (Id, Name) VALUES (1, N'foo')")
        execute_non_query(connection, "INSERT INTO Sample (Id, Name) VALUES (2, N'bar')")
    finally:
        connection.Close()

    db = SdfDatabase(str(path))
    rows = db.read_table("Sample")

    assert len(rows) == 2
    assert rows[0]["Id"] == 1
    assert rows[0]["Name"] == "foo"
    assert rows[1]["Id"] == 2
    assert rows[1]["Name"] == "bar"


def test_sdf40_plain_database_table_schema_columns(sdf_dir: Path) -> None:
    path = create_plain_database(sdf_dir)

    connection = open_connection(path)
    try:
        execute_non_query(connection, "CREATE TABLE Sample (Id int, Name nvarchar(50))")
    finally:
        connection.Close()

    db = SdfDatabase(str(path))
    schema = db.table_schema("Sample")

    names = [column.name for column in schema]

    assert "Id" in names
    assert "Name" in names
