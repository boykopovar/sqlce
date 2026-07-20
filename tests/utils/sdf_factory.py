import os
import shutil
import uuid
from pathlib import Path
from typing import Any
from typing import List
from typing import Optional
from typing import Sequence

import clr

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

_loaded_sqlserverce_version: Optional[str] = None
_loaded_sqlserverce_module: Any = None


def _assembly_candidates(version: str) -> List[str]:
    relative_parts = ASSEMBLY_RELATIVE_PARTS_BY_VERSION[version]
    return [
        os.path.join(program_files_dir, *relative_parts) for program_files_dir in PROGRAM_FILES_DIRS
    ]


def _load_sqlserverce(version: str = SDF_VERSION_40):
    global _loaded_sqlserverce_version, _loaded_sqlserverce_module

    if _loaded_sqlserverce_version is not None:
        if _loaded_sqlserverce_version != version:
            raise RuntimeError(
                f"{ASSEMBLY_NAME} {_loaded_sqlserverce_version} is already loaded in this "
                f"process; cannot load {version} in the same process. Run different SDF "
                f"versions in separate OS processes."
            )
        return _loaded_sqlserverce_module

    last_error: Optional[Exception] = None
    for candidate in [*_assembly_candidates(version), ASSEMBLY_NAME]:
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

    _loaded_sqlserverce_version = version
    _loaded_sqlserverce_module = sqlserverce
    return sqlserverce


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
    sqlserverce = _load_sqlserverce(version)

    connection_string = build_connection_string(path, password, encryption_mode)

    engine = sqlserverce.SqlCeEngine(connection_string)
    try:
        engine.CreateDatabase()
    finally:
        engine.Dispose()

    return path


def open_connection(path: Path, password: Optional[str] = None, version: str = SDF_VERSION_40):
    sqlserverce = _load_sqlserverce(version)

    connection_string = build_connection_string(path, password)
    connection = sqlserverce.SqlCeConnection(connection_string)
    connection.Open()
    return connection


def execute_non_query(connection, command_text: str) -> None:
    command = connection.CreateCommand()
    command.CommandText = command_text
    command.ExecuteNonQuery()


def execute_parameterized_non_query(
        connection,
        command_text: str,
        parameter_columns: Sequence[Any],
        parameter_names: Sequence[str],
        parameter_values: Sequence[Any],
        version: str = SDF_VERSION_40,
) -> None:
    sqlserverce = _load_sqlserverce(version)

    command = connection.CreateCommand()
    command.CommandText = command_text
    for column, name, value in zip(parameter_columns, parameter_names, parameter_values):
        parameter = sqlserverce.SqlCeParameter(name, _sql_db_type_for(column.sql_type))
        variable_length_size = _variable_length_size(column.sql_type, value)
        if variable_length_size is not None:
            parameter.Size = variable_length_size
        parameter.Value = _to_clr_value(value, column.sql_type)
        command.Parameters.Add(parameter)
    command.ExecuteNonQuery()


def _variable_length_size(sql_type: str, value: Any) -> Optional[int]:
    base_type = _base_sql_type(sql_type)
    variable_length_types = ("nvarchar", "nchar", "ntext", "varbinary", "binary", "image")
    if base_type not in variable_length_types:
        return None
    if value is None:
        return 1
    return max(len(value), 1)


def _base_sql_type(sql_type: str) -> str:
    return sql_type.split("(", 1)[0].strip().lower()


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
    base_type = _base_sql_type(sql_type)
    if base_type not in mapping:
        raise TypeError(f"unsupported sql_type for parameterized insert: {sql_type!r}")
    return mapping[base_type]


def _to_clr_value(value: Any, sql_type: str) -> Any:
    import System

    if value is None:
        return System.DBNull.Value

    base_type = _base_sql_type(sql_type)

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
