#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   bash tools/texc.sh [-r engine/assets] [-m etc1s|uastc] [-j 4]
# Requires `toktx` in PATH

ROOT="engine/assets"
MODE="etc1s"
JOBS=4

while getopts ":r:m:j:" opt; do
  case $opt in
    r) ROOT="$OPTARG" ;;
    m) MODE="$OPTARG" ;;
    j) JOBS="$OPTARG" ;;
    *) echo "Unknown option"; exit 1 ;;
  esac
done

if ! command -v toktx >/dev/null 2>&1; then
  echo "toktx not found in PATH. Install KTX-Software."
  exit 1
fi

shopt -s globstar nullglob nocaseglob
mapfile -t FILES < <(printf '%s\n' "$ROOT"/**.{png,jpg,jpeg})

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No PNG/JPG images found under $ROOT"
  exit 0
fi

echo "Converting ${#FILES[@]} images to KTX2 ($MODE)"

for f in "${FILES[@]}"; do
  out="${f%.*}.ktx2"
  if [[ -f "$out" && "$out" -nt "$f" ]]; then
    echo "Skip up-to-date: $f"
    continue
  fi

  common=(--t2 --genmipmap --threads="$JOBS")
  if [[ "$MODE" == "etc1s" ]]; then
    args=("${common[@]}" --bcmp --assign_oetf srgb -o "$out" "$f")
  else
    args=("${common[@]}" --uastc 4 --uastc_rdo_q 2.5 --uastc_rdo --assign_oetf srgb -o "$out" "$f")
  fi
  echo "toktx ${args[*]}"
  if ! toktx "${args[@]}"; then
    echo "toktx failed for $f" >&2
  fi
done

echo "Done. Generated .ktx2 next to sources."

