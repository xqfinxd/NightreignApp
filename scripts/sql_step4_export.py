import argparse
import json
import logging
import os
import sqlite3

import defines


def _table_columns(conn: sqlite3.Connection, table_name: str) -> list[str]:
    rows = conn.execute(f"PRAGMA table_info([{table_name}])").fetchall()
    if not rows:
        raise ValueError(f"表不存在或无字段: {table_name}")
    return [row[1] for row in rows]


def _table_primary_keys(conn: sqlite3.Connection, table_name: str) -> list[str]:
    rows = conn.execute(f"PRAGMA table_info([{table_name}])").fetchall()
    if not rows:
        raise ValueError(f"表不存在: {table_name}")
    pk_cols = sorted((r for r in rows if int(r[5]) > 0), key=lambda r: int(r[5]))
    if not pk_cols:
        raise ValueError(f"表未定义主键: {table_name}")
    return [r[1] for r in pk_cols]


def _load_spec(json_path: str) -> dict:
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    if not isinstance(data, dict) or 'table' not in data:
        raise ValueError(f"JSON格式错误或缺少 table 字段: {json_path}")
    return data


def export_table(
    conn: sqlite3.Connection,
    table_name: str,
    output_path: str,
    pk: str | list[str] | None = None,
    fields: list[str] | None = None,
) -> None:
    """将指定表导出为 {table, pk, headers, rows} 格式的JSON文件"""
    inferred_pks = _table_primary_keys(conn, table_name)
    if pk is None:
        pk_cols = inferred_pks
    elif isinstance(pk, str):
        pk_cols = [pk]
    else:
        pk_cols = pk

    all_columns = _table_columns(conn, table_name)
    pk_set = set(pk_cols)
    for col in pk_cols:
        if col not in all_columns:
            raise ValueError(f"主键字段不存在: {table_name}.{col}")

    if not fields:
        selected_fields = [c for c in all_columns if c not in pk_set]
    else:
        invalid = [f for f in fields if f not in all_columns]
        if invalid:
            raise ValueError(f"字段不存在: {table_name}.{', '.join(invalid)}")
        selected_fields = [f for f in fields if f not in pk_set]

    selected_columns = pk_cols + selected_fields
    quoted = ', '.join([f"[{c}]" for c in selected_columns])
    order_by = ', '.join([f"[{c}]" for c in pk_cols])
    cursor = conn.execute(f"SELECT {quoted} FROM [{table_name}] ORDER BY {order_by}")
    rows = [list(row) for row in cursor.fetchall()]

    # pk 为单列时存字符串，组合主键存列表
    pk_value: str | list[str] = pk_cols[0] if len(pk_cols) == 1 else pk_cols
    result = {
        "table": table_name,
        "pk": pk_value,
        "headers": selected_columns,
        "rows": rows,
    }

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=4)

    logging.info(f"已导出 [{table_name}] → {output_path}（{len(rows)} 行）")


def export_by_spec(conn: sqlite3.Connection, spec_path: str, output_path: str | None = None) -> None:
    """按JSON规范文件指定的 table/pk/headers 导出；输出路径默认覆写规范文件"""
    spec = _load_spec(spec_path)
    table_name = spec['table']
    raw_pk = spec.get('pk')

    # 规范化 pk：将字符串单列或列表组合主键均转为 list[str]，缺少则自动推断
    if raw_pk is None:
        pk_cols: list[str] | None = None
    elif isinstance(raw_pk, str):
        pk_cols = [raw_pk]
    else:
        pk_cols = raw_pk

    # headers 字段若存在则用于限定导出列（去掉 pk 后的部分）
    raw_headers = spec.get('headers', None)
    if raw_headers and isinstance(raw_headers, list):
        pk_set = set(pk_cols) if pk_cols else set()
        fields: list[str] | None = [h for h in raw_headers if h not in pk_set]
    else:
        fields = None

    target = output_path or spec_path
    export_table(conn, table_name, target, pk=pk_cols, fields=fields)


def discover_json_files(sql_dir: str) -> list[str]:
    files: list[str] = []
    for name in sorted(os.listdir(sql_dir)):
        full = os.path.join(sql_dir, name)
        if os.path.isfile(full) and name.lower().endswith('.json'):
            files.append(full)
    return files


def resolve_inputs(paths: defines.PathDefinitions, json_inputs: list[str], export_all: bool) -> list[str]:
    resolved: list[str] = []
    if export_all:
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

    parser = argparse.ArgumentParser(description='Step 4: 将数据库数据导出到 scripts/sql JSON文件')
    parser.add_argument('-d', '--db', default=default_db_path,
                        help=f'数据库路径 (默认: {default_db_path})')
    parser.add_argument('--export-json', action='append', default=[], metavar='FILE',
                        help='按指定JSON规范文件导出（可重复）；支持绝对路径/相对路径/仅文件名')
    parser.add_argument('-a', '--all', action='store_true', dest='export_all',
                        help='导出 scripts/sql 下全部JSON规范文件（无 --export-json 时默认开启）')
    parser.add_argument('-o', '--output', default=None,
                        help='仅在单文件导出时生效，覆盖输出路径（默认原地覆写规范文件）')
    parser.add_argument('--continue-on-error', action='store_true',
                        help='某个文件失败时继续处理后续文件')

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    use_all = args.export_all or (len(args.export_json) == 0)
    json_specs = resolve_inputs(paths, args.export_json, use_all)

    if not json_specs:
        raise ValueError("没有可导出的JSON规范文件；请使用 --export-json 或 -a")
    if args.output and len(json_specs) != 1:
        raise ValueError("--output 仅支持单个 --export-json 场景")

    conn = sqlite3.connect(args.db)
    try:
        for spec in json_specs:
            try:
                export_by_spec(conn, spec, args.output if args.output else None)
            except Exception as e:
                if not args.continue_on_error:
                    raise
                logging.error(f"导出失败: {spec}: {e}")
    finally:
        conn.close()

    logging.info("✅ JSON导出完成")


if __name__ == '__main__':
    main()
