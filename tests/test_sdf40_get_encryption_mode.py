from pathlib import Path

from sqlce import EncryptionMode
from sqlce import SdfDatabase
from tests.sdf_factory import create_engine_default_encrypted_database
from tests.sdf_factory import create_platform_default_encrypted_database

ENGINE_DEFAULT_PASSWORD = "Eng1ne-Default"
PLATFORM_DEFAULT_PASSWORD = "P1atform!Default"


def test_sdf40_engine_default_static_mode_is_aes256_sha512(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, ENGINE_DEFAULT_PASSWORD)

    assert SdfDatabase.get_encryption_mode(str(path)) == EncryptionMode.AES256_SHA512


def test_sdf40_engine_default_instance_mode_is_aes256_sha512(sdf_dir: Path) -> None:
    path = create_engine_default_encrypted_database(sdf_dir, ENGINE_DEFAULT_PASSWORD)

    db = SdfDatabase(str(path), ENGINE_DEFAULT_PASSWORD)

    assert db.get_encryption_mode() == EncryptionMode.AES256_SHA512


def test_sdf40_platform_default_static_mode_is_aes128_sha256(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PLATFORM_DEFAULT_PASSWORD)

    assert SdfDatabase.get_encryption_mode(str(path)) == EncryptionMode.AES128_SHA256


def test_sdf40_platform_default_instance_mode_is_aes128_sha256(sdf_dir: Path) -> None:
    path = create_platform_default_encrypted_database(sdf_dir, PLATFORM_DEFAULT_PASSWORD)

    db = SdfDatabase(str(path), PLATFORM_DEFAULT_PASSWORD)

    assert db.get_encryption_mode() == EncryptionMode.AES128_SHA256
