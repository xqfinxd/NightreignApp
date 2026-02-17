#!/usr/bin/env python3
"""
生成atlas.csv - 扫描textures目录下所有PNG文件，提取元数据
输出格式: path,width,height,format
"""

import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("错误: 需要安装Pillow库")
    print("运行: pip install Pillow")
    sys.exit(1)


def scan_textures(base_path):
    """扫描textures目录下所有PNG文件"""
    textures = []
    
    # 获取当前脚本所在目录
    script_dir = Path(__file__).parent
    
    # 遍历所有PNG文件
    for png_file in script_dir.rglob("*.png"):
        try:
            # 打开图片获取尺寸
            with Image.open(png_file) as img:
                width, height = img.size
                mode = img.mode  # RGB, RGBA, L, etc.
            
            # 生成相对路径（相对于assets/textures/）
            rel_path = png_file.relative_to(script_dir)
            
            # 转换为POSIX路径（使用正斜杠）
            path_str = "assets/textures/" + rel_path.as_posix()
            
            textures.append({
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
    # 按路径排序
    textures.sort(key=lambda x: x['path'])
    
    with open(output_file, 'w', encoding='utf-8') as f:
        # 写入标题行
        f.write("path,width,height,format\n")
        
        # 写入每个纹理的数据
        for tex in textures:
            f.write(f"{tex['path']},{tex['width']},{tex['height']},{tex['format']}\n")
    
    print(f"✓ 生成 {output_file}")
    print(f"  共 {len(textures)} 个纹理文件")


def main():
    script_dir = Path(__file__).parent
    output_file = script_dir / "atlas.csv"
    
    print(f"扫描目录: {script_dir}")
    textures = scan_textures(script_dir)
    
    if not textures:
        print("警告: 未找到任何PNG文件")
        return 1
    
    write_atlas_csv(textures, output_file)
    
    # 统计信息
    total_size = sum(t['width'] * t['height'] * 4 for t in textures)  # 假设RGBA
    print(f"  预估内存占用: {total_size / (1024*1024):.2f} MB")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
