from dataclasses import dataclass
from pathlib import Path
from typing import Callable
from typing import Dict
from typing import Iterator
from typing import Optional

import pytest

from tests.sdf_factory import cleanup_sdf_dir
from tests.sdf_factory import create_engine_default_encrypted_database
from tests.sdf_factory import create_platform_default_encrypted_database
from tests.sdf_factory import create_plain_database
from tests.sdf_factory import get_sdf_dir
from tests.sdf_factory import open_connection as sdf_open_connection

BASE_DIR = Path(__file__).parent

SCENARIO_PLAIN = "plain40"
SCENARIO_PLATFORM_DEFAULT = "platform_default40"
SCENARIO_ENGINE_DEFAULT = "engine_default40"

PLATFORM_DEFAULT_PASSWORD = "P1atform!Default"
ENGINE_DEFAULT_PASSWORD = "Eng1ne-Default"


@pytest.fixture
def sdf_dir() -> Iterator[Path]:
    directory = get_sdf_dir(BASE_DIR)
    yield directory
    #cleanup_sdf_dir(BASE_DIR)


@dataclass(frozen=True)
class SdfScenario:
    name: str
    path: Path
    password: Optional[str]

    def open_connection(self):
        return sdf_open_connection(self.path, self.password)

    def open_database(self):
        from sqlce import SdfDatabase

        if self.password is None:
            return SdfDatabase(str(self.path))
        return SdfDatabase(str(self.path), self.password)


def _build_plain_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_plain_database(sdf_dir)
    return SdfScenario(name=SCENARIO_PLAIN, path=path, password=None)


def _build_platform_default_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_platform_default_encrypted_database(sdf_dir, PLATFORM_DEFAULT_PASSWORD)
    return SdfScenario(name=SCENARIO_PLATFORM_DEFAULT, path=path, password=PLATFORM_DEFAULT_PASSWORD)


def _build_engine_default_scenario(sdf_dir: Path) -> SdfScenario:
    path = create_engine_default_encrypted_database(sdf_dir, ENGINE_DEFAULT_PASSWORD)
    return SdfScenario(name=SCENARIO_ENGINE_DEFAULT, path=path, password=ENGINE_DEFAULT_PASSWORD)


_SCENARIO_BUILDERS: Dict[str, Callable[[Path], SdfScenario]] = {
    SCENARIO_PLAIN: _build_plain_scenario,
    SCENARIO_PLATFORM_DEFAULT: _build_platform_default_scenario,
    SCENARIO_ENGINE_DEFAULT: _build_engine_default_scenario,
}


@pytest.fixture(params=[SCENARIO_PLAIN, SCENARIO_PLATFORM_DEFAULT, SCENARIO_ENGINE_DEFAULT])
def sdf_scenario(request, sdf_dir: Path) -> SdfScenario:
    builder = _SCENARIO_BUILDERS[request.param]
    return builder(sdf_dir)
