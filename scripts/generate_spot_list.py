#!/usr/bin/env python3

import csvutil
from pathlib import Path
from generate_pattern_list import load_pattern_list

def generate_hash(v1: int, v2: int, f1: float, f2: float, f3: float) -> int:
    return f"{v1}_{v2}_{f1:.2f}_{f2:.2f}_{f3:.2f}"

def unpack_hash(hashvalue: str):
    parts = hashvalue.split('_')
    if len(parts) != 5:
        raise ValueError(f"Invalid hash format: {hashvalue}")
    return {
        'gridXNo': int(parts[0]),
        'gridZNo': int(parts[1]),
        'posX': float(parts[2]),
        'posZ': float(parts[3]),
        'height': float(parts[4])
    }

def set2str(s: set) -> str:
    return '_'.join(str(x) for x in s)

def load_spot_list(pattern_mapbase: dict, attachbase_path:str, pointbase_path:str) -> list:
    """
    从pattern_mapbase中提取attachPoint列表
    """

    print(f"Loading attachbase from: {attachbase_path}")
    attachbase = csvutil.load_csv(attachbase_path, key_column='ID')
    
    print(f"Loading pointbase from: {pointbase_path}")
    pointbase = csvutil.load_csv(pointbase_path, key_column='ID')

    legacy_map_patterns = {}
    dlc_map_patterns = {}
    pattern_desc = {}
    legacy_point_locations = {}
    dlc_point_locations = {}
    for pattern in pattern_mapbase.values():
        mapindex = pattern['map']
        patternid = pattern['id']
        isdlc = pattern['isdlc']
        if isdlc > 0:
            dlc_map_patterns.setdefault(mapindex, []).append(patternid)
        else:
            legacy_map_patterns.setdefault(mapindex, []).append(patternid)
        pattern_desc.setdefault(patternid, {'map': mapindex, 'isdlc': isdlc})
    
    map_list = {}
    variation_list = []
    for attach in attachbase.values():
        patternid = attach['patternId']
        attachid = attach['attachId']
        variationid = attach['smallBaseMapId']
        variationtype = attach['variationId']
        id = attach['ID']
        mapindex = pattern_desc.get(patternid, {}).get('map', -1)
        isdlc = pattern_desc.get(patternid, {}).get('isdlc', 0)
        if mapindex < 0:
            continue
        point = csvutil.find_point(pointbase, attachid)
        if len(point) == 0:
            continue
        variation_list.append({
            'patternId': patternid,
            'attachId': attachid,
            'variationId': variationid,
            'variationType': variationtype,
        })
        dist = map_list.setdefault(mapindex, {
            'legacy': {},
            'dlc': {},
        })
        hashvalue = generate_hash(point['gridXNo'], point['gridZNo'], point['posX'], point['posZ'], point['height'])
        if isdlc > 0:
            dlc_point_locations.setdefault(hashvalue, set()).add(attachid)
            dist['dlc'].setdefault(hashvalue, []).append(patternid)
        else:
            legacy_point_locations.setdefault(hashvalue, set()).add(attachid)
            dist['legacy'].setdefault(hashvalue, []).append(patternid)
    
    spot_list = []
    for mapindex, dist in map_list.items():
        for hashvalue, patternids in dist['legacy'].items():
            fullnum = len(legacy_map_patterns.get(mapindex, []))
            point = unpack_hash(hashvalue)
            attachids = legacy_point_locations.get(hashvalue, set())
            for attachid in attachids:
                spot_list.append({
                    'attachId': attachid,
                    'map': mapindex,
                    'dlc': 0,
                    'rate': len(patternids) / fullnum,
                    'gridXNo': point['gridXNo'],
                    'gridZNo': point['gridZNo'],
                    'posX': point['posX'],
                    'posZ': point['posZ'],
                    'height': point['height'],
                })
        for hashvalue, patternids in dist['dlc'].items():
            fullnum = len(dlc_map_patterns.get(mapindex, []))
            point = unpack_hash(hashvalue)
            attachids = dlc_point_locations.get(hashvalue, set())
            for attachid in attachids:
                spot_list.append({
                    'attachId': attachid,
                    'map': mapindex,
                    'dlc': 1,
                    'rate': len(patternids) / fullnum,
                    'gridXNo': point['gridXNo'],
                    'gridZNo': point['gridZNo'],
                    'posX': point['posX'],
                    'posZ': point['posZ'],
                    'height': point['height'],
                })
    return spot_list, variation_list


base_dir = Path(__file__).resolve().parent.parent 
assets_dir = base_dir / "nightreign" / "assets"

meta_dir = assets_dir / "metadata"
flagbase_path = meta_dir / "LotResultMapPatternFlag.csv"
createbase_path = meta_dir / "PlayAreaCreateParam.csv"
areabase_path = meta_dir / "LotResultPlayAreaParam.csv"
attachbase_path = meta_dir / "LotResultSmallBaseAndSpot.csv"
pointbase_path = meta_dir / "WorldMapPointParam.csv"

pattern_mapbase = load_pattern_list(
    str(flagbase_path),
    str(createbase_path),
    str(areabase_path)
)
spot_list, variation_list = load_spot_list(
    pattern_mapbase,
    str(attachbase_path),
    str(pointbase_path),
)

spot_csv_file = assets_dir / "datas" / "autogen_spot_list.csv"
spot_cpp_header_file = base_dir / "src" / "generated" / "SpotRow.h"
header = {
    'attachId': 'int',
    'map': 'int',
    'dlc': 'int',
    'rate': 'float',
    'gridXNo': 'int',
    'gridZNo': 'int',
    'posX': 'float',
    'posZ': 'float',
    'height': 'float',
}
csvutil.generate_csv(header, spot_list, str(spot_csv_file))
csvutil.generate_cpp_header(header, str(spot_cpp_header_file), "SpotRow")

variation_csv_file = assets_dir / "datas" / "autogen_variation_list.csv"
variation_cpp_header_file = base_dir / "src" / "generated" / "VariationRow.h"
variation_header = {
    'patternId': 'int',
    'variationId': 'int',
    'variationType': 'int',
    'attachId': 'int',
}
csvutil.generate_csv(variation_header, variation_list, str(variation_csv_file))
csvutil.generate_cpp_header(variation_header, str(variation_cpp_header_file), "VariationRow")