#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct ManualGridRow {
    int x;
    int y;
    float height;
    int map;

    ManualGridRow() {
        x = 0;
        y = 0;
        height = 0.0f;
        map = 0;
    }

    ManualGridRow(
        int x_,
        int y_,
        float height_,
        int map_
    ) {
        x = x_;
        y = y_;
        height = height_;
        map = map_;
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
        
        if (tokens.size() != 4) {
            std::cerr << "Error: Expected 4 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 x
            if (!tokens[0].empty()) {
                x = std::stoi(tokens[0]);
            } else {
                x = 0;
            }
            // 解析 y
            if (!tokens[1].empty()) {
                y = std::stoi(tokens[1]);
            } else {
                y = 0;
            }
            // 解析 height
            if (!tokens[2].empty()) {
                height = std::stof(tokens[2]);
            } else {
                height = 0.0f;
            }
            // 解析 map
            if (!tokens[3].empty()) {
                map = std::stoi(tokens[3]);
            } else {
                map = 0;
            }
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
        ss << "x: " << x
           << ", ";
        ss << "y: " << y
           << ", ";
        ss << "height: " << height
           << ", ";
        ss << "map: " << map
           << " }";
        return ss.str();
    }
};
