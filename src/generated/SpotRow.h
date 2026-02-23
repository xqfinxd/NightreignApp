#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct SpotRow {
    int attachId;
    int map;
    int dlc;
    float rate;
    int gridXNo;
    int gridZNo;
    float posX;
    float posZ;
    float height;

    SpotRow() {
        attachId = 0;
        map = 0;
        dlc = 0;
        rate = 0.0f;
        gridXNo = 0;
        gridZNo = 0;
        posX = 0.0f;
        posZ = 0.0f;
        height = 0.0f;
    }

    SpotRow(
        int attachId_,
        int map_,
        int dlc_,
        float rate_,
        int gridXNo_,
        int gridZNo_,
        float posX_,
        float posZ_,
        float height_
    ) {
        attachId = attachId_;
        map = map_;
        dlc = dlc_;
        rate = rate_;
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
        
        if (tokens.size() != 9) {
            std::cerr << "Error: Expected 9 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 attachId
            if (!tokens[0].empty()) {
                attachId = std::stoi(tokens[0]);
            } else {
                attachId = 0;
            }
            // 解析 map
            if (!tokens[1].empty()) {
                map = std::stoi(tokens[1]);
            } else {
                map = 0;
            }
            // 解析 dlc
            if (!tokens[2].empty()) {
                dlc = std::stoi(tokens[2]);
            } else {
                dlc = 0;
            }
            // 解析 rate
            if (!tokens[3].empty()) {
                rate = std::stof(tokens[3]);
            } else {
                rate = 0.0f;
            }
            // 解析 gridXNo
            if (!tokens[4].empty()) {
                gridXNo = std::stoi(tokens[4]);
            } else {
                gridXNo = 0;
            }
            // 解析 gridZNo
            if (!tokens[5].empty()) {
                gridZNo = std::stoi(tokens[5]);
            } else {
                gridZNo = 0;
            }
            // 解析 posX
            if (!tokens[6].empty()) {
                posX = std::stof(tokens[6]);
            } else {
                posX = 0.0f;
            }
            // 解析 posZ
            if (!tokens[7].empty()) {
                posZ = std::stof(tokens[7]);
            } else {
                posZ = 0.0f;
            }
            // 解析 height
            if (!tokens[8].empty()) {
                height = std::stof(tokens[8]);
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
        ss << "attachId: " << attachId
           << ", ";
        ss << "map: " << map
           << ", ";
        ss << "dlc: " << dlc
           << ", ";
        ss << "rate: " << rate
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
