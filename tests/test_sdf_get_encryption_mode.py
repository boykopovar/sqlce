from pathlib import Path

import pytest

from sqlce import EncryptionMode
from sqlce import SqlceDatabase
from tests.utils.scenarios import ENGINE_DEFAULT_40
from tests.utils.scenarios import PLATFORM_DEFAULT_40
from tests.utils.scenarios import build_scenario

ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE = (
    (ENGINE_DEFAULT_40, EncryptionMode.AES256_SHA512),
    (PLATFORM_DEFAULT_40, EncryptionMode.AES128_SHA256),
)


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_sdf_static_encryption_mode_matches_expected(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    assert SqlceDatabase.get_encryption_mode_from_file(str(scenario.path)) == expected_mode


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_sdf_instance_encryption_mode_matches_expected(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    db = scenario.open_database()

    assert db.get_encryption_mode() == expected_mode
