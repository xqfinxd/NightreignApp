require("csv")
local patterns = dofile(getPath("01_list_patterns.lua"))
local lots = loadCsv("LotResultSmallBaseAndSpot.csv", "ID")
local points = loadCsv("WorldMapPointParam.csv", "ID")
local altPoints = loadCsv("SmallBaseAndSpotAttachPoint.csv", "ID")

local function genKey(attachId)
    local point = points.rows[attachId] or altPoints.rows[attachId]
    if point then
        return string.format("%d_%d_%.2f_%.2f", point.gridXNo, point.gridZNo, point.posX, point.posZ)
    else
        return "unknown_" .. attachId
    end
end

local mapPatterns = {}
for patternId, pattern in pairs(patterns) do
    local sets = mapPatterns[pattern.map] or {}
    table.insert(sets, patternId)
    mapPatterns[pattern.map] = sets
end

local spotdist = {}
for _, row in pairs(lots.rows) do
    local patternId = tonumber(row["patternId"])
    local map = patterns[patternId].map
    assert(map ~= nil)
    local attachId = tonumber(row["attachId"])
    
    local ukey = genKey(attachId)
    local dist = spotdist[ukey] or { patterns = {}, maps = {} }

    dist.maps[map] = true
    dist.patterns[patternId] = true

    spotdist[ukey] = dist
end

local spots = {}
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
        table.insert(spots, {
                id = ukey,
                map = map,
                static = succeed,
            }
        )
    end
end

return table2Csv(spots, "step-03.csv") and spots