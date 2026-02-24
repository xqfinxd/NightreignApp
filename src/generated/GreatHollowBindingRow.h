#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct GreatHollowBindingRow {
    int gridXNo;
    int gridZNo;
    float posX;
    float posZ;
    float height;
    std::string icon;
    float iconScale;
    std::string label;
    int visible;
    int binding;

    GreatHollowBindingRow() {
        gridXNo = 0;
        gridZNo = 0;
        posX = 0.0f;
        posZ = 0.0f;
        height = 0.0f;
        icon = "";
        iconScale = 0.0f;
        label = "";
        visible = 0;
        binding = 0;
    }

    GreatHollowBindingRow(
        int gridXNo_,
        int gridZNo_,
        float posX_,
        float posZ_,
        float height_,
        std::string icon_,
        float iconScale_,
        std::string label_,
        int visible_,
        int binding_
    ) {
        gridXNo = gridXNo_;
        gridZNo = gridZNo_;
        posX = posX_;
        posZ = posZ_;
        height = height_;
        icon = icon_;
        iconScale = iconScale_;
        label = label_;
        visible = visible_;
        binding = binding_;
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
        
        if (tokens.size() != 10) {
            std::cerr << "Error: Expected 10 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 gridXNo
            if (!tokens[0].empty()) {
                gridXNo = std::stoi(tokens[0]);
            } else {
                gridXNo = 0;
            }
            // 解析 gridZNo
            if (!tokens[1].empty()) {
                gridZNo = std::stoi(tokens[1]);
            } else {
                gridZNo = 0;
            }
            // 解析 posX
            if (!tokens[2].empty()) {
                posX = std::stof(tokens[2]);
            } else {
                posX = 0.0f;
            }
            // 解析 posZ
            if (!tokens[3].empty()) {
                posZ = std::stof(tokens[3]);
            } else {
                posZ = 0.0f;
            }
            // 解析 height
            if (!tokens[4].empty()) {
                height = std::stof(tokens[4]);
            } else {
                height = 0.0f;
            }
            // 解析 icon
            icon = tokens[5];
            // 解析 iconScale
            if (!tokens[6].empty()) {
                iconScale = std::stof(tokens[6]);
            } else {
                iconScale = 0.0f;
            }
            // 解析 label
            label = tokens[7];
            // 解析 visible
            if (!tokens[8].empty()) {
                visible = std::stoi(tokens[8]);
            } else {
                visible = 0;
            }
            // 解析 binding
            if (!tokens[9].empty()) {
                binding = std::stoi(tokens[9]);
            } else {
                binding = 0;
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
        ss << "gridXNo: " << gridXNo
           << ", ";
        ss << "gridZNo: " << gridZNo
           << ", ";
        ss << "posX: " << posX
           << ", ";
        ss << "posZ: " << posZ
           << ", ";
        ss << "height: " << height
           << ", ";
        ss << "icon: " << icon
           << ", ";
        ss << "iconScale: " << iconScale
           << ", ";
        ss << "label: " << label
           << ", ";
        ss << "visible: " << visible
           << ", ";
        ss << "binding: " << binding
           << " }";
        return ss.str();
    }
};
