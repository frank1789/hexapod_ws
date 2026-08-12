"""Checks on the URDF model that need neither a robot nor a running ROS graph.

The model refers to its meshes through ``package://`` URIs, and a launch file
refers to the model by name. Both are strings: nothing verifies them until
something tries to display the robot, which is how a launch file pointing at a
URDF that did not exist survived in the repository for a long time.

SPDX-License-Identifier: MIT
Copyright (c) 2021-2026 Francesco Argentieri
"""

import xml.etree.ElementTree as ElementTree
from pathlib import Path

import yaml

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
URDF = PACKAGE_ROOT / "urdf" / "Hexapod.urdf"
RVIZ_CONFIG = PACKAGE_ROOT / "rviz" / "hexapod.rviz"
MESH_PREFIX = "package://hexapod_description/"


def test_urdf_file_exists():
    assert URDF.is_file(), f"{URDF} is missing"


def test_urdf_parses():
    root = ElementTree.parse(URDF).getroot()
    assert root.tag == "robot"
    assert root.get("name"), "the robot element carries no name"


def test_model_has_links_and_joints():
    root = ElementTree.parse(URDF).getroot()
    assert len(root.findall("link")) > 0, "the model declares no link"
    assert len(root.findall("joint")) > 0, "the model declares no joint"


def test_every_joint_refers_to_declared_links():
    root = ElementTree.parse(URDF).getroot()
    links = {link.get("name") for link in root.findall("link")}

    for joint in root.findall("joint"):
        for end in ("parent", "child"):
            element = joint.find(end)
            assert element is not None, f"joint {joint.get('name')} has no {end}"
            link = element.get("link")
            assert link in links, f"joint {joint.get('name')} refers to unknown link {link}"


def test_every_mesh_exists():
    root = ElementTree.parse(URDF).getroot()
    missing = []

    for mesh in root.iter("mesh"):
        filename = mesh.get("filename", "")
        assert filename.startswith(MESH_PREFIX), f"unexpected mesh URI: {filename}"
        relative = filename[len(MESH_PREFIX) :]
        if not (PACKAGE_ROOT / relative).is_file():
            missing.append(relative)

    assert not missing, f"meshes referenced but not present: {missing}"


def test_launch_file_points_at_the_real_model():
    launch_file = PACKAGE_ROOT / "launch" / "display.launch.py"
    assert launch_file.is_file(), "display.launch.py is missing"
    assert URDF.name in launch_file.read_text(
        encoding="utf-8"
    ), f"display.launch.py does not mention {URDF.name}"


def test_rviz_configuration_exists_and_is_launched():
    """The launch file names an RViz configuration; check it is really there.

    Same failure mode as the URDF above: the name is a string that nothing
    verifies until someone starts RViz and gets an empty scene.
    """
    launch_file = PACKAGE_ROOT / "launch" / "display.launch.py"
    assert RVIZ_CONFIG.is_file(), f"{RVIZ_CONFIG} is missing"
    assert RVIZ_CONFIG.name in launch_file.read_text(
        encoding="utf-8"
    ), f"display.launch.py does not mention {RVIZ_CONFIG.name}"


def test_rviz_configuration_matches_the_model():
    """The Fixed Frame has to be a link the URDF declares.

    RViz draws nothing at all when the Fixed Frame does not exist, and says so
    only in a panel nobody has open. Its default is "map", which this model does
    not have.
    """
    configuration = yaml.safe_load(RVIZ_CONFIG.read_text(encoding="utf-8"))
    fixed_frame = configuration["Visualization Manager"]["Global Options"]["Fixed Frame"]

    root = ElementTree.parse(URDF).getroot()
    links = {link.get("name") for link in root.findall("link")}
    assert fixed_frame in links, f"Fixed Frame {fixed_frame!r} is not a link of the model"
