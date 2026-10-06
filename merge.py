#!/usr/bin/env python3
"""
merge.py — merge source files from a directory into a single .txt file.
"""

import sys
from pathlib import Path

# ============================================================
#  Settings
# ============================================================
DEFAULT_FOLDER = "code/renderer/"
DEFAULT_OUTPUT = "merged.txt"
DEFAULT_EXTS   = [".c", ".h", ".glsl"]
# ============================================================


def read_text_auto(path: Path) -> tuple[str, str] | None:
    try:
        with open(path, "rb") as f:
            head = f.read(8192)
    except OSError:
        return None

    if b"\x00" in head:
        return None  # binary

    for enc in ("utf-8", "cp1251", "latin-1"):
        try:
            return path.read_text(encoding=enc), enc
        except UnicodeDecodeError:
            continue
    return None


def collect_files(root: Path, exts: list[str]) -> list[Path]:
    exts = [e.lower() if e.startswith(".") else "." + e.lower() for e in exts]
    files = [
        f for f in root.rglob("*")
        if f.is_file() and f.suffix.lower() in exts
    ]
    files.sort(key=lambda p: str(p).lower())
    return files


def build_header(file_path: Path, root: Path) -> str:
    try:
        rel = file_path.relative_to(root)
    except ValueError:
        rel = file_path
    bar = "=" * 78
    return f"\n{bar}\n// FILE: {rel}\n{bar}\n"


def main():
    root = Path(DEFAULT_FOLDER).resolve()
    if not root.exists() or not root.is_dir():
        print(f"Error: not a directory: {root}")
        sys.exit(1)

    output_path = Path(DEFAULT_OUTPUT)
    if not output_path.is_absolute():
        output_path = Path.cwd() / output_path

    files = collect_files(root, DEFAULT_EXTS)
    files = [f for f in files if f.resolve() != output_path.resolve()]

    print(f"Folder : {root}")
    print(f"Output : {output_path}")
    print(f"Exts   : {' '.join(DEFAULT_EXTS)}")
    print(f"Files  : {len(files)}\n")

    total_bytes = 0
    written = 0
    skipped = 0

    with output_path.open("w", encoding="utf-8", newline="\n") as out:
        for f in files:
            data = read_text_auto(f)
            if data is None:
                print(f"  skipped (binary/unreadable): {f}")
                skipped += 1
                continue

            text, _enc = data

            out.write(build_header(f, root))
            out.write(text)
            if not text.endswith("\n"):
                out.write("\n")

            written += 1
            total_bytes += len(text.encode("utf-8"))

    size_kb = total_bytes / 1024
    print("\n" + "=" * 50)
    print("SUMMARY:")
    print(f"  Files merged : {written}")
    print(f"  Files skipped: {skipped}")
    print(f"  Output size  : {size_kb:.1f} KB")
    print(f"  Output file  : {output_path}")
    print("=" * 50)


if __name__ == "__main__":
    main()