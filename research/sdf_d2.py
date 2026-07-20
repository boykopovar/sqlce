"""
    python sdf_d2.py test_35_protected_mode2.sdf --password secret123 --verify
    python sdf_d2.py test_35_protected_mode2.sdf --password secret123 --out decrypted.sdf
"""

import argparse
import hashlib
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

PAGE_SIZE = 4096


def _crypt_derive_key_sha1_aes128(base_hash: bytes, extra: bytes) -> bytes:
    page_hash = hashlib.sha1(base_hash + extra).digest()

    ipad = bytearray([0x36] * 64)
    opad = bytearray([0x5C] * 64)
    for i, b in enumerate(page_hash):
        ipad[i] ^= b
        opad[i] ^= b

    derived = hashlib.sha1(bytes(ipad)).digest() + hashlib.sha1(bytes(opad)).digest()
    return derived[:16]


def derive_key(password: str, key_param: bytes) -> bytes:
    base_hash = password.encode("utf-16-le")
    return _crypt_derive_key_sha1_aes128(base_hash, key_param)


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
    ap.add_argument("--password", help="Password (careful with shell quoting/escaping)")
    ap.add_argument("--password-file", help="Path to a file containing the password "
                                             "(exact bytes on the first line)")
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--out")
    args = ap.parse_args()

    if args.password_file:
        with open(args.password_file, "r", encoding="utf-8", newline="") as pf:
            password = pf.readline()
            if password.endswith("\r\n"):
                password = password[:-2]
            elif password.endswith("\n") or password.endswith("\r"):
                password = password[:-1]
        args.password = password
    elif args.password is None:
        ap.error("either --password or --password-file is required")

    data = open(args.sdf_path, "rb").read()
    page0 = data[0:PAGE_SIZE]

    mode = int.from_bytes(page0[184:188], "little")
    if mode != 2:
        print(f"warning: page0 reports mode={mode}, this script is for mode 2")

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
