"""Launch the ZeroMQ bridge alone, with the parameters from config/.

Values that change between deployments come from the environment, which is how
docker compose configures the container: anything set as HEXAPOD_BRIDGE_* wins
over the matching entry in bridge.yaml. Nothing else is configurable from the
environment, so an unknown variable is a typo rather than a silent default.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def _as_bool(text):
    """Read a boolean the way a person writes it in a compose file."""
    lowered = text.strip().lower()
    if lowered in ("1", "true", "yes", "on"):
        return True
    if lowered in ("0", "false", "no", "off"):
        return False
    raise ValueError(f"{text!r} is not a boolean; use true or false")


# Environment variable -> (parameter name, how to read it).
_ENVIRONMENT_OVERRIDES = {
    "HEXAPOD_BRIDGE_ENDPOINT": ("endpoint", str),
    "HEXAPOD_BRIDGE_BIND": ("bind", _as_bool),
    "HEXAPOD_BRIDGE_OUTPUT_TOPIC": ("output_topic", str),
    "HEXAPOD_BRIDGE_POLL_PERIOD_MS": ("poll_period_ms", int),
    "HEXAPOD_BRIDGE_OVERRIDE_TIMEOUT_MS": ("override_timeout_ms", int),
    "HEXAPOD_BRIDGE_THUMBSTICK_DEADZONE": ("thumbstick_deadzone", float),
}


def environment_overrides():
    """Collect the parameter overrides the environment asks for.

    A variable that is set but cannot be read is an error: continuing with the
    file value would run the robot on a configuration nobody asked for.
    """
    overrides = {}
    for variable, (parameter, convert) in _ENVIRONMENT_OVERRIDES.items():
        raw = os.environ.get(variable)
        if raw is None or raw == "":
            continue
        try:
            overrides[parameter] = convert(raw)
        except ValueError as error:
            raise ValueError(f"{variable}={raw!r} could not be read: {error}") from error
    return overrides


def generate_launch_description():
    parameters = os.path.join(
        get_package_share_directory("hexapod_bridge"),
        "config",
        "bridge.yaml",
    )

    return LaunchDescription(
        [
            Node(
                package="hexapod_bridge",
                executable="hexapod_bridge_node",
                name="hexapod_bridge",
                output="screen",
                parameters=[parameters, environment_overrides()],
            ),
        ]
    )
