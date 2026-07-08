from pathlib import Path
from typing import Iterator

import pytest

from tests.sdf_factory import cleanup_sdf_dir
from tests.sdf_factory import get_sdf_dir

BASE_DIR = Path(__file__).parent


@pytest.fixture
def sdf_dir() -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    cleanup_sdf_dir(BASE_DIR)
