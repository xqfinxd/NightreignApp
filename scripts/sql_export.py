import argparse
import json
import os
import sqlite3

import defines


def get_table_columns(conn: sqlite3.Connection, table_name: str) -> list[str]:
	"""获取表字段列表"""
	rows = conn.execute(f"PRAGMA table_info([{table_name}])").fetchall()
	if not rows:
		raise ValueError(f"表不存在或无字段: {table_name}")
	return [row[1] for row in rows]


def get_table_primary_key(conn: sqlite3.Connection, table_name: str) -> str:
	"""获取表的单列主键名"""
	rows = conn.execute(f"PRAGMA table_info([{table_name}])").fetchall()
	if not rows:
		raise ValueError(f"表不存在或无字段: {table_name}")

	pk_cols = sorted((row for row in rows if int(row[5]) > 0), key=lambda r: int(r[5]))
	if not pk_cols:
		raise ValueError(f"表未定义主键: {table_name}")
	if len(pk_cols) > 1:
		raise ValueError(f"表为复合主键，当前导出格式只支持单列主键: {table_name}")
	return pk_cols[0][1]


def discover_json_files(sql_dir: str) -> list[str]:
	files: list[str] = []
	for name in sorted(os.listdir(sql_dir)):
		full = os.path.join(sql_dir, name)
		if os.path.isfile(full) and name.lower().endswith('.json'):
			files.append(full)
	return files


def load_json_spec(json_path: str) -> dict:
	with open(json_path, 'r', encoding='utf-8') as f:
		data = json.load(f)
	if not isinstance(data, dict):
		raise ValueError(f"JSON格式错误（应为对象）: {json_path}")
	if 'table' not in data:
		raise ValueError(f"JSON缺少 table 字段: {json_path}")
	return data


def resolve_json_inputs(paths: defines.PathDefinitions, json_inputs: list[str], export_all: bool) -> list[str]:
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

	# 去重并保持顺序
	unique: list[str] = []
	seen = set()
	for p in resolved:
		ap = os.path.abspath(p)
		if ap not in seen:
			seen.add(ap)
			unique.append(ap)
	return unique


def fields_from_spec_or_rows(spec: dict, rows: list[dict], pk_col: str, all_columns: list[str]) -> list[str]:
	fields = spec.get('fields', None)
	if fields is not None:
		if not isinstance(fields, list):
			raise ValueError("fields 必须为数组")
		return [f for f in fields if f != pk_col]

	if rows:
		if not isinstance(rows[0], dict):
			raise ValueError("rows 元素必须为对象")
		return [k for k in rows[0].keys() if k != pk_col]

	return [c for c in all_columns if c != pk_col]


def export_table_to_json(
	conn: sqlite3.Connection,
	table_name: str,
	output_path: str,
	fields: list[str] | None = None,
	pk: str | None = None,
) -> dict:
	"""将指定表导出为 {table, pk, rows} 结构的JSON文件"""
	inferred_pk = get_table_primary_key(conn, table_name)
	pk_col = pk or inferred_pk

	all_columns = get_table_columns(conn, table_name)
	if pk_col not in all_columns:
		raise ValueError(f"主键字段不存在: {table_name}.{pk_col}")

	if fields is None or len(fields) == 0:
		selected_fields = [c for c in all_columns if c != pk_col]
	else:
		invalid = [f for f in fields if f not in all_columns]
		if invalid:
			raise ValueError(f"字段不存在: {table_name}.{', '.join(invalid)}")
		selected_fields = [f for f in fields if f != pk_col]

	selected_columns = [pk_col] + selected_fields
	quoted_columns = ", ".join([f"[{c}]" for c in selected_columns])
	query = f"SELECT {quoted_columns} FROM [{table_name}] ORDER BY [{pk_col}]"

	cursor = conn.execute(query)
	rows = [dict(zip(selected_columns, row)) for row in cursor.fetchall()]

	result = {
		"table": table_name,
		"pk": pk_col,
		"rows": rows,
	}

	os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
	with open(output_path, "w", encoding="utf-8") as f:
		json.dump(result, f, ensure_ascii=False, indent=4)

	return result


def export_by_json_spec(conn: sqlite3.Connection, spec_path: str, output_path: str | None = None) -> dict:
	spec = load_json_spec(spec_path)
	table_name = spec['table']
	pk_col = spec.get('pk') or get_table_primary_key(conn, table_name)
	all_columns = get_table_columns(conn, table_name)
	fields = fields_from_spec_or_rows(spec, spec.get('rows', []), pk_col, all_columns)
	target = output_path or spec_path
	return export_table_to_json(conn, table_name, target, fields=fields, pk=pk_col)


def main() -> None:
	parser = argparse.ArgumentParser(description="按JSON规范导出SQLite数据")
	paths = defines.PathDefinitions(__file__)
	default_db_path = paths.get_output("game_data.db")
	parser.add_argument("-d", "--db", default=default_db_path, help="SQLite数据库路径")
	parser.add_argument("--export-json", action="append", default=[],
						help="导出单个JSON规范文件（可重复传参），支持绝对路径/相对路径/仅文件名")
	parser.add_argument("-a", "--all", action="store_true",
						help="导出 scripts/sql 下全部JSON规范文件")
	parser.add_argument("-o", "--output", default=None,
						help="仅在单文件导出时生效，覆盖输出路径")
	parser.add_argument("--continue-on-error", action="store_true",
						help="某个文件失败时继续处理后续文件")

	args = parser.parse_args()
	json_specs = resolve_json_inputs(paths, args.export_json, args.all)
	if not json_specs:
		raise ValueError("没有可导出的JSON规范文件；请使用 --export-json 或 -a")
	if args.output and len(json_specs) != 1:
		raise ValueError("--output 仅支持单个 --export-json 场景")

	conn = sqlite3.connect(args.db)
	try:
		for spec in json_specs:
			try:
				target = args.output if args.output else None
				export_by_json_spec(conn, spec, target)
				print(f"已导出: {spec}" if target is None else f"已导出: {spec} -> {target}")
			except Exception:
				if not args.continue_on_error:
					raise
				print(f"导出失败: {spec}")
	finally:
		conn.close()


if __name__ == "__main__":
	main()

