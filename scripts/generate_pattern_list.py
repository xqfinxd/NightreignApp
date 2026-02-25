#!/usr/bin/env python3
"""
解析CSV文件并生成pattern列表的Python程序
对应Lua程序：gen_pattern_list.lua
"""

import os
import sys
import csvutil
import defines
from pathlib import Path


def load_pattern_list(flagbase_path: str, createbase_path: str, areabase_path: str) -> dict:
    """
    加载CSV文件并生成pattern列表
    
    Args:
        flagbase_path: LotResultMapPatternFlag.csv 路径
        createbase_path: PlayAreaCreateParam.csv 路径
        areabase_path: LotResultPlayAreaParam.csv 路径
    
    Returns:
        pattern_mapbase: 包含所有pattern信息的字典
    """
    
    # 1. 加载基础CSV文件
    print(f"Loading flagbase from: {flagbase_path}")
    flagbase = csvutil.load_csv(flagbase_path, key_column='ID')
    
    print(f"Loading createbase from: {createbase_path}")
    createbase = csvutil.load_csv(createbase_path, key_column='ID')
    
    print(f"Loading areabase from: {areabase_path}")
    areabase = csvutil.load_csv(areabase_path, key_column='ID')
    
    # 2. 处理flagbase构建pattern_mapbase
    pattern_mapbase = {}
    
    for row in flagbase.values():
        pattern_id = int(row['patternId'])
        pattern = pattern_mapbase.get(pattern_id)
        
        if pattern is None:
            pattern = {
                'id': pattern_id,
                'map': int(row['rareMap']),
                'boss': int(row['targetBoss']),
                'isdlc': int(row['patternSetId']) >= 1000,
                'starter': 0  # 默认值
            }
            pattern_mapbase[pattern_id] = pattern
        
        modifier_set = int(row['modifierSet'])
        if modifier_set in (190, 160):
            pattern['starter'] = int(row['modifier'])
    
    # 4. 处理areabase，补充pattern信息
    for row in areabase.values():
        pattern_id = int(row['patternId'])
        pattern = pattern_mapbase.get(pattern_id)
        
        if pattern is not None:
            # 确保pattern有id
            pattern['id'] = pattern_id
            
            # 处理playArea1
            play_area1 = csvutil.find_point(createbase, _safe_int(row.get('playArea1')))
            pattern['playArea1_gridXNo'] = play_area1.get('gridXNo', 0)
            pattern['playArea1_gridZNo'] = play_area1.get('gridZNo', 0)
            pattern['playArea1_posX'] = play_area1.get('posX', 0)
            pattern['playArea1_posZ'] = play_area1.get('posZ', 0)
            pattern['playArea1_height'] = play_area1.get('height', 0)
            
            # 处理playArea2
            play_area2 = csvutil.find_point(createbase, _safe_int(row.get('playArea2')))
            pattern['playArea2_gridXNo'] = play_area2.get('gridXNo', 0)
            pattern['playArea2_gridZNo'] = play_area2.get('gridZNo', 0)
            pattern['playArea2_posX'] = play_area2.get('posX', 0)
            pattern['playArea2_posZ'] = play_area2.get('posZ', 0)
            pattern['playArea2_height'] = play_area2.get('height', 0)
            # 处理boss IDs
            pattern['bossId1'] = _safe_int(row.get('bossId1'), 0)
            pattern['bossId2'] = _safe_int(row.get('bossId2'), 0)
            pattern['extraBossId1'] = _safe_int(row.get('extraBossId1'), 0)
            pattern['extraBossId2'] = _safe_int(row.get('extraBossId2'), 0)
    
    return pattern_mapbase

def _safe_int(value, default=0):
    """安全地将值转换为整数"""
    if value is None or value == '':
        return default
    try:
        return int(value)
    except (ValueError, TypeError):
        return default

def main():
    """主函数"""
    paths = defines.PathDefinitions(__file__)

    flagbase_path = Path(paths.get_metadata("LotResultMapPatternFlag.csv"))
    createbase_path = Path(paths.get_metadata("PlayAreaCreateParam.csv"))
    areabase_path = Path(paths.get_metadata("LotResultPlayAreaParam.csv"))

    csv_file = paths.get_output("autogen_pattern_list.csv")
    cpp_header_file = paths.get_cpp_header("PatternRow")

    header = {
        'id': 'int',
        'map': 'int',
        'boss': 'int',
        'starter': 'int',
        'isdlc': 'int',
        'playArea1_gridXNo': 'int',
        'playArea1_gridZNo': 'int',
        'playArea1_posX': 'float',
        'playArea1_posZ': 'float',
        'playArea1_height': 'float',
        'playArea2_gridXNo': 'int',
        'playArea2_gridZNo': 'int',
        'playArea2_posX': 'float',
        'playArea2_posZ': 'float',
        'playArea2_height': 'float',
        'bossId1': 'int',
        'bossId2': 'int',
        'extraBossId1': 'int',
        'extraBossId2': 'int'
    }
    
    # 检查文件是否存在
    for path in [flagbase_path, createbase_path, areabase_path]:
        if not path.exists():
            print(f"Error: File not found: {path}")
            return 1
    
    try:
        # 加载并处理数据
        pattern_mapbase = load_pattern_list(
            str(flagbase_path),
            str(createbase_path),
            str(areabase_path)
        )
        
        # 保存结果
        csvutil.generate_csv(header, list(pattern_mapbase.values()), csv_file)
        csvutil.generate_cpp_header(header, cpp_header_file, "PatternRow")
        
        print("\nProcessing completed successfully!")
        return 0
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


# 使用示例和测试
if __name__ == "__main__":
    sys.exit(main())