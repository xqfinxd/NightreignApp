#!/usr/bin/env python3
"""
为assets子目录生成manifest文件
"""

import os
import csv
import zlib
from pathlib import Path
from typing import List, Dict, Optional
import argparse
import csvutil
import defines


def calculate_crc32(file_path: str, chunk_size: int = 8192) -> str:
    """
    计算文件的CRC32校验值
    """
    crc = 0
    try:
        with open(file_path, 'rb') as f:
            for chunk in iter(lambda: f.read(chunk_size), b''):
                crc = zlib.crc32(chunk, crc)
        return f"{crc & 0xFFFFFFFF:08x}"
    except Exception:
        return "ERROR"


def generate_subdir_manifest(
    assets_path: str,
    subdirs: List[str],
    default_priority: int = 1,
    extensions: Optional[Dict[str, List[str]]] = None
) -> List[Dict[str, str]]:
    """
    为assets下的子目录生成manifest文件
    
    Args:
        assets_path: assets根目录路径
        subdirs: 子目录列表，如 ['textures', 'shaders', 'models']
        default_priority: 默认优先级
        extensions: 每个子目录的文件扩展名过滤，如 {'textures': ['.png', '.jpg'], 'shaders': ['.vert', '.frag']}
    """
    assets_root = Path(assets_path).resolve()
    
    if not assets_root.exists():
        raise FileNotFoundError(f"assets路径不存在: {assets_path}")
    files_info = []
    for subdir in subdirs:
        subdir_path = assets_root / subdir
        
        if not subdir_path.exists():
            print(f"警告: 子目录不存在，跳过: {subdir_path}")
            continue
        
        if not subdir_path.is_dir():
            print(f"警告: 不是目录，跳过: {subdir_path}")
            continue
        
        # 收集该子目录下的所有文件
        
        
        # 获取该子目录的扩展名过滤列表
        subdir_extensions = extensions.get(subdir, []) if extensions else []
        
        for item_path in subdir_path.rglob('*'):
            if not item_path.is_file():
                continue
            
            # 检查文件扩展名
            if subdir_extensions:
                if item_path.suffix.lower() not in [ext.lower() for ext in subdir_extensions]:
                    continue
            
            # 生成相对路径: assets/subdir/filename
            # 相对于assets根目录的路径
            rel_path = str(item_path.relative_to(assets_root.parent)).replace('\\', '/')
            
            # 计算CRC
            crc = calculate_crc32(str(item_path))
            
            files_info.append({
                'path': rel_path,
                'priority': default_priority,
                'crc': crc
            })

    atlas_path = assets_root / 'atlas.csv'
    if atlas_path.exists():
        files_info.append({
            'path': str(atlas_path.relative_to(assets_root.parent)).replace('\\', '/'),
            'priority': default_priority + 1,
            'crc': calculate_crc32(str(atlas_path))
        })
    # 按文件路径排序
    files_info.sort(key=lambda x: x['path'])    
    return files_info

paths = defines.PathDefinitions(__file__)

csv_file = paths.get_manifest_csv()

manifest = generate_subdir_manifest(
    assets_path=str(paths.asset_root_dir),
    subdirs=['assets/shaders', 'assets/fonts', 'assets/datas'],
    default_priority=1,
)
header = {
    'path': 'string',
    'priority': 'int',
    'crc': 'string'
}
csvutil.generate_csv(header, manifest, str(csv_file))