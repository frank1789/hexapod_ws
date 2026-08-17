# What the kinematic model costs

Short version: a full six-leg solve takes about a microsecond, the code is not
limited by vector arithmetic, and **adding `-mavx2` makes it slower**. This page
records the measurement so the next person to have the idea can read the numbers
instead of running the experiment again.

## The question

`hexapod_model` is built on Eigen, and Eigen vectorises. It is a reasonable
instinct to ask CMake to detect the host architecture and switch on the widest
instruction set it finds — AVX2 on an Intel or AMD machine, something equivalent
on the Raspberry Pi. `src/hexapod_model/CMakeLists.txt` deliberately does not do
that.

## The measurement

Two million full six-leg solves per configuration — `FeetFromJoints` followed by
`SolveJoints`, which is eighteen forward-kinematic evaluations and eighteen
inverse ones — on an Intel Core i9-9900 at 3.10 GHz, GCC with `-O2 -DNDEBUG`,
Eigen from vcpkg. Runs interleaved so thermal drift cannot favour one build.

| Configuration | Per solve | Against baseline |
|---|---:|---:|
| baseline, the x86-64 default (SSE2) | ~925 ns | — |
| `-mavx2 -mfma` | ~1040 ns | **+12 %** |
| `-march=native` | ~1030 ns | **+11 %** |
| `-DEIGEN_DONT_VECTORIZE` | ~942 ns | +2 % |

Three interleaved repetitions, baseline then AVX2 each time: 938.8 / 1049.7,
922.2 / 1023.9, 920.8 / 1046.6 ns. The gap is consistent, not noise.

## Why wider registers lose

The shape of the data decides it, not the compiler.

`Eigen::Vector3d` is three doubles. An AVX packet holds four. Eigen therefore
never fills a packet, and what would have been the payoff becomes overhead:
partial loads, `vzeroupper` transitions between the wide and narrow encodings,
and no reduction in the number of instructions that matter. `Matrix3d` is nine
doubles, which fits a four-wide packet no better.

The last row of the table is the same fact from the other side. Turning Eigen's
vectorisation off completely costs only about 2 %, which means there was almost
no vector work being done to begin with. The time goes to scalar `sin`, `cos`,
`atan2`, `acos` and `sqrt` — the trigonometry in `ForwardKinematics` and
`InverseKinematics` — and those go through libm either way.

## It is also not ABI-neutral

`-mavx2` moves `EIGEN_MAX_ALIGN_BYTES` from 16 to 32. That changes the alignment
of every fixed-size vectorisable Eigen type, and `BodyPose` holds an
`Eigen::Quaterniond`, which is exactly 32 bytes and therefore one of them.

Compiling one package with the flag and its consumers without it is a silent
mismatch: the type has a different alignment requirement on each side of the
boundary. It does not fail to build. It surfaces later as a misaligned load on
the robot, which is a far worse thing to debug than a compiler error. So the
flag could never be set for `hexapod_model` alone — it would have to apply to
the whole workspace, uniformly, forever.

## On the Raspberry Pi there is nothing to switch on

`scripts/install-raspberrypi.sh` refuses a 32-bit userspace, so the robot is
always `aarch64`. ARMv8-A makes Advanced SIMD (NEON) part of the baseline
instruction set: it is already enabled and there is no flag that turns it on.
`-mfpu=neon` is the ARMv7 spelling and the compiler will not accept it on a
64-bit target.

Only `-mcpu` tuning would change anything there — `cortex-a72` for a Pi 4,
`cortex-a76` for a Pi 5 — and that schedules instructions for a particular core
rather than widening them. It also produces a binary that will not run on the
other board, which is a poor trade for a workload measured in microseconds.

## The bottleneck is somewhere else entirely

`ServoController::WriteOnMotor` sleeps `settle_time_ms` per motor, so writing one
pose to eighteen joints takes `18 × settle_time_ms` — **900 ms** at the shipped
default. A solve costs roughly a microsecond.

Tuning the arithmetic is therefore a change of about 0.0001 % to the loop the
robot actually runs. Work on making the robot move smoothly belongs in the servo
write path, as [the architecture notes](architecture.md) describe, not here.

## Reproducing it

Build the two kinematics translation units together with a driver that calls
them in a loop, against the vcpkg Eigen, and vary only the flags:

```sh
cd src/hexapod_model
EIGEN=/opt/vcpkg_installed/x64-linux/include/eigen3

for flags in "" "-mavx2 -mfma" "-march=native" "-DEIGEN_DONT_VECTORIZE"; do
  g++ -std=c++20 -O2 -DNDEBUG ${flags} \
      -I include -isystem "${EIGEN}" \
      src/leg_model.cc src/model.cc bench.cc -o /tmp/bench
  printf '%-24s ' "${flags:-baseline}"; /tmp/bench
done
```

The driver needs to defeat the optimiser — mutate an input inside the loop and
accumulate something from the result — or the whole thing is hoisted out and
every configuration reports the same implausible figure. Printing
`EIGEN_MAX_ALIGN_BYTES` and `Eigen::internal::packet_traits<double>::size` from
the same binary is what shows the alignment change and the packet width behind
the numbers above.

## If you want to try it anyway

Measure first, on the machine that will run the code, with this workload rather
than a synthetic one. If a future gait engine adds large matrix work — a
whole-body dynamics solve, an optimiser over a horizon — the shape of the data
changes and the answer may change with it. The reasoning above is about
three-element vectors and scalar trigonometry, and it stops applying the moment
that is no longer what the package spends its time on.
