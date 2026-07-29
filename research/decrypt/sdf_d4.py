"""
    python sdf_d4.py test_engine_default.sdf --password secret123 --verify
    python sdf_d4.py test_engine_default.sdf --password secret123 --out decrypted.sdf
"""

import argparse
import hashlib
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

PAGE_SIZE = 4096


def derive_key(password: str, key_param: bytes) -> bytes:
    digest = hashlib.sha512(password.encode("utf-16-le") + key_param).digest()
    return digest[:32]  # CALG_AES_256 -> 32-byte key


def aes_cbc_zero_iv(key: bytes):
    return Cipher(algorithms.AES(key), modes.CBC(b"\x00" * 16))


def verify_password(page0: bytes, password: str) -> bool:
    key_param = page0[188:192]
    key = derive_key(password, key_param)
    dec = aes_cbc_zero_iv(key).decryptor()
    pt = dec.update(page0[76:172]) + dec.finalize()
    expected = password.encode("utf-16-le").ljust(96, b"\x00")
    return pt == expected


def decrypt_page(page: bytes, password: str) -> bytes:
    page_type = (int.from_bytes(page[4:8], "little") >> 20) & 0xF
    if page_type <= 2:
        return page
    key_param = page[0:4]
    key = derive_key(password, key_param)
    dec = aes_cbc_zero_iv(key).decryptor()
    tail = dec.update(page[16:PAGE_SIZE]) + dec.finalize()
    return page[0:16] + tail


def decrypt_page0(page0: bytes, password: str) -> bytes:
    key_param = page0[188:192]
    key = derive_key(password, key_param)
    dec = aes_cbc_zero_iv(key).decryptor()
    pt = dec.update(page0[76:172]) + dec.finalize()
    return page0[0:76] + pt + page0[172:]


def decrypt_file(data: bytes, password: str) -> bytes:
    pages = [data[i:i + PAGE_SIZE] for i in range(0, len(data), PAGE_SIZE)]
    out = []
    for i, page in enumerate(pages):
        if i == 0:
            out.append(decrypt_page0(page, password))
        else:
            out.append(decrypt_page(page, password))
    return b"".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sdf_path")
    ap.add_argument("--password", required=True)
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--out")
    args = ap.parse_args()

    data = open(args.sdf_path, "rb").read()
    page0 = data[0:PAGE_SIZE]

    mode = int.from_bytes(page0[184:188], "little")
    if mode != 4:
        print(f"warning: page0 reports mode={mode}, this script is for mode 4")

    ok = verify_password(page0, args.password)
    print(f"password valid: {ok}")

    if args.verify:
        return

    if not ok:
        print("refusing to decrypt: password verification failed")
        return

    if args.out:
        decrypted = decrypt_file(data, args.password)
        with open(args.out, "wb") as f:
            f.write(decrypted)
        print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
