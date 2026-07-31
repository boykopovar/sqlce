from pathlib import Path

import pytest

from sqlce import EncryptionMode
from sqlce import SqlceDatabase
from tests.infrastructure.scenarios import ENGINE_DEFAULT_40
from tests.infrastructure.scenarios import PLATFORM_DEFAULT_40
from tests.infrastructure.scenarios import PLAIN_40
from tests.infrastructure.scenarios import build_scenario

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


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_verify_password_accepts_correct_password(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    assert SqlceDatabase.verify_password(str(scenario.path), scenario.password) is True


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_verify_password_rejects_wrong_password(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    assert SqlceDatabase.verify_password(str(scenario.path), "wrong-password") is False


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_verify_password_with_mode_accepts_correct_password_and_mode(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    assert SqlceDatabase.verify_password_with_mode(str(scenario.path), scenario.password, expected_mode) is True


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_verify_password_with_mode_rejects_wrong_password(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)

    assert SqlceDatabase.verify_password_with_mode(str(scenario.path), "wrong-password", expected_mode) is False


@pytest.mark.parametrize("scenario_name, expected_mode", ENCRYPTED_SCENARIOS_WITH_EXPECTED_MODE)
def test_verify_password_with_mode_rejects_correct_password_under_wrong_mode(
        sdf_dir: Path, scenario_name: str, expected_mode: EncryptionMode,
) -> None:
    scenario = build_scenario(scenario_name, sdf_dir)
    wrong_mode = next(
        mode for mode in EncryptionMode.__members__.values() if mode not in (expected_mode, EncryptionMode.NONE)
    )

    assert SqlceDatabase.verify_password_with_mode(str(scenario.path), scenario.password, wrong_mode) is False


def test_verify_password_on_unencrypted_file_returns_false(sdf_dir: Path) -> None:
    scenario = build_scenario(PLAIN_40, sdf_dir)

    assert SqlceDatabase.verify_password(str(scenario.path), "any-password") is False
