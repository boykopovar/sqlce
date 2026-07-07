import datetime
import decimal
import uuid
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

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
]

Row = Dict[str, ColumnValue]


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
    def __init__(self, path: str) -> None: ...

    def list_tables(self) -> List[str]: ...

    def table_schema(self, table_name: str) -> List[ColumnSchema]: ...

    def read_table(self, table_name: str) -> List[Row]: ...
