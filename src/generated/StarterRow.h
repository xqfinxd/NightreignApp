#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct StarterRow {
    int id;
    int gridXNo;
    int gridZNo;
    float posX;
    float posZ;
    float height;

    StarterRow() {
        id = 0;
        gridXNo = 0;
        gridZNo = 0;
        posX = 0.0f;
        posZ = 0.0f;
        height = 0.0f;
    }

    StarterRow(
        int id_,
        int gridXNo_,
        int gridZNo_,
        float posX_,
        float posZ_,
        float height_
    ) {
        id = id_;
        gridXNo = gridXNo_;
        gridZNo = gridZNo_;
        posX = posX_;
        posZ = posZ_;
        height = height_;
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
        
        if (tokens.size() != 6) {
            std::cerr << "Error: Expected 6 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 gridXNo
            if (!tokens[1].empty()) {
                gridXNo = std::stoi(tokens[1]);
            } else {
                gridXNo = 0;
            }
            // 解析 gridZNo
            if (!tokens[2].empty()) {
                gridZNo = std::stoi(tokens[2]);
            } else {
                gridZNo = 0;
            }
            // 解析 posX
            if (!tokens[3].empty()) {
                posX = std::stof(tokens[3]);
            } else {
                posX = 0.0f;
            }
            // 解析 posZ
            if (!tokens[4].empty()) {
                posZ = std::stof(tokens[4]);
            } else {
                posZ = 0.0f;
            }
            // 解析 height
            if (!tokens[5].empty()) {
                height = std::stof(tokens[5]);
            } else {
                height = 0.0f;
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
        ss << "gridXNo: " << gridXNo
           << ", ";
        ss << "gridZNo: " << gridZNo
           << ", ";
        ss << "posX: " << posX
           << ", ";
        ss << "posZ: " << posZ
           << ", ";
        ss << "height: " << height
           << " }";
        return ss.str();
    }
};
