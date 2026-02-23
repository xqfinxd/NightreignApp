#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct AtlasRow {
    std::string alias;
    std::string path;
    int width;
    int height;
    std::string format;

    AtlasRow() {
        alias = "";
        path = "";
        width = 0;
        height = 0;
        format = "";
    }

    AtlasRow(
        std::string alias_,
        std::string path_,
        int width_,
        int height_,
        std::string format_
    ) {
        alias = alias_;
        path = path_;
        width = width_;
        height = height_;
        format = format_;
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
        
        if (tokens.size() != 5) {
            std::cerr << "Error: Expected 5 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 alias
            alias = tokens[0];
            // 解析 path
            path = tokens[1];
            // 解析 width
            if (!tokens[2].empty()) {
                width = std::stoi(tokens[2]);
            } else {
                width = 0;
            }
            // 解析 height
            if (!tokens[3].empty()) {
                height = std::stoi(tokens[3]);
            } else {
                height = 0;
            }
            // 解析 format
            format = tokens[4];
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
        ss << "alias: " << alias
           << ", ";
        ss << "path: " << path
           << ", ";
        ss << "width: " << width
           << ", ";
        ss << "height: " << height
           << ", ";
        ss << "format: " << format
           << " }";
        return ss.str();
    }
};
