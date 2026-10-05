#!/usr/bin/env python3
"""
merge.py — merge source files from a directory into a single .txt file.

Usage:
    python merge.py [folder] [-o output.txt] [-e .c .h ...] [--no-header]

Examples:
    python merge.py
    python merge.py ./src -o all_code.txt
    python merge.py ./src -o dump.txt -e .c .h .cpp .hpp
    python merge.py ./src --no-header
"""

import argparse
import sys
from pathlib import Path

# ============================================================
#  DEFAULTS — change if you like
# ============================================================
DEFAULT_FOLDER   = "code/renderer/"
DEFAULT_OUTPUT   = "merged.txt"
DEFAULT_EXTS     = [".c", ".h", ".glsl"]
# ============================================================


def read_text_auto(path: Path) -> tuple[str, str] | None:
    """Try UTF-8, then cp1251, then latin-1. None if binary/unreadable."""
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
    parser = argparse.ArgumentParser(
        description="Merge source files into a single .txt file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("folder", nargs="?", default=DEFAULT_FOLDER,
                        help=f"folder to scan (default: {DEFAULT_FOLDER!r})")
    parser.add_argument("-o", "--output", default=DEFAULT_OUTPUT,
                        help=f"output file (default: {DEFAULT_OUTPUT!r})")
    parser.add_argument("-e", "--extensions", nargs="+", default=DEFAULT_EXTS,
                        help=f"file extensions (default: {' '.join(DEFAULT_EXTS)})")
    parser.add_argument("--no-header", action="store_true",
                        help="do not insert per-file headers")
    parser.add_argument("--no-recursive", action="store_true",
                        help="do not descend into subdirectories")

    args = parser.parse_args()

    root = Path(args.folder).resolve()
    if not root.exists() or not root.is_dir():
        print(f"Error: not a directory: {root}")
        sys.exit(1)

    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = Path.cwd() / output_path

    if args.no_recursive:
        exts = [e.lower() if e.startswith(".") else "." + e.lower()
                for e in args.extensions]
        files = [
            f for f in root.glob("*")
            if f.is_file() and f.suffix.lower() in exts
        ]
        files.sort(key=lambda p: str(p).lower())
    else:
        files = collect_files(root, args.extensions)

    # don't merge the output file into itself
    files = [f for f in files if f.resolve() != output_path.resolve()]

    print(f"Folder : {root}")
    print(f"Output : {output_path}")
    print(f"Exts   : {' '.join(args.extensions)}")
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

            if not args.no_header:
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
