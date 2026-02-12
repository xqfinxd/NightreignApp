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
}

GameData& GameData::getInstance()
{
    static GameData instance;
    return instance;
}

bool GameData::loadFromCSV(const std::string& dataPath)
{
    SDL_Log("GameData: Loading CSV data from %s", dataPath.c_str());
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
    
	applyVariationLabels();

    SDL_Log("GameData: Loaded %zu patterns, %zu spots, %zu variations",
            m_patterns.size(), m_spots.size(), m_variations.size());
    
    return true;
}

std::vector<const char *> GameData::getMapNames() const
{
    std::vector<const char *> names;
    for (const auto& pair : m_maps) {
        names.push_back(pair.second.name.c_str());
    }
    return names;
}

int GameData::getMapCount() const
{
    return static_cast<int>(m_maps.size());
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
        map.isdlc = map.id == 4;
        m_maps[map.id] = map;
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
        
        pattern.playArea1_gridXNo = getInt(csv, i, "playArea1_gridXNo");
        pattern.playArea1_gridZNo = getInt(csv, i, "playArea1_gridZNo");
        pattern.playArea1_posX = getFloat(csv, i, "playArea1_posX");
        pattern.playArea1_posZ = getFloat(csv, i, "playArea1_posZ");
        
        pattern.playArea2_gridXNo = getInt(csv, i, "playArea2_gridXNo");
        pattern.playArea2_gridZNo = getInt(csv, i, "playArea2_gridZNo");
        pattern.playArea2_posX = getFloat(csv, i, "playArea2_posX");
        pattern.playArea2_posZ = getFloat(csv, i, "playArea2_posZ");
        
        m_patterns[pattern.id] = pattern;
        m_maps[pattern.map].patterns.push_back(pattern.id);
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
        std::string spotsStr = getString(csv, i, "spots");
        
        std::vector<int>& spots = m_staticSpotsByMap[map];
        std::stringstream ss(spotsStr);
        std::string spotId;
        
        while (std::getline(ss, spotId, '_'))
        {
            if (!spotId.empty())
            {
                spots.push_back(std::stoi(spotId));
            }
        }
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
        SpotPosition spot;
        spot.id = getInt(csv, i, "id");
        spot.gridXNo = getInt(csv, i, "gridXNo");
        spot.gridZNo = getInt(csv, i, "gridZNo");
        spot.posX = getFloat(csv, i, "posX");
        spot.posZ = getFloat(csv, i, "posZ");
        
        m_spots[spot.id] = spot;
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
    
    m_variations.reserve(csv.getRowCount());
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        VariationInfo var;
        var.spotId = getInt(csv, i, "spotId");
        var.patternId = getInt(csv, i, "patternId");
        var.variationId = getInt(csv, i, "variationId");
        var.variationType = getInt(csv, i, "variationType");
        
        m_variations.push_back(var);
        
        // Build indices
        m_variationsBySpot.insert({var.spotId, &m_variations.back()});
        m_variationsByPattern.insert({var.patternId, &m_variations.back()});
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
    
    // Store additional variation metadata
    struct VariationMetadata {
        std::string label;
        std::string icon;
        bool visible;
        float iconScale;
    };
    
    std::map<int, VariationMetadata> metadata;
    
    for (size_t i = 0; i < csv.getRowCount(); ++i)
    {
        int id = getInt(csv, i, "id");
        std::string label = getString(csv, i, "label");
        std::string sublabel = getString(csv, i, "sublabel");
        
        if (!sublabel.empty())
            label = label + " - " + sublabel;
        
        VariationMetadata meta;
        meta.label = label;
        meta.icon = getString(csv, i, "icon", "");
        meta.visible = getInt(csv, i, "visible", 1) != 0;
        meta.iconScale = getFloat(csv, i, "iconScale", 1.0f);
        
        m_variationLabels[id] = label;
        metadata[id] = meta;
    }
    
    // Apply metadata to loaded variations
    for (auto& var : m_variations)
    {
        int key = var.getKey();
        auto it = metadata.find(key);
        if (it != metadata.end())
        {
            var.icon = it->second.icon;
            var.visible = it->second.visible;
            var.iconScale = it->second.iconScale;
        }
    }
    
    return true;
}

void GameData::applyVariationLabels()
{
    for(auto& var : m_variations)
    {
        auto it = m_variationLabels.find(var.getKey());
        if (it != m_variationLabels.end())
        {
            var.label = it->second;
        }
        else
        {
            var.label = "???-" + std::to_string(var.variationId);
        }
	}
}

const PatternInfo* GameData::getPattern(int patternId) const
{
    auto it = m_patterns.find(patternId);
    return (it != m_patterns.end()) ? &it->second : nullptr;
}

const std::vector<int>& GameData::getPatternsByMap(int map) const
{
    auto it = m_maps.find(map);
    return (it != m_maps.end()) ? it->second.patterns : s_emptyIntVector;
}

const SpotPosition* GameData::getSpot(int spotId) const
{
    auto it = m_spots.find(spotId);
    return (it != m_spots.end()) ? &it->second : nullptr;
}

const std::vector<int>& GameData::getStaticSpotsByMap(int map) const
{
    auto it = m_staticSpotsByMap.find(map);
    return (it != m_staticSpotsByMap.end()) ? it->second : s_emptyIntVector;
}

glm::vec2 GameData::normalizeSpotPosition(const SpotPosition& spot, int tileSize) const
{
    float gridX = spot.posX / tileSize + spot.gridXNo - 41;
    float gridZ = spot.posZ / tileSize + spot.gridZNo - 35;
    return glm::vec2(gridX, gridZ);
}

std::vector<const VariationInfo*> GameData::getVariationsAtSpot(int spotId, int map) const
{
    std::vector<const VariationInfo*> result;
    const auto& patternsForMap = getPatternsByMap(map);
    std::set<int> validPatterns(patternsForMap.begin(), patternsForMap.end());
    
    return getVariationsAtSpotInPatterns(spotId, validPatterns);
}

std::vector<const VariationInfo*> GameData::getVariationsAtSpotInPatterns(
    int spotId,
    const std::set<int>& validPatterns) const
{
    std::vector<const VariationInfo*> result;
    auto range = m_variationsBySpot.equal_range(spotId);
    
    for (auto it = range.first; it != range.second; ++it)
    {
        if (validPatterns.count(it->second->patternId) > 0)
        {
            result.push_back(it->second);
        }
    }
    
    return result;
}

const std::string& GameData::getVariationLabel(int key) const
{
    auto it = m_variationLabels.find(key);
    return (it != m_variationLabels.end()) ? it->second : s_emptyString;
}

std::vector<const VariationInfo*> GameData::getVariationsForPattern(int patternId) const
{
    std::vector<const VariationInfo*> result;
    auto range = m_variationsByPattern.equal_range(patternId);

    for (auto it = range.first; it != range.second; ++it)
    {
        result.push_back(it->second);
    }

    return result;
}

std::vector<int> GameData::filterPatternsBySpotVariations(
    int mapIndex,
    const std::map<int, int>& spotVariations) const
{
    if (spotVariations.empty())
        return std::vector<int>();
    
    // Build pattern -> spot -> variations map
    std::map<int, std::map<int, std::set<int>>> patternSpots;
    
    const auto& patternsForMap = getPatternsByMap(mapIndex);
    for (int patternId : patternsForMap)
    {
        auto range = m_variationsByPattern.equal_range(patternId);
        for (auto it = range.first; it != range.second; ++it)
        {
            const auto* var = it->second;
            patternSpots[patternId][var->spotId].insert(var->getKey());
        }
    }
    
    // Filter patterns that match ALL marked spots
    std::vector<int> matchingPatterns;
    
    for (const auto& [patternId, spots] : patternSpots)
    {
        bool matches = true;
        
        for (const auto& [spotId, requiredVarKey] : spotVariations)
        {
            auto spotIt = spots.find(spotId);
            if (spotIt == spots.end() || spotIt->second.count(requiredVarKey) == 0)
            {
                matches = false;
                break;
            }
        }
        
        if (matches)
        {
            matchingPatterns.push_back(patternId);
        }
    }
    
    std::sort(matchingPatterns.begin(), matchingPatterns.end());
    return matchingPatterns;
}
