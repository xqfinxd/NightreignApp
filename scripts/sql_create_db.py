import sqlite3
import os
import defines
import csvutil
import logging

def print_db_changes(db_conn: sqlite3.Connection):
    """打印数据库更改统计信息"""
    cursor = db_conn.cursor()
    changes = db_conn.total_changes
    logging.info(f"数据库更改统计: {changes} 行受影响")

def create_database(db_path: str, sqlfile: str):
    """创建SQLite数据库并执行建表SQL脚本"""
    # 如果数据库文件已存在，提示用户是否覆盖
    if os.path.exists(db_path):
        response = input(f"数据库文件 {db_path} 已存在。是否删除重建？(y/n): ")
        if response.lower() == 'y':
            os.remove(db_path)
            logging.info(f"已删除现有数据库: {db_path}")
        else:
            logging.info("使用现有数据库")
            return db_path
    
    # 创建新的数据库连接并执行建表脚本
    conn = sqlite3.connect(db_path)
    
    cursor = conn.cursor()
    
    cursor.execute("PRAGMA foreign_keys = ON")
    cursor.execute("PRAGMA cache_size = -30000")
    
    logging.info("开始创建数据库表...")
    with open(sqlfile, 'r', encoding='utf-8') as f:
        cursor.executescript(f.read())
    
    conn.commit()
    print_db_changes(conn)

    cursor.execute("PRAGMA journal_mode")
    journal_mode = cursor.fetchone()[0]
    
    cursor.execute("PRAGMA foreign_keys")
    foreign_keys = cursor.fetchone()[0]
    
    # 输出创建结果
    logging.info("✅ 数据库创建成功！")
    logging.info(f"📁 数据库文件: {os.path.abspath(db_path)}")
    logging.info(f"⚙️  日志模式: {journal_mode}")
    logging.info(f"🔗 外键约束: {'启用' if foreign_keys else '禁用'}")
    
    # 关闭连接
    conn.close()
    return db_path

def init_table_by_sqlfiles(db_path: str, sqlfiles: list):
    """通过SQL文件列表初始化表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    for sqlfile in sqlfiles:
        with open(sqlfile, 'r', encoding='utf-8') as f:
            # 执行SQL脚本
            logging.info(f"正在执行SQL文件: {sqlfile}")
            cursor.executescript(f.read())
    
    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

CastleAttachIds = [190, 2190]
LTDivineTowerAttachIds = [1114, 1115, 1116]
RBDivineTowerAttachIds = [1111, 1112, 1113]
LTCityAttachIds = [1142]
RBCityAttachIds = [1136]
def remap_attachpoint(attachid: int, attach_point_csv: dict) -> int:
    if attachid in CastleAttachIds:
        castle = attach_point_csv.filter(worldMapPointIconId=11)[0].get("ID", attachid)
        return int(castle)
    if attachid in LTDivineTowerAttachIds:
        towers = attach_point_csv.filter(worldMapPointIconId=73)
        sorted_towers = sorted(towers, key=lambda t: t.get("gridXNo", 0)) # x -> left or right
        return int(sorted_towers[0].get("ID", attachid)) # left tower
    if attachid in RBDivineTowerAttachIds:
        towers = attach_point_csv.filter(worldMapPointIconId=73)
        sorted_towers = sorted(towers, key=lambda t: t.get("gridXNo", 0)) # x -> left or right
        return int(sorted_towers[-1].get("ID", attachid)) # right tower
    if attachid in LTCityAttachIds:
        city = attach_point_csv.filter(worldMapPointIconId=75)[0].get("ID", attachid)
        return int(city)
    if attachid in RBCityAttachIds:
        city = attach_point_csv.filter(worldMapPointIconId=74)[0].get("ID", attachid)
        return int(city)
    return attachid

def init_table_attachpoint(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化AttachPoint表数据"""

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化AttachPoint表")
    spot_config_csv = csvutil.load_csv(paths.get_metadata('LotResultSmallBaseAndSpot.csv'), key_column='ID')
    attach_point_csv = csvutil.load_csv(paths.get_metadata('WorldMapPointParam.csv'), key_column='ID')
    used_attach_points = spot_config_csv.group_by('attachId').keys()
    for attach_id in used_attach_points:
        remapped_id = remap_attachpoint(attach_id, attach_point_csv)
        rowdata = attach_point_csv.get(remapped_id, None)
        if rowdata is None:
            logging.warning(f"attachId {attach_id} 在 WorldMapPointParam.csv 中未找到对应数据，跳过")
            continue
        grid_x = rowdata.get("gridXNo", 0)
        grid_z = rowdata.get("gridZNo", 0)
        pos_x = rowdata.get("posX", 0)
        pos_z = rowdata.get("posZ", 0)
        height = rowdata.get("posY", 0)
        cursor.execute(
            """INSERT INTO AttachPoint (attach_id, grid_x, grid_z, pos_x, pos_z, height) VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(attach_id) DO UPDATE SET
                    grid_x=excluded.grid_x,
                    grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x,
                    pos_z=excluded.pos_z,
                    height=excluded.height""", 
            (attach_id, grid_x, grid_z, pos_x, pos_z, height))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_smallbasemap(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化SmallBase表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化SmallBaseMap表")
    spot_config_csv = csvutil.load_csv(paths.get_metadata('LotResultSmallBaseAndSpot.csv'), key_column='ID')
    used_small_base_maps = set(spot_config_csv.group_by('smallBaseMapId').keys())
    pattern_playarea_csv = csvutil.load_csv(paths.get_metadata('LotResultPlayAreaParam.csv'), key_column='ID')
    for playarea_rowdata in pattern_playarea_csv.values():
        bossId1 = int(playarea_rowdata.get("bossId1", 0))
        bossId2 = int(playarea_rowdata.get("bossId2", 0))
        extraBossId1 = int(playarea_rowdata.get("extraBossId1", 0))
        extraBossId2 = int(playarea_rowdata.get("extraBossId2", 0))
        if bossId1 > 0:
            used_small_base_maps.add(bossId1)
        if bossId2 > 0:
            used_small_base_maps.add(bossId2)
        if extraBossId1 > 0:
            used_small_base_maps.add(extraBossId1)
        if extraBossId2 > 0:
            used_small_base_maps.add(extraBossId2)
    used_small_base_maps.discard(0) # 移除无效ID
    for smallbase_id in used_small_base_maps:
        cursor.execute(
            """INSERT INTO SmallBaseMap (smallbase_id) VALUES (?)
                ON CONFLICT(smallbase_id) DO NOTHING""", 
            (smallbase_id,))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_playarea(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化PlayArea表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化PlayArea表")
    playarea_csv = csvutil.load_csv(paths.get_metadata('PlayAreaCreateParam.csv'), key_column='ID')
    for playarea_id, rowdata in playarea_csv.items():
        grid_x = int(rowdata.get("gridXNo", 0))
        grid_z = int(rowdata.get("gridZNo", 0))
        pos_x = float(rowdata.get("posX", 0))
        pos_z = float(rowdata.get("posZ", 0))
        height = float(rowdata.get("posY", 0))
        cursor.execute(
            """INSERT INTO PlayArea (playarea_id, grid_x, grid_z, pos_x, pos_z, height) VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(playarea_id) DO UPDATE SET
                    grid_x=excluded.grid_x,
                    grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x,
                    pos_z=excluded.pos_z,
                    height=excluded.height""", 
            (playarea_id, grid_x, grid_z, pos_x, pos_z, height))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_variationparam(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化VariationParam表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化VariationParam表")
    spot_config_csv = csvutil.load_csv(paths.get_metadata('LotResultSmallBaseAndSpot.csv'), key_column='ID')
    for rowdata in spot_config_csv.values():
        smallbase_id = int(rowdata.get("smallBaseMapId", 0))
        variation_id = int(rowdata.get("variationId", 0))
        cursor.execute(
            """INSERT INTO VariationParam (smallbase_id, variation_id) VALUES (?, ?)
                ON CONFLICT(smallbase_id, variation_id) DO UPDATE SET
                    smallbase_id=excluded.smallbase_id,
                    variation_id=excluded.variation_id""",
            (smallbase_id, variation_id))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_pattern(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化Pattern表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化Pattern表")
    pattern_playarea_csv = csvutil.load_csv(paths.get_metadata('LotResultPlayAreaParam.csv'), key_column='ID')
    pattern_flag_csv = csvutil.load_csv(paths.get_metadata("LotResultMapPatternFlag.csv"), key_column='ID')
    for pattern_id, rowdata in pattern_playarea_csv.items():
        day1_playarea_id = int(rowdata.get("playArea1", 0))
        day2_playarea_id = int(rowdata.get("playArea2", 0))
        day1boss_smallbase_id = int(rowdata.get("bossId1", 0))
        day2boss_smallbase_id = int(rowdata.get("bossId2", 0))
        day1extraboss_smallbase_id = int(rowdata.get("extraBossId1", 0))
        if day1extraboss_smallbase_id <= 0:
            day1extraboss_smallbase_id = None
        day2extraboss_smallbase_id = int(rowdata.get("extraBossId2", 0))
        if day2extraboss_smallbase_id <= 0:
            day2extraboss_smallbase_id = None
        
        starter_rows = pattern_flag_csv.filter(patternId=pattern_id)
        starter_rowdata = None
        for row in starter_rows:
            modifierSet = int(row.get("modifierSet", 0))
            if modifierSet == 190 or modifierSet == 160: # 基础模式
                starter_rowdata = row
                break
        if not starter_rowdata:
            raise ValueError(f"在 LotResultMapPatternFlag.csv 中未找到数据，pattern_id={pattern_id}")
        dlc = int(starter_rowdata.get("patternSetId", 0)) >= 1000
        starter_id = int(starter_rowdata.get("modifier", 0))
        map_id = int(starter_rowdata.get("rareMap", 0))
        nightlord_id = int(starter_rowdata.get("targetBoss", 0))
        
        cursor.execute(
            """INSERT INTO Pattern (pattern_id, map_id, nightlord_id, starter_id,
                day1_playarea_id, day2_playarea_id,
                day1boss_smallbase_id, day2boss_smallbase_id,
                day1extraboss_smallbase_id, day2extraboss_smallbase_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(pattern_id) DO UPDATE SET
                    map_id=excluded.map_id,
                    nightlord_id=excluded.nightlord_id,
                    starter_id=excluded.starter_id,
                    day1_playarea_id=excluded.day1_playarea_id,
                    day2_playarea_id=excluded.day2_playarea_id,
                    day1boss_smallbase_id=excluded.day1boss_smallbase_id,
                    day2boss_smallbase_id=excluded.day2boss_smallbase_id,
                    day1extraboss_smallbase_id=excluded.day1extraboss_smallbase_id,
                    day2extraboss_smallbase_id=excluded.day2extraboss_smallbase_id""",
            (pattern_id, map_id, nightlord_id, starter_id,
            day1_playarea_id, day2_playarea_id,
            day1boss_smallbase_id, day2boss_smallbase_id,
            day1extraboss_smallbase_id, day2extraboss_smallbase_id))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_spotconfig(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化SpotConfig表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化SpotConfig表")
    spot_config_csv = csvutil.load_csv(paths.get_metadata('LotResultSmallBaseAndSpot.csv'), key_column='ID')
    for rowdata in spot_config_csv.values():
        pattern_id = int(rowdata.get("patternId", 0))
        attach_id = int(rowdata.get("attachId", 0))
        smallbase_id = int(rowdata.get("smallBaseMapId", 0))
        variation_id = int(rowdata.get("variationId", 0))
        
        cursor.execute(
            """INSERT INTO SpotConfig (pattern_id, attach_id, smallbase_id, variation_id) VALUES (?, ?, ?, ?)
                ON CONFLICT(pattern_id, attach_id) DO UPDATE SET
                    pattern_id=excluded.pattern_id,
                    attach_id=excluded.attach_id,
                    smallbase_id=excluded.smallbase_id,
                    variation_id=excluded.variation_id""",
            (pattern_id, attach_id, smallbase_id, variation_id))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_eventconfig(db_path: str, paths: defines.PathDefinitions):
    """通过CSV文件初始化EventConfig表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    logging.info("正在初始化EventConfig表")
    pattern_flag_csv = csvutil.load_csv(paths.get_metadata('LotResultMapPatternFlag.csv'), key_column='ID')
    for rowdata in pattern_flag_csv.values():
        eventflag = int(rowdata.get("eventFlag", 0))
        if eventflag == 0 or eventflag not in defines.EventDefinitionsCHS.keys():
            continue
        pattern_id = int(rowdata.get("patternId", 0))
        eventvalue = defines.EventDefinitionsCHS.get(eventflag)
        content = eventvalue if isinstance(eventvalue, str) else ""
        if eventvalue is None:
            logging.warning(f"未找到 eventFlag {eventflag} 的中文描述，pattern_id={pattern_id}")
            continue
        elif callable(eventvalue):
            modifier = int(rowdata.get("modifier", 0))
            modifierSet = int(rowdata.get("modifierSet", 0))
            content = eventvalue(modifier, modifierSet)
        
        cursor.execute(
            """INSERT INTO EventConfig (pattern_id, content) VALUES (?, ?)
                ON CONFLICT(pattern_id) DO UPDATE SET
                    pattern_id=excluded.pattern_id,
                    content=excluded.content""",
            (pattern_id, content))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def main():
    """主函数"""
    import argparse

    paths = defines.PathDefinitions(__file__)
    
    parser = argparse.ArgumentParser(description='创建地图模式配置数据库')
    default_db_path = paths.get_metadata('game_data.db')
    parser.add_argument('-o', '--output', default=default_db_path, 
                       help=f'输出数据库文件路径 (默认: {default_db_path})')
    
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    
    # 创建数据库
    db_path = create_database(args.output, paths.get_queries('create_tables.sql'))
    # 使用SQL文件初始化表数据
    init_table_by_sqlfiles(db_path, [
        paths.get_queries('init_table_map.sql'),
        paths.get_queries('init_table_nightlord.sql'),
        paths.get_queries('init_table_starter.sql')
    ])
    
    # 使用CSV文件初始化AttachPoint表数据
    init_table_attachpoint(db_path, paths)
    # 使用CSV文件初始化SmallBaseMap表数据
    init_table_smallbasemap(db_path, paths)
    # 使用CSV文件初始化PlayArea表数据
    init_table_playarea(db_path, paths)
    # 使用CSV文件初始化VariationParam表数据
    init_table_variationparam(db_path, paths)
    # 使用CSV文件初始化Pattern表数据
    init_table_pattern(db_path, paths)
    # 使用CSV文件初始化SpotConfig表数据
    init_table_spotconfig(db_path, paths)
    # 使用CSV文件初始化EventConfig表数据
    init_table_eventconfig(db_path, paths)

if __name__ == "__main__":
    main()