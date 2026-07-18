# sqlce

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus&logoColor=white)
![Python](https://img.shields.io/badge/python-3.8%2B-blue?logo=python&logoColor=white)
![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Reverse engineered](https://img.shields.io/badge/Reverse%20engineered%20with-🔬-red)

## [En README](../../README.md)

Реверс-инжиниринг бинарного формата `.sdf` (SQL Server Compact Edition).

Анализ внутреннего устройства формата по декомпилированным компонентам SQL CE. Цель - предоставить кроссплатформенную и независимую от рантайма реализацию чтения `.sdf`.

> Исключительно для интероперабельности с существующими файлами `.sdf` формата. Не содержит оригинальный код продукта.

Браузерная версия (WebAssembly, работает локально - файл не покидает вкладку): [boykopovar.github.io/sqlce](https://boykopovar.github.io/sqlce/)


- `core/` - реализация парсера на C++
- `bindings/` - Биндинги C++ библиотеки к другим языкам
- `python/sqlce/` - Python API
- `research/` - Экспериментальные скрипты

### Пример

```bash
pip install sqlce
```

```python
from sqlce import SqlceDatabase

db = SqlceDatabase("example.sdf", password="secret123")

for table_name in db.list_tables():
    print(table_name, db.table_schema(table_name))
    for row in db.read_table(table_name):
        print(row)
```

Текущая [реализация](main.py) поддерживает чтение (поддерживается передача пароля) списка таблиц, структуры таблиц, содержимого записей. Использует c++.

> На данный момент не реализовано чтение зашифрованных `.sdf` версии `3.5` (незашифрованные читаются).

### Тестирование

```bash
pytest
```

Тесты используют установленный на устройстве SQL CE движок (через ODBC) для генерации `.sdf`-файлов с заранее известными данными и сверяют их с результатом чтения реализованным парсером - так что запуск тестов возможен только на Windows с установленным SQL Server Compact 4.0. Файлы создаются в `tests/sdf/` и автоматически удаляются в начале и после каждого прогона.


### Документация

На данный момент документация краткая, но есть примеры и тесты.

[Документация](python.md)


[Соглашения по написанию кода](docs/CONVENTIONS.md) (PR приветствуются)
