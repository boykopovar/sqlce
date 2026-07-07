import sys
from pathlib import Path

from sqlce import SdfDatabase

DEFAULT_DIR = Path("research/raw/examples")
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


def dump_file(sdf_path: Path):
    print(f"\n{sdf_path.name}")
    db = SdfDatabase(str(sdf_path))

    table_names = db.list_tables()
    if not table_names:
        print("Таблиц не найдено.")
        return

    for table_name in table_names:
        print(f"\nТаблица: {table_name}")

        for col in db.table_schema(table_name):
            extra = ""
            if col.precision is not None:
                extra = f", precision={col.precision}, scale={col.scale}"
            print(f"  {col.ordinal:2d}. {col.name:20s} {col.type_name:20s} size={col.declared_size}{extra}")

        rows = db.read_table(table_name)

        if not rows:
            print("  (нет строк)")
            continue

        cols = list(rows[0].keys())
        print_table_rows(cols, rows)


def main():
    directory = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_DIR

    if not directory.is_dir():
        print(f"Не найдена директория: {directory.resolve()}")
        sys.exit(1)

    sdf_files = sorted(directory.glob("*.sdf"))
    if not sdf_files:
        print(f"В {directory.resolve()} не найдено .sdf файлов")
        sys.exit(0)

    for sdf_file in sdf_files:
        dump_file(sdf_file)


if __name__ == "__main__":
    main()
