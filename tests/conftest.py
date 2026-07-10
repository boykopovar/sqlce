from dataclasses import dataclass
from pathlib import Path
from typing import Callable
from typing import Dict
from typing import Iterator
from typing import Optional

import pytest

from tests.sdf_factory import SDF_VERSION_35
from tests.sdf_factory import SDF_VERSION_40
from tests.sdf_factory import cleanup_sdf_dir
from tests.sdf_factory import create_engine_default_encrypted_database
from tests.sdf_factory import create_platform_default_encrypted_database
from tests.sdf_factory import create_plain_database
from tests.sdf_factory import get_sdf_dir
from tests.sdf_factory import open_connection as sdf_open_connection

BASE_DIR = Path(__file__).parent

SCENARIO_PLAIN_35 = "plain35"
SCENARIO_PLAIN_40 = "plain40"
SCENARIO_PLATFORM_DEFAULT = "platform_default40"
SCENARIO_ENGINE_DEFAULT = "engine_default40"

PLATFORM_DEFAULT_PASSWORD = "P1atform!Default"
ENGINE_DEFAULT_PASSWORD = "Eng1ne-Default"


@pytest.fixture(scope="session", autouse=True)
def _clean_sdf_dir_at_session_start() -> None:
    cleanup_sdf_dir(BASE_DIR)

@pytest.fixture
def sdf_dir() -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    cleanup_sdf_dir(BASE_DIR)


@dataclass(frozen=True)
class SdfScenario:
    name: str
    path: Path
    password: Optional[str]
    version: str = SDF_VERSION_40

    def open_connection(self):
        return sdf_open_connection(self.path, self.password, self.version)

    def open_database(self):
        from sqlce import SdfDatabase

        if self.password is None:
            return SdfDatabase(str(self.path))
        return SdfDatabase(str(self.path), self.password)


def _build_plain_35_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_plain_database(sdf_dir, prefix=SCENARIO_PLAIN_35, version=SDF_VERSION_35)
    return SdfScenario(name=SCENARIO_PLAIN_35, path=path, password=None, version=SDF_VERSION_35)


def _build_plain_40_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_plain_database(sdf_dir, prefix=SCENARIO_PLAIN_40, version=SDF_VERSION_40)
    return SdfScenario(name=SCENARIO_PLAIN_40, path=path, password=None, version=SDF_VERSION_40)


def _build_platform_default_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_platform_default_encrypted_database(sdf_dir, PLATFORM_DEFAULT_PASSWORD)
    return SdfScenario(name=SCENARIO_PLATFORM_DEFAULT, path=path, password=PLATFORM_DEFAULT_PASSWORD)


def _build_engine_default_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_engine_default_encrypted_database(sdf_dir, ENGINE_DEFAULT_PASSWORD)
    return SdfScenario(name=SCENARIO_ENGINE_DEFAULT, path=path, password=ENGINE_DEFAULT_PASSWORD)


_SCENARIO_BUILDERS: Dict[str, Callable[[Path], SdfScenario]] = {
    SCENARIO_PLAIN_35: _build_plain_35_scenario,
    SCENARIO_PLAIN_40: _build_plain_40_scenario,
    SCENARIO_PLATFORM_DEFAULT: _build_platform_default_scenario,
    SCENARIO_ENGINE_DEFAULT: _build_engine_default_scenario,
}


@pytest.fixture(
    params=[SCENARIO_PLAIN_35, SCENARIO_PLAIN_40, SCENARIO_PLATFORM_DEFAULT, SCENARIO_ENGINE_DEFAULT]
)
def sdf_scenario(request, sdf_dir: Path) -> SdfScenario:
    builder = _SCENARIO_BUILDERS[request.param]
    return builder(sdf_dir)
