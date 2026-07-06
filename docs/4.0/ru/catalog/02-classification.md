# Классификация строк каталога

См. `01-row-layout.md` для того, как читаются поля, использованные ниже.

```
1 TABLE (SysObjectOwner = '__SysObjects', SysObjectName = имя таблицы)
2 INDEX (SysObjectOwner = имя таблицы, SysObjectName = имя индекса)
4 COLUMN (SysObjectOwner = имя таблицы, SysObjectName = имя колонки)
8 CONSTRAINT (SysObjectOwner = имя таблицы, SysObjectName = имя constraint'а)
```

`result['_row_kind']` = `ROW_KIND_BY_SYSOBJECTTYPE.get(sys_object_type, 'UNKNOWN')`.

## Эталонные числа строк

```
examples/sample.sdf: 27 строк - 2 TABLE, 21 COLUMN, 2 INDEX, 2 CONSTRAINT
examples/test.sdf: 5 строк - 1 TABLE, 2 COLUMN, 1 INDEX, 1 CONSTRAINT
examples/large_rows.sdf: 6 строк - 1 TABLE, 3 COLUMN, 1 INDEX, 1 CONSTRAINT
examples/edge_cases.sdf: 94 строки - по подсчёту decode_syscatalog (58 COLUMN, 12 TABLE, 12 INDEX, 12 CONSTRAINT)
```

Эти числа не менялись правкой предыдущей сессии (проверено регрессией) -
баг был только в значениях fixed-полей, не в классификации/количестве
строк.

## protected.sdf (зашифрованный файл)

Поиск catalog-страниц идёт по литералу `b"__Sys"` в открытом виде на
странице. В зашифрованном файле такого литерала нет ни на одной
странице - множество catalog-строк пустое, без исключений. Ожидаемое
поведение, не баг.
