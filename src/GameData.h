#pragma once
#include "public.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <glm/glm.hpp>

// Map information
struct MapInfo {
    int id;
    std::string name;
    bool isdlc;
    std::vector<int> patterns;
};

// Pattern information
struct PatternInfo {
    int id;
    int map;
    int boss;
    int bossId1;
    int bossId2;
    int extraBossId1;
    int extraBossId2;
    bool isdlc;
    
    // Play area 1
    int playArea1_gridXNo;
    int playArea1_gridZNo;
    float playArea1_posX;
    float playArea1_posZ;
    
    // Play area 2
    int playArea2_gridXNo;
    int playArea2_gridZNo;
    float playArea2_posX;
    float playArea2_posZ;
};

// Spot position
struct SpotPosition {
    int id;
    int gridXNo;
    int gridZNo;
    float posX;
    float posZ;
};

// Variation info
struct VariationInfo {
    int spotId;          // Which spot this variation is at
    int patternId;       // Which pattern this variation appears in
    int variationId;     // smallBaseMapId
    int variationType;   // variationType
    int getKey() const { return variationId * 10 + variationType; }
    std::string label;
    std::string sublabel;
    std::string icon;           // Icon texture name
    bool visible = true;        // Is visible on map
    float iconScale = 1.0f;     // Icon scale multiplier
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
    const SpotPosition* getSpot(int spotId) const;
    const std::vector<int>& getStaticSpotsByMap(int map) const;
    glm::vec2 normalizeSpotPosition(const SpotPosition& spot, int tileSize = 256) const;
    
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
    
    std::map<int, PatternInfo> m_patterns;
    std::map<int, MapInfo> m_maps;
    
    std::map<int, SpotPosition> m_spots;
    std::map<int, std::vector<int>> m_staticSpotsByMap;
    
    std::vector<VariationInfo> m_variations;
    std::map<int, std::string> m_variationLabels;  // key -> label
    
    // Index for fast lookups
    std::multimap<int, const VariationInfo*> m_variationsBySpot;
    std::multimap<int, const VariationInfo*> m_variationsByPattern;
    
    static const std::vector<int> s_emptyIntVector;
    static const std::string s_emptyString;
};
