# Python API

Биндинг sqlce поверх [libsdf](cpp.md) (pybind11). Парсит .sdf файлы (SQL Server Compact).

## Установка

```
pip install sqlce
```

## Пример

```python
from sqlce import SdfDatabase

db = SdfDatabase("file.sdf")
# или с паролем
db = SdfDatabase("file.sdf", "password")

for name in db.list_tables():
    print(name)

for col in db.table_schema("MyTable"):
    print(col.ordinal, col.name, col.type_name, col.declared_size, col.precision, col.scale)

rows = db.read_table("MyTable")  # список dict-ов {имя колонки: значение}
```

## SdfDatabase

- `__init__(path)` / `__init__(path, password)` - открыть файл
- `list_tables() -> List[str]`
- `table_schema(name) -> List[ColumnSchema]`
- `read_table(name) -> List[Row]`

## ColumnSchema

Свойства: `ordinal`, `name`, `type_name`, `declared_size`, `precision`, `scale` (последние два - `Optional[int]`).

## Типы

Значение колонки - обычный Python-объект: `None`, `int`, `float`, `bool`, `str`, `bytes`, `datetime.datetime`, `decimal.Decimal`, `uuid.UUID`.

`Row = Dict[str, ColumnValue]` - просто словарь.

## Исключения

- `UnsupportedEncryptionModeError(ValueError)` - неподдержанный режим шифрования .sdf
- `InvalidPasswordError(ValueError)` - неверный пароль

Рабочий пример использования - `main.py` в корне репозитория.
