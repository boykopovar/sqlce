import atexit
import itertools
import multiprocessing as mp
import os
from typing import Any
from typing import Dict
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

IN_PROCESS_VERSION = SDF_VERSION_35


class _Dispatcher:
    def __init__(self, version: str) -> None:
        self._version = version
        self._handles: Dict[int, Any] = {}
        self._next_handle_id = itertools.count(1)
        self._loaded_module: Any = None
        self._methods = {
            "create_database": self._create_database,
            "open_connection": self._open_connection,
            "close_connection": self._close_connection,
            "execute_non_query": self._execute_non_query,
            "execute_parameterized_non_query": self._execute_parameterized_non_query,
            "execute_batch": self._execute_batch,
        }

    def dispatch(self, method_name: str, *args: Any, **kwargs: Any) -> Any:
        return self._methods[method_name](*args, **kwargs)

    def _load(self) -> Any:
        if self._loaded_module is not None:
            return self._loaded_module

        import clr

        relative_parts = ASSEMBLY_RELATIVE_PARTS_BY_VERSION[self._version]
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
                f"{ASSEMBLY_NAME} {self._version} assembly not found, install SQL Server Compact {self._version}"
            ) from last_error

        import System.Data.SqlServerCe as sqlserverce

        self._loaded_module = sqlserverce
        return sqlserverce

    def _store(self, obj: Any) -> int:
        handle_id = next(self._next_handle_id)
        self._handles[handle_id] = obj
        return handle_id

    @staticmethod
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

    @staticmethod
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

    @staticmethod
    def _variable_length_size(sql_type: str, value: Any) -> Optional[int]:
        base_type = sql_type.split("(", 1)[0].strip().lower()
        variable_length_types = ("nvarchar", "nchar", "ntext", "varbinary", "binary", "image")
        if base_type not in variable_length_types:
            return None
        if value is None:
            return 1
        return max(len(value), 1)

    @staticmethod
    def _build_connection_string(path: str, password: Optional[str], encryption_mode: Optional[str]) -> str:
        parts = [f"Data Source='{path}'"]
        if password is not None:
            parts.append(f"Password='{password}'")
        if encryption_mode is not None:
            parts.append(f"Encryption Mode='{encryption_mode}'")
        return "; ".join(parts) + ";"

    def _create_database(self, path: str, password: Optional[str], encryption_mode: Optional[str]) -> None:
        sqlserverce = self._load()
        connection_string = self._build_connection_string(path, password, encryption_mode)
        engine = sqlserverce.SqlCeEngine(connection_string)
        try:
            engine.CreateDatabase()
        finally:
            engine.Dispose()

    def _open_connection(self, path: str, password: Optional[str]) -> int:
        sqlserverce = self._load()
        connection_string = self._build_connection_string(path, password, None)
        connection = sqlserverce.SqlCeConnection(connection_string)
        connection.Open()
        return self._store(connection)

    def _close_connection(self, handle_id: int) -> None:
        connection = self._handles.pop(handle_id)
        connection.Close()

    def _execute_non_query(self, handle_id: int, command_text: str) -> None:
        connection = self._handles[handle_id]
        command = connection.CreateCommand()
        command.CommandText = command_text
        command.ExecuteNonQuery()

    def _run_parameterized(
        self,
        sqlserverce: Any,
        connection: Any,
        command_text: str,
        parameter_sql_types: Sequence[str],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
    ) -> None:
        command = connection.CreateCommand()
        command.CommandText = command_text
        for sql_type, name, value in zip(parameter_sql_types, parameter_names, parameter_values):
            parameter = sqlserverce.SqlCeParameter(name, self._sql_db_type_for(sql_type))
            variable_length_size = self._variable_length_size(sql_type, value)
            if variable_length_size is not None:
                parameter.Size = variable_length_size
            parameter.Value = self._to_clr_value(value, sql_type)
            command.Parameters.Add(parameter)
        command.ExecuteNonQuery()

    def _execute_parameterized_non_query(
        self,
        handle_id: int,
        command_text: str,
        parameter_sql_types: Sequence[str],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
    ) -> None:
        sqlserverce = self._load()
        connection = self._handles[handle_id]
        self._run_parameterized(
            sqlserverce, connection, command_text, parameter_sql_types, parameter_names, parameter_values
        )

    def _execute_batch(self, handle_id: int, operations: Sequence[tuple]) -> None:
        sqlserverce = self._load()
        connection = self._handles[handle_id]
        for operation in operations:
            if operation[0] == "sql":
                _, command_text = operation
                command = connection.CreateCommand()
                command.CommandText = command_text
                command.ExecuteNonQuery()
            else:
                _, command_text, parameter_sql_types, parameter_names, parameter_values = operation
                self._run_parameterized(
                    sqlserverce, connection, command_text, parameter_sql_types, parameter_names, parameter_values
                )


def _worker_main(version: str, request_queue: "mp.Queue", response_queue: "mp.Queue") -> None:
    dispatcher = _Dispatcher(version)

    while True:
        request_id, method_name, args, kwargs = request_queue.get()
        if method_name == "_shutdown":
            response_queue.put((request_id, "ok", None))
            return
        try:
            result = dispatcher.dispatch(method_name, *args, **kwargs)
            response_queue.put((request_id, "ok", result))
        except Exception as error:
            response_queue.put((request_id, "error", error))


class _LocalRuntime:
    def __init__(self, version: str) -> None:
        self._dispatcher = _Dispatcher(version)

    def call(self, method_name: str, *args: Any, **kwargs: Any) -> Any:
        return self._dispatcher.dispatch(method_name, *args, **kwargs)

    def shutdown(self) -> None:
        pass


class _WorkerRuntime:
    def __init__(self, version: str) -> None:
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


_runtimes: Dict[str, Any] = {}


def _runtime_for(version: str):
    runtime = _runtimes.get(version)
    if runtime is None:
        runtime = _LocalRuntime(version) if version == IN_PROCESS_VERSION else _WorkerRuntime(version)
        _runtimes[version] = runtime
    return runtime


@atexit.register
def _shutdown_all_runtimes() -> None:
    for runtime in _runtimes.values():
        runtime.shutdown()
    _runtimes.clear()


def prewarm_runtimes() -> None:
    for version in (SDF_VERSION_35, SDF_VERSION_40):
        _runtime_for(version)


def call(version: str, method_name: str, *args: Any, **kwargs: Any) -> Any:
    return _runtime_for(version).call(method_name, *args, **kwargs)
