import datetime
import decimal
import enum
import uuid
from typing import Dict
from typing import List
from typing import Optional
from typing import Union
from typing import overload

class LazyLob:
    def read(self) -> Union[str, bytes]: ...
    def read_chunks(self) -> List[bytes]: ...


ColumnValue = Union[
    None,
    int,
    float,
    bool,
    str,
    bytes,
    datetime.datetime,
    decimal.Decimal,
    uuid.UUID,
    LazyLob,
]

Row = Dict[str, ColumnValue]


class EncryptionMode(enum.Enum):
    NONE = ...
    RC4_SHA1 = ...
    AES128_SHA1 = ...
    AES128_SHA256 = ...
    AES256_SHA512 = ...


class UnsupportedEncryptionModeError(ValueError):
    ...


class InvalidPasswordError(ValueError):
    ...


class ColumnSchema:
    @property
    def ordinal(self) -> int: ...

    @property
    def name(self) -> str: ...

    @property
    def type_name(self) -> str: ...

    @property
    def declared_size(self) -> int: ...

    @property
    def precision(self) -> Optional[int]: ...

    @property
    def scale(self) -> Optional[int]: ...


class SdfDatabase:
    @overload
    def __init__(self, path: str) -> None: ...

    @overload
    def __init__(self, path: str, password: str) -> None: ...

    def list_tables(self) -> List[str]: ...

    def table_schema(self, table_name: str) -> List[ColumnSchema]: ...

    def read_table(self, table_name: str) -> List[Row]: ...

    def get_encryption_mode(self) -> EncryptionMode: ...

    @staticmethod
    def get_encryption_mode_from_file(path: str) -> EncryptionMode: ...
