#!/usr/bin/env bash
# Join the per-package compile_commands.json that colcon leaves under build/
# into one file at the root of the workspace, which is what clangd and any
# standalone clang-tidy run expect to find.
#
# Called by build_exapod.sh; safe to run on its own after any build.
set -eo pipefail

workspace="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="${workspace}/compile_commands.json"

mapfile -t fragments < <(find "${workspace}/build" -maxdepth 2 -name compile_commands.json 2>/dev/null | sort)

if [ "${#fragments[@]}" -eq 0 ]; then
  echo "no compile_commands.json under build/, nothing to merge" >&2
  echo "build the workspace first: ./build_exapod.sh" >&2
  exit 0
fi

python3 - "${output}" "${fragments[@]}" <<'PY'
import json
import sys

output, *fragments = sys.argv[1:]
entries = []
for fragment in fragments:
    with open(fragment, encoding="utf-8") as handle:
        entries.extend(json.load(handle))

with open(output, "w", encoding="utf-8") as handle:
    json.dump(entries, handle, indent=2)

print(f"merged {len(fragments)} files, {len(entries)} entries -> {output}")
PY
