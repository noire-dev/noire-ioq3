#!/usr/bin/env python3
# CPL — Code Patch Language
# The order of `REPLACE->` blocks in `patch.txt` matters,
# because each rule is applied strictly in sequence 
# and can affect whether later rules match.
"""
CODE_PATCH

REPLACE->
oldcode
WITH->
newcode
END

REPLACE->
onemoreoldcode
WITH->
onemorenewcode
END

END_CODE_PATCH
"""

import sys
from pathlib import Path
from dataclasses import dataclass

# ============================================================
#  Settings
# ============================================================
PATCH_FILE = "patch.txt"
TARGET_DIR = "."
FILE_EXTENSIONS = [".js", ".c", ".h", ".glsl"]
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
        raise ValueError("File does not begin with CODE_PATCH")

    patches: list[Patch] = []
    i = 1
    n = len(lines)
    found_end = False

    while i < n:
        line = lines[i].strip()

        if line == "":
            i += 1
            continue

        if line == "END_CODE_PATCH":
            found_end = True
            i += 1
            break

        if line != "REPLACE->":
            raise ValueError(f"Line {i+1}: expected 'REPLACE->', found: {line!r}")

        start_line = i + 1
        i += 1
        old_lines: list[str] = []

        while i < n and lines[i].strip() != "WITH->":
            if lines[i].strip() == "END_CODE_PATCH":
                raise ValueError(f"Line {start_line}: 'WITH->' not found before END_CODE_PATCH")
            old_lines.append(lines[i])
            i += 1

        if i >= n:
            raise ValueError(f"Line {start_line}: 'WITH->' not found")

        i += 1
        new_lines: list[str] = []

        while i < n and lines[i].strip() != "END":
            if lines[i].strip() == "END_CODE_PATCH":
                raise ValueError(f"Line {start_line}: 'END' not found before END_CODE_PATCH")
            new_lines.append(lines[i])
            i += 1

        if i >= n:
            raise ValueError(f"Line {start_line}: 'END' not found")

        i += 1

        old = "\n".join(old_lines)
        new = "\n".join(new_lines)

        if not old:
            raise ValueError(f"Line {start_line}: empty oldcode")

        patches.append(Patch(old=old, new=new, line=start_line))

    if not found_end:
        raise ValueError("END_CODE_PATCH not found")

    return patches

def read_source(path: Path) -> tuple[str, str]:
    raw = path.read_bytes()
    for enc in ("utf-8", "latin-1"):
        try:
            return raw.decode(enc), enc
        except UnicodeDecodeError:
            continue
    raise UnicodeDecodeError("unknown", raw, 0, 1, "cannot decode")

def collect_files(root: Path, exts: list[str], recursive: bool) -> list[Path]:
    it = root.rglob("*") if recursive else root.glob("*")
    return [
        f for f in it
        if f.is_file() and f.suffix.lower() in exts
    ]

def find_lines(text: str, sub: str) -> list[int]:
    lines = []
    start = 0
    while True:
        idx = text.find(sub, start)
        if idx == -1:
            break
        line_no = text.count('\n', 0, idx) + 1
        lines.append(line_no)
        start = idx + len(sub)
    return lines

def main():
    script_dir = Path(__file__).resolve().parent
    patch_path = script_dir / PATCH_FILE

    if not patch_path.exists():
        print(f"Patch file not found: {patch_path}")
        sys.exit(1)

    print(f"Patch: {patch_path}")
    patches = parse_patch(patch_path)
    print(f"Rules found: {len(patches)}\n")

    target = script_dir / TARGET_DIR
    files = collect_files(target, FILE_EXTENSIONS, RECURSIVE)
    print(f"Files to process: {len(files)}\n")

    # Per-rule status across all files
    rule_replaced = [False] * len(patches)
    rule_conflict = [False] * len(patches)
    rule_seen = [False] * len(patches)

    for f in files:
        if f.resolve() == patch_path.resolve():
            continue

        try:
            original, enc = read_source(f)
        except Exception as e:
            print(f"SKIP {f}: {e}")
            continue

        rel = f.relative_to(script_dir) if script_dir in f.parents else f
        result = original
        file_changed = False

        for idx, p in enumerate(patches):
            count = result.count(p.old)
            if count == 0:
                continue
            rule_seen[idx] = True
            if count > 1:
                rule_conflict[idx] = True
                line_nos = find_lines(result, p.old)
                print(f"CONFLICT {rel}: patch line {p.line}, {count} matches at source lines: {', '.join(map(str, line_nos))}")
                continue
            result = result.replace(p.old, p.new)
            rule_replaced[idx] = True
            file_changed = True

        if file_changed and not DRY_RUN:
            f.write_text(result, encoding=enc)

        if file_changed:
            status = "DRY" if DRY_RUN else "OK"
            print(f"{status} {rel}")

    total_replaced = sum(rule_replaced)
    total_conflict = sum(rule_conflict)
    total_not_found = sum(1 for i in range(len(patches))
                          if not rule_seen[i] and not rule_conflict[i])
    total = total_replaced + total_not_found + total_conflict

    print("\n" + "=" * 50)
    print("RESULT:")
    print(f"   Replaced  : {total_replaced}")
    print(f"   Not found : {total_not_found}")
    print(f"   Conflict  : {total_conflict}")
    print(f"   Total     : {total}")
    print("=" * 50)

    if total_not_found:
        print("\nNOT FOUND RULES:")
        for idx, p in enumerate(patches):
            if not rule_seen[idx] and not rule_conflict[idx]:
                first_line = p.old.splitlines()[0] if p.old.splitlines() else ""
                print(f"   patch.txt line {p.line}: {first_line[:70]}{'...' if len(first_line) > 70 else ''}")

    if total_conflict:
        sys.exit(2)

if __name__ == "__main__":
    main()