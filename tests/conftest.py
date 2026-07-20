import os
import subprocess
import sys
from pathlib import Path
from typing import Iterator
from typing import List

import pytest

from tests.utils.scenarios import ALL_SCENARIO_NAMES
from tests.utils.scenarios import SdfScenario
from tests.utils.scenarios import build_scenario
from tests.utils.scenarios import scenario_version
from tests.utils.sdf_factory import SDF_VERSION_35
from tests.utils.sdf_factory import SDF_VERSION_40
from tests.utils.sdf_factory import cleanup_sdf_dir
from tests.utils.sdf_factory import get_sdf_dir

BASE_DIR = Path(__file__).parent

_SDF_VERSION_ENV_VAR = "_SQLCE_SDF_VERSION"


def pytest_cmdline_main(config) -> int:
    if os.environ.get(_SDF_VERSION_ENV_VAR):
        return None

    exit_codes: List[int] = []
    for version in (SDF_VERSION_35, SDF_VERSION_40):
        print(f"\n===== Running SDF {version} tests in a dedicated process =====\n", flush=True)
        result = subprocess.run(
            [sys.executable, "-m", "pytest", *sys.argv[1:]],
            env={**os.environ, _SDF_VERSION_ENV_VAR: version},
        )
        exit_codes.append(result.returncode)

    no_tests_collected = 5
    real_codes = [code for code in exit_codes if code != no_tests_collected]
    return max(real_codes) if real_codes else 0


@pytest.fixture(scope="session", autouse=True)
def _clean_sdf_dir_at_session_start() -> None:
    cleanup_sdf_dir(BASE_DIR)


@pytest.fixture
def sdf_dir() -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    cleanup_sdf_dir(BASE_DIR)


@pytest.fixture(params=ALL_SCENARIO_NAMES)
def sdf_scenario(request, sdf_dir: Path) -> SdfScenario:
    return build_scenario(request.param, sdf_dir)


def pytest_collection_modifyitems(config, items) -> None:
    selected_version = os.environ.get(_SDF_VERSION_ENV_VAR)
    if selected_version is None:
        return

    remaining = []
    deselected = []
    for item in items:
        scenario_name = _scenario_name_from_item(item)
        if scenario_name is None or scenario_version(scenario_name) == selected_version:
            remaining.append(item)
        else:
            deselected.append(item)

    if deselected:
        config.hook.pytest_deselected(items=deselected)
    items[:] = remaining


def _scenario_name_from_item(item):
    callspec = getattr(item, "callspec", None)
    if callspec is None:
        return None
    for param_name in ("sdf_scenario", "scenario_name"):
        if param_name in callspec.params:
            value = callspec.params[param_name]
            if isinstance(value, str) and value in ALL_SCENARIO_NAMES:
                return value
    return None
