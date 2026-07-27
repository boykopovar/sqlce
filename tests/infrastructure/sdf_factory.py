import shutil
import uuid
from pathlib import Path
from typing import Any
from typing import Optional
from typing import Sequence
from typing import Tuple

from tests.utils.sqlce_runtime import SDF_VERSION_35
from tests.utils.sqlce_runtime import SDF_VERSION_40
from tests.utils.sqlce_runtime import call as runtime_call
from tests.utils.sqlce_runtime import prewarm_runtimes

ENCRYPTION_MODE_PLATFORM_DEFAULT = "Platform Default"
ENCRYPTION_MODE_ENGINE_DEFAULT = "Engine Default"

PLATFORM_DEFAULT_MODE = ENCRYPTION_MODE_PLATFORM_DEFAULT
ENGINE_DEFAULT_MODE = ENCRYPTION_MODE_ENGINE_DEFAULT

SDF_DIR_NAME = "sdf"


class RemoteConnection:
    def __init__(self, version: str, handle_id: int) -> None:
        self._version = version
        self._handle_id = handle_id

    def Close(self) -> None:
        runtime_call(self._version, "close_connection", self._handle_id)

    def CreateCommand(self) -> "_RemoteCommand":
        return _RemoteCommand(self)


class _RemoteCommand:
    def __init__(self, connection: RemoteConnection) -> None:
        self._connection = connection
        self.CommandText: str = ""

    def ExecuteNonQuery(self) -> None:
        runtime_call(
            self._connection._version, "execute_non_query", self._connection._handle_id, self.CommandText
        )


def get_sdf_dir(base_dir: Path) -> Path:
    sdf_dir = base_dir / SDF_DIR_NAME
    sdf_dir.mkdir(parents=True, exist_ok=True)
    return sdf_dir


def cleanup_sdf_dir(base_dir: Path) -> None:
    sdf_dir = base_dir / SDF_DIR_NAME
    if sdf_dir.exists():
        shutil.rmtree(sdf_dir, ignore_errors=True)


def make_sdf_path(sdf_dir: Path, prefix: str) -> Path:
    file_name = f"{prefix}_{uuid.uuid4().hex}.sdf"
    return sdf_dir / file_name


def create_sdf_database(
        path: Path,
        password: Optional[str] = None,
        encryption_mode: Optional[str] = None,
        version: str = SDF_VERSION_40,
) -> Path:
    runtime_call(version, "create_database", str(path), password, encryption_mode)
    return path


def compact_database(
        path: Path,
        password: Optional[str] = None,
        encryption_mode: Optional[str] = None,
        version: str = SDF_VERSION_40,
) -> Path:
    runtime_call(version, "compact_database", str(path), password, encryption_mode)
    return path


def open_connection(path: Path, password: Optional[str] = None, version: str = SDF_VERSION_40) -> RemoteConnection:
    handle_id = runtime_call(version, "open_connection", str(path), password)
    return RemoteConnection(version, handle_id)


def execute_non_query(connection: RemoteConnection, command_text: str) -> None:
    runtime_call(connection._version, "execute_non_query", connection._handle_id, command_text)


def execute_parameterized_non_query(
        connection: RemoteConnection,
        command_text: str,
        parameter_columns: Sequence[Any],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
        version: str = SDF_VERSION_40,
) -> None:
    parameter_sql_types = [column.sql_type for column in parameter_columns]
    runtime_call(
        version,
        "execute_parameterized_non_query",
        connection._handle_id,
        command_text,
        parameter_sql_types,
        parameter_names,
        list(parameter_values),
    )


def encode_sql_operation(command_text: str) -> Tuple:
    return ("sql", command_text)


def encode_parameterized_operation(
        command_text: str,
        parameter_columns: Sequence[Any],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
) -> Tuple:
    parameter_sql_types = [column.sql_type for column in parameter_columns]
    return ("parameterized", command_text, parameter_sql_types, list(parameter_names), list(parameter_values))


def execute_batch(
        connection: RemoteConnection,
        operations: Sequence[Tuple],
        version: str = SDF_VERSION_40,
) -> None:
    runtime_call(connection._version, "execute_batch", connection._handle_id, list(operations))


def prewarm_workers() -> None:
    prewarm_runtimes()


def create_plain_database(sdf_dir: Path, prefix: str = "plain", version: str = SDF_VERSION_40) -> Path:
    path = make_sdf_path(sdf_dir, prefix)
    create_sdf_database(path, version=version)
    return path


def create_password_protected_database(
        sdf_dir: Path,
        password: str,
        encryption_mode: str,
        prefix: str = "protected",
        version: str = SDF_VERSION_40,
) -> Path:
    path = make_sdf_path(sdf_dir, prefix)
    create_sdf_database(path, password=password, encryption_mode=encryption_mode, version=version)
    return path


def create_platform_default_encrypted_database(
        sdf_dir: Path,
        password: str,
        prefix: str = "platform_default",
        version: str = SDF_VERSION_40,
) -> Path:
    return create_password_protected_database(
        sdf_dir, password, ENCRYPTION_MODE_PLATFORM_DEFAULT, prefix, version
    )


def create_engine_default_encrypted_database(
        sdf_dir: Path,
        password: str,
        prefix: str = "engine_default",
        version: str = SDF_VERSION_40,
) -> Path:
    return create_password_protected_database(
        sdf_dir, password, ENCRYPTION_MODE_ENGINE_DEFAULT, prefix, version
    )
