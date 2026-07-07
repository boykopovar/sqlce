# C++ API

Parser for .sdf files (SQL Server Compact). Reads tables, schema, and rows.

## Build

CMake, C++20, ninja/gcc or msvc. The `SDF_BUILD_PYTHON_BINDINGS` flag is not needed for plain C++, it is `OFF` by default.

Builds the `libsdf` library and the `sdf_dump` executable (`main.cpp`, dumps all .sdf files from a folder).

## Example

```cpp
#include "sdf/application/SdfDatabase.hpp"

sdf::application::SdfDatabase db("file.sdf");
// or with a password
sdf::application::SdfDatabase db("file.sdf", "password");

for (auto& name : db.ListTables()) { ... }

auto schema = db.TableSchema("MyTable");
auto rows = db.ReadTable("MyTable");
```

## SdfDatabase (sdf/application/SdfDatabase.hpp)

- `ListTables() -> vector<string>`
- `TableSchema(name) -> vector<ColumnSchema>`
- `ReadTable(name) -> vector<Row>`

## ColumnSchema (sdf/application/ColumnSchema.hpp)

Fields: `ordinal`, `name`, `typeName`, `declaredSize`, `precision`, `scale` (the last two are `optional`).

## Row (sdf/domain/Row.hpp)

Pairs of (column name, ColumnValue). `Values()` returns all values, `Find(name)` looks up a single value.

## ColumnValue (sdf/domain/ColumnValue.hpp)

Variant: `null`, `int64`, `uint64`, `double`, `bool`, `string`, `bytes`, `DateTimeValue`, `NumericValue`, `Guid`. `ToDisplayString()` returns a display-ready string.

## Exceptions (sdf/domain/EncryptionMode.hpp)

- `UnsupportedEncryptionModeException` - the encryption mode is not supported (currently only two modes are supported, both used by version 4.0)
- `InvalidPasswordException` - incorrect password
