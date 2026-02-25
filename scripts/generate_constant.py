#!/usr/bin/env python3

import os
import sys
import csvutil
import defines
from pathlib import Path

def load_constant(pointbase_path:str, flagbase_path:str, variationbase_path:str):
    print(f"Loading flagbase from: {flagbase_path}")
    flagbase = csvutil.load_csv(flagbase_path, key_column='ID')
    
    print(f"Loading createbase from: {pointbase_path}")
    pointbase = csvutil.load_csv(pointbase_path, key_column='ID')
    
    print(f"Loading variationbase from: {variationbase_path}")
    variationbase = csvutil.load_csv(variationbase_path, key_column='ID')

    constant_list = []
    filter_types = [2, 16, 28, 61, 71, 76]
    filter_maps = [1, 2, 4, 5]
    fieldboss_excludes = lambda map,rowdata: int(rowdata.get("eventFlagId6", 0)) == 0 or map == 4
    for point_id, point_data in pointbase.items():
        typeid = point_data.get("worldMapPointIconId", 0)
        map = int(point_data.get("pad", 0)) / 10
        if typeid not in filter_types or map not in filter_maps:
            continue
        if typeid == 16 and fieldboss_excludes(map, point_data):
            continue
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
            "label": defines.KnownIcons.get(typeid, "unknown"),
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
    
    great_hollow_bindings = []
    great_hollow_fieldboss = pointbase.filter(worldMapPointIconId=16, pad=40)
    for boss in great_hollow_fieldboss:
        flagid2 = int(boss.get("eventFlagId2", 0))
        if flagid2 == 0:
            continue
        variationid = 0
        variationindex = 0
        flagid0 = int(boss.get("eventFlagId0", 0))
        variationid = flagid0 // 10000
        variationindex = flagid0 % 10000
        if variationid == 0 or variationid not in variationbase:
            continue
        
        gridXNo = int(boss.get("gridXNo", 0))
        gridZNo = int(boss.get("gridZNo", 0))
        posX = float(boss.get("posX", 0))
        posZ = float(boss.get("posZ", 0))
        height = float(boss.get("posY", 0))
        great_hollow_bindings.append({
            "gridXNo": gridXNo,
            "gridZNo": gridZNo,
            "posX": posX,
            "posZ": posZ,
            "height": height,
            'icon': "boss",
            'iconScale': 1.0,
            'label': f'{variationid}-{variationindex}',
            'visible': 255,
            'binding': variationid,
        })
    return constant_list, rotted_power_dict, great_hollow_bindings
        
paths = defines.PathDefinitions(__file__)

pointbase_path = paths.get_metadata("WorldMapPointParam.csv")
flagbase_path = paths.get_metadata("LotResultMapPatternFlag.csv")
variationbase_path = paths.get_metadata("SmallBaseMapVariationParam.csv")
constant_list, rotted_power_dict, great_hollow_bindings = load_constant(
    str(pointbase_path),
    str(flagbase_path),
    str(variationbase_path))

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
csvutil.generate_csv(constant_header,
    constant_list,
    paths.get_output("manual_constant_template.csv"))
csvutil.generate_cpp_header(constant_header,
    paths.get_cpp_header("ConstantRow"),
    "ConstantRow")

rotted_power_header = {
    "patternId": "int",
    "gridXNo": "int",
    "gridZNo": "int",
    "posX": "float",
    "posZ": "float",
    "height": "float",
}
csvutil.generate_csv(rotted_power_header,
    rotted_power_dict,
    paths.get_output("autogen_rotted_power.csv"))
csvutil.generate_cpp_header(rotted_power_header,
    paths.get_cpp_header("RottedPowerRow"),
    "RottedPowerRow")

great_hollow_binding_header = {
    "gridXNo": "int",
    "gridZNo": "int",
    "posX": "float",
    "posZ": "float",
    "height": "float",
    "icon": "std::string",
    "iconScale": "float",
    "label": "std::string",
    "visible": "int",
    "binding": "int",
}
csvutil.generate_csv(great_hollow_binding_header,
    great_hollow_bindings,
    paths.get_output("manual_great_hollow_binding_template.csv"))
csvutil.generate_cpp_header(great_hollow_binding_header,
    paths.get_cpp_header("GreatHollowBindingRow"),
    "GreatHollowBindingRow")