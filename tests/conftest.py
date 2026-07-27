from pathlib import Path
from typing import Iterator

import pytest

from tests.infrastructure.custom_sdf import discover_custom_sdf_files
from tests.infrastructure.scenarios import ALL_SCENARIO_NAMES
from tests.infrastructure.scenarios import SdfScenario
from tests.infrastructure.scenarios import build_scenario
from tests.infrastructure.sdf_factory import cleanup_sdf_dir
from tests.infrastructure.sdf_factory import get_sdf_dir
from tests.infrastructure.sdf_factory import prewarm_workers

BASE_DIR = Path(__file__).parent

CUSTOM_SDF_TEST_MODULE = "test_custom_sdf.py"


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
    parser.addoption(
        "--custom",
        action="store_true",
        default=False,
        help="enable tests/custom_sdf comparison tests (requires at least one .sdf file in tests/custom_sdf)",
    )


def pytest_ignore_collect(collection_path, config) -> bool:
    is_custom_module = collection_path.name == CUSTOM_SDF_TEST_MODULE
    if config.getoption("--custom"):
        return not is_custom_module
    return is_custom_module


def pytest_collection_modifyitems(config, items) -> None:
    if not config.getoption("--custom"):
        return
    if not discover_custom_sdf_files(BASE_DIR):
        raise pytest.UsageError(
            f"--custom was passed but no .sdf files were found in {BASE_DIR / 'custom_sdf'}"
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

