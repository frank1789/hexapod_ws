#!/bin/bash -e
# -*- coding: utf-8 -*-

colcon build --packages-select hexapod_joypad --cmake-args -DCMAKE_BUILD_TYPE=Release
colcon build --packages-select hexapod_servomotor --cmake-args -DCMAKE_BUILD_TYPE=Release
