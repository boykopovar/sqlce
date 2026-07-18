# Python API

`sqlce` bindings for [libsqlce](cpp.md) (pybind11). Parses .sdf files (SQL Server Compact).

## Installation

```bash
pip install sqlce
```

## Example

```python
from sqlce import SqlceDatabase

db = SqlceDatabase("file.sdf")
# or with a password
db = SqlceDatabase("file.sdf", "password")

for name in db.list_tables():
    print(name)

for col in db.table_schema("MyTable"):
    print(col.ordinal, col.name, col.type_name, col.declared_size, col.precision, col.scale)

rows = db.read_table("MyTable")  # list of dicts {column name: value}
```

## SqlceDatabase

- `__init__(path)` / `__init__(path, password)` - open a file
- `list_tables() -> List[str]`
- `table_schema(name) -> List[ColumnSchema]`
- `read_table(name) -> List[Row]`

## ColumnSchema

Properties: `ordinal`, `name`, `type_name`, `declared_size`, `precision`, `scale` (the last two are `Optional[int]`).

## Types

Column values are regular Python objects: `None`, `int`, `float`, `bool`, `str`, `bytes`, `datetime.datetime`, `decimal.Decimal`, `uuid.UUID`.

`Row = Dict[str, ColumnValue]` - simply a dictionary.

## Exceptions

- `UnsupportedEncryptionModeError(ValueError)` - unsupported .sdf encryption mode
- `InvalidPasswordError(ValueError)` - incorrect password

A working usage example can be found in `main.py` in the repository root.
