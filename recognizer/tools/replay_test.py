#!/usr/bin/env python3
"""Deterministic offline replay for an elevator recognizer implementation."""
import argparse
import importlib.util
from importlib.machinery import SourceFileLoader
import os
import subprocess
import json
import sys
from pathlib import Path

from PIL import Image


class FakeTime:
    def __init__(self, fps):
        self.value = 1000.0
        self.step = 1.0 / fps

    def monotonic(self):
        return self.value

    def advance(self):
        self.value += self.step


def main():
    package_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("frames")
    parser.add_argument("--script", default=str(package_root / "lift-recognizer-v6.py"))
    parser.add_argument("--fps", type=float, default=4.0)
    parser.add_argument("--diagnostics", action="store_true")
    parser.add_argument("--json-output")
    args = parser.parse_args()

    os.environ["LIFT_RTSP_URL"] = "offline"
    os.environ["LIFT_TEMPLATES"] = str(package_root / "templates-v4.json")
    os.environ["LIFT_SEGMENT_MODEL"] = str(package_root / "segment-v5.json")
    os.environ["LIFT_RED_SEGMENT_MODEL"] = str(package_root / "segment-red-v6.json")
    os.environ["LIFT_ALIGNMENT_MODEL"] = str(package_root / "panel-alignment.json")

    sys.path.insert(0, str(Path(args.script).resolve().parent))
    loader = SourceFileLoader("recognizer_under_test", args.script)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.mqtt_publish = lambda *unused_args, **unused_kwargs: None
    clock = FakeTime(args.fps)
    module.time = clock

    recognizer = module.Recognizer()
    payloads = []
    recognizer.publish = lambda payload, force=False: payloads.append(payload.copy())
    source=Path(args.frames)
    if source.is_file():
        raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(source),'-vf',f'fps={args.fps},scale=120:94','-f','rawvideo','-pix_fmt','rgb24','-'])
        frames=(Image.frombytes('RGB',(120,94),raw[i:i+33840]) for i in range(0,len(raw)-33839,33840))
    else:
        paths=sorted(p for p in source.iterdir() if p.suffix.lower() in {'.jpg','.jpeg','.png'})
        frames=(Image.open(path).convert('RGB') for path in paths)
    count=0
    for frame in frames:
        recognizer.process(frame)
        count+=1
        clock.advance()

    if args.json_output:
        Path(args.json_output).write_text(json.dumps(payloads))

    events = []
    for index, payload in enumerate(payloads, start=3):
        event = (payload["floor"], payload["direction"], payload["state"])
        if not events or events[-1][1] != event:
            diagnostics = None
            if args.diagnostics:
                diagnostics = {
                    "d": payload.get("distance"), "m": payload.get("margin"),
                    "seg": payload.get("segment_floor"), "se": payload.get("segment_errors"),
                    "red": payload.get("red_segment_floor"), "re": payload.get("red_segment_errors"),
                    "geo": payload.get("geometry_floor"), "ge": payload.get("geometry_errors"),
                }
            events.append((index, event, payload.get("floor_source"), diagnostics))
    for index, event, source, diagnostics in events:
        print(f"{index:05d} {index / args.fps:7.2f}s {event} {source}", diagnostics or "")
    print("frames", count, "events", len(events),
          "final", events[-1] if events else None)


if __name__ == "__main__":
    main()
