local csv = require("csv")

local pattern_mapbase = dofile(getPath("gen_pattern_list.lua"))
local spotbase = csv.loadCsv("LotResultSmallBaseAndSpot.csv", "ID")
local pointbase = csv.loadCsv("WorldMapPointParam.csv", "ID")
local altpointbase = csv.loadCsv("SmallBaseAndSpotAttachPoint.csv", "ID")

local function genKey(attachId)
    local point = pointbase.rows[attachId] or altpointbase.rows[attachId]
    if point then
        return string.format("%d_%d_%.2f_%.2f", point.gridXNo, point.gridZNo, point.posX, point.posZ)
    else
        return "unknown_spot_" .. attachId
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

local map_patternlist = {}
for patternId, pattern in pairs(pattern_mapbase) do
    local patternlist = map_patternlist[pattern.map] or {}
    table.insert(patternlist, patternId)
    map_patternlist[pattern.map] = patternlist
end

local spot_distribution = {}
local spot_index = 1
for _, row in pairs(spotbase.rows) do
    local patternId = tonumber(row["patternId"])
    local map = pattern_mapbase[patternId].map
    assert(map ~= nil)

    local attachId = tonumber(row["attachId"])
    local ukey = genKey(attachId)
    local distribution = spot_distribution[ukey]
    if distribution == nil then
        distribution = {
            maps = {},
            patterns = {},
            variations = {},
            index = spot_index,
        }
        spot_index = spot_index + 1
    end

    distribution.maps[map] = true
    distribution.patterns[patternId] = true
    table.insert(distribution.variations, {
        id = tonumber(row["smallBaseMapId"]),
        type = tonumber(row["variationId"]),
        patternId = patternId,
    })

    spot_distribution[ukey] = distribution
end

local remapped_spot_distribution = {}
for ukey, distribution in pairs(spot_distribution) do
    local id = distribution.index
    local point = unpackUkey(ukey)
    remapped_spot_distribution[id] = {
        id = id,
        gridXNo = point.gridXNo,
        gridZNo = point.gridZNo,
        posX = point.posX,
        posZ = point.posZ,
        maps = distribution.maps,
        patterns = distribution.patterns,
        variations = distribution.variations,
    }
end

local static_spotlist = {}
local pattern_variationlist = {}
local variation_index = 1
for id, distribution in pairs(remapped_spot_distribution) do
    for map, _ in pairs(distribution.maps) do
        local expected_patternlist = map_patternlist[map]
        assert(expected_patternlist ~= nil)

        local succeed = true
        for _, patternId in ipairs(expected_patternlist) do
            if not distribution.patterns[patternId] then
                succeed = false
                break
            end
        end

        if succeed then
            local spotlist = static_spotlist[map] or {map = map, spots = {}}
            table.insert(spotlist.spots, id)
            static_spotlist[map] = spotlist
        end
    end

    for _, variation in ipairs(distribution.variations) do
        table.insert(pattern_variationlist, {
            id = variation_index,
            patternId = variation.patternId,
            spotId = id,
            variationType = variation.type,
            variationId = variation.id,
        })
        variation_index = variation_index + 1
    end
end

csv.table2Csv(static_spotlist, "autogen_static_spotlist.csv", function(k, v)
    if k == "spots" and type(v) == "table" then
        return table.concat(v, "_")
    else
        return tostring(v)
    end
end)
csv.table2Csv(remapped_spot_distribution, "autogen_spot_distribution.csv", function(k, v)
    if k == "patterns" or k == "maps" or k == "variations" then
        return "_"
    else
        return tostring(v)
    end
end)
csv.table2Csv(pattern_variationlist, "autogen_pattern_variationlist.csv")
