import struct

PAGE_SIZE = 4096
DATA_PAGE_TYPE = 0x40
LOB_PAGE_TYPE = 0x50
LOB_HEADER_LEN = 16
SLOT_BASE = 1017


def u16(b: bytes, off: int) -> int:
    return struct.unpack_from("<H", b, off)[0]


def u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def i32(b: bytes, off: int) -> int:
    return struct.unpack_from("<i", b, off)[0]


class Page:
    __slots__ = ("num", "raw")

    def __init__(self, num: int, raw: bytes):
        self.num = num
        self.raw = raw

    @property
    def page_type(self) -> int:
        return self.raw[6]

    @property
    def owner_object_id(self) -> int:
        return self.raw[16]

    @property
    def is_data_page(self) -> bool:
        return self.page_type == DATA_PAGE_TYPE

    @property
    def num_slots(self) -> int:
        return u32(self.raw, 20) & 0xFFF

    def iter_rows(self):
        n = self.num_slots
        for slot_index in range(n):
            slot_off = 4 * (SLOT_BASE - slot_index)
            if slot_off + 28 > PAGE_SIZE or slot_off < 0:
                continue
            flags = self.raw[slot_off + 27]
            if flags & 0x01:
                continue
            slot_dword = u32(self.raw, slot_off + 24)
            record_offset = slot_dword & 0xFFF
            record_len = ((slot_dword >> 12) & 0xFFF) - 4
            if record_len <= 0:
                continue
            start = record_offset + 28
            end = start + record_len
            if end > PAGE_SIZE:
                continue
            yield slot_index, self.raw[start:end]


def load_pages(path: str) -> list:
    with open(path, "rb") as f:
        raw = f.read()
    if len(raw) % PAGE_SIZE != 0:
        raise ValueError("file size is not a multiple of 4096 -- not a valid .sdf")
    num_pages = len(raw) // PAGE_SIZE
    return [Page(pnum, raw[pnum * PAGE_SIZE:(pnum + 1) * PAGE_SIZE]) for pnum in range(num_pages)]


def find_catalog_object_ids(pages: list) -> set:
    cat_ids = set()
    for page in pages:
        if not page.is_data_page:
            continue
        if b"__Sys" in page.raw:
            cat_ids.add(page.owner_object_id)
    return cat_ids


def iter_catalog_rows(pages: list):
    cat_ids = find_catalog_object_ids(pages)
    for page in pages:
        if not page.is_data_page:
            continue
        if page.owner_object_id not in cat_ids:
            continue
        for _slot_index, row in page.iter_rows():
            yield row
