#pragma once
#include "public.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <optional>
#include <glm/glm.hpp>

struct sqlite3;

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

enum SpotFlag {
    SpotFlag_None = 0,
    SpotFlag_Icon = 1 << 0,
    SpotFlag_Label = 1 << 1,
    SpotFlag_All = 0xFF
};

struct SmallBaseView {
    int smallBaseId = -1;
    int variationId = -1;
    int groupId = -1;
    std::string majorName;
    std::string minorName;
    std::string iconAlias;
    SpotFlag flag = SpotFlag_None;
};

struct AttachPointView {
    int attachId = -1;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

struct StarterView {
    int starterId = -1;
    MapPoint point;
    glm::vec2 normalize() const {
        return point.normalize();
    }
};

struct SpotConfigView {
    AttachPointView attachPoint;
    SmallBaseView smallBase;
};

struct PatternView {
    PatternView(int id) : patternId(id) {}
    int patternId = -1;
    int mapId = -1;
    bool isdlc = false;
    std::string nightlordName;
    std::string day1BossName;
    std::string day2BossName;
    std::string day1ExtraBossName;
    std::string day2ExtraBossName;
    std::string eventContent;
    MapPoint day1PlayArea;
    MapPoint day2PlayArea;
    MapPoint starter;
    std::vector<SpotConfigView> spots;
};

struct MapView {
    MapView(int id) : mapId(id) {}
    int mapId = -1;
    std::vector<AttachPointView> attachPoints;
    std::vector<StarterView> starters;
};

struct MapCache {
    int id = -1;
    std::string name;
    MapView view{ -1 };
};

class GameDataDB {
public:
    GameDataDB();
    ~GameDataDB();

    bool open(const std::string& dbPath);
    void close();

    // Map info (served from cache)
    std::vector<const char*>            getMapNames() const;

    // Single-row queries; return std::nullopt when not found
    std::optional<SpotInfo>             getSpot(int attachId) const;
    std::optional<StarterInfo>          getStarter(int starterId) const;
    std::optional<SpotOption>           getSpotOption(int attachId) const;
    std::optional<SpotLabelOption>      getAttachOption(int attachId) const;
    std::optional<GridOption>           getGridOption(int map, int x, int y) const;

    // Multi-row queries
    std::vector<VariationInfo>          listVariations(int attachId, const std::set<int>& patterns) const;

    // Pattern filtering — temp-table based
    void          resetFilter(int mapId = -1);
    std::set<int> getFilteredPatterns() const;

    // Filter helpers
    std::set<int> filterByMap(int map);
    std::set<int> filterByVariation(const std::set<int>& patterns, int attachId, int varKey) const;
    std::set<int> filterByStarter(const std::set<int>& patterns, int starterId) const;
    std::set<int> filterByNightlord(const std::set<int>& patterns, int nightlordId) const;

    const MapView& getMapView(int mapId) const;
    std::optional<PatternView> getPatternView(int patternId) const;

private:
    bool createTempViews();
    bool loadCache();
    bool loadMaps();
    std::optional<MapView> loadMapView(int mapId) const;

private:
    sqlite3* m_db = nullptr;
    std::string m_tempTable = "temp_pattern";
    std::map<int, MapCache> m_cachedMaps;
};