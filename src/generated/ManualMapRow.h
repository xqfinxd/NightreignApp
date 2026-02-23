#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct ManualMapRow {
    int id;
    std::string name;

    ManualMapRow() {
        id = 0;
        name = "";
    }

    ManualMapRow(
        int id_,
        std::string name_
    ) {
        id = id_;
        name = name_;
    }

    /**
     * 从CSV一行字符串解析数据
     * @param line CSV一行数据
     * @param delimiter 分隔符，默认为逗号
     * @return 是否解析成功
     */
    bool parseFromCSV(const std::string& line, char delimiter = ',') {
        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        
        // 分割字符串
        while (std::getline(ss, token, delimiter)) {
            // 去除首尾空白
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            tokens.push_back(token);
        }
        
        if (tokens.size() != 2) {
            std::cerr << "Error: Expected 2 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 name
            name = tokens[1];
        } catch (const std::exception& e) {
            std::cerr << "Parse error: " << e.what() << std::endl;
            return false;
        }
        
        return true;
    }

    /**
     * 转换为字符串表示
     */
    std::string toString() const {
        std::stringstream ss;
        ss << "{ ";
        ss << "id: " << id
           << ", ";
        ss << "name: " << name
           << " }";
        return ss.str();
    }
};
