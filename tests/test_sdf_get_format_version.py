from pathlib import Path

import pytest

from sqlce import FormatVersion
from sqlce import SqlceDatabase
from tests.utils.sdf_factory import SDF_VERSION_35
from tests.utils.sdf_factory import SDF_VERSION_40
from tests.utils.scenarios import ALL_SCENARIO_NAMES
from tests.utils.scenarios import build_scenario

SDF_VERSION_BY_FORMAT_VERSION = {
    FormatVersion.SQLCE_35: SDF_VERSION_35,
    FormatVersion.SQLCE_35_SP2: SDF_VERSION_35,
    FormatVersion.SQLCE_40: SDF_VERSION_40,
}


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_static_format_version_matches_expected(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    actual_version = SqlceDatabase.get_format_version_from_file(str(scenario.path))

    assert SDF_VERSION_BY_FORMAT_VERSION[actual_version] == scenario.version


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_instance_format_version_matches_expected(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    db = scenario.open_database()
    actual_version = db.get_format_version()

    assert SDF_VERSION_BY_FORMAT_VERSION[actual_version] == scenario.version


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_instance_and_static_format_version_agree(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    db = scenario.open_database()

    assert db.get_format_version() == SqlceDatabase.get_format_version_from_file(str(scenario.path))


def test_sdf_format_version_never_unknown_for_runtime_created_files(sdf_scenario) -> None:
    assert sdf_scenario.open_database().get_format_version() != FormatVersion.UNKNOWN
