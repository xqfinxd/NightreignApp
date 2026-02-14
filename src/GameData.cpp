#include "GameData.h"
#include "CsvReader.h"
#include <sstream>
#include <algorithm>
#include <SDL_log.h>

const std::vector<int> GameData::s_emptyIntVector;
const std::string GameData::s_emptyString = "Unknown";

// Helper functions for CSV parsing
namespace {
    int getInt(const CsvReader& csv, size_t row, const std::string& column, int defaultVal = 0) {
        std::string val = csv.getValue(row, column);
        return val.empty() ? defaultVal : std::stoi(val);
    }
    
    float getFloat(const CsvReader& csv, size_t row, const std::string& column, float defaultVal = 0.0f) {
        std::string val = csv.getValue(row, column);
        return val.empty() ? defaultVal : std::stof(val);
    }
    
    std::string getString(const CsvReader& csv, size_t row, const std::string& column, const std::string& defaultVal = "") {
        std::string val = csv.getValue(row, column);
        return val.empty() ? defaultVal : val;
    }

    MapPoint getMapPoint(const CsvReader& csv, size_t row, const std::string& prefix = "") {
        MapPoint point;
        point.gridXNo = getInt(csv, row, prefix + "gridXNo");
        point.gridZNo = getInt(csv, row, prefix + "gridZNo");
        point.posX = getFloat(csv, row, prefix + "posX");
        point.posZ = getFloat(csv, row, prefix + "posZ");
		return point;
	}

    std::vector<int> getIntList(const CsvReader& csv, size_t row, const std::string& column, char delimiter = '_') {
        std::vector<int> result;
		std::string str = getString(csv, row, column);
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            if (!item.empty()) {
                result.push_back(std::stoi(item));
            }
        }
        return result;
	}
}

GameData& GameData::getInstance()
{
    static GameData instance;
    return instance;
}

bool GameData::loadFromCSV(const std::string& dataPath)
{
    if (!loadMaps(dataPath + "/manual_maps.csv"))
        return false;
    
    if (!loadPatterns(dataPath + "/autogen_pattern_list.csv"))
        return false;
    
    if (!loadStaticSpots(dataPath + "/autogen_static_spotlist.csv"))
        return false;
    
    if (!loadSpotDistribution(dataPath + "/autogen_spot_distribution.csv"))
        return false;
    
    if (!loadVariations(dataPath + "/autogen_pattern_variationlist.csv"))
        return false;
    
    if (!loadVariationLabels(dataPath + "/manual_variations.csv"))
        return false;

    if (!loadStarterList(dataPath + "/autogen_starter_list.csv"))
        return false;

    if (!loadStarterDist(dataPath + "/autogen_starter_distribution.csv"))
        return false;
    
    return true;
}

std::vector<const char *> GameData::getMapNames() const
{
    std::vector<const char *> names;
    for (const auto& pair : m_mapDB) {
        names.push_back(pair.second.name.c_str());
    }
    return names;
}

int GameData::getMapCount() const
{
    return static_cast<int>(m_mapDB.size());
}

bool GameData::loadMaps(const std::string &filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        MapInfo map;
        map.id = getInt(csv, i, "id");
        map.name = getString(csv, i, "name");
        m_mapDB[map.id] = map;
    }
    
    return true;
}

bool GameData::loadPatterns(const std::string &filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        PatternInfo pattern;
        pattern.id = getInt(csv, i, "id");
        pattern.map = getInt(csv, i, "map");
        pattern.boss = getInt(csv, i, "boss");
        pattern.bossId1 = getInt(csv, i, "bossId1");
        pattern.bossId2 = getInt(csv, i, "bossId2");
        pattern.extraBossId1 = getInt(csv, i, "extraBossId1");
        pattern.extraBossId2 = getInt(csv, i, "extraBossId2");
        pattern.isdlc = (getString(csv, i, "isdlc") == "true");
        pattern.starter = getInt(csv, i, "starter");
        
        pattern.playArea1 = getMapPoint(csv, i, "playArea1_");
        pattern.playArea2 = getMapPoint(csv, i, "playArea2_");
        
        m_patternDB[pattern.id] = pattern;
        m_mapDB[pattern.map].patterns.push_back(pattern.id);
    }
    
    return true;
}

bool GameData::loadSpotDistribution(const std::string& filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int id = getInt(csv, i, "id");
		auto point = getMapPoint(csv, i);
		auto attachIds = getIntList(csv, i, "attachIds", '_');
        FilterSpot filterSpot;
        filterSpot.id = id;
        filterSpot.point = point;
        filterSpot.attachIds = attachIds;
        m_filterSpotDB[filterSpot.id] = filterSpot;

        for (auto attachId : attachIds)
        {
            BaseSpot baseSpot;
            baseSpot.id = attachId;
            baseSpot.point = point;
            m_baseSpotDB[baseSpot.id] = baseSpot;
        }
    }
    
    return true;
}

bool GameData::loadStaticSpots(const std::string& filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int map = getInt(csv, i, "map");
		m_mapDB[map].staticSpots = getIntList(csv, i, "spots", '_');
    }
    
    return true;
}

bool GameData::loadVariations(const std::string& filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        VariationInfo var;
        var.variationId = getInt(csv, i, "variationId");
        var.variationType = getInt(csv, i, "variationType");
        
        m_variationDB[var.getKey()] = var;

        VariationDist dist;
        dist.attachId = getInt(csv, i, "attachId");
        dist.patternId = getInt(csv, i, "patternId");
        dist.variationKey = var.getKey();
        m_variationDist.push_back(dist);
    }
    
    return true;
}

bool GameData::loadVariationLabels(const std::string& filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int id = getInt(csv, i, "id");
        auto it = m_variationDB.find(id);
        if (it == m_variationDB.end()) continue;

        it->second.label = getString(csv, i, "label");
        it->second.sublabel = getString(csv, i, "sublabel");
        it->second.icon = getString(csv, i, "icon", "");
        it->second.visible = getInt(csv, i, "visible", 1) != 0;
        it->second.iconScale = getFloat(csv, i, "iconScale", 1.0f);
    }
    
    return true;
}

bool GameData::loadStarterList(const std::string &filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }

    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int id = getInt(csv, i, "id");
        auto& starter = m_starterSpotDB[id];
        starter.id = id;
        starter.point = getMapPoint(csv, i);
    }
    
    return true;
}

bool GameData::loadStarterDist(const std::string &filePath)
{
    CsvReader csv;
    if (!csv.load(filePath))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GameData: Failed to load %s", filePath.c_str());
        return false;
    }

    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int map = getInt(csv, i, "id");
        auto& mapInfo = m_mapDB[map];
        mapInfo.starterSpots = getIntList(csv, i, "starters", '_');
    }
    
    return true;
}

const PatternInfo* GameData::getPattern(int patternId) const
{
    auto it = m_patternDB.find(patternId);
    return (it != m_patternDB.end()) ? &it->second : nullptr;
}

const std::vector<int>& GameData::getPatternsByMap(int map) const
{
    auto it = m_mapDB.find(map);
    return (it != m_mapDB.end()) ? it->second.patterns : s_emptyIntVector;
}

const FilterSpot* GameData::getSpot(int spotId) const
{
    auto it = m_filterSpotDB.find(spotId);
    return (it != m_filterSpotDB.end()) ? &it->second : nullptr;
}

const BaseSpot* GameData::getAttach(int attachId) const
{
    auto it = m_baseSpotDB.find(attachId);
    return (it != m_baseSpotDB.end()) ? &it->second : nullptr;
}

const std::vector<int>& GameData::getStaticSpotsByMap(int map) const
{
    auto it = m_mapDB.find(map);
    return (it != m_mapDB.end()) ? it->second.staticSpots : s_emptyIntVector;
}

const std::vector<int>& GameData::getStarterSpotsByMap(int map) const
{
    auto it = m_mapDB.find(map);
    return (it != m_mapDB.end()) ? it->second.starterSpots : s_emptyIntVector;
}

const VariationInfo *GameData::getVariation(int varKey) const
{
    auto it = m_variationDB.find(varKey);
    return it != m_variationDB.end() ? &it->second : nullptr;
}

const VariationInfo *GameData::getVariation(int patternId, int attachId) const
{
    for(const auto& dist : m_variationDist)
    {
        if (dist.attachId == attachId && dist.patternId == patternId)
            return getVariation(dist.variationKey);
    }
    return nullptr;
}

std::vector<const VariationDist*> GameData::getDists(int patternId) const
{
    std::vector<const VariationDist*> result;
    for (auto& dist : m_variationDist)
    {
        if(dist.patternId == patternId)
            result.push_back(&dist);
    }
    return result;
}

const StarterSpot *GameData::getStarterSpot(int starterId) const
{
    auto it = m_starterSpotDB.find(starterId);
    return it != m_starterSpotDB.end() ? &it->second : nullptr;
}

std::vector<const VariationInfo*> GameData::getVariationsAtSpot(int spotId, const std::set<int>& patterns) const
{
    std::vector<const VariationInfo*> result;
    
    std::set<int> attachIds;
    auto fspotIt = m_filterSpotDB.find(spotId);
    if (fspotIt != m_filterSpotDB.end())
    {
        attachIds.insert(
            fspotIt->second.attachIds.begin(),
            fspotIt->second.attachIds.end());
    }

    for(const auto& varDist : m_variationDist)
    {
        if (!attachIds.count(varDist.attachId) || !patterns.count(varDist.patternId))
            continue;
        auto varIt = m_variationDB.find(varDist.variationKey);
        if (varIt == m_variationDB.end())
            continue;
        result.push_back(&varIt->second);
    }
    
    return result;
}

std::vector<const VariationInfo*> GameData::getVariationsAtSpot(int spotId, int map) const
{
    std::set<int> patterns;
    auto mapIt = m_mapDB.find(map);
    if (mapIt != m_mapDB.end())
    {
        patterns.insert(
            mapIt->second.staticSpots.begin(),
            mapIt->second.staticSpots.begin());
    }
    return getVariationsAtSpot(spotId, patterns);
}

std::set<int> GameData::filterByVariation(const std::set<int> &patterns, int spotId, int varKey) const
{
    std::set<int> result;
    std::set<int> attachIds;
    auto spotIt = m_filterSpotDB.find(spotId);
    if (spotIt != m_filterSpotDB.end())
    {
        attachIds.insert(
            spotIt->second.attachIds.begin(),
            spotIt->second.attachIds.end());
    }
    for(const auto& dist : m_variationDist)
    {
        if (dist.variationKey == varKey
            && attachIds.count(dist.attachId)
            && patterns.count(dist.patternId))
            result.insert(dist.patternId);
    }

    return result;
}

std::set<int> GameData::filterByStarter(const std::set<int> &patterns, int starterId) const
{
    std::set<int> result;
    
    for (auto patternId : patterns)
    {
        auto patternInfo = getPattern(patternId);
        if (!patternInfo)
            continue;
        if (patternInfo->starter == starterId)
            result.insert(patternId);
    }

    return result;
}
