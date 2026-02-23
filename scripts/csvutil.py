#!/usr/bin/env python3
"""
CSV to C++ Struct Generator
提供CSV文件生成和对应的C++结构体代码生成功能
"""

import csv
import os
from datetime import datetime
from typing import Dict, List, Any, Union, Optional, Iterator
from collections import namedtuple

class CSVHeaderField:
    """CSV头部字段定义"""
    def __init__(self, name: str, type_str: str):
        self.name = name
        self.type_str = type_str
        
        # C++类型到Python类型的映射
        self.type_map = {
            'int': int,
            'int32_t': int,
            'int64_t': int,
            'uint32_t': int,
            'uint64_t': int,
            'float': float,
            'double': float,
            'bool': bool,
            'std::string': str,
            'string': str,
            'char*': str
        }
        
        # C++类型到默认值的映射
        self.default_map = {
            'int': '0',
            'int32_t': '0',
            'int64_t': '0',
            'uint32_t': '0U',
            'uint64_t': '0ULL',
            'float': '0.0f',
            'double': '0.0',
            'bool': 'false',
            'std::string': '""',
            'string': '""',
            'char*': 'nullptr'
        }
    
    def get_python_type(self):
        """获取对应的Python类型"""
        return self.type_map.get(self.type_str, str)
    
    def get_default_value(self):
        """获取C++默认值"""
        return self.default_map.get(self.type_str, '{}()'.format(self.type_str))
    
    def is_string_type(self):
        """是否是字符串类型"""
        return self.type_str in ['std::string', 'string', 'char*']

def generate_csv(header: Dict[str, str], row_data: List[Dict[str, Any]], output_path: str):
    """
    根据header生成CSV文件
    
    Args:
        header: 键值对，key是字段名，value是C++类型
        row_data: 行数据列表，每行是一个字典，key是字段名，value是值
        output_path: 输出文件路径
    """
    # 转换header为字段列表
    fields = [CSVHeaderField(name, type_str) for name, type_str in header.items()]
    field_names = [field.name for field in fields]
    
    # 确保输出目录存在
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    
    # 写入CSV文件
    with open(output_path, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=field_names)
        
        # 写入标题行（带类型注释）
        header_row = {name: f"{name}:{header[name]}" for name in field_names}
        writer.writerow(header_row)
        
        # 写入数据行
        for row in row_data:
            # 类型检查和转换
            processed_row = {}
            for field in fields:
                value = row.get(field.name, '')
                python_type = field.get_python_type()
                
                try:
                    if value == '' and field.is_string_type():
                        processed_row[field.name] = ''
                    elif value == '':
                        processed_row[field.name] = '0'
                    else:
                        if python_type == bool:
                            # 处理布尔值
                            if isinstance(value, str):
                                processed_row[field.name] = value.lower() in ('true', '1', 'yes', 'on')
                            else:
                                processed_row[field.name] = bool(value)
                        else:
                            # 尝试转换类型
                            processed_row[field.name] = python_type(value)
                except (ValueError, TypeError) as e:
                    print(f"警告: 字段 '{field.name}' 的值 '{value}' 无法转换为 {field.type_str}，使用默认值")
                    processed_row[field.name] = python_type() if python_type != bool else False
            
            writer.writerow(processed_row)
    
    print(f"CSV文件已生成: {output_path}")

def generate_cpp_header(header: Dict[str, str], output_path: str, struct_name: str = "CSVRow"):
    """
    根据header生成对应的C++结构体和解析函数
    
    Args:
        header: 键值对，key是字段名，value是C++类型
        output_path: 输出头文件路径
        struct_name: 结构体名称
    """
    fields = [CSVHeaderField(name, type_str) for name, type_str in header.items()]
    
    # 确保输出目录存在
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    
    # 生成头文件内容
    content = _generate_header_content(fields, struct_name, output_path)
    
    # 写入文件
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"C++头文件已生成: {output_path}")

def _generate_header_content(fields: List[CSVHeaderField], struct_name: str, output_path: str) -> str:
    """生成头文件内容"""
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <string>")
    lines.append("#include <vector>")
    lines.append("#include <sstream>")
    lines.append("#include <iostream>")
    lines.append("#include <fstream>")
    lines.append("#include <algorithm>")
    lines.append("")
    
    # 结构体定义
    lines.append(f"struct {struct_name} {{")
    
    # 成员变量
    for field in fields:
        lines.append(f"    {field.type_str} {field.name};")
    
    lines.append("")
    lines.append(f"    {struct_name}() {{")
    for field in fields:
        lines.append(f"        {field.name} = {field.get_default_value()};")
    lines.append("    }")
    lines.append("")
    
    # 带参数的构造函数
    lines.append(f"    {struct_name}(")
    for i, field in enumerate(fields):
        comma = "," if i < len(fields) - 1 else ""
        lines.append(f"        {field.type_str} {field.name}_{comma}")
    lines.append("    ) {")
    for field in fields:
        lines.append(f"        {field.name} = {field.name}_;")
    lines.append("    }")
    lines.append("")
    
    # Parse函数
    lines.append("    /**")
    lines.append("     * 从CSV一行字符串解析数据")
    lines.append("     * @param line CSV一行数据")
    lines.append("     * @param delimiter 分隔符，默认为逗号")
    lines.append("     * @return 是否解析成功")
    lines.append("     */")
    lines.append(f"    bool parseFromCSV(const std::string& line, char delimiter = ',') {{")
    lines.append("        std::vector<std::string> tokens;")
    lines.append("        std::stringstream ss(line);")
    lines.append("        std::string token;")
    lines.append("        ")
    lines.append("        // 分割字符串")
    lines.append("        while (std::getline(ss, token, delimiter)) {")
    lines.append("            // 去除首尾空白")
    lines.append("            token.erase(0, token.find_first_not_of(\" \\t\\r\\n\"));")
    lines.append("            token.erase(token.find_last_not_of(\" \\t\\r\\n\") + 1);")
    lines.append("            tokens.push_back(token);")
    lines.append("        }")
    lines.append("        ")
    lines.append(f"        if (tokens.size() != {len(fields)}) {{")
    lines.append(f"            std::cerr << \"Error: Expected {len(fields)} fields, got \" << tokens.size() << std::endl;")
    lines.append("            return false;")
    lines.append("        }")
    lines.append("        ")
    lines.append("        try {")
    
    # 解析每个字段
    for i, field in enumerate(fields):
        parse_method = _get_parse_method(field.type_str)
        lines.append(f"            // 解析 {field.name}")
        if field.type_str in ['std::string', 'string']:
            lines.append(f"            {field.name} = tokens[{i}];")
        elif field.type_str in ['char*']:
            lines.append(f"            {field.name} = tokens[{i}].empty() ? nullptr : tokens[{i}].c_str();")
        else:
            lines.append(f"            if (!tokens[{i}].empty()) {{")
            lines.append(f"                {field.name} = std::{parse_method}(tokens[{i}]);")
            lines.append(f"            }} else {{")
            lines.append(f"                {field.name} = {field.get_default_value()};")
            lines.append(f"            }}")
    
    lines.append("        } catch (const std::exception& e) {")
    lines.append("            std::cerr << \"Parse error: \" << e.what() << std::endl;")
    lines.append("            return false;")
    lines.append("        }")
    lines.append("        ")
    lines.append("        return true;")
    lines.append("    }")
    lines.append("")
    
    # toString方法
    lines.append("    /**")
    lines.append("     * 转换为字符串表示")
    lines.append("     */")
    lines.append("    std::string toString() const {")
    lines.append("        std::stringstream ss;")
    lines.append("        ss << \"{ \";")
    for i, field in enumerate(fields):
        lines.append(f"        ss << \"{field.name}: \" << {field.name}")
        if i < len(fields) - 1:
            lines.append("           << \", \";")
        else:
            lines.append("           << \" }\";")
    lines.append("        return ss.str();")
    lines.append("    }")
    lines.append("};")
    lines.append("")
        
    return "\n".join(lines)

def _get_parse_method(cpp_type: str) -> str:
    """获取对应的解析方法名"""
    parse_map = {
        'int': 'stoi',
        'int32_t': 'stoi',
        'int64_t': 'stoll',
        'uint32_t': 'stoul',
        'uint64_t': 'stoull',
        'float': 'stof',
        'double': 'stod',
        'bool': 'stoi'  # 布尔值需要特殊处理
    }
    return parse_map.get(cpp_type, 'stoi')

class CSVData:
    """
    CSV数据容器类，支持通过键值或索引访问行数据
    示例: 
        - 通过键值: data[key].column_name 或 data[key]['column_name']
        - 通过索引: data[index].column_name (当没有提供键值或键不存在时)
    """
    
    def __init__(self, rows: List[Dict[str, Any]], key_column: Optional[str] = None):
        """
        初始化CSV数据
        
        Args:
            rows: 行数据列表，每行为字典
            key_column: 用作键的列名，如果为None则使用索引
        """
        self.key_column = key_column
        self._index_data: List['CSVRow'] = []  # 索引访问的数据
        self._key_data: Dict[Any, 'CSVRow'] = {}  # 键值访问的数据
        self._columns = list(rows[0].keys()) if rows else []
        
        # 构建数据
        for i, row in enumerate(rows):
            csv_row = CSVRow(row, self._columns, index=i)
            self._index_data.append(csv_row)
            
            # 如果指定了键列，同时构建键值索引
            if key_column is not None:
                key_value = row.get(key_column)
                if key_value is None:
                    raise ValueError(f"Key column '{key_column}' not found in row: {row}")
                
                if key_value in self._key_data:
                    raise ValueError(f"Duplicate key value '{key_value}' in column '{key_column}'")
                
                self._key_data[key_value] = csv_row
        
        # 确定主访问模式
        self._use_key_mode = key_column is not None
    
    def __getitem__(self, key: Union[Any, int, str]) -> 'CSVRow':
        """
        通过键或索引访问行数据
        
        行为:
        - 如果指定了键列: 
            - 先尝试作为键值查找
            - 如果键值不存在且key是整数，则尝试索引访问
            - 如果键值不存在且key不是整数，则抛出KeyError
        - 如果没有指定键列: 始终使用索引访问
        """
        if self._use_key_mode:
            # 有键列的模式
            if key in self._key_data:
                return self._key_data[key]
            
            # 如果key是整数且没有找到键值，尝试索引访问
            if isinstance(key, (int, str)) and key.isdigit() if isinstance(key, str) else isinstance(key, int):
                try:
                    idx = int(key)
                    if 0 <= idx < len(self._index_data):
                        print(f"Warning: Key '{key}' not found, using index {idx} instead")
                        return self._index_data[idx]
                except (ValueError, IndexError):
                    pass
            
            # 键不存在且不是有效的索引
            available_keys = list(self._key_data.keys())[:5]  # 只显示前5个
            raise KeyError(f"Key '{key}' not found. Available keys: {available_keys}...")
        else:
            # 无键列的模式，使用索引
            if isinstance(key, (int, str)) and str(key).isdigit():
                idx = int(key)
                if 0 <= idx < len(self._index_data):
                    return self._index_data[idx]
                raise IndexError(f"Index {key} out of range. Valid indices: 0-{len(self._index_data)-1}")
            else:
                # 尝试将key作为索引处理
                try:
                    idx = int(key)
                    if 0 <= idx < len(self._index_data):
                        return self._index_data[idx]
                except (ValueError, TypeError):
                    raise TypeError(f"Expected integer index, got '{key}'")
    
    def __contains__(self, key) -> bool:
        """检查键是否存在"""
        if self._use_key_mode:
            return key in self._key_data
        else:
            if isinstance(key, (int, str)) and str(key).isdigit():
                idx = int(key)
                return 0 <= idx < len(self._index_data)
            return False
    
    def get(self, key, default=None) -> Optional['CSVRow']:
        """安全地获取行数据"""
        try:
            return self[key]
        except (KeyError, IndexError, TypeError):
            return default
    
    def keys(self):
        """返回所有键（如果有键列）"""
        if self._use_key_mode:
            return self._key_data.keys()
        else:
            return range(len(self._index_data))
    
    def values(self):
        """返回所有行数据"""
        return self._index_data.copy()
    
    def items(self):
        """返回键值对（如果有键列）"""
        if self._use_key_mode:
            return self._key_data.items()
        else:
            return [(i, row) for i, row in enumerate(self._index_data)]
    
    def __len__(self):
        return len(self._index_data)
    
    def __iter__(self) -> Iterator['CSVRow']:
        """迭代所有行"""
        return iter(self._index_data)
    
    @property
    def columns(self):
        """返回所有列名"""
        return self._columns.copy()
    
    def to_dict(self) -> Dict[Any, Dict[str, Any]]:
        """转换为普通字典"""
        if self._use_key_mode:
            return {key: row.to_dict() for key, row in self._key_data.items()}
        else:
            return {i: row.to_dict() for i, row in enumerate(self._index_data)}
    
    def to_list(self) -> List[Dict[str, Any]]:
        """转换为列表"""
        return [row.to_dict() for row in self._index_data]
    
    def filter(self, **conditions) -> List['CSVRow']:
        """
        根据条件过滤行
        
        Args:
            **conditions: 列名和值的条件
        
        Returns:
            符合条件的行列表
        """
        results = []
        for row in self._index_data:
            match = True
            for col, val in conditions.items():
                if col not in row:
                    match = False
                    break
                if row[col] != val:
                    match = False
                    break
            if match:
                results.append(row)
        return results
    
    def group_by(self, column: str) -> Dict[Any, List['CSVRow']]:
        """
        按指定列分组
        
        Args:
            column: 列名
        
        Returns:
            分组后的字典
        """
        groups = {}
        for row in self._index_data:
            key = row.get(column)
            if key is None:
                raise ValueError(f"Column '{column}' not found")
            
            if key not in groups:
                groups[key] = []
            groups[key].append(row)
        
        return groups
    
    def first(self) -> Optional['CSVRow']:
        """返回第一行"""
        return self._index_data[0] if self._index_data else None
    
    def last(self) -> Optional['CSVRow']:
        """返回最后一行"""
        return self._index_data[-1] if self._index_data else None
    
    def __repr__(self):
        mode = f"key='{self.key_column}'" if self.key_column else "index"
        return f"CSVData(rows={len(self._index_data)}, columns={self._columns}, mode={mode})"

class CSVRow:
    """
    CSV行数据类，支持通过属性访问
    """
    
    def __init__(self, data: Dict[str, Any], columns: List[str], index: int = -1):
        """
        初始化行数据
        
        Args:
            data: 行数据字典
            columns: 列名列表
            index: 行索引
        """
        self._data = data.copy()  # 创建副本以避免修改原始数据
        self._columns = columns
        self._index = index
        
        # 将所有数据转换为整数类型（如果可能）
        for key, value in self._data.items():
            try:
                # 尝试转换为整数
                if isinstance(value, str) and value.strip().isdigit():
                    self._data[key] = int(value)
                elif isinstance(value, (int, float)):
                    self._data[key] = int(value) if value == int(value) else value
            except (ValueError, TypeError):
                pass  # 保持原值
            
            # 动态创建属性
            setattr(self, key, self._data[key])
        
        # 添加索引属性
        self.index = index
    
    def __getitem__(self, key) -> Any:
        """通过键访问数据，支持 row['column'] 语法"""
        if key not in self._data:
            raise KeyError(f"Column '{key}' not found. Available columns: {self._columns}")
        return self._data[key]
    
    def __setitem__(self, key, value):
        """设置数据，支持 row['column'] = value 语法"""
        if key not in self._columns:
            raise KeyError(f"Cannot add new column '{key}'. Use add_column() if you need to add new columns")
        self._data[key] = value
        setattr(self, key, value)
    
    def get(self, key, default=None):
        """安全地获取列值"""
        return self._data.get(key, default)
    
    def keys(self):
        """返回所有列名"""
        return self._data.keys()
    
    def values(self):
        """返回所有值"""
        return self._data.values()
    
    def items(self):
        """返回列名和值的对"""
        return self._data.items()
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return self._data.copy()
    
    def __contains__(self, key) -> bool:
        return key in self._data
    
    def __len__(self):
        return len(self._data)
    
    def __repr__(self):
        items = ", ".join([f"{k}={v}" for k, v in list(self._data.items())[:3]])
        if len(self._data) > 3:
            items += f", ... ({len(self._data)} columns)"
        return f"CSVRow({items})"

def load_csv(filepath: str, key_column: Optional[str] = None, 
             encoding: str = 'utf-8', delimiter: str = ',',
             auto_convert_ints: bool = True) -> CSVData:
    """
    加载CSV文件并返回CSVData对象
    
    Args:
        filepath: CSV文件路径
        key_column: 用作键的列名，如果为None则使用索引
        encoding: 文件编码
        delimiter: 分隔符
        auto_convert_ints: 是否自动将数字字符串转换为整数
    
    Returns:
        CSVData对象
        
    Example:
        # 使用键值访问
        data = load_csv('data.csv', key_column='id')
        value = data[1001].name  # 通过id访问
        
        # 使用索引访问
        data = load_csv('data.csv')  # 没有键列
        value = data[0].name  # 通过索引访问
        
        # 混合模式（当键不存在时尝试索引）
        data = load_csv('data.csv', key_column='id')
        row = data[0]  # 如果0不是有效的id，则作为索引使用
    """
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"CSV file not found: {filepath}")
    
    rows = []
    
    with open(filepath, 'r', encoding=encoding) as f:
        reader = csv.DictReader(f, delimiter=delimiter)
        
        for row in reader:
            # 处理空值
            processed_row = {}
            for key, value in row.items():
                if value == '':
                    processed_row[key] = None
                elif auto_convert_ints and value and value.strip().isdigit():
                    processed_row[key] = int(value)
                else:
                    processed_row[key] = value
            rows.append(processed_row)
    
    return CSVData(rows, key_column)

 # 3. 定义find_area函数
def find_point(db:CSVData, area_id: int) -> dict:
    """根据areaId查找区域信息"""
    if area_id is None:
        return {}
    
    if area_id not in db:
        return {}
    
    row = db.get(area_id, {})
    return {
        'gridXNo': int(row.get('gridXNo', 0)),
        'gridZNo': int(row.get('gridZNo', 0)),
        'posX': float(row.get('posX', 0)),
        'posZ': float(row.get('posZ', 0)),
        'height': float(row.get('posY', 0))
    }