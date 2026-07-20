from pathlib import Path
from typing import Iterator

import pytest

from tests.utils.scenarios import ALL_SCENARIO_NAMES
from tests.utils.scenarios import SdfScenario
from tests.utils.scenarios import build_scenario
from tests.utils.sdf_factory import cleanup_sdf_dir
from tests.utils.sdf_factory import get_sdf_dir
from tests.utils.sdf_factory import prewarm_workers

BASE_DIR = Path(__file__).parent


@pytest.fixture(scope="session", autouse=True)
def _clean_sdf_dir_at_session_start() -> None:
    cleanup_sdf_dir(BASE_DIR)
    prewarm_workers()


@pytest.fixture
def sdf_dir() -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    cleanup_sdf_dir(BASE_DIR)


@pytest.fixture(params=ALL_SCENARIO_NAMES)
def sdf_scenario(request, sdf_dir: Path) -> SdfScenario:
    return build_scenario(request.param, sdf_dir)
