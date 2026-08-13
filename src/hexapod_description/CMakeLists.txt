cmake_minimum_required(VERSION 3.16)
project(hexapod_description)

find_package(ament_cmake REQUIRED)

# The URDF refers to its meshes through package:// URIs, so meshes and urdf must
# both end up in the package share directory. display.launch.py resolves the
# RViz configuration the same way, so rviz belongs here as well.
install(DIRECTORY launch meshes rviz urdf DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)
  # Parses the model and checks that everything it refers to is really there. No
  # robot and no ROS graph involved.
  ament_add_pytest_test(test_urdf test/test_urdf.py WORKING_DIRECTORY
                        ${CMAKE_CURRENT_SOURCE_DIR})
endif()

ament_package()
