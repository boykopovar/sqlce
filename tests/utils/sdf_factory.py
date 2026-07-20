import atexit
import itertools
import multiprocessing as mp
import os
import shutil
import uuid
from pathlib import Path
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Sequence

ASSEMBLY_NAME = "System.Data.SqlServerCe"
ASSEMBLY_FILE_NAME = f"{ASSEMBLY_NAME}.dll"

PROGRAM_FILES_DIRS = [r"C:\Program Files", r"C:\Program Files (x86)"]

SDF_VERSION_35 = "3.5"
SDF_VERSION_40 = "4.0"

ASSEMBLY_RELATIVE_PARTS_BY_VERSION = {
    SDF_VERSION_35: ("Microsoft SQL Server Compact Edition", "v3.5", "Desktop", ASSEMBLY_FILE_NAME),
    SDF_VERSION_40: ("Microsoft SQL Server Compact Edition", "v4.0", "Desktop", ASSEMBLY_FILE_NAME),
}

ENCRYPTION_MODE_PLATFORM_DEFAULT = "Platform Default"
ENCRYPTION_MODE_ENGINE_DEFAULT = "Engine Default"

SDF_DIR_NAME = "sdf"

def _worker_main(version: str, request_queue: "mp.Queue", response_queue: "mp.Queue") -> None:
    import clr

    handles: Dict[int, Any] = {}
    next_handle_id = itertools.count(1)
    loaded_module: Any = None

    def _load() -> Any:
        nonlocal loaded_module
        if loaded_module is not None:
            return loaded_module

        relative_parts = ASSEMBLY_RELATIVE_PARTS_BY_VERSION[version]
        candidates = [
            os.path.join(program_files_dir, *relative_parts) for program_files_dir in PROGRAM_FILES_DIRS
        ]
        last_error: Optional[Exception] = None
        for candidate in [*candidates, ASSEMBLY_NAME]:
            try:
                clr.AddReference(candidate)
                break
            except Exception as error:
                last_error = error
        else:
            raise RuntimeError(
                f"{ASSEMBLY_NAME} {version} assembly not found, install SQL Server Compact {version}"
            ) from last_error

        import System.Data.SqlServerCe as sqlserverce

        loaded_module = sqlserverce
        return sqlserverce

    def _store(obj: Any) -> int:
        handle_id = next(next_handle_id)
        handles[handle_id] = obj
        return handle_id

    def _sql_db_type_for(sql_type: str):
        import System
        import System.Data

        mapping = {
            "tinyint": System.Data.SqlDbType.TinyInt,
            "smallint": System.Data.SqlDbType.SmallInt,
            "int": System.Data.SqlDbType.Int,
            "bigint": System.Data.SqlDbType.BigInt,
            "float": System.Data.SqlDbType.Float,
            "real": System.Data.SqlDbType.Real,
            "money": System.Data.SqlDbType.Money,
            "bit": System.Data.SqlDbType.Bit,
            "datetime": System.Data.SqlDbType.DateTime,
            "uniqueidentifier": System.Data.SqlDbType.UniqueIdentifier,
            "nvarchar": System.Data.SqlDbType.NVarChar,
            "nchar": System.Data.SqlDbType.NChar,
            "ntext": System.Data.SqlDbType.NText,
            "varbinary": System.Data.SqlDbType.VarBinary,
            "binary": System.Data.SqlDbType.Binary,
            "image": System.Data.SqlDbType.Image,
        }
        base_type = sql_type.split("(", 1)[0].strip().lower()
        if base_type not in mapping:
            raise TypeError(f"unsupported sql_type for parameterized insert: {sql_type!r}")
        return mapping[base_type]

    def _to_clr_value(value: Any, sql_type: str) -> Any:
        import System

        if value is None:
            return System.DBNull.Value

        base_type = sql_type.split("(", 1)[0].strip().lower()

        if isinstance(value, (bytes, bytearray)):
            return System.Array[System.Byte](value)
        if base_type == "tinyint":
            return System.Byte(value)
        if base_type == "smallint":
            return System.Int16(value)
        if base_type == "int":
            return System.Int32(value)
        if base_type == "bigint":
            return System.Int64(value)
        if base_type in ("float", "money"):
            return System.Double(value)
        if base_type == "real":
            return System.Single(value)
        if base_type == "datetime":
            return System.DateTime(
                value.year,
                value.month,
                value.day,
                value.hour,
                value.minute,
                value.second,
                value.microsecond // 1000,
            )
        if base_type == "bit":
            return System.Boolean(value)
        if base_type == "uniqueidentifier":
            return System.Guid(str(value))
        if base_type in ("nvarchar", "nchar", "ntext"):
            return System.String(value)
        return value

    def _variable_length_size(sql_type: str, value: Any) -> Optional[int]:
        base_type = sql_type.split("(", 1)[0].strip().lower()
        variable_length_types = ("nvarchar", "nchar", "ntext", "varbinary", "binary", "image")
        if base_type not in variable_length_types:
            return None
        if value is None:
            return 1
        return max(len(value), 1)

    def _build_connection_string(path: str, password: Optional[str], encryption_mode: Optional[str]) -> str:
        parts = [f"Data Source='{path}'"]
        if password is not None:
            parts.append(f"Password='{password}'")
        if encryption_mode is not None:
            parts.append(f"Encryption Mode='{encryption_mode}'")
        return "; ".join(parts) + ";"

    def _handle_create_database(path: str, password: Optional[str], encryption_mode: Optional[str]) -> None:
        sqlserverce = _load()
        connection_string = _build_connection_string(path, password, encryption_mode)
        engine = sqlserverce.SqlCeEngine(connection_string)
        try:
            engine.CreateDatabase()
        finally:
            engine.Dispose()

    def _handle_open_connection(path: str, password: Optional[str]) -> int:
        sqlserverce = _load()
        connection_string = _build_connection_string(path, password, None)
        connection = sqlserverce.SqlCeConnection(connection_string)
        connection.Open()
        return _store(connection)

    def _handle_close_connection(handle_id: int) -> None:
        connection = handles.pop(handle_id)
        connection.Close()

    def _handle_execute_non_query(handle_id: int, command_text: str) -> None:
        connection = handles[handle_id]
        command = connection.CreateCommand()
        command.CommandText = command_text
        command.ExecuteNonQuery()

    def _handle_execute_parameterized_non_query(
        handle_id: int,
        command_text: str,
        parameter_sql_types: Sequence[str],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
    ) -> None:
        sqlserverce = _load()
        connection = handles[handle_id]
        command = connection.CreateCommand()
        command.CommandText = command_text
        for sql_type, name, value in zip(parameter_sql_types, parameter_names, parameter_values):
            parameter = sqlserverce.SqlCeParameter(name, _sql_db_type_for(sql_type))
            variable_length_size = _variable_length_size(sql_type, value)
            if variable_length_size is not None:
                parameter.Size = variable_length_size
            parameter.Value = _to_clr_value(value, sql_type)
            command.Parameters.Add(parameter)
        command.ExecuteNonQuery()

    dispatch = {
        "create_database": _handle_create_database,
        "open_connection": _handle_open_connection,
        "close_connection": _handle_close_connection,
        "execute_non_query": _handle_execute_non_query,
        "execute_parameterized_non_query": _handle_execute_parameterized_non_query,
    }

    while True:
        request_id, method_name, args, kwargs = request_queue.get()
        if method_name == "_shutdown":
            response_queue.put((request_id, "ok", None))
            return
        try:
            result = dispatch[method_name](*args, **kwargs)
            response_queue.put((request_id, "ok", result))
        except Exception as error:
            response_queue.put((request_id, "error", error))


class _RuntimeWorker:
    def __init__(self, version: str) -> None:
        self._version = version
        context = mp.get_context("spawn")
        self._request_queue: "mp.Queue" = context.Queue()
        self._response_queue: "mp.Queue" = context.Queue()
        self._process = context.Process(
            target=_worker_main,
            args=(version, self._request_queue, self._response_queue),
            daemon=True,
        )
        self._process.start()
        self._next_request_id = itertools.count(1)

    def call(self, method_name: str, *args: Any, **kwargs: Any) -> Any:
        request_id = next(self._next_request_id)
        self._request_queue.put((request_id, method_name, args, kwargs))
        response_id, status, payload = self._response_queue.get()
        assert response_id == request_id
        if status == "error":
            raise payload
        return payload

    def shutdown(self) -> None:
        if self._process.is_alive():
            try:
                self.call("_shutdown")
            except Exception:
                pass
            self._process.join(timeout=5)
            if self._process.is_alive():
                self._process.terminate()


_workers: Dict[str, _RuntimeWorker] = {}


def _worker_for(version: str) -> _RuntimeWorker:
    worker = _workers.get(version)
    if worker is None:
        worker = _RuntimeWorker(version)
        _workers[version] = worker
    return worker


@atexit.register
def _shutdown_all_workers() -> None:
    for worker in _workers.values():
        worker.shutdown()
    _workers.clear()


class RemoteConnection:
    def __init__(self, version: str, handle_id: int) -> None:
        self._version = version
        self._handle_id = handle_id

    def Close(self) -> None:
        _worker_for(self._version).call("close_connection", self._handle_id)

    def CreateCommand(self) -> "_RemoteCommand":
        return _RemoteCommand(self)


class _RemoteCommand:
    def __init__(self, connection: RemoteConnection) -> None:
        self._connection = connection
        self.CommandText: str = ""

    def ExecuteNonQuery(self) -> None:
        _worker_for(self._connection._version).call(
            "execute_non_query", self._connection._handle_id, self.CommandText
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


def build_connection_string(
        path: Path,
        password: Optional[str] = None,
        encryption_mode: Optional[str] = None,
) -> str:
    parts = [f"Data Source='{path}'"]
    if password is not None:
        parts.append(f"Password='{password}'")
    if encryption_mode is not None:
        parts.append(f"Encryption Mode='{encryption_mode}'")
    return "; ".join(parts) + ";"


def create_sdf_database(
        path: Path,
        password: Optional[str] = None,
        encryption_mode: Optional[str] = None,
        version: str = SDF_VERSION_40,
) -> Path:
    _worker_for(version).call("create_database", str(path), password, encryption_mode)
    return path


def open_connection(path: Path, password: Optional[str] = None, version: str = SDF_VERSION_40) -> RemoteConnection:
    handle_id = _worker_for(version).call("open_connection", str(path), password)
    return RemoteConnection(version, handle_id)


def execute_non_query(connection: RemoteConnection, command_text: str) -> None:
    _worker_for(connection._version).call("execute_non_query", connection._handle_id, command_text)


def execute_parameterized_non_query(
        connection: RemoteConnection,
        command_text: str,
        parameter_columns: Sequence[Any],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
        version: str = SDF_VERSION_40,
) -> None:
    parameter_sql_types = [column.sql_type for column in parameter_columns]
    _worker_for(version).call(
        "execute_parameterized_non_query",
        connection._handle_id,
        command_text,
        parameter_sql_types,
        parameter_names,
        list(parameter_values),
    )


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
