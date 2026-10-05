#!/usr/bin/env python3
# CPL — Code Patch Language
"""
CODE_PATCH

REPLACE->
oldcode
WITH->
newcode
END
"""

import sys
import re
from pathlib import Path
from dataclasses import dataclass

# ============================================================
#  Settings
# ============================================================
PATCH_FILE = "patch.txt"
TARGET_DIR = "."
FILE_EXTENSIONS = [".js", ".c", ".h"]
RECURSIVE = True
DRY_RUN = False
# ============================================================


@dataclass
class Patch:
    old: str
    new: str
    line: int


def parse_patch(path: Path) -> list[Patch]:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    if not lines or lines[0].strip() != "CODE_PATCH":
        raise ValueError("File begins not from CODE_PATCH")

    patches: list[Patch] = []
    i = 1
    n = len(lines)

    while i < n:
        line = lines[i].strip()

        if line == "":
            i += 1
            continue

        if line != "REPLACE->":
            raise ValueError(f"String {i+1}: expected 'REPLACE->', found: {line!r}")

        start_line = i + 1
        i += 1
        old_lines: list[str] = []

        while i < n and lines[i].strip() != "WITH->":
            old_lines.append(lines[i])
            i += 1

        if i >= n:
            raise ValueError(f"String {start_line}: 'WITH->' not found")

        i += 1
        new_lines: list[str] = []

        while i < n and lines[i].strip() != "END":
            new_lines.append(lines[i])
            i += 1

        if i >= n:
            raise ValueError(f"String {start_line}: 'END' not found")

        i += 1

        old = "\n".join(old_lines)
        new = "\n".join(new_lines)

        if not old:
            raise ValueError(f"String {start_line}: empty oldcode")

        patches.append(Patch(old=old, new=new, line=start_line))

    return patches


def read_source(path: Path) -> tuple[str, str]:
    raw = path.read_bytes()
    for enc in ("utf-8", "latin-1"):
        try:
            return raw.decode(enc), enc
        except UnicodeDecodeError:
            continue
    raise UnicodeDecodeError("unknown", raw, 0, 1, "cannot decode")


def apply_patches_to_file(file_path: Path, patches: list[Patch]) -> tuple[int, int]:
    try:
        original, enc = read_source(file_path)
    except Exception as e:
        print(f"  ⏭️  SKIP {file_path}: {e}")
        return 0, 0

    result = original
    applied = 0
    conflicts = 0

    for p in patches:
        count = result.count(p.old)
        if count == 0:
            continue
        if count > 1:
            conflicts += 1
            print(f"  ⚠️  CONFLICT: pattern from {p.line} "
                  f"founded {count} times — replacing ALL {count} patterns")
        result = result.replace(p.old, p.new)
        applied += count

    if result != original and not DRY_RUN:
        file_path.write_text(result, encoding=enc)

    return applied, conflicts


def collect_files(root: Path, exts: list[str], recursive: bool) -> list[Path]:
    it = root.rglob("*") if recursive else root.glob("*")
    return [
        f for f in it
        if f.is_file() and f.suffix.lower() in exts
    ]


def main():
    script_dir = Path(__file__).resolve().parent
    patch_path = script_dir / PATCH_FILE

    if not patch_path.exists():
        print(f"❌ Patch-file not found: {patch_path}")
        sys.exit(1)

    print(f"📄 Patch: {patch_path}")
    patches = parse_patch(patch_path)
    print(f"   Found rules: {len(patches)}\n")

    target = script_dir / TARGET_DIR
    files = collect_files(target, FILE_EXTENSIONS, RECURSIVE)
    print(f"📂 Files to process: {len(files)}\n")

    total_applied = 0
    total_conflicts = 0
    changed_files = 0

    for f in files:
        if f.resolve() == patch_path.resolve():
            continue

        applied, conflicts = apply_patches_to_file(f, patches)

        if applied > 0:
            changed_files += 1
            total_applied += applied
            total_conflicts += conflicts
            rel = f.relative_to(script_dir) if script_dir in f.parents else f
            status = "🧪 DRY" if DRY_RUN else "✅"
            print(f"{status} {rel}: replaced {applied}, conflicts {conflicts}")

    print("\n" + "=" * 50)
    print(f"📊 RESULT:")
    print(f"   Edited files     : {changed_files}")
    print(f"   Total replaces   : {total_applied}")
    print(f"   Conflicts        : {total_conflicts}")
    print("=" * 50)

    if total_conflicts:
        sys.exit(2)


if __name__ == "__main__":
    main()
