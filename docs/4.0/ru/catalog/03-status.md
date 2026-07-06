# Статус подтверждённости полей каталога

Подтверждено правильными (сверено с DDL sample.sdf, не только "есть
ключ", а конкретное значение): `_row_kind`, `SysObjectOwner`,
`SysObjectName`, `SysColumnType`, `SysColumnSize`, `SysColumnPrecision`,
`SysColumnScale`, `SysColumnFixed`, `SysColumnNullable`, `SysTablePageId`.

Три пункта, ранее помеченные как "неправильно/подозрительно",
расследованы до конца и закрыты - код не менялся, это были ложные
тревоги (значения корректны, просто неочевидны):

## SysTableNick/SysTableTrackingType/SysTableDdlGranted = 0 на обеих таблицах sample.sdf

Проверено: `is_null(8)`, `is_null(9)`, `is_null(10)` на TABLE-строках =
False (поля present, не NULL) - но подряд на всех TABLE-строках всех 4
example-файлов (sample.sdf, test.sdf, large_rows.sdf, edge_cases.sdf,
разные имена таблиц, разные DDL) сырые байты offset 31-41 нулевые без
единого исключения. Это не похоже на offset-баг (баг давал бы
правдоподобно-неверные, но не нулевые значения, как это было с
SysColumnType при старом баге) - это выглядит как настоящее поведение
движка: SQL CE Desktop просто не заполняет nick/tracking-type/
ddl-granted для обычной CREATE TABLE без явных nick/tracking/DDL-опций
(эти поля относятся к replication/merge-функциональности, не
используемой в тестовых DDL). Вывод: 0 корректен, не баг. Закрыто.

## SysColumnAutoType = 1000 на всех 21 колонках sample.sdf

Проверено по raw/sqlcese40.dll.c: в конструкторе Column::Column
(offset+86, что и есть DLL-offset поля SysColumnAutoType в RAW_COLUMNS)
явно стоит `*(_WORD *)(a1 + 86) = 1000;` - то есть 1000 - это
дефолт/sentinel-константа "нет auto-increment типа" в самом движке, а не
мусорное значение от бага чтения. В коде того же файла есть проверки
вида `if ( *v26 != 1000 )` - 1000 используется как явный маркер "не
задано". Вывод: 1000 корректен, не баг. Не проверено на файле с реальной
identity-колонкой (ни в одном из 4 example-файлов такой колонки нет) -
это ограничение тестовых данных, а не сомнение в значении 1000 как
таковом. Закрыто как "не баг"; отдельно остаётся пункт "не хватает
тестовых данных с identity" на будущее.

## SysColumnPosition не монотонно растёт

Досверено на Customers (5 fixed non-bit колонок: Id, BirthDate,
CreditLimit, Score, ExternalId) в дополнение к Invoices из прошлой
сессии. Полная картина по обеим таблицам:

```
Customers: Id=0, BirthDate=4, CreditLimit=12, Score=20, ExternalId=28
           (running byte-offset: 0, +4(int)->4, +8(datetime)->12,
            +8(money)->20, +8(float)->28 -- цепочка идеально
            согласована с SysColumnSize каждой предыдущей колонки)
Invoices: Id=0, CustomerId=4, Amount=8, Tax=27, Created=46, Paid=54
           (та же модель: +4, +19(numeric,SysColumnSize), +19, +8, +8)
var-колонки (Code/Name/Phone/Email/Photo/Description на Customers,
InvoiceNo/Comment на Invoices): 0,1,2,3,4,5 и 0,1 -- порядковый
индекс, не байты.
bit-колонки (IsActive на Customers, IsPaid на Invoices): 0,0 --
порядковый индекс среди bit-колонок, единственная в обеих таблицах.
```

Подтверждает теорию из прошлой сессии: SysColumnPosition - это три
независимых счётчика, по одному на storage-класс (fixed non-bit:
cumulative byte-offset с шагом SysColumnSize; var: порядковый индекс;
bit: порядковый индекс), в порядке DDL-объявления колонок внутри
таблицы. Не баг, поведение подтверждено на 100% колонок обеих таблиц
sample.sdf. Закрыто.

## Остальное

Поля SysIndex*, SysConstraint*, часть SysObject*/SysTable* bit-полей
помимо Fixed/Nullable не сверены с независимым источником правды ни в
этой, ни в прошлых сессиях - статус неизвестен, не путать с
"подтверждено верным".
