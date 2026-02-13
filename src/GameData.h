#pragma once
#include "public.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <glm/glm.hpp>

struct MapPoint
{
	int gridXNo = 0, gridZNo = 0;
	float posX = 0.0f, posZ = 0.0f;
};

// Map information
struct MapInfo {
    int id = -1;
    std::string name;

    std::vector<int> patterns;
    std::vector<int> staticSpots;
    std::vector<int> starterSpots;
};

// Pattern information
struct PatternInfo {
    int id = -1;
    int map = -1;
    int boss = 0;
    int bossId1 = 0;
    int bossId2 = 0;
    int extraBossId1 = 0;
    int extraBossId2 = 0;
    bool isdlc = false;
    
	MapPoint playArea1;
	MapPoint playArea2;

    std::vector<int> spots;
    int starter = -1;
};

// Spot
struct BaseSpot {
    int id;
	MapPoint point;
    glm::vec2 normalize(int tileSize = 256) const {
        float gridX = point.posX / tileSize + point.gridXNo - 41;
        float gridZ = point.posZ / tileSize + point.gridZNo - 35;
        return glm::vec2(gridX, gridZ);
	}
};

struct FilterSpot : public BaseSpot {
    std::vector<int> attachIds;
	int variationKey = -1;
	int attachIndex = -1;
	int attachId() const {
        if (attachIndex >= 0 && attachIndex < static_cast<int>(attachIds.size())) {
            return attachIds[attachIndex];
        }
        return -1;
	}
};

// Variation info
struct VariationInfo {
    int variationId;     // smallBaseMapId
    int variationType;   // variationType
    int getKey() const { return variationId * 10 + variationType; }
    std::string label;
    std::string sublabel;
    std::string icon;           // Icon texture name
    bool visible = true;        // Is visible on map
    float iconScale = 1.0f;     // Icon scale multiplier
};

struct VariationDistribution {
    int variationKey;
    int attachId;
    int patternId;
};

// Game data manager - loads and manages all CSV data
class GameData {
public:
    static GameData& getInstance();
    
    bool loadFromCSV(const std::string& dataPath);
    
    // Map queries
    std::vector<const char*> getMapNames() const;
    int getMapCount() const;

    // Pattern queries
    const PatternInfo* getPattern(int patternId) const;
    const std::vector<int>& getPatternsByMap(int map) const;
    
    // Spot queries
    const BaseSpot* getSpot(int spotId) const;
    const std::vector<int>& getStaticSpotsByMap(int map) const;
    
    // Variation queries
    std::vector<const VariationInfo*> getVariationsAtSpot(int spotId, int map) const;
    std::vector<const VariationInfo*> getVariationsAtSpotInPatterns(int spotId, const std::set<int>& validPatterns) const;
    const std::string& getVariationLabel(int key) const;
	std::vector<const VariationInfo*> getVariationsForPattern(int patternId) const;
    
    // Pattern filtering
    std::vector<int> filterPatternsBySpotVariations(
        int mapIndex,
        const std::map<int, int>& spotVariations  // spotId -> variationKey
    ) const;
    
private:
    GameData() = default;
    ~GameData() = default;
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;
    
    bool loadMaps(const std::string& filePath);
    bool loadPatterns(const std::string& filePath);
    bool loadStaticSpots(const std::string& filePath);
    bool loadSpotDistribution(const std::string& filePath);
    bool loadVariations(const std::string& filePath);
    bool loadVariationLabels(const std::string& filePath);
	void applyVariationLabels();

	std::map<int, MapInfo> m_mapDB;
	std::map<int, PatternInfo> m_patternDB;
	std::map<int, BaseSpot> m_baseSpotDB;
	std::map<int, FilterSpot> m_filterSpotDB;
	std::map<int, VariationInfo> m_variationDB;
    
    std::map<int, PatternInfo> m_patterns;
    std::map<int, MapInfo> m_maps;
    
    std::map<int, BaseSpot> m_spots;
    std::map<int, std::vector<int>> m_staticSpotsByMap;
    
    std::vector<VariationInfo> m_variations;
    std::map<int, std::string> m_variationLabels;  // key -> label
    
    // Index for fast lookups
    std::multimap<int, const VariationInfo*> m_variationsBySpot;
    std::multimap<int, const VariationInfo*> m_variationsByPattern;
    
    static const std::vector<int> s_emptyIntVector;
    static const std::string s_emptyString;
};
