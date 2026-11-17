from pathlib import Path
from sqlce import SqlceDatabase
from tests.infrastructure.custom_sdf import assert_table_rows_match_runtime
from tests.infrastructure.sdf_factory import RemoteConnection
from tests.infrastructure.sdf_factory import create_plain_database
from tests.infrastructure.sdf_factory import execute_non_query
from tests.infrastructure.sdf_factory import open_connection


def test_sdf_reads_row_continuation_on_page_0x41(sdf_dir: Path) -> None:
    path: Path = create_plain_database(sdf_dir, prefix="row_continuation")
    connection: RemoteConnection
    with open_connection(path) as connection:
        execute_non_query(
            connection,
            "CREATE TABLE WideRows (Id int NOT NULL PRIMARY KEY, "
            "A nvarchar(2000), B nvarchar(2000), C nvarchar(2000))",
        )
        row_id: int
        for row_id in range(80):
            execute_non_query(connection, f"INSERT INTO WideRows VALUES ({row_id}, N'a', N'b', N'c')")

        execute_non_query(connection, "CREATE TABLE Fill (Id int IDENTITY PRIMARY KEY, Payload ntext NOT NULL)")
        padding: str = "x" * 50000
        execute_non_query(connection, f"INSERT INTO Fill (Payload) VALUES (N'{padding}')")
        copy_rows: str = "INSERT INTO Fill (Payload) SELECT Payload FROM Fill"
        for _ in range(11):
            execute_non_query(connection, copy_rows)
        execute_non_query(connection, f"{copy_rows} WHERE Id <= 565")

        a: str = "a" * 1800
        b: str = "b" * 1800
        c: str = "c" * 1800
        execute_non_query(connection, f"UPDATE WideRows SET A = N'{a}', B = N'{b}', C = N'{c}'")
    db: SqlceDatabase = SqlceDatabase(str(path))
    with open_connection(path) as connection:
        assert_table_rows_match_runtime(connection, db, "WideRows")
