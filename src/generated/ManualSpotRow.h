#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct ManualSpotRow {
    int id;
    int disable_filter;
    int disable_view;

    ManualSpotRow() {
        id = 0;
        disable_filter = 0;
        disable_view = 0;
    }

    ManualSpotRow(
        int id_,
        int disable_filter_,
        int disable_view_
    ) {
        id = id_;
        disable_filter = disable_filter_;
        disable_view = disable_view_;
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
        
        if (tokens.size() != 3) {
            std::cerr << "Error: Expected 3 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 disable_filter
            if (!tokens[1].empty()) {
                disable_filter = std::stoi(tokens[1]);
            } else {
                disable_filter = 0;
            }
            // 解析 disable_view
            if (!tokens[2].empty()) {
                disable_view = std::stoi(tokens[2]);
            } else {
                disable_view = 0;
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
        ss << "id: " << id
           << ", ";
        ss << "disable_filter: " << disable_filter
           << ", ";
        ss << "disable_view: " << disable_view
           << " }";
        return ss.str();
    }
};
