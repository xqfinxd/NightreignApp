#include "GameData.h"
#include <sstream>
#include <algorithm>
#include <SDL_log.h>
#include <SDL_assert.h>
#include "sqlite3.h"

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

    void sortAndUnique(std::vector<int>& vec) {
        std::sort(vec.begin(), vec.end());
        auto last = std::unique(vec.begin(), vec.end());
        vec.erase(last, vec.end());
        vec.shrink_to_fit();
    }
} // anonymous namespace

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

// ============================================================
//  Multi-row queries
// ============================================================
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
