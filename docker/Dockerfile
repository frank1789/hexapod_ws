# syntax=docker/dockerfile:1
#
# Runtime image for hexapod_ws on a Raspberry Pi 4 Model B or a Raspberry Pi 5.
#
# Two stages: the first builds the workspace, the second carries only what is
# needed to run it. The compiler, vcpkg, the sources and the object files stay
# behind in the builder, so the image that ships to the robot is a fraction of
# the size.
#
#   docker build -f docker/Dockerfile -t hexapod:latest .
#   docker buildx build --platform linux/arm64 -f docker/Dockerfile -t hexapod:latest .
#
# Every version-specific value is an ARG: upgrading ROS is one line here or one
# line in docker/compose.yaml, and nothing below hard-codes a distribution.

ARG ROS_DISTRO=jazzy

# ----------------------------------------------------------------- build stage
FROM ros:${ROS_DISTRO}-ros-base AS builder

ARG ROS_DISTRO
ARG VCPKG_REF=master

# vcpkg owns fmt, ZeroMQ, cppzmq, Eigen and nlohmann-json — see vcpkg.json at
# the root of the workspace. It builds them from source, which on arm64 is the
# only option anyway; paid once here and cached in this layer, it costs nothing
# on a rebuild that only touches the robot's own sources.
ENV VCPKG_ROOT=/opt/vcpkg \
    VCPKG_INSTALLED=/opt/vcpkg_installed \
    VCPKG_DISABLE_METRICS=1 \
    VCPKG_FORCE_SYSTEM_BINARIES=1 \
    DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# curl, zip, unzip and tar are vcpkg's own requirements; the rest is what the
# workspace needs to compile. liblua5.3-dev carries lua.h, which
# find_package(Lua 5.3 REQUIRED) needs and the interpreter package does not.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        curl \
        git \
        libi2c-dev \
        liblua5.3-dev \
        lua5.3 \
        ninja-build \
        pkg-config \
        python3-pip \
        tar \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch "${VCPKG_REF}" https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}" \
    && "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

# Only the manifest is copied first, so editing a source file does not throw
# away the hours of third-party compilation cached in this layer.
WORKDIR /ws
COPY vcpkg.json /ws/vcpkg.json

# The triplet follows the architecture of the machine doing the build, which is
# what makes the same Dockerfile work on arm64 and on an x86 workstation.
RUN VCPKG_TRIPLET="$(uname -m | sed 's/aarch64/arm64-linux/; s/x86_64/x64-linux/')" \
    && echo "${VCPKG_TRIPLET}" > /opt/vcpkg_triplet \
    && "${VCPKG_ROOT}/vcpkg" install \
        --x-manifest-root=/ws \
        --x-install-root="${VCPKG_INSTALLED}" \
        --triplet "${VCPKG_TRIPLET}" \
        --clean-after-build

COPY src /ws/src

# VCPKG_MANIFEST_MODE is off on purpose: the manifest was already resolved
# above, once, for the whole workspace. Left on, every colcon package would try
# to resolve it again for itself.
#
# clang-tidy is not run here. It runs in the dev container, where it is part of
# the compilation; a release image should not fail to build over a lint finding.
RUN VCPKG_TRIPLET="$(cat /opt/vcpkg_triplet)" \
    && source "/opt/ros/${ROS_DISTRO}/setup.bash" \
    && colcon build \
        --merge-install \
        --cmake-args \
            -DCMAKE_BUILD_TYPE=Release \
            -DHEXAPOD_ENABLE_CLANG_TIDY=OFF \
            -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
            -DVCPKG_MANIFEST_MODE=OFF \
            -DVCPKG_INSTALLED_DIR="${VCPKG_INSTALLED}" \
            -DVCPKG_TARGET_TRIPLET="${VCPKG_TRIPLET}"

# --------------------------------------------------------------- runtime stage
FROM ros:${ROS_DISTRO}-ros-base AS runtime

ARG ROS_DISTRO
ENV ROS_DISTRO=${ROS_DISTRO} \
    DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Runtime libraries only, no -dev packages. vcpkg links fmt, ZeroMQ and the rest
# statically, so they need nothing here; Lua and I2C come from the distribution
# and do.
#
# i2c-tools is kept deliberately: when a joint does not move, the first question
# is whether the boards answer, and answering it inside the container beats
# guessing from outside it.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        i2c-tools \
        libi2c0 \
        liblua5.3-0 \
        ros-${ROS_DISTRO}-joy \
        ros-${ROS_DISTRO}-robot-state-publisher \
        ros-${ROS_DISTRO}-realsense2-camera \
        ros-${ROS_DISTRO}-realsense2-description \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /ws/install /opt/hexapod
COPY --chmod=0755 docker/entrypoint.sh /usr/local/bin/entrypoint.sh

# The container runs as a normal user in the i2c, input, and video groups. Root
# would work, but a process that can reach every device on the bus is not what
# should be listening on a network port.
ARG USERNAME=hexapod
ARG USER_UID=1000
ARG USER_GID=1000
RUN if getent passwd ${USER_UID} >/dev/null; then \
        userdel -r "$(getent passwd ${USER_UID} | cut -d: -f1)" 2>/dev/null || true; \
    fi \
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} --create-home --shell /bin/bash ${USERNAME} \
    && groupadd -f i2c \
    && groupadd -f input \
    && groupadd -f video \
    && usermod -aG i2c,input,video ${USERNAME}

USER ${USERNAME}
WORKDIR /opt/hexapod

# The ZeroMQ endpoint the bridge binds. Publishing it is compose's business.
EXPOSE 5556

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["ros2", "launch", "hexapod_bridge", "hexapod.launch.py"]
