import argparse
import os

from sqlce import InvalidPasswordError
from sqlce import SqlceDatabase
from sqlce import UnsupportedEncryptionModeError

RAW_DIR = os.path.join("research", "raw")


def resolve_path(path):
    root_exists = os.path.isfile(path)
    raw_path = os.path.join(RAW_DIR, path)
    raw_exists = os.path.isfile(raw_path)

    if root_exists and raw_exists:
        raise SystemExit(
            f"error: '{path}' exists both in '.' and in '{RAW_DIR}', pass a full path"
        )
    if root_exists:
        return path
    if raw_exists:
        return raw_path
    raise SystemExit(f"error: '{path}' not found in '.' or '{RAW_DIR}'")


def format_value(value):
    if isinstance(value, bytes):
        return value.hex()
    return repr(value)


def view_table(db, table_name, limit):
    print(f"=== {table_name} ===")
    columns = db.table_schema(table_name)
    print("columns:", ", ".join(c.name for c in columns))

    count = 0
    for row in db.iterate_table(table_name):
        if count >= limit:
            break
        values = ", ".join(f"{k}={format_value(v)}" for k, v in row.items())
        print(f"[{count}] {values}")
        count += 1
    print()


def main(argv=None):
    parser = argparse.ArgumentParser(prog="python -m sqlce.view")
    parser.add_argument("path", help="path to .sdf file")
    parser.add_argument("-n", "--limit", type=int, default=10, help="rows per table")
    parser.add_argument("-p", "--password", default=None, help="database password")
    args = parser.parse_args(argv)

    resolved = resolve_path(args.path)

    try:
        if args.password is not None:
            db = SqlceDatabase(resolved, args.password)
        else:
            db = SqlceDatabase(resolved)
    except InvalidPasswordError:
        raise SystemExit("error: invalid password")
    except UnsupportedEncryptionModeError:
        raise SystemExit("error: unsupported encryption mode")

    print(f"File: {resolved}")
    print(f"Encryption: {db.resolved_encryption_mode()}")
    print(f"Format: {db.get_format_version()}")
    print()

    for table_name in db.list_tables():
        view_table(db, table_name, args.limit)


if __name__ == "__main__":
    main()
