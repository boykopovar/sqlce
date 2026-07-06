# Физический layout строки каталога

См. `00-schema.md` для схемы (38 колонок, RAW_COLUMNS).

```
offset 0..4 внутренний заголовок (4 байта)
offset 4..9 presence bitmap, PRESENCE_BYTES=5 байт (ceil(38/8)), бит на col_id
offset 9..11 bit-регион, 2 байта, кодирует все 9 bit-колонок разом
offset 11..69 fixed-регион, КОНСТАНТНО 58 байт для ЛЮБОЙ строки (union всех 4 веток)
offset 69..89 var-директория, КОНСТАНТНО 20 байт = 10 записей u16 LE
offset 89.. var-данные, до конца физической строки
```

`CATALOG_FIXED_REGION_END = 69` вычисляется в коде как
`4 + PRESENCE_BYTES + 2 + sum(INTTYPE_SIZE[inttype] for non-var non-bit fields)`,
не захардкожено.

Порог обрубленной строки: `len(row) < 89` (fixed-регион + var-директория).
Меньше - `_decode_catalog_row` возвращает None, строка пропускается.

## presence bitmap / is_null

`presence = row[4:9]`. `is_null(col_id) = not bit (col_id % 8) of byte (col_id // 8)`.
Если `is_null(0)` (SysObjectType) - строка не классифицируется, None.

## Bit-регион (offset 9-11) - статус: работает, подтверждено

`region = row[9:11]`; `val = region[0] | (region[1]<<8)`; i-й бит
(i = индекс в BIT_COLUMNS_GLOBAL по возрастанию col_id) -> значение
соответствующей bit-колонки, None если is_null(col_id). Кодирует бит для
всех 9 bit-колонок разом, независимо от SysObjectType строки
(union-принцип, см. ниже). Подтверждено 21/21 COLUMN-строк sample.sdf по
DDL (Fixed/Nullable совпадают с NOT NULL/NULL и var/fixed типом в DDL).

## Fixed-регион (offset 11-69, 58 байт) - union по всем 4 веткам

Не "только поля этой строки" - под каждую non-var non-bit fixed-колонку
из ЛЮБОЙ ветки (TABLE/COLUMN/INDEX/CONSTRAINT) зарезервировано место
всегда, независимо от SysObjectType текущей строки. Порядок слотов -
строго по возрастанию col_id по ВСЕЙ схеме разом (union_cols в коде):
common(255) -> TABLE(1) -> COLUMN(4) -> INDEX(2) -> CONSTRAINT(8).

Алгоритм чтения (_decode_catalog_row, цикл по union_cols):
- для каждого поля union, если table_id поля не в {255, свой
  SysObjectType} - байты пропускаются (`off += size`), значение не
  пишется;
- если применимо и is_null(col_id) - пишется None, off всё равно
  продвигается на size (слот физически занят, просто NULL);
- если применимо и not null - читается size байт по read_fixed, off
  продвигается.

SysObjectType читается заранее (`row[off] | row[off+1]<<8`, offset
11-13, до входа в цикл) - нужен, чтобы знать applicable_table_ids до
того, как дойдём до его слота в цикле.

Исправлено в предыдущей сессии: до правки код читал common_cols, потом
(отдельным циклом) specific_cols - без учёта зарезервированных слотов
двух других неактивных веток между ними. Это сдвигало все
branch-specific поля влево на суммарный размер пропущенных чужих веток
(конкретно: SysColumnType читался на offset 29 вместо правильных 41,
сдвиг ровно на 14 байт = размер TABLE-ветки, которую предыдущий код не
резервировал). Итог бага - SysColumnType/Size/Precision/Scale/AutoType/
Position и SysTableNick/TrackingType/DdlGranted читали чужие байты (в
основном хвост SysObjectCedbInfo/зарезервированные нулевые байты) и
давали константный 0 на всех строках. Текущий код читает все поля union
по единому проходу с продвижением off на каждом слоте (свой/чужой/null)
- это единственно верная модель, подтверждено (см. `03-status.md`).

SysObjectCedbInfo (offset 23-27, 4 байта) - это два u16 LE подряд, не
одно 4-байтное число: SysObjectCedbInfo_dbtype (первые 2 байта),
SysObjectCedbInfo_ordinal (следующие 2 байта). В result пишется именно
под этими двумя именами, ключа SysObjectCedbInfo в result нет - это
ожидаемо, не баг.

## Var-директория (offset 69-89, 20 байт = 10 записей u16 LE) - статус: работает

Директория константного размера `2 * N_VAR_TOTAL_IN_SCHEMA` (=20) байт,
начинается на константном абсолютном offset CATALOG_FIXED_REGION_END
(=69), независимо от SysObjectType строки.

`entries = struct.unpack('<10H', row[69:89])`.
`entries[0]` - sentinel (0x8000 ожидаемо), не данные.
`entries[1..9]` - cumulative end-offset (биты & 0x7FFF) для первых 9 из
10 SCHEMA_VAR_COLS (в порядке col_id), позиционно по всей схеме, не по
compacted списку присутствующих в этой строке var-колонок. 10-я
(последняя) var-колонка схемы (SysConstraintColumn) не имеет своей
записи - её диапазон идёт от предыдущего cumulative-offset до конца
var-данных.

`var_data = row[89:]` (до конца физической строки).

Для каждой SCHEMA_VAR_COLS[i]: если col_id не в применимых для этой
строки (table_id не common/не свой) или is_null(col_id) - None, prev_end
всё равно продвигается до cum_end этого слота (слот физически
присутствует, нулевая длина). Если применимо и present - `chunk =
var_data[prev_end:cum_end]`; декодируется по inttype схемы, НЕ по
compressed-биту в directory-записи (бит ненадёжен как индикатор
текст/не текст):
- `inttype == 0x8` (nvarchar) -> `chunk.split(b'\x00',1)[0].decode('latin1')`
- `inttype in (0xB, 0xC)` (varbinary/image) -> `chunk.hex()`

Compressed-бит (`entry & 0x8000`) сохраняется в
`result[f'{name}_compressed']` только для диагностики, не используется
как переключатель логики.

Подтверждено на 27/27 строк sample.sdf: имена таблиц/колонок совпадают с
DDL на 100% в правильном порядке.
