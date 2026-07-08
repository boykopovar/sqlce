# sqlce

Реверс-инжиниринг бинарного формата `.sdf` (SQL Server Compact Edition 4.0).
 
Анализ внутреннего устройства формата по декомпилированным компонентам SQL CE. Цель - предоставить кроссплатформенную и независимую от рантайма реализацию чтения `.sdf` и документацию формата.


- `core/` - реализация парсера на C++
- `bindings/` - Биндинги для C++ библиотеки к другим языкам
- `python/sqlce/` - Python API
- `research/` - Экспериментальные скрипты


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

Текущая [реализация](main.py) поддерживает чтение (поддерживается передача пароля) списка таблиц, структуры таблиц, содержимого записей. Использует c++.

[Документация](docs/ru/python.md)

[Documentation](docs/en/python.md)

[Соглашения по написанию кода](docs/CONVENTIONS.md) (PR приветствуются)

> Исключительно для интероперабельности с существующими файлами `.sdf` формата. Не содержит оригинальный код продукта.
