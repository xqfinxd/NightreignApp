#pragma once
#include "public.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <glm/glm.hpp>

// Nightlord information
struct NightlordInfo {
    int id;
    std::string name;
};

// Map information
struct MapInfo {
    int id = -1;
    std::string name;

    std::vector<int> legacyPatterns;
    std::vector<int> dlcPatterns;
    std::vector<int> legacyFilterPoints;
    std::vector<int> dlcFilterPoints;
    std::vector<int> starters;
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

    int starter = -1;
};

// Spot information
struct SpotInfo {
    int attachId = 0;
	MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
	}
};

// Constant Spot information
struct ConstantInfo {
    int type = 0;
    int map = 0;
    float iconScale = 1.0f;
    std::string label;
    std::string icon;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

// Rotted Power information
struct RottedPowerInfo
{
    int patternId = -1;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

struct GreatHollowBindingInfo {
    float iconScale = 1.0f;
    int visible = 255;
    int binding = -1;
    std::string icon;
    std::string label;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

// Starter information
struct StarterInfo {
    int starterId;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

struct SpotOption {
    int attachId = -1;
    bool disable_filter = false;
    bool disable_view = false;
};

struct SpotLabelOption {
    int attachId = -1;
    int direction = 0;
    glm::vec2 offset{0.0f, 0.0f};
    bool showIcon = false;
};

struct GridOption {
    int x = 0;
    int y = 0;
    float height = 0.0f;
    int map = -1;
};

// Variation info
struct VariationInfo {
    int variationId = 0;       // smallBaseMapId
    int variationType = 0;      // variationType
    int getKey() const { return variationId * 10 + variationType; }

    std::string label;
    std::string sublabel;

    std::string getText() const {
        if (!sublabel.empty())
            return sublabel;
        return label;
    }

    std::string icon;           // Icon texture name
    int visible = 0xFF;            // Is visible on map
    float iconScale = 1.0f;     // Icon scale multiplier

    bool isShowIcon() const { return visible & (1); }
    bool isShowText() const { return visible & (1 << 1); }
    void showAll() { visible |= 0xFF; }
};

struct VariationDist {
    int variationKey = -1;
    int attachId = -1;
    int patternId = -1;
};

// Game data manager - loads and manages all CSV data
class GameData {
public:
    GameData();
    ~GameData();
    // Singleton access (optional)
    static GameData* getInstance(){ return s_instance; }
    static GameData& getRef() { return *getInstance(); }
    
    bool loadFromCSV(const std::string& dataPath);
    
    // Map data for IMGUI
    std::vector<const char*> getMapNames() const;

    // queries
    const PatternInfo* getPattern(int patternId) const;
    const SpotInfo* getSpot(int spotId) const;
    const StarterInfo* getStarter(int starterId) const;
    const VariationInfo* getVariation(int varKey) const;
    const VariationInfo* getVariation(int patternId, int attachId) const;
    const SpotOption* getSpotOption(int attachId) const;
    const SpotLabelOption* getAttachOption(int attachId) const;
    const GridOption* getGridOption(int map, int x, int y) const;
    const RottedPowerInfo* getRottedPower(int patternId) const;

    std::vector<int> getSpotsByMap(int map, bool legacy = true, bool dlc = true) const;
    std::vector<int> getStarterByMap(int map) const;
    
    // Variation queries
    std::vector<const VariationInfo*> listVariations(int attachId, const std::set<int>& patterns) const;
    std::vector<const VariationInfo*> listVariations(int attachId, int map) const;
    std::vector<const VariationDist*> listDistribution(int patternId) const;
    std::vector<const ConstantInfo*> listConstants(int map) const;
    std::vector<const GreatHollowBindingInfo*> listGreatHollowBinding(int patternId) const;
    
    // Pattern filtering
    std::set<int> filterByMap(int map) const;
    std::set<int> filterByVariation(const std::set<int>& patterns, int attachId, int varKey) const;
    std::set<int> filterByStarter(const std::set<int>& patterns, int starterId) const;
    std::set<int> filterByNightlord(const std::set<int>& patterns, int nightlordId) const;

private:
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;
    
    bool loadNightlords(const std::string& filePath);
    bool loadMaps(const std::string& filePath);
    bool loadPatterns(const std::string& filePath);

    bool loadSpots(const std::string& filePath);
    bool loadStarters(const std::string& filePath);
    bool loadStarterDist(const std::string& filePath);

    bool loadVariations(const std::string& filePath);
    bool loadVariationsEx(const std::string& filePath);
    
    bool loadSpotsEx(const std::string& filePath);
    bool loadSpotsLabelEx(const std::string& filePath);
    bool loadGridEx(const std::string& filePath);

    bool loadConstant(const std::string& filePath);
    bool loadRottedPowers(const std::string& filePath);
    bool loadGreatHollowBindings(const std::string& filePath);

    std::map<int, NightlordInfo> m_nightlordDB;
	std::map<int, MapInfo> m_mapDB;
	std::map<int, PatternInfo> m_patternDB;
	std::map<int, SpotInfo> m_spotDB;
	std::map<int, StarterInfo> m_starterDB;
	std::map<int, VariationInfo> m_variationDB;
    std::vector<VariationDist> m_variationDist;

    std::map<int, SpotOption> m_spotOptions;
    std::map<int, SpotLabelOption> m_spotLabelOptions;
    std::vector<GridOption> m_gridOptions;

    std::vector<ConstantInfo> m_constantDB;
    std::map<int, RottedPowerInfo> m_rottedPowers;
    std::vector<GreatHollowBindingInfo> m_greatHollowBindings;
    
    static const std::vector<int> s_emptyIntVector;
    static const std::string s_emptyString;
    static GameData* s_instance;
};
