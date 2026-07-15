import sys
from pathlib import Path
from typing import List
from typing import Tuple

from sqlce import InvalidPasswordError
from sqlce import SqlceDatabase
from sqlce import UnsupportedEncryptionModeError

DEFAULT_DIR = Path("research/raw/examples")
DEFAULT_PROTECTED_PASSWORD = "secret123"
MAX_LEN = 20


def truncate(value: str, max_len: int = MAX_LEN) -> str:
    if len(value) > max_len:
        return value[:max_len] + "..."
    return value


def print_table_rows(cols, rows):
    str_rows = [[truncate(str(row.get(c, ""))) for c in cols] for row in rows]

    widths = [len(c) for c in cols]
    for str_row in str_rows:
        for i, val in enumerate(str_row):
            widths[i] = max(widths[i], len(val))

    header = "  ".join(c.ljust(widths[i]) for i, c in enumerate(cols))
    print("  " + header)
    for str_row in str_rows:
        line = "  ".join(val.ljust(widths[i]) for i, val in enumerate(str_row))
        print("  " + line)


def dump_file(sdf_path: Path, password: str) -> None:
    print(f"\n{sdf_path.name}")

    try:
        db = SqlceDatabase(str(sdf_path), password) if password else SqlceDatabase(str(sdf_path))
    except (UnsupportedEncryptionModeError, InvalidPasswordError) as error:
        print(f"skipped: {error}")
        return

    table_names = db.list_tables()
    if not table_names:
        print("no tables found")
        return

    for table_name in table_names:
        print(f"\ntable: {table_name}")

        for col in db.table_schema(table_name):
            extra = ""
            if col.precision is not None:
                extra = f", precision={col.precision}, scale={col.scale}"
            print(f"  {col.ordinal:2d}. {col.name:20s} {col.type_name:20s} size={col.declared_size}{extra}")

        rows = db.read_table(table_name)

        if not rows:
            print("  (no rows)")
            continue

        cols = list(rows[0].keys())
        print_table_rows(cols, rows)


def scan_directory(directory: Path, password: str) -> None:
    print(f"looking for: {directory.resolve()}")

    if not directory.is_dir():
        print(f"directory not found: {directory}", file=sys.stderr)
        return

    sdf_files = sorted(directory.glob("*.sdf"))
    if not sdf_files:
        print(f"no .sdf files found in {directory}")
        return

    for sdf_file in sdf_files:
        dump_file(sdf_file, password)


def main() -> None:
    directory = DEFAULT_DIR
    password = ""

    args: List[str] = sys.argv[1:]
    i = 0
    while i < len(args):
        argument = args[i]
        if argument == "--password" and i + 1 < len(args):
            password = args[i + 1]
            i += 2
        else:
            directory = Path(argument)
            i += 1

    print(f"current working directory: {Path.cwd()}")

    directories_to_scan: List[Tuple[Path, str]] = [
        (directory, password),
        (directory / "protected", password if password else DEFAULT_PROTECTED_PASSWORD),
    ]

    for scan_directory_path, scan_password in directories_to_scan:
        scan_directory(scan_directory_path, scan_password)


if __name__ == "__main__":
    main()
