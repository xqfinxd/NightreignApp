import csv as _csv
import json
import sqlite3
import os
import defines
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

def init_table_by_jsonfile(db_path: str, jsonfile: str):
    """通过JSON文件初始化表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    with open(jsonfile, 'r', encoding='utf-8') as f:
        data = json.load(f)

    table_name = data['table']
    pk_col = data['pk']
    rows = data.get('rows', [])

    if not rows:
        conn.close()
        return

    logging.info(f"正在从JSON文件初始化 {table_name} 表: {jsonfile}")
    columns = list(rows[0].keys())
    placeholders = ', '.join(['?' for _ in columns])
    col_names = ', '.join(columns)
    updates = ', '.join([f"{col}=excluded.{col}" for col in columns if col != pk_col])
    sql = f"""INSERT INTO {table_name} ({col_names}) VALUES ({placeholders})
        ON CONFLICT({pk_col}) DO UPDATE SET {updates}"""

    for row in rows:
        cursor.execute(sql, list(row.values()))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def load_csv_to_mem(paths: defines.PathDefinitions, csv_names: list) -> sqlite3.Connection:
    """将CSV文件加载到内存SQLite数据库中，每个文件对应一个表"""
    mem = sqlite3.connect(':memory:')
    mem.row_factory = sqlite3.Row
    for name in csv_names:
        filepath = paths.get_metadata(f'{name}.csv')
        with open(filepath, newline='', encoding='utf-8') as f:
            reader = _csv.DictReader(f)
            cols = reader.fieldnames
            mem.execute(f'CREATE TABLE [{name}] ({', '.join(f'[{c}]' for c in cols)})')
            mem.executemany(
                f'INSERT INTO [{name}] VALUES ({', '.join('?' for _ in cols)})',
                (list(row.values()) for row in reader)
            )
    mem.commit()
    return mem

CastleAttachIds = [190, 2190]
LTDivineTowerAttachIds = [1114, 1115, 1116]
RBDivineTowerAttachIds = [1111, 1112, 1113]
LTCityAttachIds = [1142]
RBCityAttachIds = [1136]
def remap_attachpoint(attachid: int, mem: sqlite3.Connection) -> int:
    if attachid in CastleAttachIds:
        row = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=11 LIMIT 1").fetchone()
        return int(row['ID']) if row else attachid
    if attachid in LTDivineTowerAttachIds:
        rows = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=73 ORDER BY CAST(gridXNo AS INTEGER)").fetchall()
        return int(rows[0]['ID']) if rows else attachid  # left tower
    if attachid in RBDivineTowerAttachIds:
        rows = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=73 ORDER BY CAST(gridXNo AS INTEGER)").fetchall()
        return int(rows[-1]['ID']) if rows else attachid  # right tower
    if attachid in LTCityAttachIds:
        row = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=75 LIMIT 1").fetchone()
        return int(row['ID']) if row else attachid
    if attachid in RBCityAttachIds:
        row = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=74 LIMIT 1").fetchone()
        return int(row['ID']) if row else attachid
    return attachid

def init_table_attachpoint(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化AttachPoint表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化AttachPoint表")
    used_attach_points = [
        int(row[0]) for row in mem.execute(
            "SELECT DISTINCT CAST(attachId AS INTEGER) FROM LotResultSmallBaseAndSpot WHERE attachId IS NOT NULL"
        ).fetchall()
    ]
    for attach_id in used_attach_points:
        remapped_id = remap_attachpoint(attach_id, mem)
        rowdata = mem.execute(
            "SELECT gridXNo, gridZNo, posX, posZ, posY FROM WorldMapPointParam WHERE CAST(ID AS INTEGER)=?",
            (remapped_id,)
        ).fetchone()
        if rowdata is None:
            logging.warning(f"attachId {attach_id} 在 WorldMapPointParam.csv 中未找到对应数据，跳过")
            continue
        grid_x = int(rowdata['gridXNo'] or 0)
        grid_z = int(rowdata['gridZNo'] or 0)
        pos_x = float(rowdata['posX'] or 0)
        pos_z = float(rowdata['posZ'] or 0)
        height = float(rowdata['posY'] or 0)
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

def init_table_smallbasemap(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化SmallBase表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化SmallBaseMap表")
    used_small_base_maps = set(
        int(row[0]) for row in mem.execute(
            "SELECT DISTINCT CAST(smallBaseMapId AS INTEGER) FROM LotResultSmallBaseAndSpot WHERE smallBaseMapId IS NOT NULL"
        ).fetchall()
    )
    for rowdata in mem.execute("SELECT bossId1, bossId2, extraBossId1, extraBossId2 FROM LotResultPlayAreaParam").fetchall():
        for col in ('bossId1', 'bossId2', 'extraBossId1', 'extraBossId2'):
            val = int(rowdata[col] or 0)
            if val > 0:
                used_small_base_maps.add(val)
    used_small_base_maps.discard(0)  # 移除无效ID
    for smallbase_id in used_small_base_maps:
        cursor.execute(
            """INSERT INTO SmallBaseMap (smallbase_id) VALUES (?)
                ON CONFLICT(smallbase_id) DO NOTHING""",
            (smallbase_id,))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_playarea(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化PlayArea表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化PlayArea表")
    for rowdata in mem.execute("SELECT * FROM PlayAreaCreateParam").fetchall():
        playarea_id = int(rowdata['ID'])
        grid_x = int(rowdata['gridXNo'] or 0)
        grid_z = int(rowdata['gridZNo'] or 0)
        pos_x = float(rowdata['posX'] or 0)
        pos_z = float(rowdata['posZ'] or 0)
        cursor.execute(
            """INSERT INTO PlayArea (playarea_id, grid_x, grid_z, pos_x, pos_z, height) VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(playarea_id) DO UPDATE SET
                    grid_x=excluded.grid_x,
                    grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x,
                    pos_z=excluded.pos_z,
                    height=excluded.height""",
            (playarea_id, grid_x, grid_z, pos_x, pos_z, 0))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_variationparam(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化VariationParam表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化VariationParam表")
    for rowdata in mem.execute("SELECT smallBaseMapId, variationId FROM LotResultSmallBaseAndSpot").fetchall():
        smallbase_id = int(rowdata['smallBaseMapId'] or 0)
        variation_id = int(rowdata['variationId'] or 0)
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

def init_table_pattern(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化Pattern表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化Pattern表")
    for rowdata in mem.execute("SELECT * FROM LotResultPlayAreaParam").fetchall():
        pattern_id = int(rowdata['ID'])
        day1_playarea_id = int(rowdata['playArea1'] or 0)
        day2_playarea_id = int(rowdata['playArea2'] or 0)
        day1boss_smallbase_id = int(rowdata['bossId1'] or 0)
        day2boss_smallbase_id = int(rowdata['bossId2'] or 0)
        day1extraboss_smallbase_id = int(rowdata['extraBossId1'] or 0)
        if day1extraboss_smallbase_id <= 0:
            day1extraboss_smallbase_id = None
        day2extraboss_smallbase_id = int(rowdata['extraBossId2'] or 0)
        if day2extraboss_smallbase_id <= 0:
            day2extraboss_smallbase_id = None

        flag_rows = mem.execute(
            "SELECT * FROM LotResultMapPatternFlag WHERE CAST(patternId AS INTEGER)=?",
            (pattern_id,)
        ).fetchall()
        starter_rowdata = None
        for row in flag_rows:
            modifierSet = int(row['modifierSet'] or 0)
            if modifierSet == 190 or modifierSet == 160:  # 基础模式
                starter_rowdata = row
                break
        if not starter_rowdata:
            raise ValueError(f"在 LotResultMapPatternFlag.csv 中未找到数据，pattern_id={pattern_id}")
        dlc = int(starter_rowdata['patternSetId'] or 0) >= 1000
        starter_id = int(starter_rowdata['modifier'] or 0)
        map_id = int(starter_rowdata['rareMap'] or 0)
        nightlord_id = int(starter_rowdata['targetBoss'] or 0)

        cursor.execute(
            """INSERT INTO Pattern (pattern_id, map_id, nightlord_id, starter_id,
                day1_playarea_id, day2_playarea_id, dlc,
                day1boss_smallbase_id, day2boss_smallbase_id,
                day1extraboss_smallbase_id, day2extraboss_smallbase_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(pattern_id) DO UPDATE SET
                    map_id=excluded.map_id,
                    nightlord_id=excluded.nightlord_id,
                    starter_id=excluded.starter_id,
                    day1_playarea_id=excluded.day1_playarea_id,
                    day2_playarea_id=excluded.day2_playarea_id,
                    day1boss_smallbase_id=excluded.day1boss_smallbase_id,
                    day2boss_smallbase_id=excluded.day2boss_smallbase_id,
                    day1extraboss_smallbase_id=excluded.day1extraboss_smallbase_id,
                    day2extraboss_smallbase_id=excluded.day2extraboss_smallbase_id,
                    dlc=excluded.dlc""",
            (pattern_id, map_id, nightlord_id, starter_id,
             day1_playarea_id, day2_playarea_id, dlc,
             day1boss_smallbase_id, day2boss_smallbase_id,
             day1extraboss_smallbase_id, day2extraboss_smallbase_id))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def init_table_spotconfig(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化SpotConfig表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化SpotConfig表")
    for rowdata in mem.execute("SELECT patternId, attachId, smallBaseMapId, variationId FROM LotResultSmallBaseAndSpot").fetchall():
        pattern_id = int(rowdata['patternId'] or 0)
        attach_id = int(rowdata['attachId'] or 0)
        smallbase_id = int(rowdata['smallBaseMapId'] or 0)
        variation_id = int(rowdata['variationId'] or 0)
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

def init_table_eventconfig(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化EventConfig表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化EventConfig表")
    for rowdata in mem.execute("SELECT * FROM LotResultMapPatternFlag").fetchall():
        eventflag = int(rowdata['eventFlag'] or 0)
        if eventflag == 0 or eventflag not in defines.EventDefinitionsCHS.keys():
            continue
        pattern_id = int(rowdata['patternId'] or 0)
        eventvalue = defines.EventDefinitionsCHS.get(eventflag)
        content = eventvalue if isinstance(eventvalue, str) else ""
        if eventvalue is None:
            logging.warning(f"未找到 eventFlag {eventflag} 的中文描述，pattern_id={pattern_id}")
            continue
        elif callable(eventvalue):
            modifier = int(rowdata['modifier'] or 0)
            modifierSet = int(rowdata['modifierSet'] or 0)
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

def init_table_bindings(db_path: str, mem: sqlite3.Connection):
    """通过CSV文件初始化AttachBinding数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # handle shift earth power points
    logging.info("正在初始化特殊地形力量数据")
    for rowdata in mem.execute("SELECT * FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=61").fetchall():
        attach_id = int(rowdata['ID'] or 0)
        grid_x = int(rowdata['gridXNo'] or 0)
        grid_z = int(rowdata['gridZNo'] or 0)
        pos_x = float(rowdata['posX'] or 0)
        pos_z = float(rowdata['posZ'] or 0)
        height = float(rowdata['posY'] or 0)
        map_id = int(rowdata['pad'] or 0) // 10
        cursor.execute(
            """INSERT INTO AttachPoint (attach_id, grid_x, grid_z, pos_x, pos_z, height) VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(attach_id) DO UPDATE SET
                    grid_x=excluded.grid_x,
                    grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x,
                    pos_z=excluded.pos_z,
                    height=excluded.height""",
            (attach_id, grid_x, grid_z, pos_x, pos_z, height))
        if map_id == 3:  # rotted wood only
            flag_id = int(rowdata['eventFlagId1'] or 0)
            for pattern in mem.execute(
                "SELECT patternId FROM LotResultMapPatternFlag WHERE CAST(modifierSet AS INTEGER)=500 AND CAST(eventFlag AS INTEGER)=?",
                (flag_id,)
            ).fetchall():
                pattern_id = int(pattern['patternId'] or -1)
                if pattern_id >= 0:
                    cursor.execute(
                        """INSERT INTO AttachPatternBinding (pattern_id, attach_id) VALUES (?, ?)
                            ON CONFLICT(pattern_id, attach_id) DO NOTHING""",
                        (pattern_id, attach_id))
        else:
            cursor.execute(
                """INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?)
                    ON CONFLICT(attach_id, map_id) DO NOTHING""",
                (attach_id, map_id))

    logging.info("正在初始化地形固定点数据")
    enabled_types = [2, 16, 28, 71, 76]
    enabled_maps = [1, 2, 3, 4, 5]
    point_types = [
        int(row[0]) for row in mem.execute(
            "SELECT DISTINCT CAST(worldMapPointIconId AS INTEGER) FROM WorldMapPointParam"
        ).fetchall()
    ]
    for point_type in point_types:
        if point_type not in enabled_types:
            continue
        for rowdata in mem.execute(
            "SELECT * FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=?",
            (point_type,)
        ).fetchall():
            map_id = int(rowdata['pad'] or 0) // 10
            if map_id not in enabled_maps:
                continue
            attach_id = int(rowdata['ID'] or 0)
            grid_x = int(rowdata['gridXNo'] or 0)
            grid_z = int(rowdata['gridZNo'] or 0)
            pos_x = float(rowdata['posX'] or 0)
            pos_z = float(rowdata['posZ'] or 0)
            height = float(rowdata['posY'] or 0)
            cursor.execute(
                """INSERT INTO AttachPoint (attach_id, grid_x, grid_z, pos_x, pos_z, height) VALUES (?, ?, ?, ?, ?, ?)
                    ON CONFLICT(attach_id) DO UPDATE SET
                        grid_x=excluded.grid_x,
                        grid_z=excluded.grid_z,
                        pos_x=excluded.pos_x,
                        pos_z=excluded.pos_z,
                        height=excluded.height""",
                (attach_id, grid_x, grid_z, pos_x, pos_z, height))
            flagid6 = int(rowdata['eventFlagId6'] or 0)
            if point_type == 16:
                flagid2 = int(rowdata['eventFlagId2'] or 0)
                flagid0 = int(rowdata['eventFlagId0'] or 0)
                smallbase_id = flagid0 // 10000
                # great hollow boss points, bind to small base
                if map_id == 4 and flagid2 > 0 and smallbase_id > 0:
                    cursor.execute("""
                        INSERT INTO AttachSmallBaseBinding (attach_id, smallbase_id)
                        SELECT ?, ?
                        WHERE EXISTS (SELECT 1 FROM SmallBaseMap WHERE smallbase_id = ?)
                        ON CONFLICT(attach_id, smallbase_id) DO NOTHING
                    """, (attach_id, smallbase_id, smallbase_id))
                elif flagid6 > 0:  # other field boss points, bind to map
                    cursor.execute(
                        """INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?)
                            ON CONFLICT(attach_id, map_id) DO NOTHING""",
                        (attach_id, map_id))
            else:
                cursor.execute(
                    """INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?)
                        ON CONFLICT(attach_id, map_id) DO NOTHING""",
                    (attach_id, map_id))

    conn.commit()
    print_db_changes(conn)
    # 关闭连接
    conn.close()

def main():
    """主函数"""
    import argparse

    paths = defines.PathDefinitions(__file__)
    
    parser = argparse.ArgumentParser(description='创建地图模式配置数据库')
    default_db_path = paths.get_output('game_data2.db')
    parser.add_argument('-o', '--output', default=default_db_path, 
                       help=f'输出数据库文件路径 (默认: {default_db_path})')
    
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    
    # 创建数据库
    db_path = create_database(args.output, paths.get_sql('create_tables.sql'))
    
    # 使用JSON文件初始化表数据
    init_table_by_jsonfile(db_path, paths.get_sql('maps.json'))
    init_table_by_jsonfile(db_path, paths.get_sql('nightlords.json'))
    init_table_by_jsonfile(db_path, paths.get_sql('starters.json'))

    mem = load_csv_to_mem(paths, [
        'LotBaseMapPatternFlag',
        'LotResultMapPatternFlag',
        'LotResultPlayAreaParam',
        'LotResultSmallBaseAndSpot',
        'NightBossMenuParam',
        'PlayAreaCreateParam',
        'SmallBaseAndSpotAttachPoint',
        'SmallBaseMapVariationParam',
        'WorldMapPointParam',
        ])
    
    # 使用CSV文件初始化AttachPoint表数据
    init_table_attachpoint(db_path, mem)
    # 使用CSV文件初始化SmallBaseMap表数据
    init_table_smallbasemap(db_path, mem)
    # 使用CSV文件初始化PlayArea表数据
    init_table_playarea(db_path, mem)
    # 使用CSV文件初始化VariationParam表数据
    init_table_variationparam(db_path, mem)
    # 使用CSV文件初始化Pattern表数据
    init_table_pattern(db_path, mem)
    # 使用CSV文件初始化SpotConfig表数据
    init_table_spotconfig(db_path, mem)
    # 使用CSV文件初始化EventConfig表数据
    init_table_eventconfig(db_path, mem)
    # 使用CSV文件初始化Binding表数据
    init_table_bindings(db_path, mem)

    mem.close()

if __name__ == "__main__":
    main()