import json
import struct
import sys
from .sdf_pages import PAGE_SIZE, load_pages, iter_catalog_rows

RAW_COLUMNS = [
    ('SysObjectType', 0x2, 0, 255, 12),
    ('SysObjectOwner', 0x8, 1, 255, 24),
    ('SysObjectName', 0x8, 2, 255, 40),
    ('SysObjectIsSystem', 0xF, 3, 255, 68),
    ('SysObjectVersion', 0xA, 4, 255, 16),
    ('SysObjectOrdinal', 0x2, 5, 255, 14),
    ('SysObjectCedbInfo', 0x4, 6, 255, 64),

    ('SysTablePageId', 0x4, 7, 1, 120),
    ('SysTableNick', 0x3, 8, 1, 136),
    ('SysTableTrackingType', 0x2, 9, 1, 124),
    ('SysTableDdlGranted', 0x4, 10, 1, 128),
    ('SysTableReadOnly', 0xF, 11, 1, 132),
    ('SysTableCompressed', 0xF, 12, 1, 304),

    ('SysColumnType', 0x2, 13, 4, 72),
    ('SysColumnSize', 0x2, 14, 4, 74),
    ('SysColumnPrecision', 0x0, 15, 4, 76),
    ('SysColumnScale', 0x0, 16, 4, 78),
    ('SysColumnFixed', 0xF, 17, 4, 80),
    ('SysColumnNullable', 0xF, 18, 4, 82),
    ('SysColumnWriteable', 0xF, 19, 4, 84),
    ('SysColumnAutoType', 0x2, 20, 4, 86),
    ('SysColumnPosition', 0x2, 21, 4, 96),
    ('SysColumnDefault', 0x8, 22, 4, 104),
    ('SysColumnCompressed', 0xF, 23, 4, 120),

    ('SysIndexRoot', 0x4, 24, 2, 72),
    ('SysIndexKey', 0xB, 25, 2, 80),
    ('SysIndexUnique', 0xF, 26, 2, 96),
    ('SysIndexNullOption', 0x2, 27, 2, 98),
    ('SysIndexPositional', 0xF, 28, 2, 100),
    ('SysIndexHistogram', 0xC, 29, 2, 12),

    ('SysConstraintType', 0x4, 30, 8, 72),
    ('SysConstraintIndex', 0x8, 31, 8, 80),
    ('SysConstraintTargetIndex', 0x8, 32, 8, 96),
    ('SysConstraintTargetTable', 0x8, 33, 8, 112),
    ('SysConstraintOnDelete', 0x4, 34, 8, 128),
    ('SysConstraintOnUpdate', 0x4, 35, 8, 132),
    ('SysConstraintKey', 0xB, 36, 8, 136),
    ('SysConstraintColumn', 0x8, 37, 8, 152),
]

INTTYPE_SIZE = {0x0: 1, 0x2: 2, 0x3: 4, 0x4: 4, 0xA: 8}
VARLIKE_INTTYPES = {0x8, 0xB, 0xC}
BIT_INTTYPE = 0xF

ROW_KIND_BY_SYSOBJECTTYPE = {1: 'TABLE', 2: 'INDEX', 4: 'COLUMN', 8: 'CONSTRAINT'}

N_TOTAL_COLUMNS = len(RAW_COLUMNS)
PRESENCE_BYTES = (N_TOTAL_COLUMNS + 7) // 8
N_VAR_TOTAL_IN_SCHEMA = sum(1 for c in RAW_COLUMNS if c[1] in VARLIKE_INTTYPES)

CATALOG_FIXED_REGION_END = (
    4 + PRESENCE_BYTES + 2
    + sum(
        INTTYPE_SIZE[c[1]]
        for c in RAW_COLUMNS
        if c[1] not in VARLIKE_INTTYPES and c[1] != BIT_INTTYPE
    )
)

def _scan_catalog_rows(path: str) -> list:
    pages = load_pages(path)
    return [row for row in iter_catalog_rows(pages) if len(row) >= 30]

def _decode_catalog_row(row: bytes):
    if len(row) < 4 + PRESENCE_BYTES:
        return None

    offset = 4
    presence = row[offset:offset + PRESENCE_BYTES]
    offset += PRESENCE_BYTES

    def is_null(col_id):
        byte_i, bit_i = divmod(col_id, 8)
        if byte_i >= len(presence):
            return True
        return bool((presence[byte_i] >> bit_i) & 1)

    if is_null(0):
        return None

    common_cols = sorted([c for c in RAW_COLUMNS if c[3] == 255], key=lambda c: c[2])

    result = {}
    off = offset

    BIT_COLUMNS_GLOBAL = sorted([c for c in RAW_COLUMNS if c[1] == BIT_INTTYPE], key=lambda c: c[2])
    bitregion = row[off:off + 2]
    if len(bitregion) < 2:
        return None
    off += 2
    bit_val = bitregion[0] | (bitregion[1] << 8)
    for i, (name, inttype, col_id, table_id, _off) in enumerate(BIT_COLUMNS_GLOBAL):
        if is_null(col_id):
            result[name] = None
        else:
            result[name] = bool((bit_val >> i) & 1)

    def read_fixed(inttype, name):
        nonlocal off
        size = INTTYPE_SIZE.get(inttype)
        if size is None:
            return None
        chunk = row[off:off + size]
        if len(chunk) < size:
            return '_TRUNCATED_'
        off += size
        if name == 'SysObjectCedbInfo':
            u16a, u16b = struct.unpack('<HH', chunk)
            result['SysObjectCedbInfo_dbtype'] = u16a
            result['SysObjectCedbInfo_ordinal'] = u16b
            return True
        if size == 1:
            result[name] = chunk[0]
        elif size == 2:
            result[name] = struct.unpack('<H', chunk)[0]
        elif size == 4:
            result[name] = struct.unpack('<I', chunk)[0]
        elif size == 8:
            result[name] = chunk.hex()
        return True

    if is_null(0):
        return None
    sys_object_type = row[off] | (row[off + 1] << 8)

    union_cols = sorted(
        [c for c in RAW_COLUMNS if c[1] not in VARLIKE_INTTYPES and c[1] != BIT_INTTYPE],
        key=lambda c: c[2],
    )

    applicable_table_ids = {255, sys_object_type}

    for name, inttype, col_id, table_id, _off in union_cols:
        if table_id not in applicable_table_ids:
            size = INTTYPE_SIZE[inttype]
            off += size
            continue
        if is_null(col_id):
            if name == 'SysObjectCedbInfo':
                result['SysObjectCedbInfo_dbtype'] = None
                result['SysObjectCedbInfo_ordinal'] = None
            else:
                result[name] = None
            size = INTTYPE_SIZE[inttype]
            off += size
            continue
        r = read_fixed(inttype, name)
        if r == '_TRUNCATED_':
            return None
        if name == 'SysObjectType':
            result['SysObjectType'] = sys_object_type

    specific_cols = sorted([c for c in RAW_COLUMNS if c[3] == sys_object_type], key=lambda c: c[2])

    all_applicable = common_cols + specific_cols

    var_cols_applicable = sorted([c for c in all_applicable if c[1] in VARLIKE_INTTYPES], key=lambda c: c[2])

    SCHEMA_VAR_COLS = sorted([c for c in RAW_COLUMNS if c[1] in VARLIKE_INTTYPES], key=lambda c: c[2])

    dir_start = CATALOG_FIXED_REGION_END
    dir_len = 2 * N_VAR_TOTAL_IN_SCHEMA
    directory_bytes = row[dir_start:dir_start + dir_len]
    if len(directory_bytes) < dir_len:
        return None

    entries = struct.unpack('<%dH' % N_VAR_TOTAL_IN_SCHEMA, directory_bytes)

    applicable_ids = {c[2] for c in var_cols_applicable}
    present_ids = {c[2] for c in var_cols_applicable if not is_null(c[2])}

    var_data_start = dir_start + dir_len
    var_data = row[var_data_start:]

    prev_end = 0
    last_i = len(SCHEMA_VAR_COLS) - 1
    for i, (name, inttype, col_id, table_id, _off) in enumerate(SCHEMA_VAR_COLS):
        if i == last_i:
            entry = None
            cum_end = len(var_data)
        else:
            entry = entries[1 + i]
            cum_end = entry & 0x7FFF
        if col_id not in present_ids:
            prev_end = cum_end
            if col_id in applicable_ids:
                result[name] = None
            continue
        chunk = var_data[prev_end:cum_end]
        prev_end = cum_end
        result[f'{name}_compressed'] = bool(entry & 0x8000) if entry is not None else None
        if inttype == 0x8:
            result[name] = chunk.split(b'\x00', 1)[0].decode('latin1')
        elif inttype in (0xB, 0xC):
            result[name] = chunk.hex()
        else:
            result[name] = chunk.hex()

    result['_row_kind'] = ROW_KIND_BY_SYSOBJECTTYPE.get(sys_object_type, 'UNKNOWN')
    return result


def decode_syscatalog(path: str) -> list:
    """Читает файл .sdf и возвращает список декодированных строк
    системного каталога __SysObjects (по одному dict на TABLE/COLUMN/
    INDEX/CONSTRAINT). Обрубленные/нечитаемые физические строки
    пропускаются."""
    out = []
    for row in _scan_catalog_rows(path):
        d = _decode_catalog_row(row)
        if d is not None:
            out.append(d)
    return out


def _json_default(o):
    if isinstance(o, bytes):
        return o.hex()
    raise TypeError(f'Object of type {o.__class__.__name__} is not JSON serializable')


def _main():
    if len(sys.argv) < 2:
        print("usage: python3 syscatalog.py mydata.sdf")
        sys.exit(1)
    rows = decode_syscatalog(sys.argv[1])
    print(json.dumps(rows, indent=2, ensure_ascii=False, default=_json_default))


if __name__ == '__main__':
    _main()
