data = open("example.sdf", "rb").read()
PAGE=4096
BITS, SLOTS_PER_WORD, WORD_BYTES = 20, 3, 8
MAPB_FIRST_LOGICAL, MAPB_SPAN = 1027, 1527

def page(pn): return data[pn*PAGE:(pn+1)*PAGE]

def header(pn):
    p = page(pn)
    return {"type": p[6], "owner": p[16], "gen": p[17],
            "logical": int.from_bytes(p[4:8], "little") & 0xFFFFF}

def packed_slot(buf, base_offset, slot_index):
    word_index = (BITS * slot_index) // (SLOTS_PER_WORD * BITS)
    bit_offset = (BITS * slot_index) % (SLOTS_PER_WORD * BITS)
    off = base_offset + word_index * WORD_BYTES
    if off + WORD_BYTES > len(buf):
        return 0
    word = int.from_bytes(buf[off:off + WORD_BYTES], "little")
    return (word >> bit_offset) & 0xFFFFF

def find_map_a_physical():
    header_page = page(0)
    return int.from_bytes(header_page[44:48], "little")

def build_logical_to_physical(n):
    map_a_phys = find_map_a_physical()
    map_a = page(map_a_phys)
    logical_to_phys = {}
    range_start = MAPB_FIRST_LOGICAL
    slot_idx = 0
    while True:
        word_index = (BITS * slot_idx) // (SLOTS_PER_WORD * BITS)
        if 16 + word_index * WORD_BYTES + WORD_BYTES > len(map_a):
            break
        map_b_phys = packed_slot(map_a, 16, slot_idx)
        if map_b_phys and map_b_phys < n:
            map_b = page(map_b_phys)
            for i in range(MAPB_SPAN):
                phys = packed_slot(map_b, 16, i)
                if phys and phys < n:
                    logical_to_phys[range_start + i] = phys
        range_start += MAPB_SPAN
        slot_idx += 1
    return logical_to_phys

def decode_spacemap_slots(buf, off):
    count = int.from_bytes(buf[off:off+4], 'little')
    indirect = int.from_bytes(buf[off+4:off+8], 'little')
    slots = []
    if indirect == 0:
        base = off + 8
        idx = 0
        while True:
            word_index = idx // SLOTS_PER_WORD
            word_off = base + word_index*8
            if word_off+8 > off+96:
                break
            bit_offset = (BITS*idx) % 60
            word = int.from_bytes(buf[word_off:word_off+8], 'little')
            val = (word >> bit_offset) & 0xFFFFF
            if val == 0:
                if idx >= count:
                    break
            slots.append(val)
            idx += 1
            if idx > 40:
                break
    return count, indirect, [s for s in slots if s != 0]

n = len(data)//PAGE
l2p = build_logical_to_physical(n)

root_pn = 452
p = page(root_pn)
count, indirect, logical_ids = decode_spacemap_slots(p, 16)

print(f"SpaceMap[0]: count={count} indirect={indirect}")
print(f"Найдено {len(logical_ids)} heap pages:")
for lid in logical_ids:
    phys = l2p.get(lid)
    hdr = header(phys) if phys is not None else None
    print(f"logical {lid} - physical {phys}  (type={hex(hdr['type']) if hdr else '?'} owner={hdr['owner'] if hdr else '?'} gen={hdr['gen'] if hdr else '?'})")
