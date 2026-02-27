import sqlite3
import os
import defines
import csv

def create_database(db_path: str, sqlfile: str):
    """创建SQLite数据库并执行建表SQL脚本"""
    # 如果数据库文件已存在，提示用户是否覆盖
    if os.path.exists(db_path):
        response = input(f"数据库文件 {db_path} 已存在。是否删除重建？(y/n): ")
        if response.lower() == 'y':
            os.remove(db_path)
            print(f"已删除现有数据库: {db_path}")
        else:
            print("使用现有数据库")
            return
    
    # 创建新的数据库连接并执行建表脚本
    conn = sqlite3.connect(db_path)
    
    cursor = conn.cursor()
    
    cursor.execute("PRAGMA foreign_keys = ON")
    cursor.execute("PRAGMA cache_size = -30000")
    
    print("开始创建数据库表...")
    with open(sqlfile, 'r', encoding='utf-8') as f:
        cursor.executescript(f.read())
    
    conn.commit()
    
    cursor.execute("PRAGMA journal_mode")
    journal_mode = cursor.fetchone()[0]
    
    cursor.execute("PRAGMA foreign_keys")
    foreign_keys = cursor.fetchone()[0]
    
    # 输出创建结果
    print("✅ 数据库创建成功！")
    print(f"📁 数据库文件: {os.path.abspath(db_path)}")
    print(f"⚙️  日志模式: {journal_mode}")
    print(f"🔗 外键约束: {'启用' if foreign_keys else '禁用'}")
    
    # 关闭连接
    conn.close()

def init_table_by_sqlfiles(db_path: str, sqlfiles: list):
    """通过SQL文件列表初始化表数据"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    for sqlfile in sqlfiles:
        with open(sqlfile, 'r', encoding='utf-8') as f:
            # 执行SQL脚本
            print(f"正在执行SQL文件: {sqlfile}")
            cursor.executescript(f.read())
    
    conn.commit()
    conn.close()

def init_table_by_csv(db_path: str, table_name: str, csvfile: str, remmap: dict = None):
    """通过CSV文件初始化表数据 remmap: table_field -> csv_header
    若不传 remmap，则 CSV header 与 table field 同名映射。
    CSV header 支持 'field:type' 格式（取冒号前部分作为列名）。
    两种方式都会校验 table field 和 csv column 的存在性。
    使用 INSERT OR IGNORE 避免重复插入。
    """
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # 1. 获取表的所有字段名
    cursor.execute(f"PRAGMA table_info({table_name})")
    table_fields = {row[1] for row in cursor.fetchall()}
    if not table_fields:
        raise ValueError(f"表 '{table_name}' 不存在或没有任何字段")

    # 2. 读取 CSV 并解析 header（支持 field:type 格式）
    with open(csvfile, 'r', encoding='utf-8') as f:
        raw_reader = csv.reader(f)
        raw_header = next(raw_reader)
        csv_columns = [col.split(':')[0].strip() for col in raw_header]
        csv_col_set = set(csv_columns)
        rows = list(raw_reader)

    # 3. 确定映射关系 table_field -> csv_column
    if remmap:
        # 校验 remmap 中的 table 字段和 csv 列是否存在
        missing_table = [f for f in remmap if f not in table_fields]
        missing_csv   = [c for c in remmap.values() if c not in csv_col_set]
        if missing_table:
            raise ValueError(f"remmap 中以下字段在表 '{table_name}' 中不存在: {missing_table}")
        if missing_csv:
            raise ValueError(f"remmap 中以下列在 CSV '{csvfile}' 中不存在: {missing_csv}")
        mapping = remmap  # table_field -> csv_column
    else:
        # 按同名匹配（取交集）
        common = table_fields & csv_col_set
        if not common:
            raise ValueError(
                f"表 '{table_name}' 与 CSV '{csvfile}' 没有同名字段可以映射\n"
                f"  表字段: {sorted(table_fields)}\n"
                f"  CSV列: {sorted(csv_col_set)}"
            )
        missing_table = csv_col_set - table_fields
        missing_csv   = table_fields - csv_col_set
        if missing_table:
            print(f"  警告: CSV 中以下列在表 '{table_name}' 中不存在，将被忽略: {sorted(missing_table)}")
        if missing_csv:
            print(f"  警告: 表 '{table_name}' 中以下字段在 CSV 中不存在，将被忽略: {sorted(missing_csv)}")
        mapping = {f: f for f in common}

    # 4. 构建有序的 (table_field, csv_col_index) 列表
    col_index = {col: i for i, col in enumerate(csv_columns)}
    fields_ordered = sorted(mapping.keys())
    indices = [col_index[mapping[f]] for f in fields_ordered]

    # 5. 构建 INSERT OR IGNORE SQL
    placeholders = ', '.join(['?'] * len(fields_ordered))
    sql = (
        f"INSERT OR IGNORE INTO {table_name} "
        f"({', '.join(fields_ordered)}) VALUES ({placeholders})"
    )

    # 6. 批量插入
    inserted = 0
    for row in rows:
        if not any(row):   # 跳过空行
            continue
        values = tuple(row[i] if i < len(row) else None for i in indices)
        cursor.execute(sql, values)
        inserted += cursor.rowcount

    conn.commit()
    conn.close()
    print(f"  {table_name}: 插入 {inserted} 行 (from {csvfile})")

    
def main():
    """主函数"""
    import argparse

    paths = defines.PathDefinitions(__file__)
    
    parser = argparse.ArgumentParser(description='创建地图模式配置数据库')
    default_db_path = paths.get_metadata('game_data.db')
    parser.add_argument('-o', '--output', default=default_db_path, 
                       help=f'输出数据库文件路径 (默认: {default_db_path})')
    
    args = parser.parse_args()
    
    # 创建数据库
    create_database(args.output, paths.get_queries('create_tables.sql'))
    # 使用SQL文件初始化表数据
    init_table_by_sqlfiles(args.output, [
        paths.get_queries('init_table_map.sql'),
        paths.get_queries('init_table_nightlord.sql'),
        paths.get_queries('init_table_starter.sql')
    ])


if __name__ == "__main__":
    main()