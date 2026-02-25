#include "GameData.h"
#include "CsvReader.h"
#include <sstream>
#include <algorithm>
#include <SDL_log.h>
#include <SDL_assert.h>

#include "generated/ManualMapRow.h"
#include "generated/ManualNightlordRow.h"
#include "generated/ManualSpotLabelRow.h"
#include "generated/ManualSpotRow.h"
#include "generated/ManualValidationRow.h"
#include "generated/PatternRow.h"
#include "generated/SpotRow.h"
#include "generated/StarterDistributionRow.h"
#include "generated/StarterRow.h"
#include "generated/VariationRow.h"
#include "generated/ManualGridRow.h"
#include "generated/ConstantRow.h"
#include "generated/RottedPowerRow.h"
#include "generated/GreatHollowBindingRow.h"
#include "generated/EventRow.h"

const std::vector<int> GameData::s_emptyIntVector;
const std::string GameData::s_emptyString = "Unknown";
GameData* GameData::s_instance = nullptr;

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

    void sortAndUnique(std::vector<int>& vec) {
        std::sort(vec.begin(), vec.end());
        auto last = std::unique(vec.begin(), vec.end());
        vec.erase(last, vec.end());
    }
}

GameData::GameData()
{
    SDL_assert(s_instance == nullptr);
    s_instance = this;
}

GameData::~GameData()
{
    SDL_assert(s_instance != nullptr);
    s_instance = nullptr;
}

bool GameData::loadFromCSV(const std::string& dataPath)
{
    if (!loadMaps(dataPath + "/manual_maps.csv"))
        return false;
    
    if (!loadPatterns(dataPath + "/autogen_pattern_list.csv"))
        return false;
    
    if (!loadSpots(dataPath + "/autogen_spot_list.csv"))
        return false;
    
    if (!loadStarters(dataPath + "/autogen_starter_list.csv"))
        return false;

    if (!loadStarterDist(dataPath + "/autogen_starter_distribution.csv"))
        return false;

    if (!loadVariations(dataPath + "/autogen_variation_list.csv"))
        return false;
    
    if (!loadNightlords(dataPath + "/manual_nightlords.csv"))
        return false;
    
    if (!loadVariationsEx(dataPath + "/manual_variations.csv"))
        return false;

    if (!loadSpotsEx(dataPath + "/manual_spots.csv"))
        return false;

    if (!loadSpotsLabelEx(dataPath + "/manual_attachpoints.csv"))
        return false;

    if (!loadGridEx(dataPath + "/manual_grids.csv"))
        return false;

    if (!loadConstant(dataPath + "/manual_constant_spots.csv"))
        return false;

    if (!loadRottedPowers(dataPath + "/autogen_rotted_power.csv"))
        return false;

    if (!loadGreatHollowBindings(dataPath + "/manual_great_hollow_binding_spots.csv"))
        return false;

    if (!loadEventsEx(dataPath + "/manual_events.csv"))
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

bool GameData::loadNightlords(const std::string &filePath)
{
    auto nightlordRowdata = readCSVFile<ManualNightlordRow>(filePath);
    for (size_t i = 0; i < nightlordRowdata.size(); ++i)
    {
        NightlordInfo info;
        info.id = nightlordRowdata[i].id;
        info.name = nightlordRowdata[i].name;
        m_nightlordDB[info.id] = info;
    }
    return true;
}

bool GameData::loadMaps(const std::string &filePath)
{
    auto mapRowdata = readCSVFile<ManualMapRow>(filePath);
    
    for (size_t i = 0; i < mapRowdata.size(); ++i)
    {
        MapInfo map;
        map.id = mapRowdata[i].id;
        map.name = mapRowdata[i].name;
        m_mapDB[map.id] = map;
    }
    
    return true;
}

bool GameData::loadPatterns(const std::string &filePath)
{
    auto patternRowdata = readCSVFile<PatternRow>(filePath);
    
    for (size_t i = 0; i < patternRowdata.size(); ++i)
    {
        PatternInfo pattern;
        pattern.id              = patternRowdata[i].id;
        pattern.map             = patternRowdata[i].map;
        pattern.boss            = patternRowdata[i].boss;
        pattern.bossId1         = patternRowdata[i].bossId1;
        pattern.bossId2         = patternRowdata[i].bossId2;
        pattern.extraBossId1    = patternRowdata[i].extraBossId1;
        pattern.extraBossId2    = patternRowdata[i].extraBossId2;
        pattern.isdlc           = patternRowdata[i].isdlc;
        pattern.starter         = patternRowdata[i].starter;
        
        pattern.playArea1.gridXNo   = patternRowdata[i].playArea1_gridXNo;
        pattern.playArea1.gridZNo   = patternRowdata[i].playArea1_gridZNo;
        pattern.playArea1.posX      = patternRowdata[i].playArea1_posX;
        pattern.playArea1.posZ      = patternRowdata[i].playArea1_posZ;
        pattern.playArea1.height    = patternRowdata[i].playArea1_height;

        pattern.playArea2.gridXNo   = patternRowdata[i].playArea2_gridXNo;
        pattern.playArea2.gridZNo   = patternRowdata[i].playArea2_gridZNo;
        pattern.playArea2.posX      = patternRowdata[i].playArea2_posX;
        pattern.playArea2.posZ      = patternRowdata[i].playArea2_posZ;
        pattern.playArea2.height    = patternRowdata[i].playArea2_height;
        
        m_patternDB[pattern.id] = pattern;
        if (pattern.isdlc)
            m_mapDB[pattern.map].dlcPatterns.push_back(pattern.id);
        else
            m_mapDB[pattern.map].legacyPatterns.push_back(pattern.id);
    }

    for (auto& mappair : m_mapDB)
    {
        sortAndUnique(mappair.second.legacyPatterns);
        sortAndUnique(mappair.second.dlcPatterns);
    }
    
    return true;
}

bool GameData::loadSpots(const std::string& filePath)
{
    auto spotRowdata = readCSVFile<SpotRow>(filePath);
    
    for (size_t i = 0; i < spotRowdata.size(); ++i)
    {
        auto& right = spotRowdata[i];
        if (!m_spotDB.count(right.attachId))
        {
            SpotInfo spotInfo;
            spotInfo.attachId       = right.attachId;
            spotInfo.point.gridXNo  = right.gridXNo;
            spotInfo.point.gridZNo  = right.gridZNo;
            spotInfo.point.posX     = right.posX;
            spotInfo.point.posZ     = right.posZ;
            spotInfo.point.height   = right.height;
            m_spotDB[spotInfo.attachId] = spotInfo;
        }
        
        if (right.map >= 0 && right.rate >= 0.8f)
        {
            if (spotRowdata[i].dlc)
                m_mapDB[right.map].dlcFilterPoints.push_back(right.attachId);
            else
                m_mapDB[right.map].legacyFilterPoints.push_back(right.attachId);
        }
    }

    for (auto& mappair : m_mapDB)
    {
        sortAndUnique(mappair.second.legacyFilterPoints);
        sortAndUnique(mappair.second.dlcFilterPoints);
    }
    
    return true;
}

bool GameData::loadVariations(const std::string& filePath)
{
    auto variationRowdata = readCSVFile<VariationRow>(filePath);
    
    for (size_t i = 0; i < variationRowdata.size(); ++i)
    {
        VariationInfo var;
        var.variationId = variationRowdata[i].variationId;
        var.variationType = variationRowdata[i].variationType;
        
        m_variationDB[var.getKey()] = var;

        VariationDist dist;
        dist.attachId = variationRowdata[i].attachId;
        dist.patternId = variationRowdata[i].patternId;
        dist.variationKey = var.getKey();
        m_variationDist.push_back(dist);
    }
    
    return true;
}

bool GameData::loadVariationsEx(const std::string& filePath)
{
    auto variationLabelRowdata = readCSVFile<ManualValidationRow>(filePath);
    
    for (size_t i = 0; i < variationLabelRowdata.size(); ++i)
    {
        int varkey = variationLabelRowdata[i].id;
        auto it = m_variationDB.find(varkey);
        if (it == m_variationDB.end()) continue;

        it->second.label = variationLabelRowdata[i].label;
        it->second.sublabel = variationLabelRowdata[i].sublabel;
        it->second.icon = variationLabelRowdata[i].icon;
        it->second.visible = variationLabelRowdata[i].visible;
        it->second.iconScale = variationLabelRowdata[i].iconScale;
    }
    
    return true;
}

bool GameData::loadStarters(const std::string &filePath)
{
    auto starterRowdata = readCSVFile<StarterRow>(filePath);

    for (size_t i = 0; i < starterRowdata.size(); ++i)
    {
        auto& right = starterRowdata[i];
        StarterInfo starterInfo;
        starterInfo.starterId = right.id;
        starterInfo.point.gridXNo = right.gridXNo;
        starterInfo.point.gridZNo = right.gridZNo;
        starterInfo.point.posX = right.posX;
        starterInfo.point.posZ = right.posZ;
        starterInfo.point.height = right.height;
        m_starterDB[starterInfo.starterId] = starterInfo;
    }
    
    return true;
}

bool GameData::loadStarterDist(const std::string &filePath)
{
    auto starterRowdata = readCSVFile<StarterDistributionRow>(filePath);

    for (size_t i = 0; i < starterRowdata.size(); ++i)
    {
        auto& right = starterRowdata[i];
        m_mapDB[right.id].starters.push_back(right.starter);
    }
    
    for (auto& mappair : m_mapDB)
    {
        sortAndUnique(mappair.second.starters);
    }

    return true;
}

bool GameData::loadSpotsEx(const std::string &filePath)
{
    auto spotExRowdata = readCSVFile<ManualSpotRow>(filePath);

    for (size_t i = 0; i < spotExRowdata.size(); ++i)
    {
        SpotOption option;
        option.attachId = spotExRowdata[i].id;
        option.disable_filter = spotExRowdata[i].disable_filter > 0;
        option.disable_view = spotExRowdata[i].disable_view > 0;
        m_spotOptions[option.attachId] = option;
    }
    return true;
}

bool GameData::loadSpotsLabelEx(const std::string &filePath)
{
    auto spotLabelRowdata = readCSVFile<ManualSpotLabelRow>(filePath);

    for (size_t i = 0; i < spotLabelRowdata.size(); ++i)
    {
        SpotLabelOption option;
        option.attachId = spotLabelRowdata[i].id;
        option.direction = spotLabelRowdata[i].direction;
        option.offset.x = spotLabelRowdata[i].offsetx;
        option.offset.y = spotLabelRowdata[i].offsety;
        option.showIcon = spotLabelRowdata[i].showicon != 0;
        m_spotLabelOptions[option.attachId] = option;
    }
    return true;
}

bool GameData::loadGridEx(const std::string& filePath)
{
    auto gridRowdata = readCSVFile<ManualGridRow>(filePath);
    for (size_t i = 0; i < gridRowdata.size(); ++i)
    {
        GridOption option;
        option.x = gridRowdata[i].x;
        option.y = gridRowdata[i].y;
        option.height = gridRowdata[i].height;
        option.map = gridRowdata[i].map;
        m_gridOptions.push_back(option);
    }
    return true;
}

bool GameData::loadConstant(const std::string& filePath)
{
    auto constantRowdata = readCSVFile<ConstantRow>(filePath);
    for (size_t i = 0; i < constantRowdata.size(); ++i)
    {
        auto& right = constantRowdata[i];
        ConstantInfo info;
        info.type           = right.type;
        info.map            = right.map;
        info.point.gridXNo  = right.gridXNo;
        info.point.gridZNo  = right.gridZNo;
        info.point.posX     = right.posX;
        info.point.posZ     = right.posZ;
        info.point.height   = right.height;
        info.label          = constantRowdata[i].label;
        info.icon           = constantRowdata[i].icon;
        info.iconScale      = constantRowdata[i].iconScale;
        m_constantDB.push_back(info);
    }
    return true;
}

bool GameData::loadRottedPowers(const std::string& filePath)
{
    auto rottedPowerRowdata = readCSVFile<RottedPowerRow>(filePath);
    for (size_t i = 0; i < rottedPowerRowdata.size(); ++i)
    {
        auto& right = rottedPowerRowdata[i];
        RottedPowerInfo info;
        info.patternId = right.patternId;
        info.point.gridXNo = right.gridXNo;
        info.point.gridZNo = right.gridZNo;
        info.point.posX = right.posX;
        info.point.posZ = right.posZ;
        info.point.height = right.height;
        m_rottedPowers[info.patternId] = info;
    }
    return true;
}

bool GameData::loadGreatHollowBindings(const std::string &filePath)
{
    auto bindingRowdata = readCSVFile<GreatHollowBindingRow>(filePath);
    for (size_t i = 0; i < bindingRowdata.size(); ++i)
    {
        auto& right = bindingRowdata[i];
        GreatHollowBindingInfo info;
        info.iconScale = right.iconScale;
        info.visible = right.visible;
        info.binding = right.binding;
        info.icon = right.icon;
        info.label = right.label;
        info.point.gridXNo = right.gridXNo;
        info.point.gridZNo = right.gridZNo;
        info.point.posX = right.posX;
        info.point.posZ = right.posZ;
        info.point.height = right.height;
        m_greatHollowBindings.push_back(info);
    }
    return true;
}

const PatternInfo* GameData::getPattern(int patternId) const
{
    auto it = m_patternDB.find(patternId);
    return (it != m_patternDB.end()) ? &it->second : nullptr;
}

std::set<int> GameData::filterByMap(int map) const
{
    std::set<int> result;
    auto it = m_mapDB.find(map);
    if (it != m_mapDB.end())
    {
        result.insert(it->second.legacyPatterns.begin(), it->second.legacyPatterns.end());
        result.insert(it->second.dlcPatterns.begin(), it->second.dlcPatterns.end());
    }
    return result;
}

const SpotInfo* GameData::getSpot(int spotId) const
{
    auto it = m_spotDB.find(spotId);
    return (it != m_spotDB.end()) ? &it->second : nullptr;
}

std::vector<int> GameData::getSpotsByMap(int map, bool legacy, bool dlc) const
{
    auto it = m_mapDB.find(map);
    std::vector<int> result;
    if (it != m_mapDB.end())    {
        if (legacy)
            result.insert(result.end(), it->second.legacyFilterPoints.begin(), it->second.legacyFilterPoints.end());
        if (dlc)
            result.insert(result.end(), it->second.dlcFilterPoints.begin(), it->second.dlcFilterPoints.end());
    }
    return result;
}

std::vector<int> GameData::getStarterByMap(int map) const
{
    auto it = m_mapDB.find(map);
    std::vector<int> result;
    if (it != m_mapDB.end())
    {
        result.insert(result.end(), it->second.starters.begin(), it->second.starters.end());
    }
    return result;
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

const VariationInfo *GameData::getVariationById(int variationId) const
{
    for (const auto& pair : m_variationDB)
    {
        if (pair.second.variationId == variationId)
            return &pair.second;
    }
    return nullptr;
}

std::vector<const VariationDist*> GameData::listDistribution(int patternId) const
{
    std::vector<const VariationDist*> result;
    for (auto& dist : m_variationDist)
    {
        if(dist.patternId == patternId)
            result.push_back(&dist);
    }
    return result;
}

std::vector<const ConstantInfo*> GameData::listConstants(int map) const
{
    std::vector<const ConstantInfo*> result;
    for (const auto& info : m_constantDB)
    {
        if (info.map == map)
            result.push_back(&info);
    }
    return result;
}

std::vector<const GreatHollowBindingInfo *> GameData::listGreatHollowBinding(int patternId) const
{
    std::vector<const GreatHollowBindingInfo *> result;
    std::set<int> variationSet;
    for (const auto& dist : m_variationDist)
    {
        if (dist.patternId == patternId)
        {
            auto varIt = m_variationDB.find(dist.variationKey);
            if (varIt == m_variationDB.end())
                continue;
            variationSet.insert(varIt->second.variationId);
        }
    }
    for (const auto& info : m_greatHollowBindings)
    {
        if (variationSet.count(info.binding))
            result.push_back(&info);
    }
    return result;
}

const StarterInfo *GameData::getStarter(int starterId) const
{
    auto it = m_starterDB.find(starterId);
    return it != m_starterDB.end() ? &it->second : nullptr;
}

const SpotLabelOption *GameData::getAttachOption(int attachId) const
{
    auto it = m_spotLabelOptions.find(attachId);
    return it != m_spotLabelOptions.end() ? &it->second : nullptr;
}

const GridOption* GameData::getGridOption(int map, int x, int y) const
{
    auto it = std::find_if(m_gridOptions.begin(), m_gridOptions.end(), [map, x, y](const GridOption& opt) {
        return opt.x == x && opt.y == y && opt.map == map;
        });
    return it != m_gridOptions.end() ? &(*it) : nullptr;
}

const RottedPowerInfo* GameData::getRottedPower(int patternId) const
{
    auto it = m_rottedPowers.find(patternId);
    return it != m_rottedPowers.end() ? &it->second : nullptr;
}

std::vector<const VariationInfo*> GameData::listVariations(int attachId, const std::set<int>& patterns) const
{
    std::vector<const VariationInfo*> result;
    
    for(const auto& varDist : m_variationDist)
    {
        if (varDist.attachId != attachId || !patterns.count(varDist.patternId))
            continue;
        auto varIt = m_variationDB.find(varDist.variationKey);
        if (varIt == m_variationDB.end())
            continue;
        result.push_back(&varIt->second);
    }
    
    return result;
}

std::vector<const VariationInfo*> GameData::listVariations(int attachId, int map) const
{
    std::set<int> patterns = filterByMap(map);
    return listVariations(attachId, patterns);
}

std::set<int> GameData::filterByVariation(const std::set<int> &patterns, int attachId, int varKey) const
{
    std::set<int> result;
    for(const auto& dist : m_variationDist)
    {
        if (dist.variationKey == varKey
            && dist.attachId == attachId
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

std::set<int> GameData::filterByNightlord(const std::set<int> &patterns, int nightlordId) const
{
    std::set<int> result;
    
     for (auto patternId : patterns)
    {
        auto patternInfo = getPattern(patternId);
        if (!patternInfo)
            continue;
        if (patternInfo->boss == nightlordId)
            result.insert(patternId);
    }
    return result;
}

const SpotOption* GameData::getSpotOption(int attachId) const
{
    auto it = m_spotOptions.find(attachId);
    if (it != m_spotOptions.end())
    {
        return &it->second;
    }
    return nullptr;
}

bool GameData::loadEventsEx(const std::string& filePath)
{
    auto rowdata = readCSVFile<EventRow>(filePath);
    for (size_t i = 0; i < rowdata.size(); ++i)
    {
        EventInfo info;
        info.id    = rowdata[i].id;
        info.event = rowdata[i].event;
        m_eventDB[info.id] = info;
    }
    return true;
}

const EventInfo* GameData::getEvent(int patternId) const
{
    auto it = m_eventDB.find(patternId);
    return it != m_eventDB.end() ? &it->second : nullptr;
}

const NightlordInfo *GameData::getNightlord(int nightlordId) const
{
    auto it = m_nightlordDB.find(nightlordId);
    return it != m_nightlordDB.end() ? &it->second : nullptr;
}
