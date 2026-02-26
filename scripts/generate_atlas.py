#!/usr/bin/env python3
"""
生成atlas.csv - 扫描textures目录下所有PNG文件，提取元数据
输出格式: alias,path,width,height,format
"""

import os
import sys
import re
import hashlib
import csvutil
import defines
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("错误: 需要安装Pillow库")
    print("运行: pip install Pillow")
    sys.exit(1)


def generate_alias(rel_path):
    """根据相对路径生成简短别名"""
    path_str = rel_path.as_posix()
    
    # 移除.png扩展名
    name_without_ext = path_str.replace('.png', '')
    
    # 处理不同的路径模式
    if path_str.startswith('spots/'):
        # spots/village.png -> spot_village
        basename = Path(name_without_ext).name
        # 替换空格和特殊字符为下划线
        basename = re.sub(r'[^a-zA-Z0-9_]', '_', basename)
        return f"spot_{basename}"
    elif '/' in path_str:
        # 0/MENU_MapTile_L0_00_00.png -> tile_0_L0_00_00
        parts = name_without_ext.split('/')
        folder = parts[0]
        filename = parts[1]
        
        # 简化文件名：MENU_MapTile_L0_00_00 -> L0_00_00
        if filename.startswith('MENU_MapTile_'):
            simplified = filename.replace('MENU_MapTile_', '')
            return f"tile_{folder}_{simplified}"
        else:
            return f"{folder}_{filename}"
    else:
        # bg.png -> bg
        return name_without_ext


def file_md5(path: Path) -> str:
    """计算文件 MD5"""
    h = hashlib.md5()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()


def scan_textures(textures_path):
    """扫描textures目录下所有PNG文件，0-5目录间同内容图片复用0目录路径"""
    textures = []

    # --- 扫描根目录下的散装PNG文件（如 bg.png）---
    for png_file in sorted(textures_path.glob("*.png")):
        try:
            with Image.open(png_file) as img:
                width, height = img.size
                mode = img.mode
            rel_path = png_file.relative_to(textures_path)
            alias = generate_alias(rel_path)
            path_str = "nightreign/assets/textures/" + rel_path.as_posix()
            textures.append({
                'alias': alias,
                'path': path_str,
                'width': width,
                'height': height,
                'format': mode,
            })
        except Exception as e:
            print(f"警告: 无法处理 {png_file}: {e}", file=sys.stderr)

    # --- 第一步：扫描 0/ 目录，建立 hash → entry 索引 ---
    base_dir = textures_path / "0"
    base_hash_map = {}   # md5 -> dict entry
    base_name_map = {}   # filename -> md5（按文件名快速查找）

    if base_dir.exists():
        for png_file in sorted(base_dir.rglob("*.png")):
            try:
                with Image.open(png_file) as img:
                    width, height = img.size
                    mode = img.mode
                rel_path = png_file.relative_to(textures_path)
                path_str = "nightreign/assets/textures/" + rel_path.as_posix()
                alias = generate_alias(rel_path)
                md5 = file_md5(png_file)
                entry = {
                    'alias': alias,
                    'path': path_str,
                    'width': width,
                    'height': height,
                    'format': mode,
                }
                textures.append(entry)
                base_hash_map[md5] = entry
                base_name_map[png_file.name] = md5
            except Exception as e:
                print(f"警告: 无法处理 {png_file}: {e}", file=sys.stderr)

    reused_count = 0

    # --- 第二步：扫描 1-5/ 及其余目录 ---
    for sub in sorted(textures_path.iterdir()):
        if sub.name == "0" or not sub.is_dir():
            continue
        is_numbered = sub.name in {"1", "2", "3", "4", "5"}

        for png_file in sorted(sub.rglob("*.png")):
            try:
                with Image.open(png_file) as img:
                    width, height = img.size
                    mode = img.mode
                rel_path = png_file.relative_to(textures_path)
                alias = generate_alias(rel_path)

                # 对 1-5 目录：先按文件名快速匹配，再校验 MD5
                reused_path = None
                if is_numbered and png_file.name in base_name_map:
                    candidate_md5 = base_name_map[png_file.name]
                    actual_md5 = file_md5(png_file)
                    if actual_md5 == candidate_md5:
                        reused_path = base_hash_map[candidate_md5]['path']
                        reused_count += 1

                path_str = reused_path if reused_path else (
                    "nightreign/assets/textures/" + rel_path.as_posix()
                )
                textures.append({
                    'alias': alias,
                    'path': path_str,
                    'width': width,
                    'height': height,
                    'format': mode,
                })
            except Exception as e:
                print(f"警告: 无法处理 {png_file}: {e}", file=sys.stderr)

    print(f"  图片复用: {reused_count} 张（1-5目录与0目录内容相同）")
    return textures


def write_atlas_csv(textures, output_file):
    """写入atlas.csv文件"""
    # 按别名排序
    textures.sort(key=lambda x: x['alias'])
    
    with open(output_file, 'w', encoding='utf-8') as f:
        # 写入标题行
        f.write("alias,path,width,height,format\n")
        
        # 写入每个纹理的数据
        for tex in textures:
            f.write(f"{tex['alias']},{tex['path']},{tex['width']},{tex['height']},{tex['format']}\n")
    
    print(f"✓ 生成 {output_file}")
    print(f"  共 {len(textures)} 个纹理文件")
    
    # 显示前几个别名示例
    print("\n别名示例:")
    for tex in textures[:5]:
        print(f"  {tex['alias']} -> {tex['path']}")


def main():
    paths = defines.PathDefinitions(__file__)
    
    print(f"扫描目录: {paths.textures_dir}")
    textures = scan_textures(paths.textures_dir)
    header = {
        'alias': 'std::string',
        'path': 'std::string',
        'width': 'int',
        'height': 'int',
        'format': 'std::string'
    }
    
    if not textures:
        print("警告: 未找到任何PNG文件")
        return 1
    
    csvutil.generate_csv(header, textures, paths.get_atlas_csv())
    csvutil.generate_cpp_header(header, paths.get_cpp_header("AtlasRow"), "AtlasRow")
    # 统计信息
    total_size = sum(t['width'] * t['height'] * 4 for t in textures)  # 假设RGBA
    print(f"  预估内存占用: {total_size / (1024*1024):.2f} MB")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
