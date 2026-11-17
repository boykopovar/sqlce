from pathlib import Path
from typing import Any
from typing import Dict
from typing import List
from typing import Set
from typing import Tuple

from sqlce import SqlceDatabase
from tests.infrastructure.sdf_factory import RemoteConnection
from tests.infrastructure.sdf_factory import create_plain_database
from tests.infrastructure.sdf_factory import encode_sql_operation
from tests.infrastructure.sdf_factory import execute_batch
from tests.infrastructure.sdf_factory import list_tables_via_runtime
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.sdf_factory import read_rows_via_runtime
from tests.infrastructure.sdf_factory import row_count_via_runtime


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
        expected_tables: Set[str] = set(list_tables_via_runtime(connection))
        expected_count: int = row_count_via_runtime(connection, "Padding")
        expected_marker: List[Dict[str, Any]] = list(read_rows_via_runtime(connection, "TailMarker"))

    assert expected_count == row_count
    assert expected_marker == [{"Id": 42}]

    db: SqlceDatabase = SqlceDatabase(str(path))
    assert set(db.list_tables()) == expected_tables
    actual_count: int = len(db.read_table("Padding"))
    assert actual_count == expected_count
    assert db.read_table("TailMarker") == expected_marker
