#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Install everything hexapod_ws needs on a Raspberry Pi 4 Model B or a
# Raspberry Pi 5, then leave the machine ready to build the workspace.
#
#   ./scripts/install-raspberrypi.sh                # headless robot
#   ./scripts/install-raspberrypi.sh --with-gui     # add RViz and the sliders
#   ./scripts/install-raspberrypi.sh --dry-run      # print, change nothing
#
# The script is idempotent. Every step checks the state it wants before acting,
# so a second run after a failure, a reboot or a change of flags only does the
# work that is still missing.
#
# It never guesses: an unsupported architecture, a distribution with no ROS 2
# packages or a missing boot configuration stops the run with the reason and
# the fix, rather than leaving a half-installed system behind.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly SCRIPT_NAME REPO_ROOT

# ----------------------------------------------------------------- defaults

ROS_DISTRO_WANTED="${ROS_DISTRO:-jazzy}"
WITH_GUI=0
WITH_DEV_TOOLS=0
SKIP_HARDWARE=0
DRY_RUN=0
FORCE=0

# Packages every installation needs, whatever the flags.
readonly -a PACKAGES_BASE=(
  build-essential
  ca-certificates
  cmake
  curl
  git
  gnupg
  lsb-release
  pkg-config
  python3
  python3-pip
)

# The system libraries the workspace links against, plus the tools to inspect
# the bus by hand. liblua5.3-dev is the development package on purpose:
# find_package(Lua 5.3 REQUIRED) needs lua.h, the interpreter alone is not
# enough. i2c-tools provides i2cdetect, used below to probe for the boards.
#
# libzmq3-dev, libfmt-dev and nlohmann-json3-dev are what hexapod_bridge needs.
# vcpkg is the intended source for all three (see vcpkg.json), but a native
# build on the Pi should not have to compile them from scratch, so the apt
# packages are installed as well and CMake takes whichever it finds. cppzmq is
# the exception: Debian does not package that single header, so the build
# fetches it when vcpkg is not in use.
readonly -a PACKAGES_PROJECT=(
  i2c-tools
  libfmt-dev
  libi2c-dev
  liblua5.3-dev
  libzmq3-dev
  lua5.3
  nlohmann-json3-dev
)

# ------------------------------------------------------------------ logging

if [ -t 1 ]; then
  C_STEP=$'\033[1;34m'
  C_OK=$'\033[1;32m'
  C_WARN=$'\033[1;33m'
  C_ERR=$'\033[1;31m'
  C_OFF=$'\033[0m'
else
  C_STEP=""
  C_OK=""
  C_WARN=""
  C_ERR=""
  C_OFF=""
fi
readonly C_STEP C_OK C_WARN C_ERR C_OFF

step() { printf '\n%s==>%s %s\n' "${C_STEP}" "${C_OFF}" "$*"; }
info() { printf '    %s\n' "$*"; }
ok() { printf '    %s%s%s\n' "${C_OK}" "$*" "${C_OFF}"; }
warn() { printf '%s[warning]%s %s\n' "${C_WARN}" "${C_OFF}" "$*" >&2; }

die() {
  printf '\n%s[error]%s %s\n' "${C_ERR}" "${C_OFF}" "$1" >&2
  shift
  for line in "$@"; do
    printf '          %s\n' "${line}" >&2
  done
  exit 1
}

# Anything that changes the machine goes through run(), so --dry-run is honest:
# it prints exactly the commands the real run would execute.
run() {
  if [ "${DRY_RUN}" -eq 1 ]; then
    printf '    would run: %s\n' "$*"
    return 0
  fi
  "$@"
}

as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    run "$@"
  else
    run sudo "$@"
  fi
}

# ---------------------------------------------------------------- arguments

usage() {
  cat <<EOF
${SCRIPT_NAME} - install the hexapod_ws dependencies on a Raspberry Pi 4B or 5

Usage: ${SCRIPT_NAME} [options]

  --ros-distro NAME   ROS 2 distribution to install (default: ${ROS_DISTRO_WANTED})
  --with-gui          also install RViz and the joint state publisher sliders
  --with-dev-tools    also install clang, clang-tidy, clang-format and pre-commit
  --skip-hardware     do not touch the boot configuration, the modules or the groups
  --dry-run           print what would be done, change nothing
  --force             continue on an unrecognised board or distribution
  -h, --help          show this help

Without flags the script installs the lean set a headless robot needs: ROS 2
ros-base, the joy driver, robot_state_publisher, the Lua and I2C libraries and
the colcon/rosdep tooling.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --ros-distro)
      [ $# -ge 2 ] || die "--ros-distro needs a value, for instance: --ros-distro jazzy"
      ROS_DISTRO_WANTED="$2"
      shift 2
      ;;
    --ros-distro=*)
      ROS_DISTRO_WANTED="${1#*=}"
      shift
      ;;
    --with-gui)
      WITH_GUI=1
      shift
      ;;
    --with-dev-tools)
      WITH_DEV_TOOLS=1
      shift
      ;;
    --skip-hardware)
      SKIP_HARDWARE=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    --force)
      FORCE=1
      shift
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      die "unknown option: $1"
      ;;
  esac
done

readonly ROS_DISTRO_WANTED WITH_GUI WITH_DEV_TOOLS SKIP_HARDWARE DRY_RUN FORCE

# --------------------------------------------------------------- apt helpers

# A freshly flashed Pi runs unattended-upgrades on first boot and holds the dpkg
# lock for minutes. Waiting for it is the difference between an install that
# just works and one that dies on "could not get lock".
wait_for_apt() {
  local waited=0
  local -r limit=900
  while pgrep -x 'apt|apt-get|dpkg|unattended-upgr' >/dev/null 2>&1; do
    if [ "${waited}" -eq 0 ]; then
      info "another package manager is running, waiting for it to finish"
    fi
    if [ "${waited}" -ge "${limit}" ]; then
      die "the dpkg lock is still held after ${limit}s" \
        "Find the process with:  ps aux | grep -E 'apt|dpkg'" \
        "then re-run this script."
    fi
    sleep 5
    waited=$((waited + 5))
  done
}

apt_update() {
  wait_for_apt
  as_root env DEBIAN_FRONTEND=noninteractive apt-get update
}

package_installed() {
  dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q '^install ok installed$'
}

# Installs only what is missing, and repairs the package database before giving
# up: an interrupted install leaves dpkg half configured, which every later
# apt-get refuses to work around on its own.
apt_install() {
  local -a missing=()
  local pkg
  for pkg in "$@"; do
    package_installed "${pkg}" || missing+=("${pkg}")
  done

  if [ "${#missing[@]}" -eq 0 ]; then
    info "already present: $*"
    return 0
  fi

  info "installing: ${missing[*]}"
  wait_for_apt
  if as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
    --no-install-recommends "${missing[@]}"; then
    return 0
  fi

  warn "apt-get failed, repairing the package database and retrying once"
  as_root dpkg --configure -a || true
  as_root env DEBIAN_FRONTEND=noninteractive apt-get --fix-broken install -y || true
  apt_update
  as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
    --no-install-recommends "${missing[@]}" ||
    die "could not install: ${missing[*]}" \
      "Read the apt output above: it names the package that could not be resolved."
}

# ------------------------------------------------------------------- checks

# The account that will own ~/.ros, ~/.colcon and the group memberships. Under
# sudo that is the invoking user, not root, or every path below lands in /root.
TARGET_USER="${SUDO_USER:-$(id -un)}"
TARGET_HOME="$(getent passwd "${TARGET_USER}" | cut -d: -f6)"
readonly TARGET_USER TARGET_HOME

as_target_user() {
  if [ "$(id -un)" = "${TARGET_USER}" ]; then
    run "$@"
  else
    run sudo -u "${TARGET_USER}" -H "$@"
  fi
}

check_workspace() {
  [ -d "${REPO_ROOT}/src/hexapod_msgs" ] ||
    die "this is not a hexapod_ws checkout: ${REPO_ROOT}/src/hexapod_msgs is missing" \
      "Run the script from the workspace: ./scripts/${SCRIPT_NAME}"
}

check_privileges() {
  if [ "$(id -u)" -ne 0 ] && ! command -v sudo >/dev/null 2>&1; then
    die "sudo is not installed and this script is not running as root" \
      "Install sudo, or run:  su -c './scripts/${SCRIPT_NAME}'"
  fi
}

PI_MODEL="unknown"

check_board() {
  if [ -r /proc/device-tree/model ]; then
    PI_MODEL="$(tr -d '\0' </proc/device-tree/model)"
  fi

  case "${PI_MODEL}" in
    *"Raspberry Pi 5"* | *"Raspberry Pi 4 Model B"*)
      ok "board: ${PI_MODEL}"
      ;;
    *"Raspberry Pi"*)
      warn "board: ${PI_MODEL} - only the 4 Model B and the 5 are tested"
      ;;
    *)
      if [ "${FORCE}" -eq 1 ]; then
        warn "no Raspberry Pi detected, continuing because --force was given"
      else
        die "this does not look like a Raspberry Pi (model: ${PI_MODEL})" \
          "The script configures the I2C bus through the Pi boot configuration." \
          "Use --skip-hardware to install the software only, or --force to override."
      fi
      ;;
  esac
}

check_architecture() {
  local -r arch="$(uname -m)"
  case "${arch}" in
    aarch64 | arm64)
      ok "architecture: ${arch}"
      ;;
    armv7l | armv6l)
      die "this is a 32-bit userspace (${arch}) and ROS 2 publishes no 32-bit ARM packages" \
        "Flash a 64-bit image: Raspberry Pi Imager -> Ubuntu Server 24.04 LTS (64-bit)," \
        "or add 'arm_64bit=1' to the boot configuration of a 64-bit capable card."
      ;;
    *)
      warn "unexpected architecture: ${arch}"
      ;;
  esac
}

OS_ID=""
OS_CODENAME=""
OS_PRETTY=""

check_distribution() {
  [ -r /etc/os-release ] ||
    die "/etc/os-release is missing, the distribution cannot be identified"

  # shellcheck source=/dev/null
  . /etc/os-release
  OS_ID="${ID:-unknown}"
  OS_CODENAME="${VERSION_CODENAME:-unknown}"
  OS_PRETTY="${PRETTY_NAME:-${OS_ID} ${OS_CODENAME}}"
  readonly OS_ID OS_CODENAME OS_PRETTY

  case "${OS_ID}:${OS_CODENAME}" in
    ubuntu:noble)
      ok "distribution: ${OS_PRETTY} - ROS 2 ${ROS_DISTRO_WANTED} is supported here"
      ;;
    ubuntu:*)
      warn "distribution: ${OS_PRETTY} - ROS 2 ${ROS_DISTRO_WANTED} targets Ubuntu 24.04"
      ;;
    debian:* | raspbian:*)
      warn "distribution: ${OS_PRETTY} - Debian is a source-only tier for ROS 2"
      info "packages.ros.org is probed below; if it has no suite the run stops there"
      ;;
    *)
      warn "distribution: ${OS_PRETTY} - untested"
      ;;
  esac
}

# --------------------------------------------------------------------- ROS 2

ros_already_installed() {
  [ -f "/opt/ros/${ROS_DISTRO_WANTED}/setup.bash" ]
}

ros_repo_flavour() {
  case "${OS_ID}" in
    ubuntu) printf '%s\n' ubuntu ;;
    debian | raspbian) printf '%s\n' debian ;;
    *) printf '%s\n' "${OS_ID}" ;;
  esac
}

# Whether an apt source exists for a codename says nothing about whether any
# package was ever built for it: ros-apt-source ships a .deb for bookworm, but
# packages.ros.org has no bookworm suite at all. The repository index is the
# only honest test, and it costs one request before anything is modified.
#
# The URL is http on purpose. packages.ros.org presents a certificate that does
# not match the hostname, so https fails with curl error 60 and this check
# would report a perfectly good Ubuntu as unsupported. The apt source the ROS
# project publishes uses http for the same reason; the repository is trusted
# through its GPG signature, not through TLS.
check_ros_repository() {
  local flavour release_url
  flavour="$(ros_repo_flavour)"
  release_url="http://packages.ros.org/ros2/${flavour}/dists/${OS_CODENAME}/Release"

  info "checking packages.ros.org for ${flavour}/${OS_CODENAME}"
  if curl -fsIL -o /dev/null "${release_url}"; then
    ok "ROS 2 packages are published for ${OS_ID} ${OS_CODENAME}"
    return 0
  fi

  die "ROS 2 publishes no binary packages for ${OS_PRETTY}" \
    "Looked for: ${release_url}" \
    "" \
    "ROS 2 ${ROS_DISTRO_WANTED} is a Tier 1 platform on Ubuntu 24.04 (noble) only." \
    "Raspberry Pi OS is Debian based and has no suite on packages.ros.org," \
    "so there is nothing to install from apt there." \
    "" \
    "On a Raspberry Pi 4B or 5 the supported path is to flash Ubuntu Server" \
    "24.04 LTS 64-bit with Raspberry Pi Imager and run this script again." \
    "Building ROS 2 from source is possible but takes hours and is not what" \
    "this script does."
}

# The ros-apt-source .deb carries the keyring and the sources.list entry for one
# distribution codename. Asking for the asset before downloading it turns "apt
# cannot find ros-jazzy-ros-base" into a message that says what is missing.
install_ros_apt_source() {
  if [ -f /etc/apt/sources.list.d/ros2.list ] ||
    [ -f /etc/apt/sources.list.d/ros2-apt-source.list ] ||
    [ -f /etc/apt/sources.list.d/ros2-apt-source.sources ]; then
    info "the ROS 2 apt source is already configured"
    return 0
  fi

  check_ros_repository

  local version
  version="$(curl -fsSL https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest |
    grep -F '"tag_name"' | awk -F'"' '{print $4}')" ||
    die "could not reach github.com to look up the ROS apt source release" \
      "Check the network and the DNS, then re-run."

  [ -n "${version}" ] ||
    die "the ROS apt source release could not be parsed from the GitHub API"

  local -r url="https://github.com/ros-infrastructure/ros-apt-source/releases/download/${version}/ros2-apt-source_${version}.${OS_CODENAME}_all.deb"

  info "checking for an apt source built for ${OS_ID} ${OS_CODENAME}"
  if ! curl -fsIL -o /dev/null "${url}"; then
    die "no ros-apt-source package exists for ${OS_ID} ${OS_CODENAME}" \
      "Expected: ${url}" \
      "packages.ros.org carries a suite for this release, but ros-apt-source" \
      "${version} ships no keyring for it. Add the source by hand, or install" \
      "on a release the project builds for."
  fi

  info "installing the ROS 2 apt source ${version}"
  local deb
  deb="$(mktemp --suffix=.deb)"
  run curl -fsSL -o "${deb}" "${url}"
  wait_for_apt
  as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y "${deb}" ||
    die "the ROS apt source package could not be installed"
  run rm -f "${deb}"
  apt_update
}

# Once the source is in place, apt itself is the authority on whether this
# release really carries this ROS distribution. Asking before installing keeps
# the failure on the line that can explain it.
verify_ros_candidate() {
  local -r pkg="ros-${ROS_DISTRO_WANTED}-ros-base"

  [ "${DRY_RUN}" -eq 0 ] || return 0
  command -v apt-cache >/dev/null 2>&1 || return 0

  if apt-cache policy "${pkg}" 2>/dev/null | grep -q 'Candidate: (none)'; then
    die "${pkg} has no installation candidate on ${OS_PRETTY}" \
      "The apt source is configured, but this release carries no" \
      "${ROS_DISTRO_WANTED} packages for $(dpkg --print-architecture)." \
      "Inspect it with:  apt-cache policy ${pkg}"
  fi
}

install_ros_packages() {
  local -a ros_packages=(
    "ros-${ROS_DISTRO_WANTED}-ros-base"
    "ros-${ROS_DISTRO_WANTED}-joy"
    "ros-${ROS_DISTRO_WANTED}-robot-state-publisher"
    "ros-${ROS_DISTRO_WANTED}-xacro"
    ros-dev-tools
  )

  if [ "${WITH_GUI}" -eq 1 ]; then
    ros_packages+=(
      "ros-${ROS_DISTRO_WANTED}-rviz2"
      "ros-${ROS_DISTRO_WANTED}-joint-state-publisher-gui"
    )
  fi

  verify_ros_candidate
  apt_install "${ros_packages[@]}"

  ros_already_installed || [ "${DRY_RUN}" -eq 1 ] ||
    die "ROS 2 ${ROS_DISTRO_WANTED} is still not in /opt/ros after the install" \
      "Check the apt output above for a package that failed to configure."
}

# rosdep is what makes "resolve the missing dependencies" a command rather than
# a reading exercise: it maps the <depend> keys in every package.xml onto apt
# packages and installs the ones this machine does not have.
setup_rosdep() {
  if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
    info "initialising rosdep"
    as_root rosdep init
  else
    info "rosdep is already initialised"
  fi

  info "updating the rosdep database as ${TARGET_USER}"
  as_target_user rosdep update --rosdistro "${ROS_DISTRO_WANTED}" ||
    warn "rosdep update failed; the resolution below may be incomplete"
}

resolve_workspace_dependencies() {
  local -a rosdep_args=(
    install
    --from-paths "${REPO_ROOT}/src"
    --ignore-src
    --rosdistro "${ROS_DISTRO_WANTED}"
    -y
  )

  # rosdep keys are indexed per operating system. Raspberry Pi OS still reports
  # itself as raspbian on some images, which has no rules of its own, so it is
  # resolved against the Debian release it is built from.
  if [ "${OS_ID}" = "raspbian" ]; then
    rosdep_args+=(--os "debian:${OS_CODENAME}")
  fi

  wait_for_apt
  as_target_user rosdep "${rosdep_args[@]}" ||
    die "rosdep could not resolve every dependency" \
      "Re-run with the key it names, or install it by hand and run this script again."
}

install_dev_tools() {
  [ "${WITH_DEV_TOOLS}" -eq 1 ] || return 0

  step "Installing the development tooling"
  apt_install clang clang-format clang-tidy clangd cppcheck pipx shellcheck

  if command -v pre-commit >/dev/null 2>&1; then
    info "pre-commit is already available"
  else
    info "installing pre-commit and commitizen with pipx"
    as_target_user pipx install pre-commit || warn "pipx install pre-commit failed"
    as_target_user pipx install commitizen || warn "pipx install commitizen failed"
  fi
}

# ------------------------------------------------------------------ hardware

REBOOT_NEEDED=0

boot_config_path() {
  # Bookworm and Ubuntu 24.04 both mount the firmware partition on
  # /boot/firmware; older images used /boot directly.
  if [ -f /boot/firmware/config.txt ]; then
    printf '%s\n' /boot/firmware/config.txt
  elif [ -f /boot/config.txt ]; then
    printf '%s\n' /boot/config.txt
  else
    printf '%s\n' ""
  fi
}

enable_i2c() {
  local -r config="$(boot_config_path)"

  if [ -z "${config}" ]; then
    warn "no config.txt found under /boot or /boot/firmware, I2C not enabled"
    warn "add 'dtparam=i2c_arm=on' to the boot configuration by hand"
    return 0
  fi

  if grep -qE '^[[:space:]]*dtparam=i2c_arm=on' "${config}"; then
    info "I2C is already enabled in ${config}"
  else
    info "enabling I2C in ${config}"
    # Only the interface is switched on. The bus speed is deliberately left
    # alone: the PCA9685 timing depends on it and this script must not change
    # what the hardware does behind the operator's back.
    printf '%s\n' "" "# Enabled by hexapod_ws ${SCRIPT_NAME}" "dtparam=i2c_arm=on" |
      as_root tee -a "${config}" >/dev/null
    REBOOT_NEEDED=1
  fi

  if [ -f /etc/modules-load.d/hexapod-i2c.conf ]; then
    info "the i2c-dev module is already loaded at boot"
  else
    info "loading i2c-dev at every boot"
    printf 'i2c-dev\n' | as_root tee /etc/modules-load.d/hexapod-i2c.conf >/dev/null
  fi

  if lsmod 2>/dev/null | grep -q '^i2c_dev'; then
    info "the i2c-dev module is loaded"
  else
    as_root modprobe i2c-dev || warn "modprobe i2c-dev failed, a reboot will load it"
  fi
}

add_user_to_groups() {
  local group
  for group in i2c input; do
    if ! getent group "${group}" >/dev/null; then
      info "the ${group} group does not exist yet, creating it"
      as_root groupadd -f "${group}"
    fi
    if id -nG "${TARGET_USER}" | tr ' ' '\n' | grep -qx "${group}"; then
      info "${TARGET_USER} is already in the ${group} group"
    else
      info "adding ${TARGET_USER} to the ${group} group"
      as_root usermod -aG "${group}" "${TARGET_USER}"
      REBOOT_NEEDED=1
    fi
  done
}

# Read-only probe: i2cdetect uses an SMBus read for the 0x40-0x41 range, so it
# does not write to the boards. A wiring or address mistake is far cheaper to
# find here than from a node that reports an unreachable bus.
probe_boards() {
  command -v i2cdetect >/dev/null 2>&1 || {
    warn "i2cdetect is not installed, skipping the bus probe"
    return 0
  }

  local bus found_any=0 device number
  for device in /dev/i2c-*; do
    [ -e "${device}" ] || continue
    number="${device#/dev/i2c-}"
    bus="$(i2cdetect -y "${number}" 2>/dev/null)" || continue
    local left right
    left=0
    right=0
    printf '%s\n' "${bus}" | grep -qE '(^|[[:space:]])40([[:space:]]|$)' && left=1
    printf '%s\n' "${bus}" | grep -qE '(^|[[:space:]])41([[:space:]]|$)' && right=1
    if [ "${left}" -eq 1 ] || [ "${right}" -eq 1 ]; then
      found_any=1
      if [ "${left}" -eq 1 ] && [ "${right}" -eq 1 ]; then
        ok "both PCA9685 boards answer on ${device} (0x40 and 0x41)"
        if [ "${device}" != "/dev/i2c-1" ]; then
          info "set i2c_bus to ${device} in servoconfiguration.yaml, the default is /dev/i2c-1"
        fi
      elif [ "${left}" -eq 1 ]; then
        warn "only 0x40 answers on ${device}: the right board still has the stock address"
        info "solder the A0 jumper on the second board, see doc/pca9685.md"
      else
        warn "only 0x41 answers on ${device}: the left board is missing"
      fi
    fi
  done

  if [ "${found_any}" -eq 0 ]; then
    warn "no PCA9685 answered on any I2C bus"
    info "this is expected before the boards are wired, or before the reboot"
    info "check again later with:  i2cdetect -y 1"
  fi
}

# ----------------------------------------------------------------- build size

# A Pi 4B with 2 GB runs out of memory compiling several rclcpp translation
# units at once, and the OOM killer stops colcon with a message that blames the
# compiler. Sizing the workers from the memory that exists avoids the whole
# class of failure.
configure_build_parallelism() {
  local mem_kb mem_gib workers cores
  mem_kb="$(awk '/^MemTotal:/ {print $2}' /proc/meminfo)"
  mem_gib=$((mem_kb / 1024 / 1024))
  cores="$(nproc)"

  workers=$((mem_gib / 2))
  [ "${workers}" -lt 1 ] && workers=1
  [ "${workers}" -gt "${cores}" ] && workers="${cores}"

  info "${mem_gib} GiB of RAM, ${cores} cores -> ${workers} parallel colcon worker(s)"

  local -r defaults="${TARGET_HOME}/.colcon/defaults.yaml"
  if [ -e "${defaults}" ]; then
    info "${defaults} already exists, leaving it untouched"
    info "for reference, this machine can take:  colcon build --parallel-workers ${workers}"
  else
    info "writing ${defaults} so every colcon build uses ${workers} worker(s)"
    as_target_user mkdir -p "${TARGET_HOME}/.colcon"
    if [ "${DRY_RUN}" -eq 0 ]; then
      printf '%s\n' \
        '# Written by hexapod_ws scripts/install-raspberrypi.sh.' \
        '# Sized from the memory of this machine: compiling rclcpp packages in' \
        '# parallel is what exhausts a 2 GB Pi. Raise it if you add swap.' \
        'build:' \
        "  parallel-workers: ${workers}" |
        as_target_user tee "${defaults}" >/dev/null
    fi
  fi

  local swap_kb
  swap_kb="$(awk '/^SwapTotal:/ {print $2}' /proc/meminfo)"
  if [ "${mem_gib}" -lt 4 ] && [ "${swap_kb}" -lt 1048576 ]; then
    warn "${mem_gib} GiB of RAM and less than 1 GiB of swap"
    info "the build may still be killed; add swap with:"
    info "  sudo fallocate -l 2G /swapfile && sudo chmod 600 /swapfile"
    info "  sudo mkswap /swapfile && sudo swapon /swapfile"
  fi
}

# ---------------------------------------------------------------------- main

main() {
  printf '%s\n' "hexapod_ws - Raspberry Pi setup"
  [ "${DRY_RUN}" -eq 1 ] && warn "dry run: nothing will be changed"

  step "Checking the machine"
  check_workspace
  check_privileges
  check_board
  check_architecture
  check_distribution

  step "Installing the base packages"
  apt_update
  apt_install "${PACKAGES_BASE[@]}"

  step "Installing the project libraries"
  apt_install "${PACKAGES_PROJECT[@]}"

  step "Installing ROS 2 ${ROS_DISTRO_WANTED}"
  if ros_already_installed; then
    ok "ROS 2 ${ROS_DISTRO_WANTED} is already in /opt/ros"
  else
    install_ros_apt_source
  fi
  install_ros_packages

  step "Resolving the workspace dependencies"
  setup_rosdep
  resolve_workspace_dependencies

  install_dev_tools

  if [ "${SKIP_HARDWARE}" -eq 1 ]; then
    step "Hardware configuration skipped (--skip-hardware)"
  else
    step "Configuring the I2C bus"
    enable_i2c
    add_user_to_groups
    probe_boards
  fi

  step "Sizing the build for this machine"
  configure_build_parallelism

  step "Done"
  ok "the dependencies for hexapod_ws are installed"
  printf '\n'
  info "Next:"
  info "  source /opt/ros/${ROS_DISTRO_WANTED}/setup.bash"
  info "  cd ${REPO_ROOT} && ./build_exapod.sh"
  info "  source install/setup.bash"
  info "  ros2 launch hexapod_joypad hexapod_joypad.launch.py"

  if [ "${REBOOT_NEEDED}" -eq 1 ]; then
    printf '\n'
    warn "a reboot is required: the I2C interface or your group membership changed"
    info "  sudo reboot"
  fi
}

main "$@"
