#include "GameData.h"
#include "CsvReader.h"
#include <sstream>
#include <algorithm>
#include <SDL_log.h>
#include <SDL_assert.h>
#include "sqlite3.h"

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

    std::vector<int> separateIntList(const std::string& str, char delimiter = ',') {
        std::vector<int> result;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            if (!item.empty()) {
                result.push_back(std::stoi(item));
            }
        }
        return result;
    }

    std::vector<int> getIntList(const CsvReader& csv, size_t row, const std::string& column, char delimiter = '_') {
        std::vector<int> result;
		std::string str = getString(csv, row, column);
        return separateIntList(str, delimiter);
	}

    void sortAndUnique(std::vector<int>& vec) {
        std::sort(vec.begin(), vec.end());
        auto last = std::unique(vec.begin(), vec.end());
        vec.erase(last, vec.end());
        vec.shrink_to_fit();
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

// ============================================================
//  SQLite helpers
// ============================================================
namespace {
    static bool dbPrepare(sqlite3* db, const char* sql, sqlite3_stmt** stmt)
    {
        int rc = sqlite3_prepare_v2(db, sql, -1, stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "GameData DB prepare error: %s\nSQL: %s", sqlite3_errmsg(db), sql);
            return false;
        }
        return true;
    }

    static std::string dbText(sqlite3_stmt* stmt, int col)
    {
        const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return txt ? txt : "";
    }

    static MapPoint dbMapPoint(sqlite3_stmt* stmt, int firstCol, bool heightExists = true)
    {
        MapPoint p;
        p.gridXNo = sqlite3_column_int(stmt, firstCol);
        p.gridZNo = sqlite3_column_int(stmt, firstCol + 1);
        p.posX    = static_cast<float>(sqlite3_column_double(stmt, firstCol + 2));
        p.posZ    = static_cast<float>(sqlite3_column_double(stmt, firstCol + 3));
        p.height  = heightExists ? static_cast<float>(sqlite3_column_double(stmt, firstCol + 4)) : 0.0f;
        return p;
    }

    static int dbNullableInt(sqlite3_stmt* stmt, int col, int defaultVal = 0)
    {
        return sqlite3_column_type(stmt, col) == SQLITE_NULL
            ? defaultVal : sqlite3_column_int(stmt, col);
    }

    std::vector<int> split(const std::string& str, char delimiter = ',') {
        std::vector<int> result;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            if (!item.empty()) {
                result.push_back(std::stoi(item));
            }
        }
        return result;
    }

    std::string join(const std::vector<int>& v, const std::string& delimiter = ",") {
        std::string out;
        if (auto i = v.begin(), e = v.end(); i != e) {
            out += std::to_string(*i++);
            for (; i != e; ++i) out.append(delimiter).append(std::to_string(*i));
        }
        return out;
    }

} // anonymous namespace

// ============================================================
//  loadFromDB  – replaces loadFromCSV
// ============================================================
bool GameData::loadFromDB(const std::string& dbPath)
{
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "GameData: Failed to open DB '%s': %s", dbPath.c_str(), sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    bool ok = dbLoadNightlords(db)
           && dbLoadMaps(db)
           && dbLoadPatterns(db)
           && dbLoadAttachPoints(db)
           && dbLoadStarters(db)
           && dbLoadVariations(db)
           && dbLoadSpotConfig(db)
           && dbLoadFixedSpots(db)
           && dbLoadGridHeights(db)
           && dbLoadEvents(db)
           && dbLoadMapBindings(db)
           && dbLoadPatternBindings(db)
           && dbLoadSmallBaseBindings(db);

    sqlite3_close(db);
    return ok;
}

// ============================================================
//  Nightlord  →  m_nightlordDB
// ============================================================
bool GameData::dbLoadNightlords(sqlite3* db)
{
    static const char* sql =
        "SELECT nightlord_id, name FROM Nightlord";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        NightlordInfo info;
        info.id   = sqlite3_column_int(stmt, 0);
        info.name = dbText(stmt, 1);
        m_nightlordDB[info.id] = info;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d nightlords", (int)m_nightlordDB.size());
    return true;
}

// ============================================================
//  Map  →  m_mapDB (name only; lists filled by later steps)
// ============================================================
bool GameData::dbLoadMaps(sqlite3* db)
{
    static const char* sql =
        "SELECT map_id, name FROM Map";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        MapInfo map;
        map.id   = sqlite3_column_int(stmt, 0);
        map.name = dbText(stmt, 1);
        m_mapDB[map.id] = map;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d maps", (int)m_mapDB.size());
    return true;
}

// ============================================================
//  Pattern + PlayArea JOIN  →  m_patternDB
//  Side-effect: fills m_mapDB[].legacyPatterns/dlcPatterns/starters
// ============================================================
bool GameData::dbLoadPatterns(sqlite3* db)
{
    // Columns:
    //  0  pattern_id      1  map_id          2  nightlord_id    3  dlc
    //  4  starter_id
    //  5  day1boss        6  day2boss        7  day1extraboss   8  day2extraboss
    //  9  pa1.grid_x     10 pa1.grid_z      11 pa1.pos_x       12 pa1.pos_z      13 pa1.height
    // 14  pa2.grid_x     15 pa2.grid_z      16 pa2.pos_x       17 pa2.pos_z      18 pa2.height
    static const char* sql =
        "SELECT p.pattern_id, p.map_id, p.nightlord_id, p.dlc, p.starter_id,"
        "       p.day1boss_smallbase_id, p.day2boss_smallbase_id,"
        "       p.day1extraboss_smallbase_id, p.day2extraboss_smallbase_id,"
        "       pa1.grid_x, pa1.grid_z, pa1.pos_x, pa1.pos_z, pa1.height,"
        "       pa2.grid_x, pa2.grid_z, pa2.pos_x, pa2.pos_z, pa2.height"
        " FROM Pattern p"
        " LEFT JOIN PlayArea pa1 ON p.day1_playarea_id = pa1.playarea_id"
        " LEFT JOIN PlayArea pa2 ON p.day2_playarea_id = pa2.playarea_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PatternInfo pattern;
        pattern.id          = sqlite3_column_int(stmt,  0);
        pattern.map         = sqlite3_column_int(stmt,  1);
        pattern.boss        = sqlite3_column_int(stmt,  2);
        pattern.isdlc       = sqlite3_column_int(stmt,  3) != 0;
        pattern.starter     = dbNullableInt(stmt, 4, -1);
        pattern.bossId1     = dbNullableInt(stmt, 5);
        pattern.bossId2     = dbNullableInt(stmt, 6);
        pattern.extraBossId1 = dbNullableInt(stmt, 7);
        pattern.extraBossId2 = dbNullableInt(stmt, 8);
        pattern.playArea1   = dbMapPoint(stmt,  9);
        pattern.playArea2   = dbMapPoint(stmt, 14);

        m_patternDB[pattern.id] = pattern;

        // Populate map pattern lists
        if (pattern.isdlc)
            m_mapDB[pattern.map].dlcPatterns.push_back(pattern.id);
        else
            m_mapDB[pattern.map].legacyPatterns.push_back(pattern.id);

        // Populate starter list per map
        if (pattern.starter > 0)
            m_mapDB[pattern.map].starters.push_back(pattern.starter);
    }
    sqlite3_finalize(stmt);

    for (auto& pair : m_mapDB)
    {
        sortAndUnique(pair.second.legacyPatterns);
        sortAndUnique(pair.second.dlcPatterns);
        sortAndUnique(pair.second.starters);
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d patterns", (int)m_patternDB.size());
    return true;
}

// ============================================================
//  AttachPoint  →  m_spotDB
// ============================================================
bool GameData::dbLoadAttachPoints(sqlite3* db)
{
    static const char* sql =
        "SELECT attach_id, grid_x, grid_z, pos_x, pos_z, height FROM AttachPoint";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        SpotInfo info;
        info.attachId       = sqlite3_column_int(stmt, 0);
        info.point.gridXNo  = sqlite3_column_int(stmt, 1);
        info.point.gridZNo  = sqlite3_column_int(stmt, 2);
        info.point.posX     = static_cast<float>(sqlite3_column_double(stmt, 3));
        info.point.posZ     = static_cast<float>(sqlite3_column_double(stmt, 4));
        info.point.height   = static_cast<float>(sqlite3_column_double(stmt, 5));
        m_spotDB[info.attachId] = info;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d attach points", (int)m_spotDB.size());
    return true;
}

// ============================================================
//  Starter  →  m_starterDB
// ============================================================
bool GameData::dbLoadStarters(sqlite3* db)
{
    static const char* sql =
        "SELECT starter_id, grid_x, grid_z, pos_x, pos_z, height FROM Starter";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        StarterInfo info;
        info.starterId      = sqlite3_column_int(stmt, 0);
        info.point.gridXNo  = sqlite3_column_int(stmt, 1);
        info.point.gridZNo  = sqlite3_column_int(stmt, 2);
        info.point.posX     = static_cast<float>(sqlite3_column_double(stmt, 3));
        info.point.posZ     = static_cast<float>(sqlite3_column_double(stmt, 4));
        info.point.height   = static_cast<float>(sqlite3_column_double(stmt, 5));
        m_starterDB[info.starterId] = info;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d starters", (int)m_starterDB.size());
    return true;
}

// ============================================================
//  SmallBaseMap + VariationParam  →  m_variationDB
// ============================================================
bool GameData::dbLoadVariations(sqlite3* db)
{
    // label priority: VariationParam.label → SmallBaseMap.label
    // icon  priority: VariationParam.icon_atlas → SmallBaseMap.icon_atlas
    // visible/flags come from SmallBaseMap.flags
    static const char* sql =
        "SELECT v.smallbase_id, v.variation_id, s.label, v.label,"
        "       COALESCE(v.icon_atlas, s.icon_atlas, ''),"
        "       s.flags"
        " FROM VariationParam v"
        " JOIN SmallBaseMap s ON v.smallbase_id = s.smallbase_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        VariationInfo var;
        var.variationId   = sqlite3_column_int(stmt, 0);  // smallbase_id
        var.variationType = sqlite3_column_int(stmt, 1);  // variation_id
        var.label         = dbText(stmt, 2);
        var.sublabel      = dbText(stmt, 3);
        var.icon          = dbText(stmt, 4);
        var.visible       = dbNullableInt(stmt, 5, 0xFF);
        var.iconScale     = 1.0f;
        m_variationDB[var.getKey()] = var;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d variations", (int)m_variationDB.size());
    return true;
}

// ============================================================
//  SpotConfig  →  m_variationDist
//  Side-effect: fills m_mapDB[].legacyFilterPoints/dlcFilterPoints
// ============================================================
bool GameData::dbLoadSpotConfig(sqlite3* db)
{
    static const char* sql =
        "SELECT sc.pattern_id, sc.attach_id, sc.smallbase_id, sc.variation_id,"
        "       p.map_id, p.dlc"
        " FROM SpotConfig sc"
        " JOIN Pattern p ON sc.pattern_id = p.pattern_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        VariationDist dist;
        dist.patternId    = sqlite3_column_int(stmt, 0);
        dist.attachId     = sqlite3_column_int(stmt, 1);
        int smallbaseId   = sqlite3_column_int(stmt, 2);
        int variationId   = sqlite3_column_int(stmt, 3);
        int mapId         = sqlite3_column_int(stmt, 4);
        bool isDlc        = sqlite3_column_int(stmt, 5) != 0;

        dist.variationKey = smallbaseId * 10 + variationId;
        m_variationDist.push_back(dist);
    }
    sqlite3_finalize(stmt);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d spot configs", (int)m_variationDist.size());
    return true;
}

bool GameData::dbLoadFixedSpots(sqlite3 *db)
{
    static const char* sql =
        "SELECT p.map_id, p.dlc,"
        " COUNT(DISTINCT p.pattern_id) AS coord_pattern_count,"
        " ("
        "     SELECT COUNT(*) "
        "     FROM Pattern "
        "     WHERE map_id = p.map_id AND dlc = p.dlc"
        " ) AS map_total_patterns_by_dlc,"
        " ROUND("
        "     COUNT(DISTINCT p.pattern_id) * 1.0 / "
        "     (SELECT COUNT(*) FROM Pattern WHERE map_id = p.map_id AND dlc = p.dlc),"
        "     4"
        " ) AS occupancy_rate,"
        " ROUND("
        "     COUNT(DISTINCT p.pattern_id) * 100.0 / "
        "     (SELECT COUNT(*) FROM Pattern WHERE map_id = p.map_id AND dlc = p.dlc),"
        "     2"
        " ) || '%' AS occupancy_percentage,"
        " GROUP_CONCAT(DISTINCT ap.attach_id) AS attach_ids"
        " FROM AttachPoint ap"
        " JOIN SpotConfig sc ON ap.attach_id = sc.attach_id"
        " JOIN Pattern p ON sc.pattern_id = p.pattern_id"
        " GROUP BY ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, p.map_id, p.dlc "
        " ORDER BY occupancy_rate DESC";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int mapId           = sqlite3_column_int(stmt, 0);
        bool isDlc          = sqlite3_column_int(stmt, 1) != 0;
        float occupancyRate = static_cast<float>(sqlite3_column_double(stmt, 4));
        auto attachIds      = separateIntList(dbText(stmt, 6), ',');

        if (occupancyRate < 0.9f) // Threshold for "fixed" spots
            continue;

        auto& target = isDlc ? m_mapDB[mapId].dlcFilterPoints : m_mapDB[mapId].legacyFilterPoints;
        target.insert(target.end(), attachIds.begin(), attachIds.end());
    }
    sqlite3_finalize(stmt);

    for (auto& pair : m_mapDB)
    {
        sortAndUnique(pair.second.legacyFilterPoints);
        sortAndUnique(pair.second.dlcFilterPoints);
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d fixed spots", (int)m_variationDist.size());
    return true;
}

// ============================================================
//  GridHeight  →  m_gridOptions
// ============================================================
bool GameData::dbLoadGridHeights(sqlite3* db)
{
    static const char* sql =
        "SELECT grid_x, grid_z, height, map_id FROM GridHeight";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        GridOption opt;
        opt.x      = sqlite3_column_int(stmt, 0);
        opt.y      = sqlite3_column_int(stmt, 1);
        opt.height = static_cast<float>(sqlite3_column_double(stmt, 2));
        opt.map    = sqlite3_column_int(stmt, 3);
        m_gridOptions.push_back(opt);
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d grid heights", (int)m_gridOptions.size());
    return true;
}

// ============================================================
//  EventConfig  →  m_eventDB
// ============================================================
bool GameData::dbLoadEvents(sqlite3* db)
{
    static const char* sql =
        "SELECT pattern_id, content FROM EventConfig";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        EventInfo info;
        info.id    = sqlite3_column_int(stmt, 0);
        info.event = dbText(stmt, 1);
        m_eventDB[info.id] = info;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d events", (int)m_eventDB.size());
    return true;
}

// ============================================================
//  AttachMapBinding + AttachPoint  →  m_constantDB
//  Columns: attach_id(type), map_id, label, icon_atlas,
//           grid_x, grid_z, pos_x, pos_z, height
// ============================================================
bool GameData::dbLoadMapBindings(sqlite3* db)
{
    static const char* sql =
        "SELECT b.attach_id, b.map_id, b.label, b.icon_atlas,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height"
        " FROM AttachMapBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ConstantInfo info;
        info.type    = sqlite3_column_int(stmt, 0);   // attach_id used as type/id
        info.map     = sqlite3_column_int(stmt, 1);
        info.label   = dbText(stmt, 2);
        info.icon    = dbText(stmt, 3);
        info.iconScale = 1.0f;
        info.point   = dbMapPoint(stmt, 4);
        m_constantDB.push_back(info);
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d map bindings (constants)", (int)m_constantDB.size());
    return true;
}

// ============================================================
//  AttachPatternBinding + AttachPoint  →  m_rottedPowers
//  Columns: attach_id, pattern_id, grid_x, grid_z, pos_x, pos_z, height
// ============================================================
bool GameData::dbLoadPatternBindings(sqlite3* db)
{
    static const char* sql =
        "SELECT b.attach_id, b.pattern_id,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height"
        " FROM AttachPatternBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        RottedPowerInfo info;
        info.patternId = sqlite3_column_int(stmt, 1);
        info.point     = dbMapPoint(stmt, 2);
        m_rottedPowers[info.patternId] = info;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d pattern bindings (rotted powers)", (int)m_rottedPowers.size());
    return true;
}

// ============================================================
//  AttachSmallBaseBinding + AttachPoint + SmallBaseMap  →  m_greatHollowBindings
//  Columns: attach_id, smallbase_id(binding), label, icon_atlas,
//           grid_x, grid_z, pos_x, pos_z, height, flags(visible)
// ============================================================
bool GameData::dbLoadSmallBaseBindings(sqlite3* db)
{
    static const char* sql =
        "SELECT b.attach_id, b.smallbase_id, b.label, b.icon_atlas,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height,"
        "       s.flags"
        " FROM AttachSmallBaseBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id"
        " LEFT JOIN SmallBaseMap s ON b.smallbase_id = s.smallbase_id";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        GreatHollowBindingInfo info;
        // attach_id (col 0) is not stored; binding uses smallbase_id
        info.binding   = sqlite3_column_int(stmt, 1);
        info.label     = dbText(stmt, 2);
        info.icon      = dbText(stmt, 3);
        info.point     = dbMapPoint(stmt, 4);
        info.visible   = dbNullableInt(stmt, 9, 0xFF);
        info.iconScale = 1.0f;
        m_greatHollowBindings.push_back(info);
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d smallbase bindings (great hollow)", (int)m_greatHollowBindings.size());
    return true;
}

GameDataDB::GameDataDB()
{
}

GameDataDB::~GameDataDB()
{
}

bool GameDataDB::open(const std::string& dbPath)
{
    int rc = sqlite3_open_v2(dbPath.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK)
    {
        close();
        return false;
    }
    if (!createTempViews())
    {
        close();
        return false;
    }
    m_tempTable = "temp_pattern";
    resetFilter();
    loadCache();
    return true;
}

void GameDataDB::close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void GameDataDB::resetFilter(int mapId)
{
    SDL_assert(m_db);
    std::stringstream ss;
    ss << "DROP TABLE IF EXISTS " << m_tempTable << ";"
       << "CREATE TEMP TABLE " << m_tempTable << " AS "
       << "SELECT pattern_id FROM Pattern";
    if (mapId >= 0)
        ss << " WHERE map_id = " << mapId;
    ss << ";";
    sqlite3_exec(m_db, ss.str().c_str(), nullptr, nullptr, nullptr);
}

bool GameDataDB::loadCache()
{
    bool mapLoaded = loadMaps();
    if (!mapLoaded)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "GameDataDB: Failed to load maps");
        return false;
    }
    for (auto& pair : m_cachedMaps)
    {
        int mapId = pair.first;
        if (auto viewOpt = loadMapView(mapId))
        {
            pair.second.view = std::move(*viewOpt);
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "GameDataDB: Failed to load map view for %s", pair.second.name.c_str());
            return false;
        }
    }

    return true;
}

bool GameDataDB::loadMaps()
{
    static const char* sql =
        "SELECT map_id, name FROM Map";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        MapCache map;
        map.id   = sqlite3_column_int(stmt, 0);
        map.name = dbText(stmt, 1);
        m_cachedMaps[map.id] = map;
    }
    sqlite3_finalize(stmt);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "GameData DB: loaded %d maps", (int)m_cachedMaps.size());
    return true;
}

std::optional<MapView> GameDataDB::loadMapView(int mapId) const
{
    static const char* sql_attachpoint =
        "SELECT ap.grid_x,ap.grid_z,ap.pos_x,ap.pos_z,"
        " ROUND("
        "     COUNT(DISTINCT p.pattern_id) * 1.0 / "
        "     (SELECT COUNT(*) FROM Pattern WHERE map_id = ?),"
        "     4"
        " ) AS occupancy_rate,"
        " GROUP_CONCAT(DISTINCT ap.attach_id) AS attach_ids"
        " FROM AttachPoint ap"
        " JOIN SpotConfig sc ON ap.attach_id = sc.attach_id"
        " JOIN Pattern p ON sc.pattern_id = p.pattern_id"
        " WHERE p.map_id = ?"
        " GROUP BY ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z"
        " ORDER BY occupancy_rate DESC";

    std::optional<MapView> view{std::in_place, mapId};

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql_attachpoint, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, mapId);
    sqlite3_bind_int(stmt, 2, mapId);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        float ratio = static_cast<float>(sqlite3_column_double(stmt, 4));
        if (ratio < 0.9f) continue;

        auto attachIds = split(dbText(stmt, 5), ',');
        sortAndUnique(attachIds);
        for (int attachId : attachIds)
        {
            view->attachPoints.emplace_back();
            auto& apv = view->attachPoints.back();
            apv.attachId = attachId;
            apv.point = dbMapPoint(stmt, 0, false);
        }
    }
    static const char* sql_starter =
        "SELECT s.grid_x,s.grid_z,s.pos_x,s.pos_z,s.starter_id"
        " FROM Pattern p"
        " JOIN Starter s ON s.starter_id = p.starter_id"
        " WHERE p.map_id = ?"
        " GROUP BY p.starter_id";

    if (!dbPrepare(m_db, sql_starter, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, mapId);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        view->starters.emplace_back();
        auto& sv = view->starters.back();
        sv.point = dbMapPoint(stmt, 0, false);
        sv.starterId        = sqlite3_column_int(stmt, 4);
    }
    
    sqlite3_finalize(stmt);
    return view;
}

// ============================================================
//  GameDataDB::createTempViews  — one-time setup on open()
// ============================================================
bool GameDataDB::createTempViews()
{
    static const char* sql =
        // v_pattern: Pattern + both PlayArea rows flattened
        "CREATE TEMP VIEW IF NOT EXISTS v_pattern AS "
        "SELECT p.pattern_id, p.map_id, p.nightlord_id, p.dlc, p.starter_id,"
        "       p.day1boss_smallbase_id, p.day2boss_smallbase_id,"
        "       p.day1extraboss_smallbase_id, p.day2extraboss_smallbase_id,"
        "       pa1.grid_x AS pa1_grid_x, pa1.grid_z AS pa1_grid_z,"
        "       pa1.pos_x  AS pa1_pos_x,  pa1.pos_z  AS pa1_pos_z, pa1.height AS pa1_height,"
        "       pa2.grid_x AS pa2_grid_x, pa2.grid_z AS pa2_grid_z,"
        "       pa2.pos_x  AS pa2_pos_x,  pa2.pos_z  AS pa2_pos_z, pa2.height AS pa2_height,"
        "       s.grid_x AS starter_grid_x, s.grid_z AS starter_grid_z,"
        "       s.pos_x  AS starter_pos_x,  s.pos_z  AS starter_pos_z, s.height AS starter_height"
        " FROM Pattern p"
        " LEFT JOIN Starter s ON p.starter_id = s.starter_id"
        " LEFT JOIN PlayArea pa1 ON p.day1_playarea_id = pa1.playarea_id"
        " LEFT JOIN PlayArea pa2 ON p.day2_playarea_id = pa2.playarea_id;"

        // v_variation: SmallBaseMap + VariationParam
        "CREATE TEMP VIEW IF NOT EXISTS v_variation AS "
        "SELECT v.smallbase_id, v.variation_id,"
        "       v.smallbase_id * 10 + v.variation_id AS var_key,"
        "       COALESCE(s.label,'')                AS base_label,"
        "       COALESCE(v.label,'')                AS sub_label,"
        "       COALESCE(v.icon_atlas, s.icon_atlas,'') AS icon,"
        "       COALESCE(s.flags, 255)              AS visible"
        " FROM VariationParam v"
        " JOIN SmallBaseMap s ON v.smallbase_id = s.smallbase_id;"

        // v_constant: AttachMapBinding + AttachPoint
        "CREATE TEMP VIEW IF NOT EXISTS v_constant AS "
        "SELECT b.attach_id, b.map_id, b.label, b.icon_atlas,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height"
        " FROM AttachMapBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id;"

        // v_rotted_power: AttachPatternBinding + AttachPoint
        "CREATE TEMP VIEW IF NOT EXISTS v_rotted_power AS "
        "SELECT b.pattern_id,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height"
        " FROM AttachPatternBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id;"

        // v_great_hollow: AttachSmallBaseBinding + AttachPoint + SmallBaseMap
        "CREATE TEMP VIEW IF NOT EXISTS v_great_hollow AS "
        "SELECT b.smallbase_id AS binding, b.label, b.icon_atlas,"
        "       ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height,"
        "       COALESCE(s.flags, 255) AS visible"
        " FROM AttachSmallBaseBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id"
        " LEFT JOIN SmallBaseMap s ON b.smallbase_id = s.smallbase_id;";

    char* errmsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "GameDataDB: createTempViews error: %s", errmsg);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

// ============================================================
//  Map cache accessors
// ============================================================
std::vector<const char*> GameDataDB::getMapNames() const
{
    std::vector<const char*> result;
    for (const auto& pair : m_cachedMaps)
        result.push_back(pair.second.name.c_str());
    return result;
}

// ============================================================
//  Single-row query helpers
// ============================================================
std::optional<PatternInfo> GameDataDB::getPattern(int patternId) const
{
    static const char* sql =
        "SELECT pattern_id, map_id, nightlord_id, dlc, starter_id,"
        "       day1boss_smallbase_id, day2boss_smallbase_id,"
        "       day1extraboss_smallbase_id, day2extraboss_smallbase_id,"
        "       pa1_grid_x, pa1_grid_z, pa1_pos_x, pa1_pos_z, pa1_height,"
        "       pa2_grid_x, pa2_grid_z, pa2_pos_x, pa2_pos_z, pa2_height"
        " FROM v_pattern WHERE pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, patternId);

    std::optional<PatternInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PatternInfo p;
        p.id           = sqlite3_column_int(stmt,  0);
        p.map          = sqlite3_column_int(stmt,  1);
        p.boss         = dbNullableInt(stmt,  2);
        p.isdlc        = sqlite3_column_int(stmt,  3) != 0;
        p.starter      = dbNullableInt(stmt,  4, -1);
        p.bossId1      = dbNullableInt(stmt,  5);
        p.bossId2      = dbNullableInt(stmt,  6);
        p.extraBossId1 = dbNullableInt(stmt,  7);
        p.extraBossId2 = dbNullableInt(stmt,  8);
        p.playArea1    = dbMapPoint(stmt,  9);
        p.playArea2    = dbMapPoint(stmt, 14);
        result = p;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<SpotInfo> GameDataDB::getSpot(int attachId) const
{
    static const char* sql =
        "SELECT attach_id, grid_x, grid_z, pos_x, pos_z, height"
        " FROM AttachPoint WHERE attach_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, attachId);

    std::optional<SpotInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        SpotInfo info;
        info.attachId = sqlite3_column_int(stmt, 0);
        info.point    = dbMapPoint(stmt, 1);
        result = info;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<StarterInfo> GameDataDB::getStarter(int starterId) const
{
    static const char* sql =
        "SELECT starter_id, grid_x, grid_z, pos_x, pos_z, height"
        " FROM Starter WHERE starter_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, starterId);

    std::optional<StarterInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        StarterInfo info;
        info.starterId = sqlite3_column_int(stmt, 0);
        info.point     = dbMapPoint(stmt, 1);
        result = info;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<NightlordInfo> GameDataDB::getNightlord(int nightlordId) const
{
    static const char* sql =
        "SELECT nightlord_id, name FROM Nightlord WHERE nightlord_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, nightlordId);

    std::optional<NightlordInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        NightlordInfo info;
        info.id   = sqlite3_column_int(stmt, 0);
        info.name = dbText(stmt, 1);
        result = info;
    }
    sqlite3_finalize(stmt);
    return result;
}

namespace {
    static VariationInfo rowToVariation(sqlite3_stmt* stmt)
    {
        VariationInfo v;
        v.variationId   = sqlite3_column_int(stmt, 0);
        v.variationType = sqlite3_column_int(stmt, 1);
        v.label         = dbText(stmt, 2);
        v.sublabel      = dbText(stmt, 3);
        v.icon          = dbText(stmt, 4);
        v.visible       = sqlite3_column_int(stmt, 5);
        v.iconScale     = 1.0f;
        return v;
    }
}

std::optional<VariationInfo> GameDataDB::getVariation(int varKey) const
{
    static const char* sql =
        "SELECT smallbase_id, variation_id, base_label, sub_label, icon, visible"
        " FROM v_variation WHERE var_key = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, varKey);

    std::optional<VariationInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToVariation(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::optional<VariationInfo> GameDataDB::getVariation(int patternId, int attachId) const
{
    static const char* sql =
        "SELECT v.smallbase_id, v.variation_id, v.base_label, v.sub_label, v.icon, v.visible"
        " FROM v_variation v"
        " JOIN SpotConfig sc"
        "   ON sc.smallbase_id = v.smallbase_id AND sc.variation_id = v.variation_id"
        " WHERE sc.pattern_id = ? AND sc.attach_id = ?"
        " LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, patternId);
    sqlite3_bind_int(stmt, 2, attachId);

    std::optional<VariationInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToVariation(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::optional<VariationInfo> GameDataDB::getVariationById(int variationId) const
{
    static const char* sql =
        "SELECT smallbase_id, variation_id, base_label, sub_label, icon, visible"
        " FROM v_variation WHERE smallbase_id = ? LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, variationId);

    std::optional<VariationInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToVariation(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::optional<SpotOption> GameDataDB::getSpotOption(int attachId) const
{
    return std::nullopt;
}

std::optional<SpotLabelOption> GameDataDB::getAttachOption(int attachId) const
{
    return std::nullopt;
}

std::optional<GridOption> GameDataDB::getGridOption(int map, int x, int y) const
{
    static const char* sql =
        "SELECT grid_x, grid_z, height, map_id"
        " FROM GridHeight WHERE map_id = ? AND grid_x = ? AND grid_z = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, map);
    sqlite3_bind_int(stmt, 2, x);
    sqlite3_bind_int(stmt, 3, y);

    std::optional<GridOption> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        GridOption opt;
        opt.x      = sqlite3_column_int(stmt, 0);
        opt.y      = sqlite3_column_int(stmt, 1);
        opt.height = static_cast<float>(sqlite3_column_double(stmt, 2));
        opt.map    = sqlite3_column_int(stmt, 3);
        result = opt;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<RottedPowerInfo> GameDataDB::getRottedPower(int patternId) const
{
    static const char* sql =
        "SELECT pattern_id, grid_x, grid_z, pos_x, pos_z, height"
        " FROM v_rotted_power WHERE pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, patternId);

    std::optional<RottedPowerInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        RottedPowerInfo info;
        info.patternId = sqlite3_column_int(stmt, 0);
        info.point     = dbMapPoint(stmt, 1);
        result = info;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<EventInfo> GameDataDB::getEvent(int patternId) const
{
    static const char* sql =
        "SELECT pattern_id, content FROM EventConfig WHERE pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, patternId);

    std::optional<EventInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        EventInfo info;
        info.id    = sqlite3_column_int(stmt, 0);
        info.event = dbText(stmt, 1);
        result = info;
    }
    sqlite3_finalize(stmt);
    return result;
}

// ============================================================
//  Multi-row queries
// ============================================================
std::vector<VariationDist> GameDataDB::listDistribution(int patternId) const
{
    static const char* sql =
        "SELECT pattern_id, attach_id, smallbase_id * 10 + variation_id AS var_key"
        " FROM SpotConfig WHERE pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return {};
    sqlite3_bind_int(stmt, 1, patternId);

    std::vector<VariationDist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        VariationDist d;
        d.patternId    = sqlite3_column_int(stmt, 0);
        d.attachId     = sqlite3_column_int(stmt, 1);
        d.variationKey = sqlite3_column_int(stmt, 2);
        result.push_back(d);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<VariationInfo> GameDataDB::listVariations(int attachId, int map) const
{
    static const char* sql =
        "SELECT DISTINCT v.smallbase_id, v.variation_id,"
        "                v.base_label, v.sub_label, v.icon, v.visible"
        " FROM v_variation v"
        " JOIN SpotConfig sc"
        "   ON sc.smallbase_id = v.smallbase_id AND sc.variation_id = v.variation_id"
        " JOIN Pattern p ON sc.pattern_id = p.pattern_id"
        " WHERE sc.attach_id = ? AND p.map_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return {};
    sqlite3_bind_int(stmt, 1, attachId);
    sqlite3_bind_int(stmt, 2, map);

    std::vector<VariationInfo> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(rowToVariation(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<VariationInfo> GameDataDB::listVariations(int attachId, const std::set<int>& patterns) const
{
    if (patterns.empty()) return {};

    std::string ids;
    for (int id : patterns) {
        if (!ids.empty()) ids += ',';
        ids += std::to_string(id);
    }

    std::string sql =
        "SELECT DISTINCT v.smallbase_id, v.variation_id,"
        "                v.base_label, v.sub_label, v.icon, v.visible"
        " FROM v_variation v"
        " JOIN SpotConfig sc"
        "   ON sc.smallbase_id = v.smallbase_id AND sc.variation_id = v.variation_id"
        " WHERE sc.attach_id = ? AND sc.pattern_id IN (" + ids + ")";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, attachId);

    std::vector<VariationInfo> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(rowToVariation(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<ConstantInfo> GameDataDB::listConstants(int map) const
{
    static const char* sql =
        "SELECT attach_id, map_id, label, icon_atlas,"
        "       grid_x, grid_z, pos_x, pos_z, height"
        " FROM v_constant WHERE map_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return {};
    sqlite3_bind_int(stmt, 1, map);

    std::vector<ConstantInfo> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ConstantInfo info;
        info.type      = sqlite3_column_int(stmt, 0);  // attach_id as type identifier
        info.map       = sqlite3_column_int(stmt, 1);
        info.label     = dbText(stmt, 2);
        info.icon      = dbText(stmt, 3);
        info.iconScale = 1.0f;
        info.point     = dbMapPoint(stmt, 4);
        result.push_back(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<GreatHollowBindingInfo> GameDataDB::listGreatHollowBinding(int patternId) const
{
    static const char* sql =
        "SELECT DISTINCT gh.binding, gh.label, gh.icon_atlas,"
        "       gh.grid_x, gh.grid_z, gh.pos_x, gh.pos_z, gh.height, gh.visible"
        " FROM v_great_hollow gh"
        " JOIN SpotConfig sc ON sc.smallbase_id = gh.binding"
        " WHERE sc.pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql, &stmt)) return {};
    sqlite3_bind_int(stmt, 1, patternId);

    std::vector<GreatHollowBindingInfo> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        GreatHollowBindingInfo info;
        info.binding   = sqlite3_column_int(stmt, 0);
        info.label     = dbText(stmt, 1);
        info.icon      = dbText(stmt, 2);
        info.point     = dbMapPoint(stmt, 3);
        info.visible   = sqlite3_column_int(stmt, 8);
        info.iconScale = 1.0f;
        result.push_back(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

// ============================================================
//  Filter helpers
// ============================================================

std::set<int> GameDataDB::getFilteredPatterns() const
{
    std::string sql = "SELECT pattern_id FROM " + m_tempTable;
    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};

    std::set<int> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.insert(sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

int GameDataDB::getFilteredPatternCount() const
{
    std::string sql = "SELECT COUNT(*) FROM " + m_tempTable;
    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return 0;

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

std::set<int> GameDataDB::filterByMap(int map)
{
    resetFilter(map);
    return getFilteredPatterns();
}

std::set<int> GameDataDB::filterByVariation(const std::set<int>& patterns, int attachId, int varKey) const
{
    if (patterns.empty()) return {};

    std::string ids;
    for (int id : patterns) {
        if (!ids.empty()) ids += ',';
        ids += std::to_string(id);
    }

    std::string sql =
        "SELECT DISTINCT pattern_id FROM SpotConfig"
        " WHERE attach_id = ? AND smallbase_id * 10 + variation_id = ?"
        " AND pattern_id IN (" + ids + ")";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, attachId);
    sqlite3_bind_int(stmt, 2, varKey);

    std::set<int> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.insert(sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

std::set<int> GameDataDB::filterByStarter(const std::set<int>& patterns, int starterId) const
{
    if (patterns.empty()) return {};

    std::string ids;
    for (int id : patterns) {
        if (!ids.empty()) ids += ',';
        ids += std::to_string(id);
    }

    std::string sql =
        "SELECT pattern_id FROM Pattern"
        " WHERE starter_id = ? AND pattern_id IN (" + ids + ")";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, starterId);

    std::set<int> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.insert(sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

std::set<int> GameDataDB::filterByNightlord(const std::set<int>& patterns, int nightlordId) const
{
    if (patterns.empty()) return {};

    std::string ids;
    for (int id : patterns) {
        if (!ids.empty()) ids += ',';
        ids += std::to_string(id);
    }

    std::string sql =
        "SELECT pattern_id FROM Pattern"
        " WHERE nightlord_id = ? AND pattern_id IN (" + ids + ")";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, nightlordId);

    std::set<int> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.insert(sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

int GameDataDB::filterByMap2(int mapId)
{
    resetFilter(mapId);
    return getFilteredPatternCount();
}

int GameDataDB::filterBySmallBase(std::vector<int> attachIds, int smallBaseId, int variationId)
{
    std::string ids = join(attachIds);
    std::string sql =
        "DELETE FROM " + m_tempTable +
        " WHERE pattern_id NOT IN ("
        "  SELECT DISTINCT pattern_id FROM SpotConfig"
        "  WHERE attach_id IN (" + ids + ") AND smallbase_id = ? AND variation_id = ?"
        " )";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, smallBaseId);
    sqlite3_bind_int(stmt, 2, variationId);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return getFilteredPatternCount();
}

int GameDataDB::filterBySmallBase(std::vector<int> attachIds, int smallBaseId)
{
    std::string ids = join(attachIds);
    std::string sql =
        "DELETE FROM " + m_tempTable +
        " WHERE pattern_id NOT IN ("
        "  SELECT DISTINCT pattern_id FROM SpotConfig"
        "  WHERE attach_id IN (" + ids + ") AND smallbase_id = ?"
        " )";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, smallBaseId);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return getFilteredPatternCount();
}

int GameDataDB::filterBySmallBaseGroup(std::vector<int> attachIds, int groupId)
{
    std::string ids = join(attachIds);
    std::string sql =
        "DELETE FROM " + m_tempTable +
        " WHERE pattern_id NOT IN ("
        "  SELECT DISTINCT pattern_id FROM SpotConfig"
        "  JOIN SmallBaseMap ON SpotConfig.smallbase_id = SmallBaseMap.smallbase_id"
        "  WHERE attach_id IN (" + ids + ") AND group_id = ?"
        " )";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, groupId);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return getFilteredPatternCount();
}

int GameDataDB::filterByStarter2(int starterId)
{
    std::string sql =
        "DELETE FROM " + m_tempTable +
        " WHERE pattern_id NOT IN ("
        "  SELECT pattern_id FROM Pattern"
        "  WHERE starter_id = ?"
        " )";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, starterId);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return getFilteredPatternCount();
}

int GameDataDB::filterByNightlord2(int nightlordId)
{
    std::string sql =
        "DELETE FROM " + m_tempTable +
        " WHERE pattern_id NOT IN ("
        "  SELECT pattern_id FROM Pattern"
        "  WHERE nightlord_id = ?"
        " )";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql.c_str(), &stmt)) return {};
    sqlite3_bind_int(stmt, 1, nightlordId);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return getFilteredPatternCount();
}

const MapView& GameDataDB::getMapView(int mapId) const
{
    auto it = m_cachedMaps.find(mapId);
    if (it == m_cachedMaps.end())
        throw std::runtime_error("Map ID not found in cache");
    return it->second.view;
}

std::optional<PatternView> GameDataDB::getPatternView(int patternId) const
{
    static const char* sql_pattern =
        "SELECT v_pattern.map_id, Nightlord.name, dlc, day1boss_smallbase_id, day2boss_smallbase_id,"
        "       day1extraboss_smallbase_id, day2extraboss_smallbase_id, content,"
        "       starter_grid_x, starter_grid_z, starter_pos_x, starter_pos_z, starter_height,"
        "       pa1_grid_x, pa1_grid_z, pa1_pos_x, pa1_pos_z, pa1_height,"
        "       pa2_grid_x, pa2_grid_z, pa2_pos_x, pa2_pos_z, pa2_height"
        " FROM v_pattern "
        " LEFT JOIN Map ON v_pattern.map_id = Map.map_id "
        " LEFT JOIN EventConfig ON v_pattern.pattern_id = EventConfig.pattern_id "
        " LEFT JOIN Nightlord ON v_pattern.nightlord_id = Nightlord.nightlord_id "
        " WHERE v_pattern.pattern_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (!dbPrepare(m_db, sql_pattern, &stmt)) return std::nullopt;
    sqlite3_bind_int(stmt, 1, patternId);

    std::optional<PatternView> result{std::in_place, patternId};
    int day1bossId = 0, day2bossId = 0, day1ExtraBossId = 0, day2ExtraBossId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PatternView& p  = *result;
        p.mapId         = sqlite3_column_int(stmt,  0);
        p.nightlordName = dbText(stmt,  1);
        p.isdlc         = sqlite3_column_int(stmt,  2) != 0;
        day1bossId      = dbNullableInt(stmt,  3);
        day2bossId      = dbNullableInt(stmt,  4);
        day1ExtraBossId = dbNullableInt(stmt,  5);
        day2ExtraBossId = dbNullableInt(stmt,  6);
        p.eventContent  = dbText(stmt,  7);
        p.starter       = dbMapPoint(stmt, 8);
        p.day1PlayArea  = dbMapPoint(stmt, 13);
        p.day2PlayArea  = dbMapPoint(stmt, 18);
    }

    static const char* sql_boss =
        "SELECT label"
        " FROM SmallBaseMap"
        " WHERE smallbase_id = ?";
    if (dbPrepare(m_db, sql_boss, &stmt))
    {
        sqlite3_bind_int(stmt, 1, day1bossId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result->day1BossName = dbText(stmt, 0);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, day2bossId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result->day2BossName = dbText(stmt, 0);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, day1ExtraBossId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result->day1ExtraBossName = dbText(stmt, 0);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, day2ExtraBossId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result->day2ExtraBossName = dbText(stmt, 0);
    }

    static const char* sql_attachpoint =
        "SELECT sc.attach_id, grid_x, grid_z, pos_x, pos_z, height,"
        "       sc.smallbase_id, sc.variation_id, sb.group_id, sb.flags,"
        "       sb.label, vp.label, COALESCE(vp.icon_atlas, sb.icon_atlas)"
        " FROM SpotConfig sc"
        " LEFT JOIN AttachPoint ap ON sc.attach_id = ap.attach_id"
        " LEFT JOIN SmallBaseMap sb ON sc.smallbase_id = sb.smallbase_id"
        " LEFT JOIN VariationParam vp ON sc.smallbase_id = vp.smallbase_id AND sc.variation_id = vp.variation_id"
        " WHERE sc.pattern_id = ?";
    if (dbPrepare(m_db, sql_attachpoint, &stmt))
    {
        sqlite3_bind_int(stmt, 1, patternId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            result->spots.emplace_back();
            AttachPointView& apv = result->spots.back().attachPoint;
            apv.attachId   = sqlite3_column_int(stmt, 0);
            apv.point      = dbMapPoint(stmt, 1);

            SmallBaseView& sbv = result->spots.back().smallBase;
            sbv.smallBaseId = sqlite3_column_int(stmt, 6);
            sbv.variationId = sqlite3_column_int(stmt, 7);
            sbv.groupId     = sqlite3_column_int(stmt, 8);
            sbv.flag        = static_cast<SpotFlag>(sqlite3_column_int(stmt, 9));
            sbv.majorName   = dbText(stmt, 10);
            sbv.minorName   = dbText(stmt, 11);
            sbv.iconAlias   = dbText(stmt, 12);
        }
    }

    static const char* sql_mapbinding =
        "SELECT b.attach_id, grid_x, grid_z, pos_x, pos_z, height,"
        "       b.label, b.icon_atlas"
        " FROM AttachMapBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id"
        " WHERE b.map_id = ?";
    if (dbPrepare(m_db, sql_mapbinding, &stmt))
    {
        sqlite3_bind_int(stmt, 1, result->mapId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            result->spots.emplace_back();
            AttachPointView& apv = result->spots.back().attachPoint;
            apv.attachId   = sqlite3_column_int(stmt, 0);
            apv.point      = dbMapPoint(stmt, 1);

            SmallBaseView& sbv = result->spots.back().smallBase;
            sbv.majorName   = dbText(stmt, 6);
            sbv.iconAlias   = dbText(stmt, 7);
            sbv.flag        = SpotFlag_All;
        }
    }

    static const char* sql_patternbinding =
        "SELECT b.attach_id, grid_x, grid_z, pos_x, pos_z, height,"
        "       label, icon_atlas"
        " FROM AttachPatternBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id"
        " WHERE b.pattern_id = ?";
    if (dbPrepare(m_db, sql_patternbinding, &stmt))
    {
        sqlite3_bind_int(stmt, 1, result->patternId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            result->spots.emplace_back();
            AttachPointView& apv = result->spots.back().attachPoint;
            apv.attachId   = sqlite3_column_int(stmt, 0);
            apv.point      = dbMapPoint(stmt, 1);

            SmallBaseView& sbv = result->spots.back().smallBase;
            sbv.majorName   = dbText(stmt, 6);
            sbv.iconAlias   = dbText(stmt, 7);
            sbv.flag        = SpotFlag_All;
        }
    }

    static const char* sql_smallbasebinding =
        "SELECT ap.attach_id, ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, ap.height,"
        "       b.label, b.icon_atlas"
        " FROM AttachSmallBaseBinding b"
        " JOIN AttachPoint ap ON b.attach_id = ap.attach_id"
        " WHERE EXISTS ("
        "       SELECT 1 FROM SpotConfig sc"
        "       WHERE sc.smallbase_id = b.smallbase_id"
        "         AND sc.pattern_id = ?"
        "   )";
    if (dbPrepare(m_db, sql_smallbasebinding, &stmt))
    {
        sqlite3_bind_int(stmt, 1, result->patternId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            result->spots.emplace_back();
            AttachPointView& apv = result->spots.back().attachPoint;
            apv.attachId   = sqlite3_column_int(stmt, 0);
            apv.point      = dbMapPoint(stmt, 1);

            SmallBaseView& sbv = result->spots.back().smallBase;
            sbv.majorName   = dbText(stmt, 6);
            sbv.iconAlias   = dbText(stmt, 7);
            sbv.flag        = SpotFlag_All;
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
}
