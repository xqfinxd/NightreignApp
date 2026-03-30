import argparse
import csv as _csv
import logging
import sqlite3

import defines


CSV_INPUT_TABLES = [
    'LotBaseMapPatternFlag',
    'LotResultMapPatternFlag',
    'LotResultPlayAreaParam',
    'LotResultSmallBaseAndSpot',
    'NightBossMenuParam',
    'PlayAreaCreateParam',
    'SmallBaseAndSpotAttachPoint',
    'SmallBaseMapVariationParam',
    'WorldMapPointParam',
]


def _print_changes(conn: sqlite3.Connection, label: str) -> None:
    logging.info(f"{label} 完成，数据库更改: {conn.total_changes} 行受影响")


def load_csv_to_mem(paths: defines.PathDefinitions, csv_names: list) -> sqlite3.Connection:
    """将CSV文件加载到内存SQLite数据库中，每个文件对应一个表"""
    mem = sqlite3.connect(':memory:')
    mem.row_factory = sqlite3.Row
    for name in csv_names:
        filepath = paths.get_metadata(f'{name}.csv')
        with open(filepath, newline='', encoding='utf-8') as f:
            reader = _csv.DictReader(f)
            cols = reader.fieldnames
            if not cols:
                raise ValueError(f"CSV缺少表头: {filepath}")
            column_sql = ', '.join([f"[{c}]" for c in cols])
            value_sql = ', '.join(['?' for _ in cols])
            mem.execute(f"CREATE TABLE [{name}] ({column_sql})")
            mem.executemany(
                f"INSERT INTO [{name}] VALUES ({value_sql})",
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
        return int(rows[0]['ID']) if rows else attachid
    if attachid in RBDivineTowerAttachIds:
        rows = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=73 ORDER BY CAST(gridXNo AS INTEGER)").fetchall()
        return int(rows[-1]['ID']) if rows else attachid
    if attachid in LTCityAttachIds:
        row = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=75 LIMIT 1").fetchone()
        return int(row['ID']) if row else attachid
    if attachid in RBCityAttachIds:
        row = mem.execute("SELECT ID FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=74 LIMIT 1").fetchone()
        return int(row['ID']) if row else attachid
    return attachid


def init_table_attachpoint(db_path: str, mem: sqlite3.Connection) -> None:
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
                    grid_x=excluded.grid_x, grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x, pos_z=excluded.pos_z, height=excluded.height""",
            (attach_id, grid_x, grid_z, pos_x, pos_z, height))
    conn.commit()
    _print_changes(conn, "AttachPoint")
    conn.close()


def init_table_smallbasemap(db_path: str, mem: sqlite3.Connection) -> None:
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
    used_small_base_maps.discard(0)
    for smallbase_id in used_small_base_maps:
        cursor.execute(
            "INSERT INTO SmallBaseMap (smallbase_id) VALUES (?) ON CONFLICT(smallbase_id) DO NOTHING",
            (smallbase_id,))
    conn.commit()
    _print_changes(conn, "SmallBaseMap")
    conn.close()


def init_table_playarea(db_path: str, mem: sqlite3.Connection) -> None:
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
                    grid_x=excluded.grid_x, grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x, pos_z=excluded.pos_z, height=excluded.height""",
            (playarea_id, grid_x, grid_z, pos_x, pos_z, 0))
    conn.commit()
    _print_changes(conn, "PlayArea")
    conn.close()


def init_table_variationparam(db_path: str, mem: sqlite3.Connection) -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    logging.info("正在初始化VariationParam表")
    for rowdata in mem.execute("SELECT smallBaseMapId, variationId FROM LotResultSmallBaseAndSpot").fetchall():
        smallbase_id = int(rowdata['smallBaseMapId'] or 0)
        variation_id = int(rowdata['variationId'] or 0)
        cursor.execute(
            """INSERT INTO VariationParam (smallbase_id, variation_id) VALUES (?, ?)
                ON CONFLICT(smallbase_id, variation_id) DO UPDATE SET
                    smallbase_id=excluded.smallbase_id, variation_id=excluded.variation_id""",
            (smallbase_id, variation_id))
    conn.commit()
    _print_changes(conn, "VariationParam")
    conn.close()


def init_table_pattern(db_path: str, mem: sqlite3.Connection) -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    logging.info("正在初始化Pattern/Map/Nightlord/Starter表")

    seen_map_ids: set[int] = set()
    seen_nightlord_ids: set[int] = set()
    seen_starter_ids: set[int] = set()

    for rowdata in mem.execute("SELECT * FROM LotResultPlayAreaParam").fetchall():
        pattern_id = int(rowdata['ID'])
        day1_playarea_id = int(rowdata['playArea1'] or 0)
        day2_playarea_id = int(rowdata['playArea2'] or 0)
        day1boss_smallbase_id = int(rowdata['bossId1'] or 0)
        day2boss_smallbase_id = int(rowdata['bossId2'] or 0)
        _e1 = int(rowdata['extraBossId1'] or 0)
        day1extraboss_smallbase_id = _e1 if _e1 > 0 else None
        _e2 = int(rowdata['extraBossId2'] or 0)
        day2extraboss_smallbase_id = _e2 if _e2 > 0 else None

        flag_rows = mem.execute(
            "SELECT * FROM LotResultMapPatternFlag WHERE CAST(patternId AS INTEGER)=?",
            (pattern_id,)
        ).fetchall()
        starter_rowdata = None
        for row in flag_rows:
            modifierSet = int(row['modifierSet'] or 0)
            if modifierSet == 190 or modifierSet == 160:
                starter_rowdata = row
                break
        if not starter_rowdata:
            raise ValueError(f"在 LotResultMapPatternFlag.csv 中未找到数据，pattern_id={pattern_id}")
        dlc = int(starter_rowdata['patternSetId'] or 0) >= 1000
        starter_id = int(starter_rowdata['modifier'] or 0)
        map_id = int(starter_rowdata['rareMap'] or 0)
        nightlord_id = int(starter_rowdata['targetBoss'] or 0)

        seen_map_ids.add(map_id)
        seen_nightlord_ids.add(nightlord_id)
        seen_starter_ids.add(starter_id)

        cursor.execute(
            """INSERT INTO Pattern (pattern_id, map_id, nightlord_id, starter_id,
                day1_playarea_id, day2_playarea_id, dlc,
                day1boss_smallbase_id, day2boss_smallbase_id,
                day1extraboss_smallbase_id, day2extraboss_smallbase_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(pattern_id) DO UPDATE SET
                    map_id=excluded.map_id, nightlord_id=excluded.nightlord_id,
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

    # Map：插入遍历中收集到的所有 map_id 主键，name 由 JSON 导入步骤填充
    for mid in seen_map_ids:
        cursor.execute(
            "INSERT INTO Map (map_id) VALUES (?) ON CONFLICT(map_id) DO NOTHING",
            (mid,))

    # Nightlord：插入遍历中收集到的所有 nightlord_id 主键，name 由 JSON 导入步骤填充
    for nid in seen_nightlord_ids:
        cursor.execute(
            "INSERT INTO Nightlord (nightlord_id) VALUES (?) ON CONFLICT(nightlord_id) DO NOTHING",
            (nid,))

    # Starter：批量查询坐标并插入
    for sid in seen_starter_ids:
        cursor.execute(
            "INSERT INTO Starter (starter_id) VALUES (?) ON CONFLICT(starter_id) DO NOTHING",
            (sid,))

    conn.commit()
    _print_changes(conn, "Pattern/Map/Nightlord/Starter")
    conn.close()


def init_table_spotconfig(db_path: str, mem: sqlite3.Connection) -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    logging.info("正在初始化SpotConfig表")
    for rowdata in mem.execute(
        "SELECT patternId, attachId, smallBaseMapId, variationId FROM LotResultSmallBaseAndSpot"
    ).fetchall():
        pattern_id = int(rowdata['patternId'] or 0)
        attach_id = int(rowdata['attachId'] or 0)
        smallbase_id = int(rowdata['smallBaseMapId'] or 0)
        variation_id = int(rowdata['variationId'] or 0)
        cursor.execute(
            """INSERT INTO SpotConfig (pattern_id, attach_id, smallbase_id, variation_id) VALUES (?, ?, ?, ?)
                ON CONFLICT(pattern_id, attach_id) DO UPDATE SET
                    smallbase_id=excluded.smallbase_id, variation_id=excluded.variation_id""",
            (pattern_id, attach_id, smallbase_id, variation_id))
    conn.commit()
    _print_changes(conn, "SpotConfig")
    conn.close()


def init_table_eventconfig(db_path: str, mem: sqlite3.Connection) -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    logging.info("正在初始化EventConfig表")
    for rowdata in mem.execute("SELECT * FROM LotResultMapPatternFlag").fetchall():
        eventflag = int(rowdata['eventFlag'] or 0)
        if eventflag == 0 or eventflag not in defines.EventDefinitionsCHS:
            continue
        pattern_id = int(rowdata['patternId'] or 0)
        eventvalue = defines.EventDefinitionsCHS.get(eventflag)
        if eventvalue is None:
            logging.warning(f"未找到 eventFlag {eventflag} 的中文描述，pattern_id={pattern_id}")
            continue
        elif callable(eventvalue):
            modifier = int(rowdata['modifier'] or 0)
            modifierSet = int(rowdata['modifierSet'] or 0)
            content = eventvalue(modifier, modifierSet)
        else:
            content = eventvalue
        cursor.execute(
            """INSERT INTO EventConfig (pattern_id, content) VALUES (?, ?)
                ON CONFLICT(pattern_id) DO UPDATE SET content=excluded.content""",
            (pattern_id, content))
    conn.commit()
    _print_changes(conn, "EventConfig")
    conn.close()


def init_table_bindings(db_path: str, mem: sqlite3.Connection) -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    logging.info("正在初始化特殊地形力量数据")
    for rowdata in mem.execute(
        "SELECT * FROM WorldMapPointParam WHERE CAST(worldMapPointIconId AS INTEGER)=61"
    ).fetchall():
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
                    grid_x=excluded.grid_x, grid_z=excluded.grid_z,
                    pos_x=excluded.pos_x, pos_z=excluded.pos_z, height=excluded.height""",
            (attach_id, grid_x, grid_z, pos_x, pos_z, height))
        if map_id == 3:
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
                "INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?) ON CONFLICT(attach_id, map_id) DO NOTHING",
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
                        grid_x=excluded.grid_x, grid_z=excluded.grid_z,
                        pos_x=excluded.pos_x, pos_z=excluded.pos_z, height=excluded.height""",
                (attach_id, grid_x, grid_z, pos_x, pos_z, height))
            flagid6 = int(rowdata['eventFlagId6'] or 0)
            if point_type == 16:
                flagid2 = int(rowdata['eventFlagId2'] or 0)
                flagid0 = int(rowdata['eventFlagId0'] or 0)
                smallbase_id = flagid0 // 10000
                if map_id == 4 and flagid2 > 0 and smallbase_id > 0:
                    cursor.execute("""
                        INSERT INTO AttachSmallBaseBinding (attach_id, smallbase_id)
                        SELECT ?, ? WHERE EXISTS (SELECT 1 FROM SmallBaseMap WHERE smallbase_id = ?)
                        ON CONFLICT(attach_id, smallbase_id) DO NOTHING
                    """, (attach_id, smallbase_id, smallbase_id))
                elif flagid6 > 0:
                    cursor.execute(
                        "INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?) ON CONFLICT(attach_id, map_id) DO NOTHING",
                        (attach_id, map_id))
            else:
                cursor.execute(
                    "INSERT INTO AttachMapBinding (attach_id, map_id) VALUES (?, ?) ON CONFLICT(attach_id, map_id) DO NOTHING",
                    (attach_id, map_id))

    conn.commit()
    _print_changes(conn, "Bindings")
    conn.close()


def main() -> None:
    paths = defines.PathDefinitions(__file__)
    default_db_path = paths.get_output('game_data2.db')

    parser = argparse.ArgumentParser(description='Step 2: 通过metadata CSV文件初始化数据库数据')
    parser.add_argument('-d', '--db', default=default_db_path,
                        help=f'数据库路径 (默认: {default_db_path})')

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    mem = load_csv_to_mem(paths, CSV_INPUT_TABLES)
    try:
        init_table_attachpoint(args.db, mem)
        init_table_smallbasemap(args.db, mem)
        init_table_playarea(args.db, mem)
        init_table_variationparam(args.db, mem)
        init_table_pattern(args.db, mem)
        init_table_spotconfig(args.db, mem)
        init_table_eventconfig(args.db, mem)
        init_table_bindings(args.db, mem)
    finally:
        mem.close()
    logging.info("✅ CSV数据初始化完成")


if __name__ == '__main__':
    main()
