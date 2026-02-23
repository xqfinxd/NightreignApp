#!/usr/bin/env python3

import os
import sys
import csvutil
from pathlib import Path

KnownIcons = {
    1 : "site of grace",
    2 :"church",
    3 :"ruins",
    7 :"fort",
    11: "castle",
    13: "spectral hawk tree",
    16: "field boss",
    17: "scarab",
    21: "merchant",
    23: "mine",
    25: "scale-bearing merchant",
    28: "formidable field boss",
    30: "church-completed",
    60: "personal objective",
    61: "shifting earth power",
    62: "buried treasure",
    137: "spiritstream",
    178: "rope door",
    # dlc
    51: "city rooftop",
    71: "great crystal",
    73: "divine tower",
    74: "rb city",
    75: "lt city",
    76: "portal",
    79: "unknown", # undefined
}

def load_constant(pointbase_path:str, flagbase_path:str):
    print(f"Loading flagbase from: {flagbase_path}")
    flagbase = csvutil.load_csv(flagbase_path, key_column='ID')
    
    print(f"Loading createbase from: {pointbase_path}")
    pointbase = csvutil.load_csv(pointbase_path, key_column='ID')
    
    constant_list = []
    filter_types = [2, 16, 28, 30, 51, 71, 73, 74, 75, 76, 79]
    for point_id, point_data in pointbase.items():
        typeid = point_data.get("worldMapPointIconId", 0)
        if typeid in filter_types:
            map = int(point_data.get("pad", 0)) / 10
            gridXNo = int(point_data.get("gridXNo", 0))
            gridZNo = int(point_data.get("gridZNo", 0))
            posX = float(point_data.get("posX", 0))
            posZ = float(point_data.get("posZ", 0))
            height = float(point_data.get("posY", 0))
            constant_list.append({
                "type": typeid,
                "map": map,
                "gridXNo": gridXNo,
                "gridZNo": gridZNo,
                "posX": posX,
                "posZ": posZ,
                "height": height,
                "label": KnownIcons.get(typeid, "unknown"),
                'icon': "undefined",
                'iconScale': 1.0,
            })
    rotted_power_dict = []
    power_flags = pointbase.filter(worldMapPointIconId=61)
    for flag in power_flags:
        map = int(flag.get("pad", 0)) / 10
        gridXNo = int(flag.get("gridXNo", 0))
        gridZNo = int(flag.get("gridZNo", 0))
        posX = float(flag.get("posX", 0))
        posZ = float(flag.get("posZ", 0))
        height = float(flag.get("posY", 0))
        flag_id = int(flag.get("eventFlagId1", 0))
        if map == 3: # rotted forest only
            pattern_flags = flagbase.filter(modifierSet=500, eventFlag=flag_id)
            for pattern in pattern_flags:
                pattern_id = int(pattern.get("patternId", -1))
                if pattern_id >= 0:
                    rotted_power_dict.append({
                        "patternId": pattern_id,
                        "gridXNo": gridXNo,
                        "gridZNo": gridZNo,
                        "posX": posX,
                        "posZ": posZ,
                        "height": height,
                    })
        else:
            typeid = flag.get("worldMapPointIconId", 0)
            constant_list.append({
                "type": typeid,
                "map": map,
                "gridXNo": gridXNo,
                "gridZNo": gridZNo,
                "posX": posX,
                "posZ": posZ,
                "height": height,
                "label": KnownIcons.get(typeid, "unknown"),
                'icon': "undefined",
                'iconScale': 1.0,
            })
    
    return constant_list, rotted_power_dict
        
    
base_dir = Path(__file__).resolve().parent.parent 
assets_dir = base_dir / "nightreign" / "assets"

pointbase_path = assets_dir / "metadata" / "WorldMapPointParam.csv"
flagbase_path = assets_dir / "metadata" / "LotResultMapPatternFlag.csv"
constant_list, rotted_power_dict = load_constant(str(pointbase_path), str(flagbase_path))

constant_header = {
    "type": "int",
    "map": "int",
    "gridXNo": "int",
    "gridZNo": "int",
    "posX": "float",
    "posZ": "float",
    "height": "float",
    "label": "std::string",
    'icon': 'std::string',
    'iconScale': 'float',
}
constant_csv_file = assets_dir / "datas" / "manual_constant.csv"
constant_cpp_header_file = base_dir / "src" / "generated" / "ConstantRow.h"
csvutil.generate_csv(constant_header, constant_list, constant_csv_file)
csvutil.generate_cpp_header(constant_header, constant_cpp_header_file, "ConstantRow")

rotted_power_header = {
    "patternId": "int",
    "gridXNo": "int",
    "gridZNo": "int",
    "posX": "float",
    "posZ": "float",
    "height": "float",
}
rotted_power_csv_file = assets_dir / "datas" / "autogen_rotted_power.csv"
rotted_power_cpp_header_file = base_dir / "src" / "generated" / "RottedPowerRow.h"
csvutil.generate_csv(rotted_power_header, rotted_power_dict, rotted_power_csv_file)
csvutil.generate_cpp_header(rotted_power_header, rotted_power_cpp_header_file, "RottedPowerRow")