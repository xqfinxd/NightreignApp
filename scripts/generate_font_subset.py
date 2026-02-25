#!/usr/bin/env python3
"""
生成字体子集 - 只保留 chs.txt 中的中文字符 + ASCII + 常用标点
依赖: pip install fonttools brotli
"""

import sys
import defines
from pathlib import Path
from fontTools import subset as ftsubset


# 额外保留的 Unicode 范围（除 chs.txt 之外）
EXTRA_RANGES = [
    (0x0020, 0x007E),   # ASCII 可打印字符
    (0x00A0, 0x00FF),   # Latin-1 补充（©®°等）
    (0x2000, 0x206F),   # 常规标点
    (0x2190, 0x21FF),   # 箭头
    (0x3000, 0x303F),   # CJK 符号和标点
    (0xFF00, 0xFFEF),   # 全角字符
]


def build_unicodes(chs_txt_path: Path) -> set:
    """构建需要保留的完整 Unicode 码点集合"""
    unicodes = set()

    # 添加额外范围
    for start, end in EXTRA_RANGES:
        for cp in range(start, end + 1):
            unicodes.add(cp)

    # 添加 chs.txt 中的中文字符
    if chs_txt_path.exists():
        text = chs_txt_path.read_text(encoding='utf-8')
        for ch in text:
            unicodes.add(ord(ch))
        print(f"  Loaded {len(text)} Chinese characters from {chs_txt_path.name}")
    else:
        print(f"  Warning: {chs_txt_path} not found, only ASCII/punctuation will be included")

    return unicodes


def subset_font(input_path: Path, output_path: Path, unicodes: set) -> bool:
    """对单个字体文件进行子集化，覆盖原文件"""
    print(f"\nProcessing: {input_path.name} ({input_path.stat().st_size // 1024} KB)")

    # 构建 pyftsubset 选项
    options = ftsubset.Options()
    options.layout_features = ['*']     # 保留所有 OpenType 特性
    options.name_IDs = ['*']            # 保留所有名称记录
    options.hinting = True              # 保留 hinting（渲染质量）
    options.desubroutinize = False
    options.ignore_missing_unicodes = True

    subsetter = ftsubset.Subsetter(options=options)

    from fontTools.ttLib import TTFont
    font = TTFont(str(input_path))

    subsetter.populate(unicodes=unicodes)
    subsetter.subset(font)

    # 覆盖原文件
    font.save(str(output_path))
    font.close()

    new_size = output_path.stat().st_size
    print(f"  → Saved: {new_size // 1024} KB")
    return True


def main():
    paths = defines.PathDefinitions(__file__)

    chs_txt = Path(paths.get_output("chs.txt"))
    fonts_dir = paths.fonts_dir
    metadata_dir = paths.metadata_dir

    if not metadata_dir.exists():
        print(f"Error: metadata directory not found: {metadata_dir}")
        return 1

    unicodes = build_unicodes(chs_txt)
    print(f"Total Unicode codepoints to keep: {len(unicodes)}")

    # 处理所有 ttf/otf 字体
    font_files = list(metadata_dir.glob("*.ttf")) + list(metadata_dir.glob("*.otf"))
    if not font_files:
        print("No font files found.")
        return 1

    for font_path in font_files:
        try:
            subset_font(font_path, fonts_dir / font_path.name, unicodes)
        except Exception as e:
            print(f"  Error processing {font_path.name}: {e}")
            import traceback
            traceback.print_exc()

    print("\nFont subsetting complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
