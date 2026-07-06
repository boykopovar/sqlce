import argparse
import json
import struct
import uuid
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from typing import Any, Optional

from research.catalog_parse.sdf_pages import (
    PAGE_SIZE,
    DATA_PAGE_TYPE,
    LOB_PAGE_TYPE,
    LOB_HEADER_LEN,
    SLOT_BASE,
    Page,
    u16,
    u32,
    i32,
    load_pages,
    find_catalog_object_ids,
)
from research.catalog_parse.syscatalog import decode_syscatalog

DATETIME_EPOCH = datetime(1900, 1, 1)

INT_TYPE_INT = 3
INT_TYPE_WSTR = 8
INT_TYPE_NTEXT = 9
INT_TYPE_IMAGE = 12
INT_TYPE_DATETIME = 13
INT_TYPE_BIT = 15
INT_TYPE_FLOAT = 17
INT_TYPE_MONEY = 18
INT_TYPE_GUID = 14
INT_TYPE_NUMERIC = 19
INT_TYPE_TINYINT = 0
INT_TYPE_SMALLINT = 1
INT_TYPE_BIGINT = 5
INT_TYPE_BINARY = 10
INT_TYPE_VARBINARY = 11

FIXED_SIZE_BY_INTTYPE = {
    INT_TYPE_INT: 4,
    INT_TYPE_DATETIME: 8,
    INT_TYPE_MONEY: 8,
    INT_TYPE_FLOAT: 8,
    INT_TYPE_GUID: 16,
    INT_TYPE_NUMERIC: 19,
    INT_TYPE_TINYINT: 1,
    INT_TYPE_SMALLINT: 2,
    INT_TYPE_BIGINT: 8,
}

VARLEN_INTTYPES = {INT_TYPE_WSTR, INT_TYPE_VARBINARY}
LOB_INTTYPES = {INT_TYPE_NTEXT, INT_TYPE_IMAGE}

_UNVERIFIED_DBTYPE_NAMES = {
    0x02: "smallint", 0x12: "tinyint", 0x13: "bigint",
    0x66: "varchar/char", 0x67: "binary/varbinary",
}


@dataclass
class ColumnDef:
    name: str
    ordinal: int
    dbtype: int
    inttype: int
    declared_size: int
    precision: int = 0
    scale: int = 0

    @property
    def is_bit(self) -> bool:
        return self.inttype == INT_TYPE_BIT

    @property
    def is_varlen(self) -> bool:
        return self.inttype in VARLEN_INTTYPES or self.inttype in LOB_INTTYPES

    @property
    def is_lob(self) -> bool:
        return self.inttype in LOB_INTTYPES

    @property
    def fixed_size(self) -> int:
        if self.is_bit or self.is_varlen:
            return 0
        return FIXED_SIZE_BY_INTTYPE.get(self.inttype, self.declared_size or 4)

    @property
    def type_name(self) -> str:
        names = {
            INT_TYPE_INT: "int", INT_TYPE_WSTR: "nvarchar/nchar",
            INT_TYPE_NTEXT: "ntext", INT_TYPE_IMAGE: "image",
            INT_TYPE_DATETIME: "datetime", INT_TYPE_BIT: "bit",
            INT_TYPE_FLOAT: "float/real", INT_TYPE_MONEY: "money",
            INT_TYPE_GUID: "uniqueidentifier", INT_TYPE_NUMERIC: "numeric/decimal",
            INT_TYPE_TINYINT: "tinyint", INT_TYPE_SMALLINT: "smallint",
            INT_TYPE_BIGINT: "bigint", INT_TYPE_BINARY: "binary",
            INT_TYPE_VARBINARY: "varbinary",
        }
        if self.inttype in names:
            return names[self.inttype]
        return _UNVERIFIED_DBTYPE_NAMES.get(self.dbtype, f"unknown(dbtype={self.dbtype},inttype={self.inttype})")


@dataclass
class TableDef:
    name: str
    object_id: int
    columns: list = field(default_factory=list)
    data_pages: list = field(default_factory=list)


def _decode_datetime(chunk: bytes) -> datetime:
    lo, hi = struct.unpack("<II", chunk)
    return DATETIME_EPOCH + timedelta(days=hi, milliseconds=lo)


def _decode_money(chunk: bytes) -> float:
    return struct.unpack("<q", chunk)[0] / 10000.0


def _decode_numeric(chunk: bytes) -> float:
    precision, scale, sign = chunk[0], chunk[1], chunk[2]
    value = int.from_bytes(chunk[3:19], "little")
    if sign == 0:
        value = -value
    return value / (10 ** scale)


def _decode_guid(chunk: bytes) -> str:
    return str(uuid.UUID(bytes_le=chunk))


def _decode_var_text(chunk: bytes, compressed: bool) -> str:
    if compressed:
        return chunk.decode("latin1")
    return chunk.decode("utf-16-le", errors="replace")


class SdfFile:
    def __init__(self, path: str):
        self.path = path
        self.pages: list = load_pages(path)
        self.num_pages = len(self.pages)
        self.tables: dict = {}
        self._scan_catalog()
        self._assign_data_pages()
        self._scan_lob_chains()

    def _page(self, num: int) -> Page:
        return self.pages[num]

    def _scan_catalog(self):
        catalog_rows = decode_syscatalog(self.path)

        table_by_name: dict = {}
        pending_columns: list = []

        for r in catalog_rows:
            kind = r.get("_row_kind")
            if kind == "TABLE":
                table_name = r.get("SysObjectName")
                page_id = r.get("SysTablePageId")
                if table_name is None or page_id is None:
                    continue
                object_id = page_id & 0xFF
                table_by_name[table_name] = TableDef(name=table_name, object_id=object_id)
            elif kind == "COLUMN":
                owner = r.get("SysObjectOwner")
                col_name = r.get("SysObjectName")
                if owner is None or col_name is None:
                    continue
                ordinal = r.get("SysObjectCedbInfo_ordinal")
                dbtype = r.get("SysObjectCedbInfo_dbtype")
                inttype = r.get("SysColumnType")
                declared_size = r.get("SysColumnSize")
                if ordinal is None or inttype is None or declared_size is None:
                    continue
                col = ColumnDef(
                    name=col_name,
                    ordinal=ordinal,
                    dbtype=dbtype if dbtype is not None else 0,
                    inttype=inttype,
                    declared_size=declared_size,
                    precision=r.get("SysColumnPrecision") or 0,
                    scale=r.get("SysColumnScale") or 0,
                )
                pending_columns.append((owner, col))

        for owner, col in pending_columns:
            t = table_by_name.get(owner)
            if t is not None:
                t.columns.append(col)

        for t in table_by_name.values():
            t.columns.sort(key=lambda c: c.ordinal)

        self.catalog_object_ids = find_catalog_object_ids(self.pages)
        self.tables = table_by_name

    def _assign_data_pages(self):
        by_object_id = {t.object_id: t for t in self.tables.values()}
        for pnum in range(self.num_pages):
            page = self._page(pnum)
            if not page.is_data_page:
                continue
            if page.owner_object_id in self.catalog_object_ids:
                continue
            owner = page.owner_object_id
            if owner in by_object_id:
                by_object_id[owner].data_pages.append(pnum)

    def _scan_lob_chains(self):
        def _is_reliable(page: Page) -> bool:
            b = page.raw[16:24]
            return b[0] == b[2] == b[4] == b[6] and b[1] == b[3] == b[5] == b[7] == 0

        chains: dict = {}
        unreliable_pages: list = []
        for pnum in range(self.num_pages):
            page = self._page(pnum)
            if page.page_type != LOB_PAGE_TYPE:
                continue
            if _is_reliable(page):
                stream_id = page.owner_object_id
                seq = u32(page.raw, 8)
                chains.setdefault(stream_id, []).append((seq, pnum))
            else:
                unreliable_pages.append(pnum)

        self._lob_chains: dict = {}
        for stream_id, entries in chains.items():
            entries.sort(key=lambda e: e[0])
            self._lob_chains[stream_id] = [pnum for _, pnum in entries]

        runs: list = []
        run_pages: list = []
        prev = None
        for pnum in unreliable_pages:
            if prev is not None and pnum != prev + 1:
                runs.append(run_pages)
                run_pages = []
            run_pages.append(pnum)
            prev = pnum
        if run_pages:
            runs.append(run_pages)

        def _seqs(run):
            return [u32(self._page(p).raw, 8) for p in run]

        merged: list = []
        for run in runs:
            seqs = _seqs(run)
            if (
                merged
                and len(merged[-1]) >= 2
                and len(run) >= 1
            ):
                prev_seqs = _seqs(merged[-1])
                step = prev_seqs[-1] - prev_seqs[-2]
                if step > 0 and seqs[0] - prev_seqs[-1] == step:
                    merged[-1].extend(run)
                    continue
            merged.append(list(run))

        for run in merged:
            entries = [(u32(self._page(p).raw, 8), p) for p in run]
            entries.sort(key=lambda e: e[0])
            key = f"unreliable-run-{run[0]}"
            self._lob_chains[key] = [pnum for _, pnum in entries]

        self._lob_chains_used: set = set()

    def _lob_chain_bytes(self, stream_id: int, total_length: int) -> bytes:
        pages = self._lob_chains.get(stream_id, [])
        buf = bytearray()
        for pnum in pages:
            page = self._page(pnum)
            buf.extend(page.raw[LOB_HEADER_LEN:])
            if len(buf) >= total_length:
                break
        return bytes(buf[:total_length])

    def _resolve_lob_payload(self, inline_tail: bytes, total_length: int) -> bytes:
        if len(inline_tail) >= total_length:
            return inline_tail[:total_length]

        for stream_id, pages in self._lob_chains.items():
            if stream_id in self._lob_chains_used:
                continue
            capacity = len(pages) * (PAGE_SIZE - LOB_HEADER_LEN)
            if capacity >= total_length:
                self._lob_chains_used.add(stream_id)
                return self._lob_chain_bytes(stream_id, total_length)
        return inline_tail

    def list_tables(self) -> list:
        return sorted(self.tables.keys())

    def table_schema(self, table_name: str) -> list:
        t = self._require_table(table_name)
        return [
            {
                "ordinal": c.ordinal,
                "name": c.name,
                "type": c.type_name,
                "size": c.declared_size,
                "precision": c.precision if c.inttype == INT_TYPE_NUMERIC else None,
                "scale": c.scale if c.inttype == INT_TYPE_NUMERIC else None,
            }
            for c in t.columns
        ]

    def read_table(self, table_name: str) -> list:
        t = self._require_table(table_name)
        rows_out = []
        for pnum in t.data_pages:
            page = self._page(pnum)
            for slot_index, row_bytes in page.iter_rows():
                rows_out.append(self._decode_row(t, row_bytes))
        return rows_out

    def _require_table(self, table_name: str) -> TableDef:
        if table_name not in self.tables:
            raise KeyError(f"no such table: {table_name!r} (have: {sorted(self.tables)})")
        return self.tables[table_name]

    def _decode_row(self, t: TableDef, row: bytes) -> dict:
        cols = t.columns
        n = len(cols)

        presence_bytes = (n + 7) // 8
        bool_cols = [c for c in cols if c.is_bit]
        bit_region_bytes = (len(bool_cols) + 7) // 8

        offset = 4
        presence = row[offset:offset + presence_bytes]
        offset += presence_bytes
        bit_region = row[offset:offset + bit_region_bytes]
        offset += bit_region_bytes

        def is_null(col: ColumnDef) -> bool:
            idx = col.ordinal - 1
            byte_i, bit_i = divmod(idx, 8)
            if byte_i >= len(presence):
                return False
            return bool((presence[byte_i] >> bit_i) & 1)

        result: dict = {}

        fixed_cols = [c for c in cols if not c.is_bit and not c.is_varlen]
        fixed_values: dict = {}
        for c in fixed_cols:
            size = c.fixed_size
            chunk = row[offset:offset + size]
            offset += size
            if is_null(c):
                fixed_values[c.name] = None
                continue
            fixed_values[c.name] = self._decode_fixed(c, chunk)

        bit_values: dict = {}
        for i, c in enumerate(bool_cols):
            byte_i, bit_i = divmod(i, 8)
            if is_null(c):
                bit_values[c.name] = None
            elif byte_i < len(bit_region):
                bit_values[c.name] = bool((bit_region[byte_i] >> bit_i) & 1)
            else:
                bit_values[c.name] = None

        var_cols = [c for c in cols if c.is_varlen]
        var_values: dict = {}
        if var_cols:
            if all(is_null(c) for c in var_cols):
                for c in var_cols:
                    var_values[c.name] = None
            else:
                dir_bytes = row[offset:offset + 2 * len(var_cols)]
                offset += 2 * len(var_cols)
                var_data = row[offset:]
                entries = [u16(dir_bytes, 2 * i) for i in range(len(var_cols))]
                starts = [e & 0x7FFF for e in entries]
                compressed_flags = [bool(e & 0x8000) for e in entries]
                for i, c in enumerate(var_cols):
                    if is_null(c):
                        var_values[c.name] = None
                        continue
                    start = starts[i]
                    end = starts[i + 1] if i + 1 < len(starts) else len(var_data)
                    chunk = var_data[start:end]
                    var_values[c.name] = self._decode_varlen_instance(c, chunk, compressed_flags[i])

        for c in cols:
            if c.name in fixed_values:
                result[c.name] = fixed_values[c.name]
            elif c.name in bit_values:
                result[c.name] = bit_values[c.name]
            elif c.name in var_values:
                result[c.name] = var_values[c.name]
        return result

    @staticmethod
    def _decode_fixed(c: ColumnDef, chunk: bytes) -> Any:
        if c.inttype == INT_TYPE_INT:
            return i32(chunk, 0)
        if c.inttype == INT_TYPE_DATETIME:
            return _decode_datetime(chunk).isoformat(sep=" ")
        if c.inttype == INT_TYPE_MONEY:
            return _decode_money(chunk)
        if c.inttype == INT_TYPE_FLOAT:
            return struct.unpack("<d", chunk)[0]
        if c.inttype == INT_TYPE_GUID:
            return _decode_guid(chunk)
        if c.inttype == INT_TYPE_NUMERIC:
            return _decode_numeric(chunk)
        if c.inttype == INT_TYPE_TINYINT:
            return int.from_bytes(chunk, "little", signed=False)
        if c.inttype in (INT_TYPE_SMALLINT, INT_TYPE_BIGINT):
            return int.from_bytes(chunk, "little", signed=True)
        if c.inttype == INT_TYPE_BINARY:
            return chunk
        if len(chunk) in (1, 2, 4, 8):
            return int.from_bytes(chunk, "little", signed=True)
        return chunk.hex()

    def _decode_varlen_instance(self, c: ColumnDef, chunk: bytes, compressed: bool) -> Any:
        if c.inttype in (INT_TYPE_NTEXT, INT_TYPE_IMAGE):
            if len(chunk) < 8:
                return "" if c.inttype == INT_TYPE_NTEXT else b""
            length = u32(chunk, 0)
            inline_tail = chunk[8:]
            payload = self._resolve_lob_payload(inline_tail, length)
            if c.inttype == INT_TYPE_NTEXT:
                return _decode_var_text(payload, compressed)
            return payload
        return self._decode_varlen(c, chunk, compressed)

    @staticmethod
    def _decode_varlen(c: ColumnDef, chunk: bytes, compressed: bool) -> Any:
        if c.inttype == INT_TYPE_WSTR:
            return _decode_var_text(chunk, compressed)
        if c.inttype == INT_TYPE_NTEXT:
            if len(chunk) < 8:
                return ""
            length = u32(chunk, 0)
            payload = chunk[8:8 + length]
            return _decode_var_text(payload, compressed)
        if c.inttype == INT_TYPE_IMAGE:
            if len(chunk) < 8:
                return b""
            length = u32(chunk, 0)
            return chunk[8:8 + length]
        return chunk if not compressed else chunk.decode("latin1", errors="replace")


def open_sdf(path: str) -> SdfFile:
    return SdfFile(path)


def _main():
    """
    Usage:
        from sdf_reader import open_sdf
        db = open_sdf("mydata.sdf")
        print(db.list_tables())
        print(db.table_schema("Customers"))
        rows = db.read_table("Customers")

    CLI:
        python3 sdf_reader.py mydata.sdf                  # list tables + columns
        python3 sdf_reader.py mydata.sdf Customers         # dump rows, tab-separated
        python3 sdf_reader.py mydata.sdf Customers --json  # dump rows as JSON
    """

    ap = argparse.ArgumentParser(description="Read-only SQL CE 4.0 .sdf reader")
    ap.add_argument("sdf_path")
    ap.add_argument("table", nargs="?", help="table name to dump rows for")
    ap.add_argument("--json", action="store_true", help="dump rows as JSON instead of tab-separated")
    args = ap.parse_args()

    db = open_sdf(args.sdf_path)

    if not args.table:
        if not db.tables:
            print("No tables found (or catalog could not be parsed).")
            return
        for name in db.list_tables():
            print(f"{name}")
            for col in db.table_schema(name):
                extra = ""
                if col["precision"] is not None:
                    extra = f", precision={col['precision']}, scale={col['scale']}"
                print(f"  {col['ordinal']:2d}. {col['name']:20s} {col['type']:20s} size={col['size']}{extra}")
        return

    rows = db.read_table(args.table)
    if args.json:
        print(json.dumps(rows, indent=2, default=str, ensure_ascii=False))
        return

    if not rows:
        print("(no rows)")
        return
    cols = list(rows[0].keys())
    print("\t".join(cols))
    for row in rows:
        print("\t".join(str(row.get(c, "")) for c in cols))


if __name__ == "__main__":
    _main()
