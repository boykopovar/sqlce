import os
import shutil
import uuid
from pathlib import Path
from typing import Optional

import clr

ASSEMBLY_NAME = "System.Data.SqlServerCe"
ASSEMBLY_FILE_NAME = f"{ASSEMBLY_NAME}.dll"

PROGRAM_FILES_DIRS = [r"C:\Program Files", r"C:\Program Files (x86)"]
ASSEMBLY_RELATIVE_PARTS = ("Microsoft SQL Server Compact Edition", "v4.0", "Desktop", ASSEMBLY_FILE_NAME)

SQLCE_ASSEMBLY_CANDIDATES = [
    os.path.join(program_files_dir, *ASSEMBLY_RELATIVE_PARTS) for program_files_dir in PROGRAM_FILES_DIRS
]

ENCRYPTION_MODE_PLATFORM_DEFAULT = "Platform Default"
ENCRYPTION_MODE_ENGINE_DEFAULT = "Engine Default"

SDF_DIR_NAME = "sdf"

_sqlserverce = None


def _load_sqlserverce():
    global _sqlserverce
    if _sqlserverce is not None:
        return _sqlserverce

    last_error: Optional[Exception] = None
    for candidate in [*SQLCE_ASSEMBLY_CANDIDATES, ASSEMBLY_NAME]:
        try:
            clr.AddReference(candidate)
            break
        except Exception as error:
            last_error = error
    else:
        raise RuntimeError(
            f"{ASSEMBLY_NAME} 4.0 assembly not found, install SQL Server Compact 4.0"
        ) from last_error

    import System.Data.SqlServerCe as sqlserverce

    _sqlserverce = sqlserverce
    return _sqlserverce


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
) -> Path:
    sqlserverce = _load_sqlserverce()

    connection_string = build_connection_string(path, password, encryption_mode)

    engine = sqlserverce.SqlCeEngine(connection_string)
    try:
        engine.CreateDatabase()
    finally:
        engine.Dispose()

    return path


def open_connection(path: Path, password: Optional[str] = None):
    sqlserverce = _load_sqlserverce()

    connection_string = build_connection_string(path, password)
    connection = sqlserverce.SqlCeConnection(connection_string)
    connection.Open()
    return connection


def execute_non_query(connection, command_text: str) -> None:
    command = connection.CreateCommand()
    command.CommandText = command_text
    command.ExecuteNonQuery()


def create_plain_database(sdf_dir: Path, prefix: str = "plain40") -> Path:
    path = make_sdf_path(sdf_dir, prefix)
    create_sdf_database(path)
    return path


def create_password_protected_database(
    sdf_dir: Path,
    password: str,
    encryption_mode: str,
    prefix: str = "protected40",
) -> Path:
    path = make_sdf_path(sdf_dir, prefix)
    create_sdf_database(path, password=password, encryption_mode=encryption_mode)
    return path


def create_platform_default_encrypted_database(
    sdf_dir: Path,
    password: str,
    prefix: str = "platform_default40",
) -> Path:
    return create_password_protected_database(
        sdf_dir, password, ENCRYPTION_MODE_PLATFORM_DEFAULT, prefix
    )


def create_engine_default_encrypted_database(
    sdf_dir: Path,
    password: str,
    prefix: str = "engine_default40",
) -> Path:
    return create_password_protected_database(
        sdf_dir, password, ENCRYPTION_MODE_ENGINE_DEFAULT, prefix
    )
