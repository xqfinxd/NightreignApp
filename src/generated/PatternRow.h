#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

struct PatternRow {
    int id;
    int map;
    int boss;
    int starter;
    int isdlc;
    int playArea1_gridXNo;
    int playArea1_gridZNo;
    float playArea1_posX;
    float playArea1_posZ;
    float playArea1_height;
    int playArea2_gridXNo;
    int playArea2_gridZNo;
    float playArea2_posX;
    float playArea2_posZ;
    float playArea2_height;
    int bossId1;
    int bossId2;
    int extraBossId1;
    int extraBossId2;

    PatternRow() {
        id = 0;
        map = 0;
        boss = 0;
        starter = 0;
        isdlc = 0;
        playArea1_gridXNo = 0;
        playArea1_gridZNo = 0;
        playArea1_posX = 0.0f;
        playArea1_posZ = 0.0f;
        playArea1_height = 0.0f;
        playArea2_gridXNo = 0;
        playArea2_gridZNo = 0;
        playArea2_posX = 0.0f;
        playArea2_posZ = 0.0f;
        playArea2_height = 0.0f;
        bossId1 = 0;
        bossId2 = 0;
        extraBossId1 = 0;
        extraBossId2 = 0;
    }

    PatternRow(
        int id_,
        int map_,
        int boss_,
        int starter_,
        int isdlc_,
        int playArea1_gridXNo_,
        int playArea1_gridZNo_,
        float playArea1_posX_,
        float playArea1_posZ_,
        float playArea1_height_,
        int playArea2_gridXNo_,
        int playArea2_gridZNo_,
        float playArea2_posX_,
        float playArea2_posZ_,
        float playArea2_height_,
        int bossId1_,
        int bossId2_,
        int extraBossId1_,
        int extraBossId2_
    ) {
        id = id_;
        map = map_;
        boss = boss_;
        starter = starter_;
        isdlc = isdlc_;
        playArea1_gridXNo = playArea1_gridXNo_;
        playArea1_gridZNo = playArea1_gridZNo_;
        playArea1_posX = playArea1_posX_;
        playArea1_posZ = playArea1_posZ_;
        playArea1_height = playArea1_height_;
        playArea2_gridXNo = playArea2_gridXNo_;
        playArea2_gridZNo = playArea2_gridZNo_;
        playArea2_posX = playArea2_posX_;
        playArea2_posZ = playArea2_posZ_;
        playArea2_height = playArea2_height_;
        bossId1 = bossId1_;
        bossId2 = bossId2_;
        extraBossId1 = extraBossId1_;
        extraBossId2 = extraBossId2_;
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
        
        if (tokens.size() != 19) {
            std::cerr << "Error: Expected 19 fields, got " << tokens.size() << std::endl;
            return false;
        }
        
        try {
            // 解析 id
            if (!tokens[0].empty()) {
                id = std::stoi(tokens[0]);
            } else {
                id = 0;
            }
            // 解析 map
            if (!tokens[1].empty()) {
                map = std::stoi(tokens[1]);
            } else {
                map = 0;
            }
            // 解析 boss
            if (!tokens[2].empty()) {
                boss = std::stoi(tokens[2]);
            } else {
                boss = 0;
            }
            // 解析 starter
            if (!tokens[3].empty()) {
                starter = std::stoi(tokens[3]);
            } else {
                starter = 0;
            }
            // 解析 isdlc
            if (!tokens[4].empty()) {
                isdlc = std::stoi(tokens[4]);
            } else {
                isdlc = 0;
            }
            // 解析 playArea1_gridXNo
            if (!tokens[5].empty()) {
                playArea1_gridXNo = std::stoi(tokens[5]);
            } else {
                playArea1_gridXNo = 0;
            }
            // 解析 playArea1_gridZNo
            if (!tokens[6].empty()) {
                playArea1_gridZNo = std::stoi(tokens[6]);
            } else {
                playArea1_gridZNo = 0;
            }
            // 解析 playArea1_posX
            if (!tokens[7].empty()) {
                playArea1_posX = std::stof(tokens[7]);
            } else {
                playArea1_posX = 0.0f;
            }
            // 解析 playArea1_posZ
            if (!tokens[8].empty()) {
                playArea1_posZ = std::stof(tokens[8]);
            } else {
                playArea1_posZ = 0.0f;
            }
            // 解析 playArea1_height
            if (!tokens[9].empty()) {
                playArea1_height = std::stof(tokens[9]);
            } else {
                playArea1_height = 0.0f;
            }
            // 解析 playArea2_gridXNo
            if (!tokens[10].empty()) {
                playArea2_gridXNo = std::stoi(tokens[10]);
            } else {
                playArea2_gridXNo = 0;
            }
            // 解析 playArea2_gridZNo
            if (!tokens[11].empty()) {
                playArea2_gridZNo = std::stoi(tokens[11]);
            } else {
                playArea2_gridZNo = 0;
            }
            // 解析 playArea2_posX
            if (!tokens[12].empty()) {
                playArea2_posX = std::stof(tokens[12]);
            } else {
                playArea2_posX = 0.0f;
            }
            // 解析 playArea2_posZ
            if (!tokens[13].empty()) {
                playArea2_posZ = std::stof(tokens[13]);
            } else {
                playArea2_posZ = 0.0f;
            }
            // 解析 playArea2_height
            if (!tokens[14].empty()) {
                playArea2_height = std::stof(tokens[14]);
            } else {
                playArea2_height = 0.0f;
            }
            // 解析 bossId1
            if (!tokens[15].empty()) {
                bossId1 = std::stoi(tokens[15]);
            } else {
                bossId1 = 0;
            }
            // 解析 bossId2
            if (!tokens[16].empty()) {
                bossId2 = std::stoi(tokens[16]);
            } else {
                bossId2 = 0;
            }
            // 解析 extraBossId1
            if (!tokens[17].empty()) {
                extraBossId1 = std::stoi(tokens[17]);
            } else {
                extraBossId1 = 0;
            }
            // 解析 extraBossId2
            if (!tokens[18].empty()) {
                extraBossId2 = std::stoi(tokens[18]);
            } else {
                extraBossId2 = 0;
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
        ss << "map: " << map
           << ", ";
        ss << "boss: " << boss
           << ", ";
        ss << "starter: " << starter
           << ", ";
        ss << "isdlc: " << isdlc
           << ", ";
        ss << "playArea1_gridXNo: " << playArea1_gridXNo
           << ", ";
        ss << "playArea1_gridZNo: " << playArea1_gridZNo
           << ", ";
        ss << "playArea1_posX: " << playArea1_posX
           << ", ";
        ss << "playArea1_posZ: " << playArea1_posZ
           << ", ";
        ss << "playArea1_height: " << playArea1_height
           << ", ";
        ss << "playArea2_gridXNo: " << playArea2_gridXNo
           << ", ";
        ss << "playArea2_gridZNo: " << playArea2_gridZNo
           << ", ";
        ss << "playArea2_posX: " << playArea2_posX
           << ", ";
        ss << "playArea2_posZ: " << playArea2_posZ
           << ", ";
        ss << "playArea2_height: " << playArea2_height
           << ", ";
        ss << "bossId1: " << bossId1
           << ", ";
        ss << "bossId2: " << bossId2
           << ", ";
        ss << "extraBossId1: " << extraBossId1
           << ", ";
        ss << "extraBossId2: " << extraBossId2
           << " }";
        return ss.str();
    }
};
