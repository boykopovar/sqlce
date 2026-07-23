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


def pytest_addoption(parser) -> None:
    parser.addoption(
        "--no-clean",
        action="store_true",
        default=False,
        help="keep .sdf files created during this run (skip cleanup at test end); start-of-session cleanup still runs",
    )
    parser.addoption(
        "--no-clean-all",
        action="store_true",
        default=False,
        help="skip cleanup entirely, both at session start and at test end",
    )


@pytest.fixture(scope="session")
def no_clean(request) -> bool:
    return request.config.getoption("--no-clean") or request.config.getoption("--no-clean-all")


@pytest.fixture(scope="session")
def no_clean_all(request) -> bool:
    return request.config.getoption("--no-clean-all")


@pytest.fixture(scope="session", autouse=True)
def _clean_sdf_dir_at_session_start(no_clean_all: bool) -> None:
    if not no_clean_all:
        cleanup_sdf_dir(BASE_DIR)
    prewarm_workers()


@pytest.fixture
def sdf_dir(no_clean: bool) -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    if not no_clean:
        cleanup_sdf_dir(BASE_DIR)


@pytest.fixture(params=ALL_SCENARIO_NAMES)
def sdf_scenario(request, sdf_dir: Path) -> SdfScenario:
    return build_scenario(request.param, sdf_dir)

