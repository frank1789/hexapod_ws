"""Send hexapod poses to the ROS 2 bridge over ZeroMQ.

This is the workstation half of the link. It is meant to be imported inside
Maya or Blender, where the only dependency it adds is ``pyzmq``:

    from hexapod_pose_sender import PoseSender

    sender = PoseSender()
    sender.send({"L_coxaA": 90.0, "L_femurA": 45.0})

**There is deliberately no robot logic here.** The class moves a dictionary of
joint angles onto a socket and nothing else: no gait, no interpolation, no
inverse kinematics, no idea what a leg is. Whatever decides the angles — a
keyframed rig, a solver, a slider — stays in the animation package where it
belongs.

Angles are in degrees, 0 to 180, which is the unit ``motors.lua`` and
``homing.lua`` are written in. The bridge converts to radians when it publishes
``sensor_msgs/msg/JointState``.

Run the module directly for a connectivity check that sends a fixed neutral
pose. That exists to prove the socket, the bridge and the servos are talking to
each other, not as an example of how to drive the robot.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import time

try:
    import zmq
except ImportError as _error:  # pragma: no cover - depends on the host interpreter
    raise SystemExit(
        "pyzmq is not installed for this interpreter.\n"
        "Inside Blender or Maya, install it with that application's own Python, "
        "for example:  <application python> -m pip install pyzmq"
    ) from _error


#: Schema version this sender writes. The bridge refuses anything else outright
#: rather than half-understanding a message from a newer sender.
SCHEMA_VERSION = 1

#: Unit the angles are expressed in. Declared on the wire so that a sender
#: working in radians is rejected instead of driving every joint to a fraction
#: of its intended angle.
UNITS = "deg"

MIN_ANGLE_DEGREE = 0.0
MAX_ANGLE_DEGREE = 180.0

DEFAULT_ENDPOINT = "tcp://127.0.0.1:5556"

#: Where the bridge parameters live, relative to the root of the workspace.
DEFAULT_CONFIG = os.path.join("src", "hexapod_bridge", "config", "bridge.yaml")


def endpoint_from_configuration(
    path=DEFAULT_CONFIG, environment_variable="HEXAPOD_BRIDGE_ENDPOINT"
):
    """Work out which endpoint to talk to, from the same file the node reads.

    The order is: the environment first, so docker compose and a shell export
    win; then ``bridge.yaml``; then the built-in default. PyYAML is optional —
    Blender does not ship it — and its absence downgrades to the default rather
    than failing, because the endpoint can always be passed explicitly.

    The endpoint the file names is the one the *bridge binds*, which is usually
    ``0.0.0.0``. That is an address to listen on, not one to connect to, so a
    wildcard host is rewritten to localhost and anything else is left alone.
    """
    from_environment = os.environ.get(environment_variable)
    if from_environment:
        return from_environment

    try:
        import yaml  # noqa: WPS433 - optional, and absent inside Blender
    except ImportError:
        return DEFAULT_ENDPOINT

    try:
        with open(path, "r", encoding="utf-8") as handle:
            document = yaml.safe_load(handle) or {}
    except OSError:
        return DEFAULT_ENDPOINT

    for section in document.values():
        if isinstance(section, dict):
            parameters = section.get("ros__parameters", {})
            endpoint = parameters.get("endpoint")
            if endpoint:
                return endpoint.replace("0.0.0.0", "127.0.0.1").replace("://*:", "://127.0.0.1:")

    return DEFAULT_ENDPOINT


class PoseSender:
    """A ZeroMQ publisher that carries joint angles to the robot.

    The socket is a ``PUB``, which never blocks and never queues: if the bridge
    is not listening the poses are discarded. That is the intended behaviour for
    a stream of poses — a backlog delivered late would make the robot replay old
    motion — and it means the animation package is never stalled by the network.

    The object can be used as a context manager, which closes the socket
    promptly instead of leaving it to the interpreter.
    """

    def __init__(self, endpoint=None, connect=True, linger_ms=0):
        """Open the socket.

        :param endpoint: where to talk to; defaults to the configured endpoint
        :param connect: connect to the endpoint, the usual case, since the
            bridge on the robot is the one that binds
        :param linger_ms: how long ``close`` waits for pending messages; zero
            means never block on shutdown
        """
        self.endpoint = endpoint or endpoint_from_configuration()
        self._context = zmq.Context.instance()
        self._socket = self._context.socket(zmq.PUB)
        self._socket.setsockopt(zmq.LINGER, linger_ms)

        if connect:
            self._socket.connect(self.endpoint)
        else:
            self._socket.bind(self.endpoint)

        self._sequence = 0

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception, traceback):
        self.close()
        return False

    @staticmethod
    def validate(joints):
        """Check a pose before it goes anywhere.

        The bridge validates everything again on arrival — it has to, it cannot
        trust the network — but failing here points at the line that built the
        bad pose instead of at a log message on the robot.

        :param joints: mapping of motor name to angle in degrees
        :raises ValueError: if the mapping is empty, names a joint with an empty
            string, or carries an angle that is not a finite number within
            ``[0, 180]``
        """
        if not joints:
            raise ValueError("the pose is empty, there is nothing to send")

        for name, angle in joints.items():
            if not name:
                raise ValueError("a joint was given an empty name")
            if not isinstance(angle, (int, float)) or isinstance(angle, bool):
                raise ValueError(f"joint {name!r} carries {angle!r}, which is not a number")
            if not math.isfinite(angle):
                raise ValueError(f"joint {name!r} carries a non-finite angle")
            if not MIN_ANGLE_DEGREE <= angle <= MAX_ANGLE_DEGREE:
                raise ValueError(
                    f"joint {name!r} is at {angle} degrees, outside "
                    f"[{MIN_ANGLE_DEGREE}, {MAX_ANGLE_DEGREE}]"
                )

    def encode(self, joints):
        """Build the payload for a pose, without sending it.

        Exposed so the format can be inspected, logged or tested on its own.

        :param joints: mapping of motor name to angle in degrees
        :return: the JSON payload as a string
        """
        self.validate(joints)
        return json.dumps(
            {
                "schema": SCHEMA_VERSION,
                "seq": self._sequence,
                "units": UNITS,
                "joints": {str(name): float(angle) for name, angle in joints.items()},
            }
        )

    def send(self, joints):
        """Send one pose.

        :param joints: mapping of motor name to angle in degrees
        :return: the sequence number the pose was sent with
        :raises ValueError: if the pose does not pass :meth:`validate`
        """
        payload = self.encode(joints)
        self._socket.send_string(payload)
        self._sequence += 1
        return self._sequence - 1

    def close(self):
        """Close the socket. Safe to call more than once."""
        if not self._socket.closed:
            self._socket.close()


def _neutral_pose():
    """A mid-travel angle for every motor `motors.lua` generates.

    Only used by the connectivity check below. It is not a rest position: the
    real one lives in ``homing.lua`` on the robot, which is the single source of
    truth for where the joints belong.
    """
    joints = {}
    for side in ("L", "R"):
        for leg in ("A", "B", "C"):
            for segment in ("coxa", "femur", "tibia"):
                joints[f"{side}_{segment}{leg}"] = 90.0
    return joints


def main():
    """Send a fixed pose repeatedly, to prove the link works end to end."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--endpoint", default=None, help="where the bridge is listening")
    parser.add_argument("--rate", type=float, default=10.0, help="poses per second")
    parser.add_argument("--count", type=int, default=0, help="how many to send, 0 for forever")
    arguments = parser.parse_args()

    if arguments.rate <= 0:
        parser.error("--rate must be greater than zero")

    pose = _neutral_pose()
    period = 1.0 / arguments.rate

    with PoseSender(endpoint=arguments.endpoint) as sender:
        print(f"sending {len(pose)} joints to {sender.endpoint} at {arguments.rate} Hz")
        # A PUB socket drops whatever it sends before a subscriber has finished
        # connecting, so the first poses would silently go nowhere.
        time.sleep(0.3)

        sent = 0
        try:
            while arguments.count == 0 or sent < arguments.count:
                sender.send(pose)
                sent += 1
                time.sleep(period)
        except KeyboardInterrupt:
            pass

        print(f"sent {sent} pose(s)")


if __name__ == "__main__":
    main()
