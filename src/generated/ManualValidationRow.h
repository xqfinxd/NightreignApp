#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct ManualValidationRow {
    int id;
    int smallBaseMapId;
    int variationId;
    std::string label;
    std::string sublabel;
    std::string icon;
    int visible;
    float iconScale;

    ManualValidationRow() {
        id = 0;
        smallBaseMapId = 0;
        variationId = 0;
        label = "";
        sublabel = "";
        icon = "";
        visible = 0;
        iconScale = 0.0f;
    }

    ManualValidationRow(
        int id_,
        int smallBaseMapId_,
        int variationId_,
        std::string label_,
        std::string sublabel_,
        std::string icon_,
        int visible_,
        float iconScale_
    ) {
        id = id_;
        smallBaseMapId = smallBaseMapId_;
        variationId = variationId_;
        label = label_;
        sublabel = sublabel_;
        icon = icon_;
        visible = visible_;
        iconScale = iconScale_;
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
        
        if (tokens.size() != 8) {
            std::cerr << "Error: Expected 8 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 smallBaseMapId
            if (!tokens[1].empty()) {
                smallBaseMapId = std::stoi(tokens[1]);
            } else {
                smallBaseMapId = 0;
            }
            // 解析 variationId
            if (!tokens[2].empty()) {
                variationId = std::stoi(tokens[2]);
            } else {
                variationId = 0;
            }
            // 解析 label
            label = tokens[3];
            // 解析 sublabel
            sublabel = tokens[4];
            // 解析 icon
            icon = tokens[5];
            // 解析 visible
            if (!tokens[6].empty()) {
                visible = std::stoi(tokens[6]);
            } else {
                visible = 0;
            }
            // 解析 iconScale
            if (!tokens[7].empty()) {
                iconScale = std::stof(tokens[7]);
            } else {
                iconScale = 0.0f;
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
        ss << "smallBaseMapId: " << smallBaseMapId
           << ", ";
        ss << "variationId: " << variationId
           << ", ";
        ss << "label: " << label
           << ", ";
        ss << "sublabel: " << sublabel
           << ", ";
        ss << "icon: " << icon
           << ", ";
        ss << "visible: " << visible
           << ", ";
        ss << "iconScale: " << iconScale
           << " }";
        return ss.str();
    }
};
