#!/usr/bin/env python3

import csvutil
import defines
from generate_pattern_list import load_pattern_list

CastleAttachIds = [190, 2190]
LTDivineTowerAttachIds = [1114, 1115, 1116]
RBDivineTowerAttachIds = [1111, 1112, 1113]
LTCityAttachIds = [1142]
RBCityAttachIds = [1136]

def remap_point(attachid: int, pointbase: dict) -> int:
    if attachid in CastleAttachIds:
        castle = pointbase.filter(worldMapPointIconId=11)[0].get("ID", attachid)
        return int(castle)
    if attachid in LTDivineTowerAttachIds:
        towers = pointbase.filter(worldMapPointIconId=73)
        sorted_towers = sorted(towers, key=lambda t: t.get("gridXNo", 0)) # x -> left or right
        return int(sorted_towers[0].get("ID", attachid)) # left tower
    if attachid in RBDivineTowerAttachIds:
        towers = pointbase.filter(worldMapPointIconId=73)
        sorted_towers = sorted(towers, key=lambda t: t.get("gridXNo", 0)) # x -> left or right
        return int(sorted_towers[-1].get("ID", attachid)) # right tower
    if attachid in LTCityAttachIds:
        city = pointbase.filter(worldMapPointIconId=75)[0].get("ID", attachid)
        return int(city)
    if attachid in RBCityAttachIds:
        city = pointbase.filter(worldMapPointIconId=74)[0].get("ID", attachid)
        return int(city)
    return attachid

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
        if point == None or len(point) == 0:
            # remap
            newattachid = remap_point(attachid, pointbase)
            point = csvutil.find_point(pointbase, newattachid)
        if point == None or len(point) == 0:
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


paths = defines.PathDefinitions(__file__)

pattern_mapbase = load_pattern_list(
    paths.get_metadata("LotResultMapPatternFlag.csv"),
    paths.get_metadata("PlayAreaCreateParam.csv"),
    paths.get_metadata("LotResultPlayAreaParam.csv"),
)
spot_list, variation_list = load_spot_list(
    pattern_mapbase,
    paths.get_metadata("LotResultSmallBaseAndSpot.csv"),
    paths.get_metadata("WorldMapPointParam.csv"),
)
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
csvutil.generate_csv(header, spot_list, paths.get_output("autogen_spot_list.csv"))
csvutil.generate_cpp_header(header, paths.get_cpp_header("SpotRow"), "SpotRow")

variation_header = {
    'patternId': 'int',
    'variationId': 'int',
    'variationType': 'int',
    'attachId': 'int',
}
csvutil.generate_csv(variation_header, variation_list, paths.get_output("autogen_variation_list.csv"))
csvutil.generate_cpp_header(variation_header, paths.get_cpp_header("VariationRow"), "VariationRow")