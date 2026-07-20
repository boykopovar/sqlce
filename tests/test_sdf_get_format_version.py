from pathlib import Path

import pytest

from sqlce import FormatVersion
from sqlce import SqlceDatabase
from tests.utils.sdf_factory import SDF_VERSION_35
from tests.utils.sdf_factory import SDF_VERSION_40
from tests.utils.scenarios import ALL_SCENARIO_NAMES
from tests.utils.scenarios import build_scenario

EXPECTED_FORMAT_VERSION_BY_SDF_VERSION = {
    SDF_VERSION_35: FormatVersion.SQLCE_35_SP2,
    SDF_VERSION_40: FormatVersion.SQLCE_40,
}


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_static_format_version_matches_expected(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    expected_version = EXPECTED_FORMAT_VERSION_BY_SDF_VERSION[scenario.version]

    assert SqlceDatabase.get_format_version_from_file(str(scenario.path)) == expected_version


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_instance_format_version_matches_expected(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    expected_version = EXPECTED_FORMAT_VERSION_BY_SDF_VERSION[scenario.version]
    db = scenario.open_database()

    assert db.get_format_version() == expected_version


@pytest.mark.parametrize("scenario_name", ALL_SCENARIO_NAMES)
def test_sdf_instance_and_static_format_version_agree(sdf_dir: Path, scenario_name: str) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    db = scenario.open_database()

    assert db.get_format_version() == SqlceDatabase.get_format_version_from_file(str(scenario.path))


def test_sdf_format_version_never_unknown_for_runtime_created_files(sdf_scenario) -> None:
    assert sdf_scenario.open_database().get_format_version() != FormatVersion.UNKNOWN
