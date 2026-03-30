import argparse
import json
import logging
import os
import sqlite3

import defines


def _table_columns(conn: sqlite3.Connection, table_name: str) -> list[str]:
    rows = conn.execute(f"PRAGMA table_info([{table_name}])").fetchall()
    if not rows:
        raise ValueError(f"表不存在: {table_name}")
    return [row[1] for row in rows]


# upsert: 不存在则插入，存在则更新
# skip:   不存在则插入，存在则跳过
# update: 存在则更新，不存在则跳过
_VALID_OPS = ('upsert', 'skip', 'update')


def _validate_payload(payload: dict, jsonfile: str) -> tuple[str, list[str], str, list[str], list[list]]:
    """返回 (table_name, pk_cols, op, headers, rows)"""
    if not isinstance(payload, dict):
        raise ValueError(f"JSON格式错误（应为对象）: {jsonfile}")
    for key in ('table', 'pk', 'headers', 'rows'):
        if key not in payload:
            raise ValueError(f"JSON缺少必要字段 '{key}': {jsonfile}")

    table_name = payload['table']
    raw_pk = payload['pk']
    headers = payload['headers']
    rows = payload['rows']
    op = payload.get('op', 'upsert')

    # 规范化 pk 为列表（支持字符串单列或字符串列表组合主键）
    if isinstance(raw_pk, str):
        pk_cols: list[str] = [raw_pk]
    elif isinstance(raw_pk, list) and raw_pk and all(isinstance(c, str) for c in raw_pk):
        pk_cols = raw_pk
    else:
        raise ValueError(f"pk 必须为字符串或非空字符串列表: {jsonfile}")

    if op not in _VALID_OPS:
        raise ValueError(f"op 字段值无效 '{op}'，支持: {_VALID_OPS}: {jsonfile}")

    if not isinstance(headers, list) or not all(isinstance(h, str) for h in headers):
        raise ValueError(f"headers 必须为字符串数组: {jsonfile}")
    for col in pk_cols:
        if col not in headers:
            raise ValueError(f"pk 列 '{col}' 不在 headers 中: {jsonfile}")
    if not isinstance(rows, list):
        raise ValueError(f"rows 必须为数组: {jsonfile}")

    n = len(headers)
    for i, row in enumerate(rows):
        if not isinstance(row, list):
            raise ValueError(f"rows[{i}] 不是数组: {jsonfile}")
        if len(row) != n:
            raise ValueError(f"rows[{i}] 长度 {len(row)} 与 headers 长度 {n} 不符: {jsonfile}")

    return table_name, pk_cols, op, headers, rows


def import_from_json(db_path: str, jsonfile: str) -> None:
    """
    从JSON文件导入数据到对应表。
      op='upsert': 不存在则插入，存在则更新（默认）
      op='skip':   不存在则插入，存在则跳过
      op='update': 存在则更新，不存在则跳过
    """
    with open(jsonfile, 'r', encoding='utf-8') as f:
        payload = json.load(f)

    table_name, pk_cols, op, headers, rows = _validate_payload(payload, jsonfile)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    table_columns = _table_columns(conn, table_name)
    for col in pk_cols:
        if col not in table_columns:
            conn.close()
            raise ValueError(f"主键字段不存在: {table_name}.{col} ({jsonfile})")
    for col in headers:
        if col not in table_columns:
            conn.close()
            raise ValueError(f"字段不存在于表 {table_name}: {col} ({jsonfile})")

    if not rows:
        logging.info(f"JSON文件无数据行，跳过: {jsonfile}")
        conn.close()
        return

    logging.info(f"正在导入 [{table_name}] 表: {jsonfile}（{len(rows)} 行，op={op}）")
    pk_set = set(pk_cols)
    non_pk = [c for c in headers if c not in pk_set]

    if op == 'update':
        # 只更新已存在行；rows 中各列顺序按 headers，需重排为 SET 值 + WHERE 值
        if not non_pk:
            logging.info(f"[{table_name}] op=update 但无非主键列，跳过")
            conn.close()
            return
        set_clause = ', '.join([f"[{c}]=?" for c in non_pk])
        where_clause = ' AND '.join([f"[{c}]=?" for c in pk_cols])
        sql = f"UPDATE [{table_name}] SET {set_clause} WHERE {where_clause}"
        # 构建参数索引：SET 列在前，WHERE（pk）列在后
        non_pk_idx = [headers.index(c) for c in non_pk]
        pk_idx = [headers.index(c) for c in pk_cols]
        param_idx = non_pk_idx + pk_idx
        for row in rows:
            cursor.execute(sql, [row[i] for i in param_idx])
    else:
        col_names = ', '.join([f"[{c}]" for c in headers])
        placeholders = ', '.join(['?' for _ in headers])
        conflict_cols = ', '.join([f"[{c}]" for c in pk_cols])
        if op == 'upsert' and non_pk:
            updates = ', '.join([f"[{c}]=excluded.[{c}]" for c in non_pk])
            sql = (
                f"INSERT INTO [{table_name}] ({col_names}) VALUES ({placeholders}) "
                f"ON CONFLICT({conflict_cols}) DO UPDATE SET {updates}"
            )
        else:
            # op == 'skip' 或无非主键列
            sql = (
                f"INSERT INTO [{table_name}] ({col_names}) VALUES ({placeholders}) "
                f"ON CONFLICT({conflict_cols}) DO NOTHING"
            )
        for row in rows:
            cursor.execute(sql, row)

    conn.commit()
    logging.info(f"[{table_name}] 完成，数据库更改: {conn.total_changes} 行受影响")
    conn.close()


def discover_json_files(sql_dir: str) -> list[str]:
    files: list[str] = []
    for name in sorted(os.listdir(sql_dir)):
        full = os.path.join(sql_dir, name)
        if os.path.isfile(full) and name.lower().endswith('.json'):
            files.append(full)
    return files


def resolve_inputs(paths: defines.PathDefinitions, json_inputs: list[str], import_all: bool) -> list[str]:
    resolved: list[str] = []
    if import_all:
        resolved.extend(discover_json_files(paths.get_sql('')))

    for item in json_inputs:
        if os.path.isabs(item) and os.path.exists(item):
            resolved.append(item)
            continue
        direct = os.path.abspath(item)
        if os.path.exists(direct):
            resolved.append(direct)
            continue
        by_sql_dir = paths.get_sql(item)
        if os.path.exists(by_sql_dir):
            resolved.append(by_sql_dir)
            continue
        raise FileNotFoundError(f"JSON文件不存在: {item}")

    unique: list[str] = []
    seen: set[str] = set()
    for p in resolved:
        ap = os.path.abspath(p)
        if ap not in seen:
            seen.add(ap)
            unique.append(ap)
    return unique


def main() -> None:
    paths = defines.PathDefinitions(__file__)
    default_db_path = paths.get_output('game_data2.db')

    parser = argparse.ArgumentParser(description='Step 3: 从 scripts/sql JSON文件导入自定义数据')
    parser.add_argument('-d', '--db', default=default_db_path,
                        help=f'数据库路径 (默认: {default_db_path})')
    parser.add_argument('--import-json', action='append', default=[], metavar='FILE',
                        help='导入指定JSON文件（可重复）；支持绝对路径/相对路径/仅文件名')
    parser.add_argument('-a', '--all', action='store_true', dest='import_all',
                        help='导入 scripts/sql 下全部JSON文件（无 --import-json 时默认开启）')
    parser.add_argument('--continue-on-error', action='store_true',
                        help='某个文件失败时继续处理后续文件')

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    use_all = args.import_all or (len(args.import_json) == 0)
    json_files = resolve_inputs(paths, args.import_json, use_all)

    if not json_files:
        logging.info('未发现可导入JSON，退出')
        return

    for json_file in json_files:
        try:
            import_from_json(args.db, json_file)
        except Exception as e:
            if not args.continue_on_error:
                raise
            logging.error(f"导入失败: {json_file}: {e}")

    logging.info("✅ JSON导入完成")


if __name__ == '__main__':
    main()
