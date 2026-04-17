#!/usr/bin/env python3
"""Render captured terminal transcripts into PNG images."""

from pathlib import Path
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as exc:
    raise SystemExit("Pillow is required: python3 -m pip install pillow") from exc


ROOT = Path(__file__).resolve().parents[1]
SCREENSHOTS = ROOT / "screenshots"

MAPPING = {
    "1A-test_objects.txt": "1A_test_objects.png",
    "1B-object-store-find.txt": "1B_objects_find.png",
    "2A-test_tree.txt": "2A_test_tree.png",
    "2B-tree-object-xxd.txt": "2B_tree_xxd.png",
    "3A-init-add-status.txt": "3A_init_add_status.png",
    "3B-index-cat.txt": "3B_index_cat.png",
    "4A-log.txt": "4A_pes_log.png",
    "4B-pes-find.txt": "4B_find_pes_files.png",
    "4C-refs.txt": "4C_refs_and_head.png",
    "final-integration.txt": "final_integration_test.png",
}


def load_font(size: int) -> ImageFont.ImageFont:
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
        "C:/Windows/Fonts/consola.ttf",
    ]
    for candidate in candidates:
        path = Path(candidate)
        if path.exists():
            return ImageFont.truetype(str(path), size=size)
    return ImageFont.load_default()


def render_text(text: str, output: Path) -> None:
    font = load_font(18)
    lines = text.splitlines() or [""]
    max_cols = max(len(line) for line in lines)
    line_height = 26
    char_width = 11
    pad_x = 28
    pad_y = 24
    width = max(900, min(1800, pad_x * 2 + max_cols * char_width))
    height = max(260, pad_y * 2 + len(lines) * line_height)

    image = Image.new("RGB", (width, height), "#101418")
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width, 38), fill="#22272e")
    draw.ellipse((18, 13, 30, 25), fill="#ff5f57")
    draw.ellipse((38, 13, 50, 25), fill="#ffbd2e")
    draw.ellipse((58, 13, 70, 25), fill="#28c840")

    y = pad_y + 30
    for line in lines:
        draw.text((pad_x, y), line, fill="#e6edf3", font=font)
        y += line_height

    image.save(output)


def main() -> int:
    missing = []
    for source_name, image_name in MAPPING.items():
        source = SCREENSHOTS / source_name
        if not source.exists():
            missing.append(source_name)
            continue
        render_text(source.read_text(encoding="utf-8", errors="replace"), SCREENSHOTS / image_name)

    if missing:
        print("Missing transcript files:", ", ".join(missing), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
