local csv = require("csv")

local pattern_mapbase = dofile(getPath("01_list_patterns.lua"))
local spotbase = csv.loadCsv("LotResultSmallBaseAndSpot.csv", "ID")
local pointbase = csv.loadCsv("WorldMapPointParam.csv", "ID")
local altpointbase = csv.loadCsv("SmallBaseAndSpotAttachPoint.csv", "ID")

local function genKey(attachId)
    local point = pointbase.rows[attachId] or altpointbase.rows[attachId]
    if point then
        return string.format("%d_%d_%.2f_%.2f", point.gridXNo, point.gridZNo, point.posX, point.posZ)
    else
        return "unknown_" .. attachId
    end
end

local function unpackUkey(ukey)
    local parts = {}
    for ep in ukey:gmatch("([^_]*)") do
        table.insert(parts, ep)
    end
    return {
        gridXNo = tonumber(parts[1]),
        gridZNo = tonumber(parts[2]),
        posX = tonumber(parts[3]),
        posZ = tonumber(parts[4]),
    }
end

local mapPatterns = {}
for patternId, pattern in pairs(pattern_mapbase) do
    local sets = mapPatterns[pattern.map] or {}
    table.insert(sets, patternId)
    mapPatterns[pattern.map] = sets
end

local spotdist = {}
for _, row in pairs(spotbase.rows) do
    local patternId = tonumber(row["patternId"])
    local map = pattern_mapbase[patternId].map
    assert(map ~= nil)
    local attachId = tonumber(row["attachId"])
    
    local ukey = genKey(attachId)
    local dist = spotdist[ukey] or { patterns = {}, maps = {} }

    dist.maps[map] = true
    dist.patterns[patternId] = true

    spotdist[ukey] = dist
end

local pattern_spots = {}
local dynamicSpots = {}
for ukey, records in pairs(spotdist) do
    for map, _ in pairs(records.maps) do
       local expectedSets = mapPatterns[map]
        assert(expectedSets ~= nil)
        local succeed = true
        for _, patternId in ipairs(expectedSets) do
            if not records.patterns[patternId] then
                succeed = false
                break
            end
        end
        table.insert(pattern_spots, {
                id = unpackUkey(ukey),
                map = map,
                static = succeed,
            }
        )
        if not succeed then
            for patternId, _ in pairs(records.patterns) do
                local sets = dynamicSpots[patternId] or {}
                table.insert(sets, ukey)
                dynamicSpots[patternId] = sets
            end
        end
    end
end

csv.table2Csv(pattern_spots, "step-03.csv", function(k, v)
    if k == "id" and v then
        return string.format("%d_%d_%.2f_%.2f", v.gridXNo, v.gridZNo, v.posX, v.posZ)
    else
        return tostring(v)
    end
end)

return pattern_spots