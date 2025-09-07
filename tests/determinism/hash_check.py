#!/usr/bin/env python3
"""
Determinism Hash Check

Runs Glint3D headless with a given ops file and seed, renders to PNG,
and computes a SHA256 hash of the output image. Optionally repeats the
render and asserts that hashes match to validate determinism on a single
machine/GPU/driver.

Usage examples:
  python tests/determinism/hash_check.py \
    --exe builds/desktop/cmake/glint \
    --ops examples/json-ops/cube_basic.json \
    --seed 42 --width 256 --height 256 --output renders/hash_check.png \
    --repeat 2

On CI/Linux you may want to run under Xvfb. You can either wrap the call
(`xvfb-run -a python tests/determinism/hash_check.py ...`) or pass
`--xvfb` to let the script wrap the executable automatically on Linux.
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            h.update(chunk)
    return h.hexdigest()


def run_render(exe: Path, ops: Path, out_png: Path, width: int, height: int,
               seed: int, extra_args=None, xvfb: bool = False) -> None:
    cmd = [str(exe), "--ops", str(ops), "--render", str(out_png), "--w", str(width), "--h", str(height),
           "--seed", str(seed), "--log", "warn"]
    if extra_args:
        cmd += extra_args

    # Ensure output directory exists
    out_png.parent.mkdir(parents=True, exist_ok=True)

    # Wrap with xvfb-run if requested and on Linux
    if xvfb and sys.platform.startswith('linux'):
        cmd = ["xvfb-run", "-a"] + cmd

    print("Running:", " ".join(cmd))
    subprocess.check_call(cmd)

    # The engine may normalize output path into `renders/`.
    # If the requested output file doesn't exist but a name-matched file exists in `renders/`, use it.
    if not out_png.exists():
        alt = Path("renders") / out_png.name
        if alt.exists():
            # Mirror to the requested location for consistency
            out_png.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(alt, out_png)


def main():
    ap = argparse.ArgumentParser(description="Glint3D determinism hash check")
    ap.add_argument("--exe", required=True, help="Path to glint executable")
    ap.add_argument("--ops", required=True, help="Path to ops JSON file")
    ap.add_argument("--seed", type=int, default=42, help="Seed for deterministic render")
    ap.add_argument("--width", type=int, default=256, help="Render width")
    ap.add_argument("--height", type=int, default=256, help="Render height")
    ap.add_argument("--output", default="renders/hash_check.png", help="Output PNG path")
    ap.add_argument("--repeat", type=int, default=2, help="Number of renders for repeatability check")
    ap.add_argument("--xvfb", action="store_true", help="Wrap execution with xvfb-run (Linux only)")
    ap.add_argument("--print-hash-only", action="store_true", help="Only print the final hash and exit")

    args, unknown = ap.parse_known_args()

    exe = Path(args.exe)
    ops = Path(args.ops)
    out_png = Path(args.output)

    if not exe.exists():
        print(f"Executable not found: {exe}", file=sys.stderr)
        return 2
    if not ops.exists():
        print(f"Ops file not found: {ops}", file=sys.stderr)
        return 2

    hashes = []
    for i in range(max(1, args.repeat)):
        # Use unique temp name per iteration to avoid caching/path surprises
        iter_out = out_png if args.repeat == 1 else out_png.with_name(f"{out_png.stem}.{i}{out_png.suffix}")
        if iter_out.exists():
            iter_out.unlink()
        run_render(exe, ops, iter_out, args.width, args.height, args.seed, unknown, xvfb=args.xvfb)
        if not iter_out.exists():
            print(f"Render output not found: {iter_out}", file=sys.stderr)
            return 3
        h = sha256_file(iter_out)
        print(f"[{i}] {iter_out.name} sha256={h}")
        hashes.append(h)

    # Print final hash (first) for convenience
    print(hashes[0])

    if args.print_hash_only:
        return 0

    # Determinism gate: all hashes must match
    unique = set(hashes)
    if len(unique) != 1:
        print("Determinism check FAILED: mismatched hashes:", file=sys.stderr)
        for i, h in enumerate(hashes):
            print(f"  run {i}: {h}", file=sys.stderr)
        return 1

    print("Determinism check PASSED: all hashes identical")
    return 0


if __name__ == "__main__":
    sys.exit(main())

