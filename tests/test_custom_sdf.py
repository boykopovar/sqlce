from pathlib import Path

import pytest

from tests.infrastructure.custom_sdf import assert_runtime_matches_native
from tests.infrastructure.custom_sdf import discover_custom_sdf_files

BASE_DIR = Path(__file__).parent

CUSTOM_SDF_FILES = discover_custom_sdf_files(BASE_DIR)
CUSTOM_SDF_IDS = [path.name for path in CUSTOM_SDF_FILES]


@pytest.mark.parametrize("sdf_path", CUSTOM_SDF_FILES, ids=CUSTOM_SDF_IDS)
def test_custom_sdf_runtime_matches_native(sdf_path: Path) -> None:
    assert_runtime_matches_native(sdf_path)
