from dataclasses import dataclass
from pathlib import Path
from typing import Callable
from typing import Dict
from typing import Optional

from tests.utils.sdf_factory import SDF_VERSION_35
from tests.utils.sdf_factory import SDF_VERSION_40
from tests.utils.sdf_factory import create_engine_default_encrypted_database
from tests.utils.sdf_factory import create_platform_default_encrypted_database
from tests.utils.sdf_factory import create_plain_database
from tests.utils.sdf_factory import open_connection as sdf_open_connection

PLAIN_35 = "plain35"
PLAIN_40 = "plain40"
PLATFORM_DEFAULT_35 = "platform_default35"
ENGINE_DEFAULT_35 = "engine_default35"
PLATFORM_DEFAULT_40 = "platform_default40"
ENGINE_DEFAULT_40 = "engine_default40"

PLATFORM_DEFAULT_PASSWORD = "P1atform!Default"
ENGINE_DEFAULT_PASSWORD = "Eng1ne-Default"

ALL_SCENARIO_NAMES = (
    PLAIN_35,
    PLAIN_40,
    PLATFORM_DEFAULT_35,
    ENGINE_DEFAULT_35,
    PLATFORM_DEFAULT_40,
    ENGINE_DEFAULT_40,
)


@dataclass(frozen=True)
class SdfScenario:
    name: str
    path: Path
    password: Optional[str]
    version: str

    def open_connection(self):
        return sdf_open_connection(self.path, self.password, self.version)

    def open_database(self):
        from sqlce import SqlceDatabase

        if self.password is None:
            return SqlceDatabase(str(self.path))
        return SqlceDatabase(str(self.path), self.password)


def _build_plain(sdf_dir: Path, name: str, version: str) -> SdfScenario:
    path = create_plain_database(sdf_dir, prefix=name, version=version)
    return SdfScenario(name=name, path=path, password=None, version=version)


def _build_platform_default(sdf_dir: Path, name: str, version: str) -> SdfScenario:
    path = create_platform_default_encrypted_database(
        sdf_dir, PLATFORM_DEFAULT_PASSWORD, prefix=name, version=version
    )
    return SdfScenario(name=name, path=path, password=PLATFORM_DEFAULT_PASSWORD, version=version)


def _build_engine_default(sdf_dir: Path, name: str, version: str) -> SdfScenario:
    path = create_engine_default_encrypted_database(
        sdf_dir, ENGINE_DEFAULT_PASSWORD, prefix=name, version=version
    )
    return SdfScenario(name=name, path=path, password=ENGINE_DEFAULT_PASSWORD, version=version)


_SCENARIO_BUILDERS: Dict[str, Callable[[Path], SdfScenario]] = {
    PLAIN_35: lambda sdf_dir: _build_plain(sdf_dir, PLAIN_35, SDF_VERSION_35),
    PLAIN_40: lambda sdf_dir: _build_plain(sdf_dir, PLAIN_40, SDF_VERSION_40),
    PLATFORM_DEFAULT_35: lambda sdf_dir: _build_platform_default(sdf_dir, PLATFORM_DEFAULT_35, SDF_VERSION_35),
    ENGINE_DEFAULT_35: lambda sdf_dir: _build_engine_default(sdf_dir, ENGINE_DEFAULT_35, SDF_VERSION_35),
    PLATFORM_DEFAULT_40: lambda sdf_dir: _build_platform_default(sdf_dir, PLATFORM_DEFAULT_40, SDF_VERSION_40),
    ENGINE_DEFAULT_40: lambda sdf_dir: _build_engine_default(sdf_dir, ENGINE_DEFAULT_40, SDF_VERSION_40),
}


def build_scenario(name: str, sdf_dir: Path) -> SdfScenario:
    return _SCENARIO_BUILDERS[name](sdf_dir)
