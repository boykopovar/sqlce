from pathlib import Path
from typing import List

from sqlce import FormatVersion
from sqlce import SqlceDatabase
from tests.infrastructure.sdf_factory import SDF_VERSION_35
from tests.infrastructure.sdf_factory import SDF_VERSION_40
from tests.infrastructure.sdf_factory import list_tables_via_runtime
from tests.infrastructure.sdf_factory import open_connection
from tests.infrastructure.table_spec import assert_table_matches_runtime

CUSTOM_SDF_DIR_NAME = "custom_sdf"

_RUNTIME_VERSION_BY_FORMAT_VERSION = {
    FormatVersion.SQLCE_35: SDF_VERSION_35,
    FormatVersion.SQLCE_35_SP2: SDF_VERSION_35,
    FormatVersion.SQLCE_40: SDF_VERSION_40,
}


def custom_sdf_dir(base_dir: Path) -> Path:
    return base_dir / CUSTOM_SDF_DIR_NAME


def discover_custom_sdf_files(base_dir: Path) -> List[Path]:
    directory = custom_sdf_dir(base_dir)
    if not directory.is_dir():
        return []
    return sorted(directory.glob("*.sdf"))


def runtime_version_for_file(path: Path) -> str:
    format_version = SqlceDatabase.get_format_version_from_file(str(path))
    return _RUNTIME_VERSION_BY_FORMAT_VERSION[format_version]


def assert_runtime_matches_native(path: Path) -> None:
    native_db = SqlceDatabase(str(path))
    native_tables = sorted(native_db.list_tables())

    version = runtime_version_for_file(path)
    connection = open_connection(path, None, version)
    try:
        runtime_tables = sorted(list_tables_via_runtime(connection))
        only_in_native = sorted(set(native_tables) - set(runtime_tables))
        only_in_runtime = sorted(set(runtime_tables) - set(native_tables))
        assert native_tables == runtime_tables, (
            f"table list mismatch for {path}\n"
            f"sqlce lib tables:  {native_tables}\n"
            f"dll runtime tables: {runtime_tables}\n"
            f"only in sqlce lib:  {only_in_native}\n"
            f"only in dll runtime: {only_in_runtime}"
        )

        for table_name in native_tables:
            try:
                assert_table_matches_runtime(connection, native_db, table_name)
            except AssertionError as error:
                raise AssertionError(f"schema mismatch for table {table_name!r} in {path}: {error}") from error
    finally:
        connection.Close()
