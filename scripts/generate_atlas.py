#!/usr/bin/env python3
"""
生成atlas.csv - 扫描textures目录下所有PNG文件，提取元数据
输出格式: alias,path,width,height,format
"""

import os
import sys
import re
import csvutil
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


def scan_textures(textures_path):
    """扫描textures目录下所有PNG文件"""
    textures = []
        
    # 遍历所有PNG文件
    for png_file in textures_path.rglob("*.png"):
        try:
            # 打开图片获取尺寸
            with Image.open(png_file) as img:
                width, height = img.size
                mode = img.mode  # RGB, RGBA, L, etc.
            
            # 生成相对路径（相对于assets/textures/）
            rel_path = png_file.relative_to(textures_path)
            
            # 转换为POSIX路径（使用正斜杠）
            path_str = "nightreign/assets/textures/" + rel_path.as_posix()
            
            # 生成别名
            alias = generate_alias(rel_path)
            
            textures.append({
                'alias': alias,
                'path': path_str,
                'width': width,
                'height': height,
                'format': mode
            })
            
        except Exception as e:
            print(f"警告: 无法处理 {png_file}: {e}", file=sys.stderr)
    
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
    base_dir = Path(__file__).resolve().parent.parent
    textures_dir = base_dir / "nightreign" / "assets" / "textures"
    
    csv_file = base_dir / "nightreign" / "atlas.csv"
    cpp_header_file = base_dir / "src" / "generated" / "AtlasRow.h"
    
    print(f"扫描目录: {textures_dir}")
    textures = scan_textures(textures_dir)
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
    
    csvutil.generate_csv(header, textures, csv_file)
    csvutil.generate_cpp_header(header, cpp_header_file, "AtlasRow")
    # 统计信息
    total_size = sum(t['width'] * t['height'] * 4 for t in textures)  # 假设RGBA
    print(f"  预估内存占用: {total_size / (1024*1024):.2f} MB")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
