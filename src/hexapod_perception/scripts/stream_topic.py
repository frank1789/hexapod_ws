#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Publish a ROS image topic to an RTSP server as H.264.

The node subscribes to one image topic, hands every frame to an ffmpeg it owns,
and ffmpeg pushes the result to MediaMTX. MediaMTX starts this command on demand
and stops it when the last viewer leaves, so a stream nobody is watching costs
nothing — which matters on a Pi 4 that is also driving eighteen servos.

Two encodings are accepted, and the difference is the point of this node:

  rgb8    the colour stream, forwarded untouched.
  16UC1   the depth stream, in millimetres, colourised here into a JET heatmap.

Colourising here rather than in the camera node is deliberate. The RealSense
node has a `colorizer` filter that does the same job, but it *replaces* the
depth topic: turn it on and `/d455/depth/image_rect_raw` stops carrying
millimetres and starts carrying false colour, which takes metric depth away
from every other subscriber. Doing it in the streaming path leaves the topic
alone — the heatmap exists only inside the video stream.

The heatmap is not depth data. Distances are clipped to a fixed window and
mapped onto a colour ramp, so it is something to look at, not something to
measure with. Pixels the camera could not resolve are zero, and stay black
rather than being mapped to the near end of the ramp.

Run by MediaMTX via the `hexapod-stream` wrapper; see config/mediamtx.yml.
"""

import argparse
import os
import signal
import subprocess
import sys

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Image

try:
    import cv2
except ImportError as exc:  # pragma: no cover - depends on the image, not on logic
    raise SystemExit(f"stream_topic: OpenCV is required for the depth heatmap: {exc}")


def env_str(name, default):
    """Read an environment override, falling back when it is unset or empty."""
    value = os.environ.get(name, "")
    return value if value else default


def parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--topic", required=True, help="image topic to stream")
    parser.add_argument("--rtsp", required=True, help="RTSP URL to publish to")
    parser.add_argument(
        "--fps",
        type=int,
        default=int(env_str("HEXAPOD_STREAM_FPS", "30")),
        help="frame rate declared to ffmpeg",
    )
    parser.add_argument(
        "--encoder",
        default=env_str("HEXAPOD_STREAM_ENCODER", "libx264"),
        help="ffmpeg video encoder (h264_v4l2m2m uses the Pi 4 hardware encoder)",
    )
    parser.add_argument(
        "--bitrate",
        default=env_str("HEXAPOD_STREAM_BITRATE", "2M"),
        help="target video bitrate",
    )
    parser.add_argument(
        "--depth-min",
        type=float,
        default=float(env_str("HEXAPOD_STREAM_DEPTH_MIN_M", "0.3")),
        help="distance in metres mapped to the near end of the heatmap",
    )
    parser.add_argument(
        "--depth-max",
        type=float,
        default=float(env_str("HEXAPOD_STREAM_DEPTH_MAX_M", "4.0")),
        help="distance in metres mapped to the far end of the heatmap",
    )
    args = parser.parse_args(argv)
    if args.depth_max <= args.depth_min:
        parser.error("--depth-max must be greater than --depth-min")
    return args


class Streamer(Node):
    """Subscribe to one image topic and feed an ffmpeg process."""

    def __init__(self, args):
        super().__init__("stream_topic")
        self.args = args
        self.ffmpeg = None
        self.pixel_format = None
        self.frames = 0
        self.create_subscription(Image, args.topic, self.on_image, qos_profile_sensor_data)
        self.get_logger().info(f"streaming {args.topic} to {args.rtsp} via {args.encoder}")

    # -- frame conversion ---------------------------------------------------

    def to_frame(self, msg):
        """Return the bytes ffmpeg should receive, and the pixel format they are in.

        `step` is honoured rather than assumed equal to width times the pixel
        size: a driver is free to pad rows, and reshaping on width alone would
        shear the picture without ever failing loudly.
        """
        if msg.is_bigendian:
            raise RuntimeError(f"{self.args.topic} is big-endian, which is not handled")

        rows = np.frombuffer(msg.data, dtype=np.uint8).reshape(msg.height, msg.step)

        if msg.encoding == "rgb8":
            return np.ascontiguousarray(rows[:, : msg.width * 3]), "rgb24"

        if msg.encoding == "16UC1":
            packed = np.ascontiguousarray(rows[:, : msg.width * 2])
            depth_mm = packed.view(np.uint16).reshape(msg.height, msg.width)
            return self.colourise(depth_mm), "bgr24"

        raise RuntimeError(f"{self.args.topic} carries {msg.encoding}, expected rgb8 or 16UC1")

    def colourise(self, depth_mm):
        """Map millimetres onto a JET ramp, leaving pixels without data black."""
        near_mm = self.args.depth_min * 1000.0
        far_mm = self.args.depth_max * 1000.0

        span = np.clip(depth_mm.astype(np.float32), near_mm, far_mm)
        scaled = (span - near_mm) * (255.0 / (far_mm - near_mm))
        heatmap = cv2.applyColorMap(scaled.astype(np.uint8), cv2.COLORMAP_JET)

        # Zero means the camera resolved nothing there. Left alone it would clip
        # to the near end of the ramp and read as an object right in front of
        # the lens, which is the opposite of what it means.
        heatmap[depth_mm == 0] = 0
        return heatmap

    # -- ffmpeg -------------------------------------------------------------

    def start_ffmpeg(self, width, height, pixel_format):
        """Spawn ffmpeg once the first frame has told us the size to expect."""
        # Built in groups rather than as one long list: each line stays inside
        # the formatter's width, so black leaves the pairing of flag and value
        # alone instead of exploding it one token per line.
        command = ["ffmpeg", "-hide_banner", "-loglevel", "warning"]
        command += ["-f", "rawvideo", "-pix_fmt", pixel_format]
        command += ["-s", f"{width}x{height}", "-r", str(self.args.fps), "-i", "-"]
        command += ["-c:v", self.args.encoder, "-b:v", self.args.bitrate]

        # One keyframe per second. A viewer joining an RTSP stream sees nothing
        # until the next one arrives, so a longer group of pictures trades
        # startup delay for bitrate.
        command += ["-g", str(self.args.fps), "-pix_fmt", "yuv420p"]

        if self.args.encoder == "libx264":
            # Neither flag exists on the hardware encoder, which rejects the
            # whole command line rather than ignoring what it does not know.
            command += ["-preset", "ultrafast", "-tune", "zerolatency"]

        command += ["-f", "rtsp", "-rtsp_transport", "tcp", self.args.rtsp]

        self.get_logger().info(f"{width}x{height} {pixel_format}, starting ffmpeg")
        self.ffmpeg = subprocess.Popen(command, stdin=subprocess.PIPE)

    def on_image(self, msg):
        frame, pixel_format = self.to_frame(msg)

        if self.ffmpeg is None:
            self.pixel_format = pixel_format
            self.start_ffmpeg(msg.width, msg.height, pixel_format)

        if self.ffmpeg.poll() is not None:
            raise SystemExit(f"ffmpeg exited with {self.ffmpeg.returncode}")

        try:
            self.ffmpeg.stdin.write(frame.tobytes())
        except BrokenPipeError:
            raise SystemExit("ffmpeg closed its input")

        self.frames += 1

    # -- teardown -----------------------------------------------------------

    def stop_ffmpeg(self):
        """Close ffmpeg's input and let it flush before insisting."""
        if self.ffmpeg is None:
            return
        try:
            self.ffmpeg.stdin.close()
            self.ffmpeg.wait(timeout=5)
        except (BrokenPipeError, OSError):
            pass
        except subprocess.TimeoutExpired:
            self.ffmpeg.kill()


def main(argv=None):
    args = parse_args(argv if argv is not None else sys.argv[1:])

    rclpy.init()
    node = Streamer(args)

    # MediaMTX stops the command with SIGTERM when the last viewer disconnects.
    # Raising SystemExit from the handler unwinds through the same path as a
    # broken pipe, so ffmpeg is shut down once, in one place.
    def on_term(_signum, _frame):
        raise SystemExit(0)

    signal.signal(signal.SIGTERM, on_term)

    status = 0
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except SystemExit as exc:
        status = exc.code if isinstance(exc.code, int) else 1
    finally:
        node.stop_ffmpeg()
        node.get_logger().info(f"stopped after {node.frames} frames")
        node.destroy_node()
        # rclpy installs its own SIGTERM handler and shuts the context down from
        # there, so by the time the handler above has unwound this may already
        # have happened. Calling it twice raises, and raising here would replace
        # whatever actually went wrong with a traceback about shutdown.
        if rclpy.ok():
            rclpy.shutdown()

    return status


if __name__ == "__main__":
    sys.exit(main())
