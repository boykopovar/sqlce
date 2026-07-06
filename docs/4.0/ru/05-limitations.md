# Известные ограничения

- Сопоставление LOB-цепочек по ёмкости (`02-lob.md`) может ошибочно
  назначить не ту цепочку, если в файле есть два и более переполняющихся
  LOB-значения очень похожей суммарной длины без другой различающей
  информации.
- numeric/decimal декодируется как Python float, теряет точность для
  больших нескейленных значений (см. `03-column-types.md`, раздел 5).
- Порядок байт GUID (не по умолчанию) выведен из универсального
  соглашения Windows/COM (LE).
- Содержимое строки таблицы (не-LOB), растянутое на несколько страниц
  (dword-продолжение `record_offset+24`), считается равным 0 и не
  обрабатывается, если он ненулевой.
- Зашифрованные/защищённые паролем .sdf-файлы вне рамок вообще.
- Физически обрубленные строки каталога SysColumn вызывают тихую полную
  потерю затронутой колонки из `table_schema()`/`read_table()`, когда
  нигде в файле нет полного вхождения этой колонки.

## Использование скрипта

`sdf_reader.py` - самостоятельный модуль, только stdlib (`struct`,
`dataclasses`, `uuid`, `datetime`, плюс `argparse`/`json` для CLI).
Read-only; не поддерживает зашифрованные файлы, запись, обход b-tree/
индексов.
`main.py` в корне проекта автоматически импортирует `sdf_reader.py`, ожидая найти `.sdf` файлы в `research/raw/examples`

```python
from sdf_reader import open_sdf
db = open_sdf("mydata.sdf")
print(db.list_tables())                    # ['Customers', 'Invoices']
print(db.table_schema("Customers"))        # [{'ordinal':1,'name':'Id','type':'int',...}, ...]
rows = db.read_table("Customers")          # [{'Id':1,'Code':'C001',...}, ...]
```

CLI:
```
python3 sdf_reader.py mydata.sdf                    # список таблиц/колонок
python3 sdf_reader.py mydata.sdf Customers          # дамп строк, tab-separated
python3 sdf_reader.py mydata.sdf Customers --json   # дамп строк как JSON
```

Фильтрация по значению колонки (не встроена, тривиально поверх
read_table):
```python
rows = db.read_table("Customers")
active = [r for r in rows if r["IsActive"]]
```

Линейный полный скан таблицы в Python - без индексного поиска (b-tree/
индексные страницы не разбираются).
