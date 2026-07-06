# Схема системного каталога (__SysObjects)

Реализация: `catalog_parse/syscatalog.py`. Внешняя функция
`decode_syscatalog(path) -> list[dict]`. Эталонные данные:
`examples/sample.sdf` + DDL
`examples/#U043e#U043f#U0438#U0441#U0430#U043d#U0438#U0435-newsample.sdf.txt`.

См. `01-row-layout.md` для физического layout'а строки,
`02-classification.md` для того, как строка классифицируется, и
`03-status.md` для статуса подтверждённости полей.

Каталог - одна физическая widerow-таблица, 38 колонок, фиксированный
порядок (col_id 0..37). Каждая физическая строка кодирует один из
четырёх типов объектов: TABLE, COLUMN, INDEX или CONSTRAINT.

## Схема, 38 колонок (RAW_COLUMNS в коде)

`table_id`: 255 = общее для всех строк, 1 = TABLE, 4 = COLUMN, 2 = INDEX,
8 = CONSTRAINT.

`inttype`: 0 = tinyint(1б), 2 = smallint(2б), 3/4 = int(4б),
0xA = bigint/version(8б), 0x8/0xB/0xC = var-length, 0xF = bit.

```
col_id имя inttype table_id
 0 SysObjectType 0x2 255
 1 SysObjectOwner 0x8 255 (var)
 2 SysObjectName 0x8 255 (var)
 3 SysObjectIsSystem 0xF 255 (bit)
 4 SysObjectVersion 0xA 255
 5 SysObjectOrdinal 0x2 255
 6 SysObjectCedbInfo 0x4 255 -> результат: SysObjectCedbInfo_dbtype (u16), SysObjectCedbInfo_ordinal (u16)
 7 SysTablePageId 0x4 1
 8 SysTableNick 0x3 1
 9 SysTableTrackingType 0x2 1
10 SysTableDdlGranted 0x4 1
11 SysTableReadOnly 0xF 1 (bit)
12 SysTableCompressed 0xF 1 (bit)
13 SysColumnType 0x2 4
14 SysColumnSize 0x2 4
15 SysColumnPrecision 0x0 4
16 SysColumnScale 0x0 4
17 SysColumnFixed 0xF 4 (bit)
18 SysColumnNullable 0xF 4 (bit)
19 SysColumnWriteable 0xF 4 (bit)
20 SysColumnAutoType 0x2 4
21 SysColumnPosition 0x2 4
22 SysColumnDefault 0x8 4 (var)
23 SysColumnCompressed 0xF 4 (bit)
24 SysIndexRoot 0x4 2
25 SysIndexKey 0xB 2 (var)
26 SysIndexUnique 0xF 2 (bit)
27 SysIndexNullOption 0x2 2
28 SysIndexPositional 0xF 2 (bit)
29 SysIndexHistogram 0xC 2 (var)
30 SysConstraintType 0x4 8
31 SysConstraintIndex 0x8 8 (var)
32 SysConstraintTargetIndex 0x8 8 (var)
33 SysConstraintTargetTable 0x8 8 (var)
34 SysConstraintOnDelete 0x4 8
35 SysConstraintOnUpdate 0x4 8
36 SysConstraintKey 0xB 8 (var)
37 SysConstraintColumn 0x8 8 (var)
```

Пятое число в каждой строке RAW_COLUMNS (120, 136, 124, ... в таблице
выше) - это НЕ offset в физическом .sdf-файле. Это аргумент a6/offset
конструктора Column::Column в raw/sqlcese40.dll.c
(`Column::Column(this, name, inttype, col_id, table_id, offset, ...)`) -
offset поля внутри in-memory Column-объекта самого движка (проверено:
`*(_WORD *)(a1 + 86) = 1000` в конструкторе - это тот же 86, что стоит у
SysColumnAutoType, и это инициализация дефолтного значения поля в
объекте, а не что-то про диск). Разные ветки схемы переиспользуют одни и
те же offset'ы (TABLE и COLUMN обе используют, например, 120) - если бы
это было смещение в общем on-disk layout'е, это было бы противоречием.
Используется в коде только как справочная информация (взято из
декомпиляции, не из подбора), в вычислении физического layout'а
__SysObjects не участвует.

`INTTYPE_SIZE = {0x0:1, 0x2:2, 0x3:4, 0x4:4, 0xA:8}`.
`VARLIKE_INTTYPES = {0x8, 0xB, 0xC}`. `BIT_INTTYPE = 0xF`.
`ROW_KIND_BY_SYSOBJECTTYPE = {1:'TABLE', 2:'INDEX', 4:'COLUMN', 8:'CONSTRAINT'}`.

Var-колонок в схеме - 10 (SCHEMA_VAR_COLS в коде, по возрастанию col_id):
SysObjectOwner(1), SysObjectName(2), SysColumnDefault(22),
SysIndexKey(25), SysIndexHistogram(29), SysConstraintIndex(31),
SysConstraintTargetIndex(32), SysConstraintTargetTable(33),
SysConstraintKey(36), SysConstraintColumn(37).

Bit-колонок в схеме - 9 (BIT_COLUMNS_GLOBAL в коде, по возрастанию
col_id): SysObjectIsSystem(3), SysTableReadOnly(11),
SysTableCompressed(12), SysColumnFixed(17), SysColumnNullable(18),
SysColumnWriteable(19), SysColumnCompressed(23), SysIndexUnique(26),
SysIndexPositional(28).
