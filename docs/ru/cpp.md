# C++ API

Парсер .sdf файлов (SQL Server Compact). Читает таблицы, схему, строки.

## Сборка

CMake, C++20, ninja/gcc или msvc. Флаг `SDF_BUILD_PYTHON_BINDINGS` для plain C++ не нужен, по умолчанию `OFF`.

Собирает библиотеку `libsdf` и бинарник `sdf_dump` (main.cpp, дампит все .sdf файлы из папки).

## Пример

```cpp
#include "sdf/application/SdfDatabase.hpp"

sdf::application::SdfDatabase db("file.sdf");
// или с паролем
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

Поля: `ordinal`, `name`, `typeName`, `declaredSize`, `precision`, `scale` (последние два - `optional`).

## Row (sdf/domain/Row.hpp)

Пары (имя колонки, ColumnValue). `Values()` отдает всё, `Find(name)` ищет одну.

## ColumnValue (sdf/domain/ColumnValue.hpp)

Variant: `null`, `int64`, `uint64`, `double`, `bool`, `string`, `bytes`, `DateTimeValue`, `NumericValue`, `Guid`. `ToDisplayString()` для вывода как есть.

## Исключения (sdf/domain/EncryptionMode.hpp)

- `UnsupportedEncryptionModeException` - режим шифрования не поддерживается (на данный момент поддерживается только два режима, они оба в 4.0 версии)
- `InvalidPasswordException` - не тот пароль
