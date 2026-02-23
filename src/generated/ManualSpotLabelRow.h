#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct ManualSpotLabelRow {
    int id;
    int direction;
    float offsetx;
    float offsety;
    int showicon;

    ManualSpotLabelRow() {
        id = 0;
        direction = 0;
        offsetx = 0.0f;
        offsety = 0.0f;
        showicon = 0;
    }

    ManualSpotLabelRow(
        int id_,
        int direction_,
        float offsetx_,
        float offsety_,
        int showicon_
    ) {
        id = id_;
        direction = direction_;
        offsetx = offsetx_;
        offsety = offsety_;
        showicon = showicon_;
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
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 direction
            if (!tokens[1].empty()) {
                direction = std::stoi(tokens[1]);
            } else {
                direction = 0;
            }
            // 解析 offsetx
            if (!tokens[2].empty()) {
                offsetx = std::stof(tokens[2]);
            } else {
                offsetx = 0.0f;
            }
            // 解析 offsety
            if (!tokens[3].empty()) {
                offsety = std::stof(tokens[3]);
            } else {
                offsety = 0.0f;
            }
            // 解析 showicon
            if (!tokens[4].empty()) {
                showicon = std::stoi(tokens[4]);
            } else {
                showicon = 0;
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
        ss << "direction: " << direction
           << ", ";
        ss << "offsetx: " << offsetx
           << ", ";
        ss << "offsety: " << offsety
           << ", ";
        ss << "showicon: " << showicon
           << " }";
        return ss.str();
    }
};
