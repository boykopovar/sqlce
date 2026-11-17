from pathlib import Path
from typing import List
from typing import Tuple

from tests.infrastructure.custom_sdf import assert_runtime_matches_native
from tests.infrastructure.sdf_factory import RemoteConnection
from tests.infrastructure.sdf_factory import create_plain_database
from tests.infrastructure.sdf_factory import encode_sql_operation
from tests.infrastructure.sdf_factory import execute_batch
from tests.infrastructure.sdf_factory import open_connection


def test_sdf_reads_tables_beyond_first_map_b_range(sdf_dir: Path) -> None:
    path: Path = create_plain_database(sdf_dir, prefix="logical_page_mapping")
    row_count: int = 6000
    payload: str = "x" * 1800
    operations: List[Tuple[str, str]] = [encode_sql_operation(
        "CREATE TABLE Padding (Id int NOT NULL PRIMARY KEY, Payload nvarchar(2000) NOT NULL)"
    )]
    operations.extend(
        encode_sql_operation(f"INSERT INTO Padding VALUES ({row_id}, N'{payload}')")
        for row_id in range(row_count)
    )
    operations.extend([
        encode_sql_operation("CREATE TABLE TailMarker (Id int NOT NULL PRIMARY KEY)"),
        encode_sql_operation("INSERT INTO TailMarker VALUES (42)"),
    ])

    connection: RemoteConnection
    with open_connection(path) as connection:
        execute_batch(connection, operations)

    assert_runtime_matches_native(path)
