import sqlite3
import csv
import os
import defines

def export_all_tables(db_path, output_dir):
    """
    将SQLite数据库中的所有表导出为CSV文件
    """
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    # 连接数据库
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # 获取所有表名
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    
    print(f"找到 {len(tables)} 个表")
    
    for (table_name,) in tables:
        # 读取表数据
        cursor.execute(f"SELECT * FROM [{table_name}]")
        rows = cursor.fetchall()
        
        # 获取列名
        cursor.execute(f"PRAGMA table_info([{table_name}])")
        columns = [col[1] for col in cursor.fetchall()]
        
        # 写入CSV
        csv_file = os.path.join(output_dir, f"{table_name}.csv")
        with open(csv_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(columns)  # 写入表头
            writer.writerows(rows)     # 写入数据
        
        print(f"已导出: {table_name} ({len(rows)}行)")
    
    conn.close()
    print(f"所有文件已保存到: {output_dir}")

# 使用示例
if __name__ == "__main__":
    paths = defines.PathDefinitions(__file__)
    export_all_tables(paths.get_metadata('game_data.db'), paths.metadata_dir / "exported_tables")