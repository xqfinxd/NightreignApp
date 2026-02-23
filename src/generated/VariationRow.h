#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct VariationRow {
    int patternId;
    int variationId;
    int variationType;
    int attachId;

    VariationRow() {
        patternId = 0;
        variationId = 0;
        variationType = 0;
        attachId = 0;
    }

    VariationRow(
        int patternId_,
        int variationId_,
        int variationType_,
        int attachId_
    ) {
        patternId = patternId_;
        variationId = variationId_;
        variationType = variationType_;
        attachId = attachId_;
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
            // 解析 patternId
            if (!tokens[0].empty()) {
                patternId = std::stoi(tokens[0]);
            } else {
                patternId = 0;
            }
            // 解析 variationId
            if (!tokens[1].empty()) {
                variationId = std::stoi(tokens[1]);
            } else {
                variationId = 0;
            }
            // 解析 variationType
            if (!tokens[2].empty()) {
                variationType = std::stoi(tokens[2]);
            } else {
                variationType = 0;
            }
            // 解析 attachId
            if (!tokens[3].empty()) {
                attachId = std::stoi(tokens[3]);
            } else {
                attachId = 0;
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
        ss << "patternId: " << patternId
           << ", ";
        ss << "variationId: " << variationId
           << ", ";
        ss << "variationType: " << variationType
           << ", ";
        ss << "attachId: " << attachId
           << " }";
        return ss.str();
    }
};
