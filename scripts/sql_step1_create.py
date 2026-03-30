import argparse
import logging
import os
import sqlite3

import defines


def create_database(db_path: str, sqlfile: str, force: bool = False) -> str:
    """创建SQLite数据库并执行建表SQL脚本。force=True时跳过确认提示直接删除重建。"""
    if os.path.exists(db_path):
        if force:
            os.remove(db_path)
            logging.info(f"已删除现有数据库: {db_path}")
        else:
            response = input(f"数据库文件 {db_path} 已存在。是否删除重建？(y/n): ")
            if response.strip().lower() == 'y':
                os.remove(db_path)
                logging.info(f"已删除现有数据库: {db_path}")
            else:
                logging.info("使用现有数据库，跳过建表")
                return db_path

    os.makedirs(os.path.dirname(os.path.abspath(db_path)), exist_ok=True)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("PRAGMA foreign_keys = ON")
    cursor.execute("PRAGMA cache_size = -30000")

    logging.info(f"正在执行建表脚本: {sqlfile}")
    with open(sqlfile, 'r', encoding='utf-8') as f:
        cursor.executescript(f.read())

    conn.commit()
    logging.info(f"✅ 数据库创建成功: {os.path.abspath(db_path)}")
    conn.close()
    return db_path


def main() -> None:
    paths = defines.PathDefinitions(__file__)
    default_db_path = paths.get_output('game_data2.db')
    default_sql_path = paths.get_sql('create_tables.sql')

    parser = argparse.ArgumentParser(description='Step 1: 创建SQLite数据库并建表')
    parser.add_argument('-o', '--db', default=default_db_path,
                        help=f'数据库输出路径 (默认: {default_db_path})')
    parser.add_argument('--sql', default=default_sql_path,
                        help=f'建表SQL脚本路径 (默认: {default_sql_path})')
    parser.add_argument('--force', action='store_true',
                        help='若数据库已存在，直接删除重建而不询问')

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    create_database(args.db, args.sql, force=args.force)


if __name__ == '__main__':
    main()
