cmake_minimum_required(VERSION 3.16)
project(hexapod_description)

find_package(ament_cmake REQUIRED)

# The URDF refers to its meshes through package:// URIs, so meshes and urdf must
# both end up in the package share directory.
install(DIRECTORY launch meshes urdf DESTINATION share/${PROJECT_NAME})

ament_package()
