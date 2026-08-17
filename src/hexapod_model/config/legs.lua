-- Leg geometry and hip mounts for hexapod_model.
--
-- This file is the single source of truth for the robot's measurements, in the
-- same way config/motors.lua is for the motor table: no length, offset or limit
-- is compiled into C++, so re-tuning a chassis or swapping a femur needs no
-- rebuild. The C++ side rejects a leg it cannot solve rather than defaulting,
-- so an incomplete entry here fails loudly at start-up.
--
-- Units are metres and radians. Joint angles follow the URDF sign convention:
-- each joint is centred on 0 and travels +/-1.5708 rad. That is NOT the servo
-- convention in config/motors.lua, which runs 0-180 degrees with 90 as the rest
-- pose, and nothing in this workspace bridges the two yet.
--
-- Provenance: every value below is read out of, or derived from,
-- hexapod_description/urdf/Hexapod.urdf.
--   coxa_length    origin of <leg>_femur_jnt  = 0.0294 for all six legs
--   femur_length   origin of <leg>_tibia_jnt  = 0.08   for all six legs
--   tibia_length   distance from the tibia pivot to the rest-pose foot marker.
--                  Cross-checked against the tibia link's own mesh origin,
--                  which agrees to 3e-06 m; the link's inertial origin is
--                  exactly half of it, as a uniform rod's centre of mass.
--   tibia_offset   the constant bend built into the tibia link, about -93.6
--                  degrees. It is what makes the foot hang below the pivot at
--                  zero tibia angle.
--   lateral_offset sideways displacement of the leg plane, under a millimetre
--                  and mirrored between the sides.
--   sigma_coxa     -1 where the URDF coxa axis is (0, 0, -1), +1 where it is
--                  (0, 0, +1): the left legs mirror the right ones.
--   mount          origin and yaw of <leg>_coxa_jnt in the body frame. The URDF
--                  mounts differ in yaw only, so roll and pitch are not part of
--                  the schema.
--
-- Lua 5.1 only: LuaJIT tracks 5.1, so no integer division, no goto, no bitwise
-- operators. See CLAUDE.md.

-- Shared by every leg: they are built from the same parts.
local COXA_LENGTH = 0.0294
local FEMUR_LENGTH = 0.0800
local JOINT_MIN = -1.5708
local JOINT_MAX = 1.5708

-- One leg. Only the mirrored and per-leg measured values are arguments.
local function leg(sigma_coxa, tibia_length, tibia_offset, lateral_offset, mount_x, mount_y, mount_yaw)
  return {
    coxa_length = COXA_LENGTH,
    femur_length = FEMUR_LENGTH,
    tibia_length = tibia_length,
    tibia_offset = tibia_offset,
    lateral_offset = lateral_offset,
    sigma_coxa = sigma_coxa,
    joint_min = JOINT_MIN,
    joint_max = JOINT_MAX,
    mount = { x = mount_x, y = mount_y, z = 0.0, yaw = mount_yaw },
  }
end

-- Keyed by the URDF leg names. The C++ loader requires all six and makes no
-- claim about which of the A/B/C suffixes in motors.lua each one drives: that
-- mapping is not recorded anywhere in this repository.
Legs = {
  L_front = leg(-1.0, 0.117236, -1.634318, -0.000937, 0.08760, 0.05057, 1.04720),
  L_mid = leg(-1.0, 0.117236, -1.634301, -0.000940, 0.00000, 0.06985, 1.57080),
  L_back = leg(-1.0, 0.117236, -1.634283, -0.000937, -0.08760, 0.05057, 2.09440),
  R_front = leg(1.0, 0.117233, -1.633915, 0.000979, 0.08760, -0.05057, -1.04720),
  R_mid = leg(1.0, 0.117234, -1.633960, 0.000990, 0.00000, -0.06985, -1.57080),
  R_back = leg(1.0, 0.117234, -1.633968, 0.000993, -0.08760, -0.05057, -2.09440),
}
