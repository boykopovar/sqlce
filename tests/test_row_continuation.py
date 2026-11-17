from pathlib import Path
from struct import unpack_from
from typing import Any
from typing import BinaryIO
from typing import Dict
from typing import Set
from typing import Tuple

from sqlce import SqlceDatabase
from tests.infrastructure.sdf_factory import RemoteConnection
from tests.infrastructure.sdf_factory import create_plain_database
from tests.infrastructure.sdf_factory import execute_non_query
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.sdf_factory import read_rows_via_runtime


def _has_continuation_on_page_0x41(path: Path) -> bool:
    flagged_pages: Set[Tuple[int, int]] = set()
    continuation_targets: Set[Tuple[int, int]] = set()
    page: bytes
    sdf: BinaryIO
    with path.open("rb") as sdf:
        for page in iter(lambda: sdf.read(4096), b""):
            if page[6] not in (0x40, 0x41):
                continue
            owner: int = page[16]
            logical_id: int = unpack_from("<I", page, 4)[0] & 0xFFFFF
            if page[6] == 0x41:
                flagged_pages.add((owner, logical_id))
            slot_count: int = unpack_from("<I", page, 20)[0] & 0xFFF
            slot_index: int
            for slot_index in range(slot_count):
                slot: int = unpack_from("<I", page, 4092 - 4 * slot_index)[0]
                if slot & 0x01000000 or ((slot >> 12) & 0xFFF) <= 4:
                    continue
                target: int = unpack_from("<I", page, (slot & 0xFFF) + 24)[0] >> 12
                if target:
                    continuation_targets.add((owner, target))
    return bool(flagged_pages & continuation_targets)


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
        expected: Dict[int, Dict[str, Any]] = {
            row["Id"]: row for row in read_rows_via_runtime(connection, "WideRows")
        }

    assert _has_continuation_on_page_0x41(path)

    db: SqlceDatabase = SqlceDatabase(str(path))
    actual: Dict[int, Dict[str, Any]] = {row["Id"]: row for row in db.read_table("WideRows")}
    assert actual.keys() == expected.keys()
    mismatched_ids: Set[int] = {row_id for row_id in expected if actual[row_id] != expected[row_id]}
    assert not mismatched_ids
