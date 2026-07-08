# sqlce

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus&logoColor=white)
![Python](https://img.shields.io/badge/python-3.8%2B-blue?logo=python&logoColor=white)
![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Reverse engineered](https://img.shields.io/badge/Reverse%20engineered%20with-🔬-red)

## [Ru README](docs/ru/RuReadme.md)

Reverse engineering of the `.sdf` (SQL Server Compact Edition 4.0) binary file format.

Analysis of the format's internal structure based on decompiled SQL CE components. The goal is to provide a cross-platform, runtime-independent implementation for reading `.sdf` files.

> Intended solely for interoperability with existing `.sdf` files. Does not contain any original product source code.

- `core/` — C++ parser implementation
- `bindings/` — Bindings for the C++ library to other languages
- `python/sqlce/` — Python API
- `research/` — Experimental scripts

### Example

```bash
pip install sqlce
```

```python
from sqlce import SdfDatabase

db = SdfDatabase("example.sdf", password="secret123")

for table_name in db.list_tables():
    print(table_name, db.table_schema(table_name))
    for row in db.read_table(table_name):
        print(row)
```

The current implementation supports reading the list of tables, table schemas, and table contents (including password-protected databases). It is implemented in C++.

### Testing

```bash
pytest
```

The tests use a locally installed SQL CE engine (via ODBC) to generate `.sdf` files containing predefined data and verify that the parser produces identical results. Therefore, the test suite can only be run on Windows with SQL Server Compact 4.0 installed. Test files are created in `tests/sdf/` and are automatically removed before and after each test run.

### Documentation

The documentation is currently concise but includes usage examples and tests.

[Documentation](docs/en/python.md)


[Code Style Conventions](docs/CONVENTIONS.md) (Pull requests are welcome.)
