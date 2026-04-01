import os
import sys
from pathlib import Path

KnownIcons = {
    1 : "site of grace",
    2 : "church",
    3 : "ruins",
    7 : "fort",
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

# in WorldMapPointParam.csv but not in SmallBaseAndSpotAttachPoint.csv
SpecialAttachIds = [
    190, 2190, # castle
    1111, 1112, 1113,1114, 1115, 1116, # divine tower
    1144, # 左上城池BOSS点: 黄金河马或双神皮
    1137, # 右下城池BOSS点: 腐败黄金树化身或神兽战士(红)
    1136, # 右下城池
    1142, # 左上城池
    1138, 1139, 1140, 1141, 1143, 1145, 1146, 1147, 1148,
    1200, 1201, 1202, # play area
]

# Event Flags: 7705, 7725
InvasionEvents = {
    510 : "Caligo Blizzard",
    520 : "Gladius Invasion",
    540 : "Maris Bubbles",
    550 : "Gnoster Plague",
    560 : "Libra Curse",

    3040 : "Morgott Invasion",
    3050 : "Maris Bubbles",
    3060 : "Gnoster Plague",
    3070 : "Libra Curse",
    3110 : "Caligo Blizzard",
    3120 : "Gladius Invasion",
    3130 : "Balancers Raid",
}

EventDefinitions = {
    7704 : "Day 1 Night Horde",
    7724 : "Day 2 Night Horde",

    7701 : "Day 1 Meteor Strike",
    7721 : "Day 2 Meteor Strike",

    7702 : "Walking Mausoleum",
    7722 : "Walking Mausoleum",

    7700 : "Day 1 Extra Night Boss",
    7720 : "Day 2 Extra Night Boss",

    7707 : "Frenzy Tower",
    7727 : "Frenzy Tower",

    7706 : "Difficult Sorcerer's Rise",
    7726 : "Difficult Sorcerer's Rise",

    # 8075 : "Morgott Invasion",
    # 8076 : "Maris Bubbles",
    # 8077 : "Gladius Invasion",
    # 8078 : "Gnoster Plague",
    # 8079 : "Libra Curse",
    # 8080 : "Caligo Blizzard",
    # 8081 : "Balancers Raid",
    7705 : lambda modifier, modifierSet: "Day 1 " + InvasionEvents[modifierSet],
    7725 : lambda modifier, modifierSet: "Day 2 " + InvasionEvents[modifierSet],
}

InvasionEventsCHS = {
    510 : "卡莉果冰风暴",
    520 : "格拉迪乌斯入侵",
    540 : "玛利斯气泡",
    550 : "格诺斯塔瘟疫",
    560 : "天秤恶魔诅咒",

    3040 : "蒙格特入侵",
    3050 : "玛利斯气泡",
    3060 : "格诺斯塔瘟疫",
    3070 : "天秤恶魔诅咒",
    3110 : "卡莉果冰风暴",
    3120 : "格拉迪乌斯入侵",
    3130 : "平衡者突袭",
}

EventDefinitionsCHS = {
    7704 : "第一天黑夜势力",
    7724 : "第二天黑夜势力",
    7701 : "第一天陨石坠地",
    7721 : "第二天陨石坠地",
    7702 : "漫步陵庙",
    7722 : "漫步陵庙",
    7700 : "第一天额外夜晚Boss",
    7720 : "第二天额外夜晚Boss",
    7707 : "癫火塔",
    7727 : "癫火塔",
    7706 : "远古魔法塔",
    7726 : "远古魔法塔",
    7705 : lambda modifier, modifierSet: "第一天 " + InvasionEventsCHS[modifierSet],
    7725 : lambda modifier, modifierSet: "第二天 " + InvasionEventsCHS[modifierSet],
}

class PathDefinitions:
    def __init__(self, script_path: str):
        self.project_dir = Path(script_path).resolve().parent.parent 
        self.asset_root_dir= self.project_dir / "nightreign"
        self.assets_dir= self.asset_root_dir / "assets"
        self.metadata_dir = self.project_dir / "scripts" /"metadata"
        self.datas_dir = self.assets_dir / "datas"
        self.textures_dir = self.assets_dir / "textures"
        self.fonts_dir = self.assets_dir / "fonts"
        self.src_dir = self.project_dir / "src"
        self.cpp_header_dir= self.src_dir / "generated"
        self.sql_dir = self.project_dir / "scripts" / "sql"
        self.sqldata_dir = self.project_dir / "scripts" / "sqldata"

    def get_cpp_header(self, fn: str):
        return str(self.cpp_header_dir / f'{fn}.h')
    
    def get_metadata(self, fn: str):
        return str(self.metadata_dir / fn)

    def get_sql(self, fn: str):
        return str(self.sql_dir / fn)
    
    def get_sqldata_dir(self):
        return str(self.sqldata_dir)
    
    def get_sqldata(self, fn: str):
        return str(self.sqldata_dir / fn)

    def get_output(self, fn: str):
        return str(self.datas_dir / fn)

    def get_atlas_csv(self):
        return str(self.asset_root_dir / "atlas.csv")

    def get_manifest_csv(self):
        return str(self.asset_root_dir / "manifest.csv")